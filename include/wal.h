#ifndef RURAL_WAL_H
#define RURAL_WAL_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    FILE* file;
} Wal;

// Inicia o abre el log de operaciones
Wal* wal_init(const char* filepath);

// Registra la acción "PUT" en disco antes de que explote la máquina
bool wal_append_put(Wal* wal, const char* key, const char* value);
bool wal_append_del(Wal* wal, const char* key);
bool wal_append_expire(Wal* wal, const char* key, int seconds);

// Cierra el archivo de forma segura
void wal_close(Wal* wal);

#endif
