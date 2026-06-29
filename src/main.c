#include <stdio.h>
#include <stdlib.h>
#include "arena.h"
#include "hash.h"
#include "wal.h"
#include "server.h"

#define ARENA_1MB (1024 * 1024)
#define CANTIDAD_CASILLAS_HASH 1024
#define PORT 8080

int main() {
    printf("=========================================\n");
    printf("   Iniciando RuralKV Engine\n");
    printf("=========================================\n\n");

    printf("[Paso 1] Inicializando Arena Allocator (%d bytes)...\n", ARENA_1MB);
    Arena* arena = arena_init(ARENA_1MB);
    if (!arena) return 1;
    
    printf("[Paso 2] Levantando Tabla Hash O(1)...\n");
    HashTable* db = hash_create(arena, CANTIDAD_CASILLAS_HASH);
    
    printf("[Paso 3] Abriendo Write-Ahead Log de Seguridad...\n");
    Wal* wal = wal_init("ruralkv.log");

    printf("[Paso 4] Reaplicando operaciones del WAL...\n");
    wal_replay(db, "ruralkv.log");
    
    // Aquí el programa bloquea el main thread y entra en un bucle infinito
    server_start(PORT, db, wal);

    // Estas lineas solo se alcanzarán si cerramos el servidor
    wal_close(wal);
    hash_destroy(db);
    arena_free(arena);
    return 0;
}
