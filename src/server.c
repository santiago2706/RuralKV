#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Detección automática de SO para usar la librería de red correcta
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib") 
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h>
    // En linux fingimos que existen los tipos de datos de windows para no ensuciar el codigo
    #define SOCKET int
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define closesocket close
#endif

// Cabeceras HTTP Universales (Habilitan CORS para que funcione en cualquier web)
const char* HTTP_200 = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\n\r\n";
const char* HTTP_404 = "HTTP/1.1 404 Not Found\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\n\r\n";
/**
 * @brief Decodes a percent-encoded URL string into a standard ASCII string.
 * @details Converts '+' characters into spaces and '%HH' hex sequences into 
 * their respective char representations. Terminal null character is appended.
 * @param dst Pointer to the destination buffer where the decoded string will be stored.
 * @param src Pointer to the null-terminated source string containing URL-encoded data.
 */

static void url_decode(char* dst, const char* src) {
    while (*src) {
        if (*src == '+') {
            *dst++ = ' ';
        } else if (*src == '%' && isxdigit((unsigned char)src[1]) && isxdigit((unsigned char)src[2])) {
            char hex[3] = { src[1], src[2], '\0' };
            *dst++ = (char)strtol(hex, NULL, 16);
            src += 2;
        } else {
            *dst++ = *src;
        }
        src++;
    }
    *dst = '\0';
} 
/**
 * @brief Parses incoming HTTP requests and dispatches actions to the Key-Value core.
 * @details Intercepts synchronous raw TCP socket data, decodes parameters, and routes 
 * them to corresponding database primitives (PUT, GET, DEL, KEYS, EXPIRE, TTL).
 * Guarantees transactional write durability by emitting WAL logs.
 * @param client_socket Active file descriptor connected to the remote user.
 * @param db Pointer to the in-memory primary hash table cache.
 * @param wal Pointer to the write-ahead logging synchronization handle.
 */

