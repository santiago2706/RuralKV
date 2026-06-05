/**
 * @file hash.h
 * @brief In-memory Key-Value Hash Table indexing.
 * @note Resolves collisions using linked lists and allocates node memory through Arena.
 */
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
    time_t expire_at; 
    struct HashEntry* next;
} HashEntry;

// La estructura madre
typedef struct {
    HashEntry** buckets; // Arreglo de punteros de las casillas
    size_t size;         // Cuántas casillas tenemos
    Arena* arena;        // Referencia a nuestro dios salvador de la memoria
} HashTable;
/**
 * @brief Creates a new HashTable instance backed by an Arena allocator.
 * @param arena Pointer to the Arena used for data persistence in memory.
 * @param size Number of buckets in the hash table.
 * @return Pointer to the newly created HashTable.
 */
HashTable* hash_create(Arena* arena, size_t size);
/**
 * @brief Inserts or updates a key-value pair in the table.
 * @details Uses the djb2 algorithm. If the key exists, its pointer is updated.
 * @param ht Pointer to the hash table.
 * @param key Null-terminated string representing the lookup key.
 * @param value Null-terminated string representing the value data.
 * @return true if insertion/update succeeded, false otherwise.
 */
bool hash_put(HashTable* ht, const char* key, const char* value);
/**
 * @brief Retrieves a value associated with a given key.
 * @param ht Pointer to the hash table.
 * @param key Lookup key string.
 * @return Pointer to the value string, or NULL if the key is not found.
 */
const char* hash_get(HashTable* ht, const char* key);
bool hash_delete(HashTable* ht, const char* key);
bool hash_exists(HashTable* ht, const char* key);
bool hash_set_expire(HashTable* ht, const char* key, int seconds);
int hash_ttl(HashTable* ht, const char* key);

#endif
