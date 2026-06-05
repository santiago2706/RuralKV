/**
 * @file arena.h
 * @brief Arena Allocator for bounded memory management.
 * @note Prevents memory fragmentation and ensures O(1) allocation time.
 */
#ifndef RURAL_ARENA_H
#define RURAL_ARENA_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t* buffer;  // Puntero al bloque gigante de memoria
    size_t capacity;  // Tamaño máximo en bytes
    size_t offset;    // Cuánta memoria hemos usado hasta ahora
} Arena;

/**
 * @brief Initializes a new memory arena with a fixed capacity.
 * @param capacity The total size in bytes to allocate from the OS.
 * @return Pointer to the allocated Arena, or NULL if the OS allocation fails.
 */
Arena* arena_init(size_t capacity);

/**
 * @brief Allocates a block of memory from the arena.
 * @param arena Pointer to the memory arena.
 * @param size Requested size in bytes.
 * @return Aligned pointer to the allocated space, or NULL if capacity is exceeded.
 */
void* arena_alloc(Arena* arena, size_t size);

/**
 * @brief Resets the arena offset to zero without freeing the physical block.
 * @note Useful for clearing temporary data between execution cycles.
 */
void arena_free(Arena* arena);

/**
 * @brief Releases the entire arena block back to the Operating System.
 * @param arena Pointer to the memory arena.
 */
void arena_reset(Arena* arena);

#endif // RURAL_ARENA_H