static void handle_client(SOCKET client_socket, HashTable* db, Wal* wal) {
    char buffer[1024] = {0};
    int bytes_read = recv(client_socket, buffer, sizeof(buffer)-1, 0);
    
    if (bytes_read > 0) {
        char method[16] = {0};
        char url[512] = {0};
        // Un parseador extremadamente rápido de texto HTTP -> GET /ruta?k=1 HTTP/1.1
        sscanf(buffer, "%15s %511s", method, url);
        
        printf("[🌐 HTTP] Recibiendo: %s %s\n", method, url);
        
        char response[1024];
        
        // RUTA 1: Guardar un dato -> http://localhost:8080/put?k=DNI&v=Diagnostico
        if (strncmp(url, "/put?", 5) == 0) {
            char* k_ptr = strstr(url, "k=");
            char* v_ptr = strstr(url, "v=");
            if (k_ptr && v_ptr) {
                
                char temp_key[100] = {0};
                char temp_val[200] = {0};
                char encoded_key[200] = {0};
                char encoded_val[400] = {0}; 
                
                k_ptr += 2; // saltar "k="
                v_ptr += 2; // saltar "v="
                
                int i=0; while(*k_ptr != '&' && *k_ptr != '\0' && i<99) { temp_key[i++] = *k_ptr++; }
                int j=0; while(*v_ptr != ' ' && *v_ptr != '\0' && *v_ptr != '&' && j<199) { temp_val[j++] = *v_ptr++; }
                url_decode(temp_key, encoded_key);
                url_decode(temp_val, encoded_val);

                // === APLY PROTOCOLE RURAL KV ===
                wal_append_put(wal, temp_key, temp_val); // El respaldo ROM
                hash_put(db, temp_key, temp_val);        // La cache RAM
                
                sprintf(response, "%s{\"estado\":\"exito\", \"accion\":\"Guardado en WAL y Hash\", \"guardado\":\"%s\"}", HTTP_200, temp_key);
                send(client_socket, response, strlen(response), 0);
            }
        } 
        
        else if (strncmp(url, "/get?", 5) == 0) {
            char* k_ptr = strstr(url, "k=");
            if (k_ptr) {
                char temp_key[100] = {0};
                char encoded_key[200] = {0};
                k_ptr += 2;
                int i=0; while(*k_ptr != ' ' && *k_ptr != '\0' && *k_ptr != '&' && i<99) { temp_key[i++] = *k_ptr++; }
                url_decode(temp_key, encoded_key);
                // === CALL TO THE DATA BASE ===
                const char* val = hash_get(db, temp_key);
                
                if (val) {
                    sprintf(response, "%s{\"llave\":\"%s\", \"valor\":\"%s\"}", HTTP_200, temp_key, val);
                    send(client_socket, response, strlen(response), 0);
                } else {
                    sprintf(response, "%s{\"error\":\"Paciente no encontrado en RAM\"}", HTTP_404);
                    send(client_socket, response, strlen(response), 0);
                }
            }
        }

        else if (strncmp(url, "/del?", 5) == 0) {
            char* k_ptr = strstr(url, "k=");
            if (k_ptr) {
                char temp_key[200] = {0};
                char encoded_key[200] = {0};
                k_ptr += 2;
                int i=0; while(*k_ptr != ' ' && *k_ptr != '\0' && *k_ptr != '&' && i<199) { encoded_key[i++] = *k_ptr++; }
                url_decode(temp_key, encoded_key);

                bool removed = hash_delete(db, temp_key);
                if (removed) {
                    wal_append_del(wal, temp_key);
                    sprintf(response, "%s{\"deleted\":true, \"key\":\"%s\"}", HTTP_200, temp_key);
                } else {
                    sprintf(response, "%s{\"deleted\":false, \"error\":\"Clave no encontrada\"}", HTTP_404);
                }
                send(client_socket, response, strlen(response), 0);
            }
        }
        else if (strncmp(url, "/exists?", 8) == 0) {
            char* k_ptr = strstr(url, "k=");
            if (k_ptr) {
                char temp_key[200] = {0};
                char encoded_key[200] = {0};
                k_ptr += 2;
                int i=0; while(*k_ptr != ' ' && *k_ptr != '\0' && *k_ptr != '&' && i<199) { encoded_key[i++] = *k_ptr++; }
                url_decode(temp_key, encoded_key);

                bool exists = hash_exists(db, temp_key);
                sprintf(response, "%s{\"exists\":%s}", HTTP_200, exists ? "true" : "false");
                send(client_socket, response, strlen(response), 0);
            }
        }
        else if (strncmp(url, "/keys", 5) == 0) {
            char keys_list[4096] = {0};
            strcat(keys_list, "[");
            int count = 0;
            
            for (size_t bucket = 0; bucket < db->size; bucket++) {
                HashEntry* entry = db->buckets[bucket];
                HashEntry* prev = NULL;
                while (entry != NULL) {
                    if (entry->expire_at != 0 && time(NULL) >= entry->expire_at) {
                        if (prev) {
                            prev->next = entry->next;
                        } else {
                            db->buckets[bucket] = entry->next;
                        }
                        entry = (prev ? prev->next : db->buckets[bucket]);
                        continue;
                    }
                    if (count > 0) strcat(keys_list, ",");
                    strcat(keys_list, "\"");
                    strcat(keys_list, entry->key);
                    strcat(keys_list, "\"");
                    count++;
                    prev = entry;
                    entry = entry->next;
                }
            }
            strcat(keys_list, "]");
            sprintf(response, "%s{\"keys\":%s, \"count\":%d}", HTTP_200, keys_list, count);
            send(client_socket, response, strlen(response), 0);
        }
        else if (strncmp(url, "/expire?", 8) == 0) {
            char* k_ptr = strstr(url, "k=");
            char* t_ptr = strstr(url, "t=");
            if (k_ptr && t_ptr) {
                char temp_key[200] = {0};
                char encoded_key[200] = {0};
                k_ptr += 2;
                int i=0; while(*k_ptr != '&' && *k_ptr != '\0' && i<199) { encoded_key[i++] = *k_ptr++; }
                url_decode(temp_key, encoded_key);

                t_ptr += 2;
                int seconds = atoi(t_ptr);
                bool updated = hash_set_expire(db, temp_key, seconds);
                if (updated) {
                    wal_append_expire(wal, temp_key, seconds);
                    sprintf(response, "%s{\"expire\":\"ok\", \"key\":\"%s\", \"seconds\":%d}", HTTP_200, temp_key, seconds);
                } else {
                    sprintf(response, "%s{\"error\":\"No se pudo aplicar expiración\"}", HTTP_404);
                }
                send(client_socket, response, strlen(response), 0);
            }
        }
        else if (strncmp(url, "/ttl?", 5) == 0) {
            char* k_ptr = strstr(url, "k=");
            if (k_ptr) {
                char temp_key[200] = {0};
                char encoded_key[200] = {0};
                k_ptr += 2;
                int i=0; while(*k_ptr != ' ' && *k_ptr != '\0' && *k_ptr != '&' && i<199) { encoded_key[i++] = *k_ptr++; }
                url_decode(temp_key, encoded_key);

                int ttl = hash_ttl(db, temp_key);
                if (ttl >= 0) {
                    sprintf(response, "%s{\"ttl\":%d}", HTTP_200, ttl);
                } else if (ttl == -1) {
                    sprintf(response, "%s{\"ttl\":-1, \"message\":\"Sin expiración\"}", HTTP_200);
                } else {
                    sprintf(response, "%s{\"error\":\"Clave no encontrada\"}", HTTP_404);
                }
                send(client_socket, response, strlen(response), 0);
            }
        }
        else if (strncmp(url, "/ping", 5) == 0) {
            sprintf(response, "%s{\"pong\":\"PONG\"}", HTTP_200);
            send(client_socket, response, strlen(response), 0);
        } else if (strncmp(url, "/info", 5) == 0) {
            int count = 0;
            for (size_t bucket = 0; bucket < db->size; bucket++) {
                HashEntry* entry = db->buckets[bucket];
                while (entry != NULL) {
                    if (entry->expire_at == 0 || time(NULL) < entry->expire_at) {
                        count++;
                    }
                    entry = entry->next;
                }
            }
            sprintf(response, "%s{\"status\":\"ok\", \"version\":\"0.1\", \"keys\":%d}", HTTP_200, count);
            send(client_socket, response, strlen(response), 0);
        }
        else {
             sprintf(response, "%s{\"status\":\"RuralKV API V0.1 Activa\", \"uso\": \"Prueba con /put?k=clave&v=valor\"}", HTTP_200);
             send(client_socket, response, strlen(response), 0);
        }
    }
    closesocket(client_socket);
}


