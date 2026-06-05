#include "arena.h"
#include <stdlib.h>
#include <stdio.h>

Arena* arena_init(size_t capacity) {
    Arena* arena = (Arena*)malloc(sizeof(Arena));
    if (!arena) {
        perror("Fallo fatal: No se pudo crear estructura Arena");
        exit(EXIT_FAILURE);
    }
    
    // Aquí ocurre la magia: reservamos todo el bloque de golpe
    arena->buffer = (uint8_t*)malloc(capacity);
    if (!arena->buffer) {
        perror("Fallo fatal: RAM insuficiente para iniciar RuralKV");
        free(arena);
        exit(EXIT_FAILURE);
    }
    
    arena->capacity = capacity;
    arena->offset = 0;
    
    return arena; 
}

void* arena_alloc(Arena* arena, size_t size) {
    // Alinear el tamaño a múltiple de 8 bytes (Alineación natural para CPU moderna)
    // Esto hace que leer y escribir en la base de datos sea más rápido para el procesador
    size_t alignment = 8;
    size_t current_offset = arena->offset;
    size_t mask = alignment - 1;
    size_t misalignment = current_offset & mask;
    size_t adjustment = (misalignment == 0) ? 0 : (alignment - misalignment);
    
    size_t aligned_offset = current_offset + adjustment;
    size_t new_offset = aligned_offset + size;
    
    // Verificamos si no hemos superado el límite impuesto (Ej: los 8MB)
    if (new_offset > arena->capacity) {
        fprintf(stderr, "ERROR RURALKV: OOM (Out Of Memory). La Arena está llena.\n");
        return NULL;
    }
    
    void* ptr = arena->buffer + aligned_offset;
    arena->offset = new_offset; // Avanzamos el marcador
    
    return ptr; // Retornamos la memoria sin hacer llamadas al S.O.
}

void arena_free(Arena* arena) {
    if (arena) {
        if (arena->buffer) {
            free(arena->buffer);
        }
        free(arena);
    }
}

void arena_reset(Arena* arena) {
    if (arena) {
        // En una arena no hacemos "free" de los datos individuales, 
        // simplemente retrocedemos el puntero a 0. Es una operación O(1).
        arena->offset = 0;
    }
}
