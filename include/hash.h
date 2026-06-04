#ifndef RURAL_HASH_H
#define RURAL_HASH_H

#include "arena.h"
#include <stdbool.h>
#include <stddef.h>
#include <time.h>

// Nodo de la lista enlazada para manejar colisiones de colisión hash
typedef struct HashEntry {
    char* key;
    char* value;
    struct HashEntry* next;
} HashEntry;

// La estructura madre
typedef struct {
    HashEntry** buckets; // Arreglo de punteros de las casillas
    size_t size;         // Cuántas casillas tenemos
    Arena* arena;        // Referencia a nuestro dios salvador de la memoria
} HashTable;

HashTable* hash_create(Arena* arena, size_t size);
bool hash_put(HashTable* ht, const char* key, const char* value);
const char* hash_get(HashTable* ht, const char* key);
bool hash_delete(HashTable* ht, const char* key);
bool hash_exists(HashTable* ht, const char* key);
bool hash_set_expire(HashTable* ht, const char* key, int seconds);
int hash_ttl(HashTable* ht, const char* key);

#endif