void server_start(int port, HashTable* db, Wal* wal){
#ifdef _WIN32
    WSADATA wsaData;
    // Iniciar el subsistema de redes de Windows
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("Error Critico: No se pudo encender el chip de red (WSAStartup falló).\n");
        return;
    }
#endif

    // Crear un canal de comunicacion vacio
    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        printf("Error Critico: Fallo del Socket Base.\n");
        return;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Amarrar el código C al puerto de la computadora
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Error Critico: El puerto %d está siendo usado por otro programa.\n", port);
        closesocket(server_socket);
        return;
    }

    if (listen(server_socket, 10) == SOCKET_ERROR) {
        printf("Error Critico: No podemos 'escuchar' bloqueado por Firewall.\n");
        closesocket(server_socket);
        return;
    }

    printf("\n========= RURAL KV =========\n");
    printf(" Servidor HTTP Nativo C Encendido.\n");
    printf(" Escuchando en el puerto TCP :: %d\n", port);
    printf(" En tu navegador ingresa: http://localhost:%d/\n", port);
    printf("===========================================\n\n");
    printf("Esperando la conexión del Frontend (HTMX/Streamlit)...\n");

    // === EL CICLO DE VIDA ETERNO ===
    while (1) {
        struct sockaddr_in client_addr;
        int client_len = sizeof(client_addr);
        
        // El programa se "pausará" aquí adentro indefinidamente hasta que un navegador llame
        SOCKET client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        
        if (client_socket == INVALID_SOCKET) continue;
        
        // Atendemos la llamada, respondemos y seguimos escuchando
        handle_client(client_socket, db, wal);
    }

    closesocket(server_socket);
#ifdef _WIN32
    WSACleanup();
#endif
}
