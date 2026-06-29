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

typedef struct HashEntry {
    char* key;
    char* value;
    time_t expire_at;
    struct HashEntry* next;
} HashEntry;

typedef struct {
    HashEntry** buckets;
    size_t size;
    size_t count;
    bool buckets_mallocd;
    Arena* arena;
} HashTable;

HashTable* hash_create(Arena* arena, size_t size);
bool hash_put(HashTable* ht, const char* key, const char* value);
bool hash_resize(HashTable* ht);
const char* hash_get(HashTable* ht, const char* key);
bool hash_delete(HashTable* ht, const char* key);
bool hash_exists(HashTable* ht, const char* key);
bool hash_set_expire(HashTable* ht, const char* key, int seconds);
int hash_ttl(HashTable* ht, const char* key);

#endif
