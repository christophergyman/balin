#include "engine/arena.h"

#include <stdint.h>

#include "engine/log.h"

static int IsPowerOfTwo(size_t value) {
    return value != 0 && (value & (value - 1)) == 0;
}

void ArenaInit(Arena *arena, const char *name, void *memory, size_t capacity) {
    ASSERT(arena != NULL, LOG_CAT_ENGINE, "arena is null");
    ASSERT(memory != NULL, LOG_CAT_ENGINE, "arena '%s' memory is null", name);
    ASSERT(capacity > 0, LOG_CAT_ENGINE, "arena '%s' capacity is zero", name);

    arena->name = name;
    arena->memory = (unsigned char *)memory;
    arena->capacity = capacity;
    arena->offset = 0;
}

void *ArenaPush(Arena *arena, size_t size, size_t alignment) {
    ASSERT(arena != NULL, LOG_CAT_ENGINE, "arena is null");
    ASSERT(size > 0, LOG_CAT_ENGINE, "arena '%s' push size is zero", arena->name);
    ASSERT(IsPowerOfTwo(alignment), LOG_CAT_ENGINE, "arena '%s' alignment %zu is not a power of two",
        arena->name, alignment);

    // Align the absolute address, not only the offset, so the backing buffer
    // does not need to be aligned by the caller.
    uintptr_t base = (uintptr_t)arena->memory;
    uintptr_t cursor = base + arena->offset;
    uintptr_t aligned = (cursor + (uintptr_t)alignment - 1) & ~((uintptr_t)alignment - 1);
    size_t start = (size_t)(aligned - base);

    ASSERT(start <= arena->capacity && size <= arena->capacity - start, LOG_CAT_ENGINE,
        "arena '%s' overflow: capacity %zu, offset %zu, size %zu",
        arena->name, arena->capacity, start, size);

    void *result = arena->memory + start;
    arena->offset = start + size;
    return result;
}

void ArenaReset(Arena *arena) {
    ASSERT(arena != NULL, LOG_CAT_ENGINE, "arena is null");
    arena->offset = 0;
}

size_t ArenaUsed(const Arena *arena) {
    ASSERT(arena != NULL, LOG_CAT_ENGINE, "arena is null");
    return arena->offset;
}

size_t ArenaCapacity(const Arena *arena) {
    ASSERT(arena != NULL, LOG_CAT_ENGINE, "arena is null");
    return arena->capacity;
}
