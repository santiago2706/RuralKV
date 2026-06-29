#include "hash.h"
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

// Algoritmo DJB2 (Creado por Dan Bernstein)
// Convierte una palabra en un número entero único muy rápido usando shift << 5
static unsigned long hash_djb2(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; 
    }
    return hash;
}

static bool hash_entry_is_expired(HashEntry* entry) {
    //tiene una fecha de expiracion designada  && la fecha actual es mayor o igual a esa fecha
    return entry->expire_at != 0 && time(NULL) >= entry->expire_at;
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
    // Pedimos el espacio con malloc para que la estructura HashTable sea estable en memoria
    HashTable* ht = (HashTable*)malloc(sizeof(HashTable));
    if (!ht) return NULL;
    
    ht->size = size;
    ht->arena = arena;
    
    size_t buckets_size = sizeof(HashEntry*) * size;
    // Los buckets sí los asignamos en la arena
    ht->buckets = (HashEntry**)arena_alloc(arena, buckets_size);
    if (!ht->buckets) {
        free(ht);
        return NULL;
    }
    
    // Setea todo a nulo.
    memset(ht->buckets, 0, buckets_size);
    return ht;
}

void hash_destroy(HashTable* ht) {
    if (ht) {
        free(ht);
    }
}

void hash_rebuild_arena(HashTable* ht) {
    if (!ht || !ht->arena) return;

    // 1. Crear una nueva arena temporal con la misma capacidad
    Arena* new_arena = arena_init(ht->arena->capacity);
    if (!new_arena) {
        fprintf(stderr, "Error: No se pudo inicializar la nueva Arena para GC.\n");
        return;
    }

    // 2. Asignar nuevos buckets en la nueva arena
    size_t buckets_size = sizeof(HashEntry*) * ht->size;
    HashEntry** new_buckets = (HashEntry**)arena_alloc(new_arena, buckets_size);
    if (!new_buckets) {
        fprintf(stderr, "Error: No se pudo asignar memoria para los nuevos buckets en GC.\n");
        arena_free(new_arena);
        return;
    }
    memset(new_buckets, 0, buckets_size);

    // 3. Copiar las entradas activas a los nuevos buckets en la nueva arena
    for (size_t i = 0; i < ht->size; i++) {
        HashEntry* old_entry = ht->buckets[i];
        while (old_entry != NULL) {
            // Ignorar si está expirada
            if (old_entry->expire_at != 0 && time(NULL) >= old_entry->expire_at) {
                old_entry = old_entry->next;
                continue;
            }

            // Crear nuevo nodo en la nueva arena
            HashEntry* new_entry = (HashEntry*)arena_alloc(new_arena, sizeof(HashEntry));
            if (!new_entry) {
                fprintf(stderr, "Error: OOM durante reconstrucción de la Arena en GC.\n");
                arena_free(new_arena);
                return;
            }

            // Duplicar clave y valor en la nueva arena
            size_t key_len = strlen(old_entry->key) + 1;
            new_entry->key = (char*)arena_alloc(new_arena, key_len);
            if (new_entry->key) {
                memcpy(new_entry->key, old_entry->key, key_len);
            }

            size_t val_len = strlen(old_entry->value) + 1;
            new_entry->value = (char*)arena_alloc(new_arena, val_len);
            if (new_entry->value) {
                memcpy(new_entry->value, old_entry->value, val_len);
            }

            new_entry->expire_at = old_entry->expire_at;

            // Insertar al inicio de la lista del bucket correspondiente en la nueva tabla
            unsigned long idx = hash_djb2(old_entry->key) % ht->size;
            new_entry->next = new_buckets[idx];
            new_buckets[idx] = new_entry;

            old_entry = old_entry->next;
        }
    }

    // 4. Liberar la arena vieja física y actualizar punteros en el HashTable
    Arena* old_arena = ht->arena;
    ht->buckets = new_buckets;
    ht->arena = new_arena;
    arena_free(old_arena);
}

bool hash_put(HashTable* ht, const char* key, const char* value) {
    if (!ht || !ht->arena) return false;

    // Si la arena está al 80% de su capacidad, compactamos (Garbage Collection)
    if (ht->arena->offset > ht->arena->capacity * 0.8) {
        printf("[Arena GC] Iniciando recolección de basura (Uso: %.1f%%)...\n", 
               ((double)ht->arena->offset / ht->arena->capacity) * 100.0);
        hash_rebuild_arena(ht);
        printf("[Arena GC] Compactación completada (Nuevo Uso: %.1f%%).\n", 
               ((double)ht->arena->offset / ht->arena->capacity) * 100.0);
    }

    unsigned long idx = hash_djb2(key) % ht->size;
    
    // Revisar si ya existe
    HashEntry* entry = ht->buckets[idx];
    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {
            // Ya existe! Actualizar valor y resetear expiración.
            entry->value =  arena_strdup(ht->arena, value);
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

bool hash_exists(HashTable* ht, const char* key) {
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
