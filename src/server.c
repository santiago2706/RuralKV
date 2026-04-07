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
                // Truco sucio en C para aislar la llave y valor de la URL
                char temp_key[100] = {0};
                char temp_val[200] = {0};
                
                k_ptr += 2; // saltar "k="
                v_ptr += 2; // saltar "v="
                
                int i=0; while(*k_ptr != '&' && *k_ptr != '\0' && i<99) { temp_key[i++] = *k_ptr++; }
                int j=0; while(*v_ptr != ' ' && *v_ptr != '\0' && *v_ptr != '&' && j<199) { temp_val[j++] = *v_ptr++; }
                
                // === APLICAR EL PROTOCOLO RURAL KV ===
                wal_append_put(wal, temp_key, temp_val); // El respaldo ROM
                hash_put(db, temp_key, temp_val);        // La cache RAM
                
                sprintf(response, "%s{\"estado\":\"exito\", \"accion\":\"Guardado en WAL y Hash\", \"guardado\":\"%s\"}", HTTP_200, temp_key);
                send(client_socket, response, strlen(response), 0);
            }
        } 
        // RUTA 2: Leer un dato -> http://localhost:8080/get?k=DNI
        else if (strncmp(url, "/get?", 5) == 0) {
            char* k_ptr = strstr(url, "k=");
            if (k_ptr) {
                char temp_key[100] = {0};
                k_ptr += 2;
                int i=0; while(*k_ptr != ' ' && *k_ptr != '\0' && *k_ptr != '&' && i<99) { temp_key[i++] = *k_ptr++; }
                
                // === LLAMAR A LA BASE DE DATOS ===
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
        // RUTA 3: Bienvenida 
        else {
             sprintf(response, "%s{\"status\":\"RuralKV API V0.1 Activa\", \"uso\": \"Prueba con /put?k=clave&v=valor\"}", HTTP_200);
             send(client_socket, response, strlen(response), 0);
        }
    }
    closesocket(client_socket);
}

void server_start(int port, HashTable* db, Wal* wal) {
#ifdef _WIN32
    WSADATA wsaData;
    // Iniciar el subsistema de redes de Windows
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("🔥 Error Critico: No se pudo encender el chip de red (WSAStartup falló).\n");
        return;
    }
#endif

    // Crear un canal de comunicacion vacio
    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        printf("🔥 Error Critico: Fallo del Socket Base.\n");
        return;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Amarrar el código C al puerto de la computadora
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("🔥 Error Critico: El puerto %d está siendo usado por otro programa.\n", port);
        closesocket(server_socket);
        return;
    }

    if (listen(server_socket, 10) == SOCKET_ERROR) {
        printf("🔥 Error Critico: No podemos 'escuchar' bloqueado por Firewall.\n");
        closesocket(server_socket);
        return;
    }

    printf("\n========= RURAL KV (MODO DEMONIO) =========\n");
    printf(" 🚀 Servidor HTTP Nativo C Encendido.\n");
    printf(" 📡 Escuchando en el puerto TCP :: %d\n", port);
    printf(" 🔗 En tu navegador ingresa: http://localhost:%d/\n", port);
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
