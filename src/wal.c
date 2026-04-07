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

void wal_close(Wal* wal) {
    if (wal) {
        if (wal->file) {
            fclose(wal->file);
        }
        free(wal);
    }
}
