#ifndef RURAL_ARENA_H
#define RURAL_ARENA_H

#include <stddef.h>
#include <stdint.h>

/* 
 * Arena Allocator
 * El secreto de RuralKV para usar menos de 8MB de RAM sin fragmentarse.
 */
typedef struct {
    uint8_t* buffer;  // Puntero al bloque gigante de memoria
    size_t capacity;  // Tamaño máximo en bytes
    size_t offset;    // Cuánta memoria hemos usado hasta ahora
} Arena;

// Inicializa la arena con un tamaño en bytes (Ej: 1024 * 1024 para 1MB)
Arena* arena_init(size_t capacity);

// Solicita memoria a la arena. Equivalente a un malloc() pero instantáneo y seguro O(1)
void* arena_alloc(Arena* arena, size_t size);

// Libera formalmente toda la arena (Solo se llama al apagar el servidor)
void arena_free(Arena* arena);

// Reinicia el offset a 0 (Limpieza instantánea sin liberar memoria real al S.O.)
void arena_reset(Arena* arena);

#endif // RURAL_ARENA_H
