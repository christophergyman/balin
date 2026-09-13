#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>

// Bump allocator. No free, no zeroing. Overflow is fatal, per ADR-019.
#define ARENA_DEFAULT_ALIGNMENT 16

typedef struct Arena {
    const char *name;
    unsigned char *memory;
    size_t capacity;
    size_t offset;
} Arena;

void ArenaInit(Arena *arena, const char *name, void *memory, size_t capacity);
void *ArenaPush(Arena *arena, size_t size, size_t alignment);
void ArenaReset(Arena *arena);

size_t ArenaUsed(const Arena *arena);
size_t ArenaCapacity(const Arena *arena);

#endif
