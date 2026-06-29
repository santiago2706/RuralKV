/**
 * @file hash.c
 * @brief Chained HashTable implementation with lazy Time-To-Live (TTL) eviction.
 */
#include "hash.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/**
 * @brief DJB2 hashing algorithm designed by Dan Bernstein.
 * @details Uses bitwise shifts (hash << 5) for high-performance string hashing.
 */
static unsigned long hash_djb2(const char* str) {
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
    if (copy) {
        memcpy(copy, str, len);
    }
    return copy;
}

static void hash_remove_entry(HashTable* ht, size_t idx, HashEntry* entry, HashEntry* prev) {
    if (prev) {
        prev->next = entry->next;
    } else {
        ht->buckets[idx] = entry->next;
    }
    if (ht->count > 0) {
        ht->count--;
    }
}

HashTable* hash_create(Arena* arena, size_t size) {
    // Arrancamos con buckets en la Arena para mantener el costo inicial bajo.
    HashTable* ht = (HashTable*)arena_alloc(arena, sizeof(HashTable));
    if (!ht) return NULL;

    ht->size = size;
    ht->count = 0;
    ht->buckets_mallocd = false;
    ht->arena = arena;

    size_t buckets_size = sizeof(HashEntry*) * size;
    ht->buckets = (HashEntry**)arena_alloc(arena, buckets_size);
    if (!ht->buckets) return NULL;

    memset(ht->buckets, 0, buckets_size);
    return ht;
}

bool hash_resize(HashTable* ht) {
    if (!ht || ht->size == 0) return false;

    // Rehash automático: cuando la carga supera 2, duplicamos la tabla.
    size_t new_size = ht->size * 2;
    HashEntry** new_buckets = (HashEntry**)malloc(sizeof(HashEntry*) * new_size);
    if (!new_buckets) return false;

    memset(new_buckets, 0, sizeof(HashEntry*) * new_size);

    time_t now = time(NULL);
    size_t new_count = 0;

    for (size_t i = 0; i < ht->size; i++) {
        HashEntry* entry = ht->buckets[i];
        while (entry != NULL) {
            HashEntry* next = entry->next;

            // Solo migramos entradas vivas; las expiradas se descartan durante el resize.
            if (entry->expire_at == 0 || now < entry->expire_at) {
                unsigned long new_idx = hash_djb2(entry->key) % new_size;
                entry->next = new_buckets[new_idx];
                new_buckets[new_idx] = entry;
                new_count++;
            }

            entry = next;
        }
    }

    // Si el array anterior ya venía de malloc, lo liberamos; si vino de la Arena, no.
    if (ht->buckets_mallocd) {
        free(ht->buckets);
    }

    ht->buckets = new_buckets;
    ht->size = new_size;
    ht->count = new_count;
    ht->buckets_mallocd = true;
    return true;
}

bool hash_put(HashTable* ht, const char* key, const char* value) {
    if (!ht || !key || !value) return false;

    // Umbral simple: si la tabla ya va por encima de 2 elementos por bucket, crece.
    if (ht->count > ht->size * 2) {
        if (!hash_resize(ht)) {
            return false;
        }
    }

    unsigned long idx = hash_djb2(key) % ht->size;

    HashEntry* entry = ht->buckets[idx];
    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {
            entry->value = arena_strdup(ht->arena, value);
            entry->expire_at = 0;
            return true;
        }
        entry = entry->next;
    }

    HashEntry* new_entry = (HashEntry*)arena_alloc(ht->arena, sizeof(HashEntry));
    if (!new_entry) return false;

    new_entry->key = arena_strdup(ht->arena, key);
    new_entry->value = arena_strdup(ht->arena, value);
    new_entry->expire_at = 0;

    new_entry->next = ht->buckets[idx];
    ht->buckets[idx] = new_entry;
    ht->count++;

    return true;
}

bool hash_delete(HashTable* ht, const char* key) {
    if (!ht || !key) return false;

    unsigned long idx = hash_djb2(key) % ht->size;
    HashEntry* entry = ht->buckets[idx];
    HashEntry* prev = NULL;

    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {
            hash_remove_entry(ht, idx, entry, prev);
            return true;
        }
        prev = entry;
        entry = entry->next;
    }
    return false;
}

const char* hash_get(HashTable* ht, const char* key) {
    if (!ht || !key) return NULL;

    unsigned long idx = hash_djb2(key) % ht->size;
    HashEntry* entry = ht->buckets[idx];
    HashEntry* prev = NULL;

    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {
            if (hash_entry_is_expired(entry)) {
                hash_remove_entry(ht, idx, entry, prev);
                return NULL;
            }
            return entry->value;
        }
        prev = entry;
        entry = entry->next;
    }
    return NULL;
}

bool hash_exists(HashTable* ht, const char* key) {
    return hash_get(ht, key) != NULL;
}

bool hash_set_expire(HashTable* ht, const char* key, int seconds) {
    if (!ht || !key || seconds <= 0) return false;

    unsigned long idx = hash_djb2(key) % ht->size;
    HashEntry* entry = ht->buckets[idx];
    HashEntry* prev = NULL;

    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {
            if (hash_entry_is_expired(entry)) {
                hash_remove_entry(ht, idx, entry, prev);
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
    if (!ht || !key) return -2;

    unsigned long idx = hash_djb2(key) % ht->size;
    HashEntry* entry = ht->buckets[idx];
    HashEntry* prev = NULL;
    time_t now = time(NULL);

    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {
            if (hash_entry_is_expired(entry)) {
                hash_remove_entry(ht, idx, entry, prev);
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
