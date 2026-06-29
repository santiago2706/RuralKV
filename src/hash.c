/**
 * @file hash.c
 * @brief Chained HashTable implementation with lazy Time-To-Live (TTL) eviction.
 */
#include "hash.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

/**
 * @brief DJB2 hashing algorithm designed by Dan Bernstein.
 * @details Uses bitwise shifts (hash << 5) for high-performance string hashing.
 */

static unsigned long hash_djb2(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; 
    }
    return hash;
}

/**
 * @brief Checks if a hash entry has expired.
 * @param entry The hash entry to check.
 * @return true if the entry has expired, false otherwise.
 */
static bool hash_entry_is_expired(HashEntry* entry) {
    return entry->expire_at != 0 && time(NULL) >= entry->expire_at;
}

/**
 * @brief Custom strdup clone that utilizes the Arena allocator instead of malloc.
 */
static char* arena_strdup(Arena* a, const char* str) {
    size_t len = strlen(str) + 1;
    char* copy = (char*)arena_alloc(a, len);
    if(copy) {
        memcpy(copy, str, len);
    }
    return copy;
}

HashTable* hash_create(Arena* arena, size_t size) {
    // Pedimos el espacio de la Arena
    HashTable* ht = (HashTable*)arena_alloc(arena, sizeof(HashTable));
    if (!ht) return NULL;
    
    ht->size = size;
    ht->count = 0;
    ht->buckets_mallocd = 0;
    ht->arena = arena;
    
    size_t buckets_size = sizeof(HashEntry*) * size;
    ht->buckets = (HashEntry**)arena_alloc(arena, buckets_size);
    if (!ht->buckets) return NULL;
    
    // Setea todo a nulo.
    memset(ht->buckets, 0, buckets_size);
    return ht;
}

bool hash_put(HashTable* ht, const char* key, const char* value) {
    // Trigger automático: rehash si load_factor > 2
    if (ht->count > ht->size * 2) {
        hash_resize(ht);
    }

    unsigned long idx = hash_djb2(key) % ht->size;
    
    // Revisar si ya existe
    HashEntry* entry = ht->buckets[idx];
    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {
            // Ya existe! Actualizar valor 
            entry->value = arena_strdup(ht->arena, value);
            entry->expire_at = 0;
            return true;
        }
        entry = entry->next;
    }
    
    // No existe, creamos uno
    HashEntry* new_entry = (HashEntry*)arena_alloc(ht->arena, sizeof(HashEntry));
    if (!new_entry) return false; 
    
    new_entry->key = arena_strdup(ht->arena, key);
    new_entry->value = arena_strdup(ht->arena, value);
    new_entry->expire_at = 0;

    // Encadenamiento al principio del bucket
    new_entry->next = ht->buckets[idx];
    ht->buckets[idx] = new_entry;
    ht->count++;

    return true;
}

/**
 * @brief Resizes the hash table when load_factor > 2 (count / size > 2).
 * @details Allocates a new bucket array via malloc (double the current size),
 * rehashes all active non-expired entries into it, then swaps and frees the old array.
 * Node structs themselves stay in the Arena; only the bucket array is malloc'd.
 */
bool hash_resize(HashTable* ht) {
    size_t new_size = ht->size * 2;
    fprintf(stderr, "[RuralKV] Rehashing: load_factor superó 2 (%zu claves / %zu buckets). Expandiendo a %zu buckets...\n",
            ht->count, ht->size, new_size);

    // Usamos malloc para el nuevo arreglo de buckets (la Arena no puede liberar bloques individuales)
    HashEntry** new_buckets = (HashEntry**)malloc(sizeof(HashEntry*) * new_size);
    if (!new_buckets) {
        fprintf(stderr, "[RuralKV] ERROR: No se pudo reservar memoria para el rehash.\n");
        return false;
    }
    memset(new_buckets, 0, sizeof(HashEntry*) * new_size);

    time_t now = time(NULL);
    size_t new_count = 0;

    // Reinsertar todas las entradas activas (no expiradas) en la nueva tabla
    for (size_t i = 0; i < ht->size; i++) {
        HashEntry* entry = ht->buckets[i];
        while (entry != NULL) {
            HashEntry* next = entry->next;
            // Descartar entradas expiradas durante el rehash
            if (entry->expire_at == 0 || now < entry->expire_at) {
                unsigned long new_idx = hash_djb2(entry->key) % new_size;
                entry->next = new_buckets[new_idx];
                new_buckets[new_idx] = entry;
                new_count++;
            }
            entry = next;
        }
    }

    // Solo liberar el array viejo si fue asignado con malloc (no si viene de la Arena)
    if (ht->buckets_mallocd) {
        free(ht->buckets);
    }

    ht->buckets = new_buckets;
    ht->size = new_size;
    ht->count = new_count;
    ht->buckets_mallocd = 1; // A partir de aquí siempre será malloc

    fprintf(stderr, "[RuralKV] Rehash completo. Nuevos buckets: %zu, Claves activas: %zu\n", new_size, new_count);
    return true;
}
bool hash_delete(HashTable* ht, const char* key) {
    unsigned long idx = hash_djb2(key) % ht->size;
    HashEntry* entry = ht->buckets[idx];
    HashEntry* prev = NULL;

    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {
            if (prev) {
                prev->next = entry->next;
            } else {
                ht->buckets[idx] = entry->next;
            }
            ht->count--;
            return true;
        }
        prev = entry;
        entry = entry->next;
    }
    return false;
}

const char* hash_get(HashTable* ht, const char* key) {
    unsigned long idx = hash_djb2(key) % ht->size;
    HashEntry* entry = ht->buckets[idx];
    HashEntry* prev = NULL;

    // Buscar en el bucket
    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {
            if (hash_entry_is_expired(entry)) {
                if (prev) {
                    prev->next = entry->next;
                } else {
                    ht->buckets[idx] = entry->next;
                }
                return NULL;
            }
            return entry->value; // LO ENCONTRO EN O(1)!
        }
        prev = entry;
        entry = entry->next;
    }
    return NULL; 
}  
bool hash_exists(HashTable* ht, const char* key){
    return hash_get(ht, key) != NULL;
}
bool hash_set_expire(HashTable* ht, const char* key, int seconds) {
    if (seconds <= 0) return false;
    unsigned long idx = hash_djb2(key) % ht->size;
    HashEntry* entry = ht->buckets[idx];
    HashEntry* prev = NULL;

    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {
            if (hash_entry_is_expired(entry)) {
                if (prev) {
                    prev->next = entry->next;
                } else {
                    ht->buckets[idx] = entry->next;
                }
                return false;
            }
            entry->expire_at = time(NULL) + seconds;
            return true;
        }
        prev = entry;
        entry = entry->next;
    }
    return false;
}
int hash_ttl(HashTable* ht, const char* key) {
    unsigned long idx = hash_djb2(key) % ht->size;
    HashEntry* entry = ht->buckets[idx];
    HashEntry* prev = NULL;
    time_t now = time(NULL);

    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {
            if (hash_entry_is_expired(entry)) {
                if (prev) {
                    prev->next = entry->next;
                } else {
                    ht->buckets[idx] = entry->next;
                }
                return -2;
            }
            if (entry->expire_at == 0) return -1;
            int ttl = (int)(entry->expire_at - now);
            return ttl < 0 ? -2 : ttl;
        }
        prev = entry;
        entry = entry->next;
    }
    return -2;
}
