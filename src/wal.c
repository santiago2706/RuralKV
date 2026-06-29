#include "wal.h"
#include <stdlib.h>
#include <string.h>

Wal* wal_init(const char* filepath) {
    Wal* wal = (Wal*)malloc(sizeof(Wal));
    if (!wal) return NULL;
    
    // "ab+" = Append Binary. Crea el archivo si no existe, y el cabezal
    // del disco SIEMPRE escribe al final (extremadamente rápido).
    wal->file = fopen(filepath, "ab+");
    if (!wal->file) {
        free(wal);
        return NULL;
    }
    
    return wal;
}

bool wal_append_put(Wal* wal, const char* key, const char* value) {
    if (!wal || !wal->file) return false;
    
    // Formato texto seguro para sobrevivir apagones.
    // (En un motor más complejo podríamos usar binario puro + crc32)
    fprintf(wal->file, "PUT %s | %s\n", key, value);
    
    // fflush() EXIGE al Sistema Operativo que grabe estritamente en el disco 
    // en este exacto nanosegundo, ignorando sus cachés internos.
    fflush(wal->file);
    return true;
}

bool wal_append_del(Wal* wal, const char* key) {
    if (!wal || !wal->file) return false;
    fprintf(wal->file, "DEL %s\n", key);
    fflush(wal->file);
    return true;
}

bool wal_append_expire(Wal* wal, const char* key, int seconds) {
    if (!wal || !wal->file) return false;
    fprintf(wal->file, "EXPIRE %s %d\n", key, seconds);
    fflush(wal->file);
    return true;
}

void wal_replay(HashTable* db, const char* filepath) {
    FILE* file = fopen(filepath, "r");
    if (!file) return; // Si no existe, no hay nada que recuperar
    
    char line[1024];
    while (fgets(line, sizeof(line), file)) {
        // Quitar saltos de línea (\r y \n)
        line[strcspn(line, "\r\n")] = '\0';
        
        if (strncmp(line, "PUT ", 4) == 0) {
            char* key = line + 4;
            char* sep = strstr(key, " | ");
            if (sep) {
                *sep = '\0';
                char* value = sep + 3;
                hash_put(db, key, value);
            }
        } else if (strncmp(line, "DEL ", 4) == 0) {
            char* key = line + 4;
            hash_delete(db, key);
        } else if (strncmp(line, "EXPIRE ", 7) == 0) {
            char* key = line + 7;
            char* space = strchr(key, ' ');
            if (space) {
                *space = '\0';
                int seconds = atoi(space + 1);
                hash_set_expire(db, key, seconds);
            }
        }
    }
    fclose(file);
}

void wal_close(Wal* wal) {
    if (wal) {
        if (wal->file) {
            fclose(wal->file);
        }
        free(wal);
    }
}
