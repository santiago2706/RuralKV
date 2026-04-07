#include "hash.h"
#include <string.h>

// Algoritmo DJB2 (Creado por Dan Bernstein)
// Magia pura: Convierte una palabra en un número entero único muy rápido usando shift << 5
static unsigned long hash_djb2(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; 
    }
    return hash;
}

// Nuestro propio clon de 'strdup' para jamás usar malloc directamente
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
    ht->arena = arena;
    
    size_t buckets_size = sizeof(HashEntry*) * size;
    ht->buckets = (HashEntry**)arena_alloc(arena, buckets_size);
    if (!ht->buckets) return NULL;
    
    // Setea todo a nulo.
    memset(ht->buckets, 0, buckets_size);
    return ht;
}

bool hash_put(HashTable* ht, const char* key, const char* value) {
    unsigned long idx = hash_djb2(key) % ht->size;
    
    // Revisar si ya existe
    HashEntry* entry = ht->buckets[idx];
    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {
            // Ya existe! Actualizar valor (esto deja sucio el viejo valor en la Arena
            // pero como la Arena se borra entera al final, no hay leaks de C).
            entry->value = arena_strdup(ht->arena, value);
            return true;
        }
        entry = entry->next;
    }
    
    // No existe, creamos uno
    HashEntry* new_entry = (HashEntry*)arena_alloc(ht->arena, sizeof(HashEntry));
    if (!new_entry) return false; 
    
    new_entry->key = arena_strdup(ht->arena, key);
    new_entry->value = arena_strdup(ht->arena, value);
    
    // Encadenamiento al principio del bucket
    new_entry->next = ht->buckets[idx];
    ht->buckets[idx] = new_entry;
    
    return true;
}

const char* hash_get(HashTable* ht, const char* key) {
    unsigned long idx = hash_djb2(key) % ht->size;
    HashEntry* entry = ht->buckets[idx];
    
    // Buscar en el bucket
    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {
            return entry->value; // LO ENCONTRO EN O(1)!
        }
        entry = entry->next;
    }
    
    return NULL;
}
