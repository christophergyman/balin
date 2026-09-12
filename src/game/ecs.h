#ifndef ECS_H
#define ECS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Small custom ECS, per ADR-005. An entity is a 32-bit handle: a 16-bit slot
// index plus a 16-bit generation. Destroy is deferred: EcsDestroy marks the
// entity dead at once so queries skip it, and EcsFlush frees the slot and
// bumps its generation after the system list has run. A stale handle then
// fails its generation check and can never target a reused slot.
//
// Component storage is one fixed array per component, indexed by entity slot.
// Presence lives in a 64-bit mask per entity, so queries scan the slot range
// and test one value. Component ids are capped at 64 by the mask word.
#define ECS_MAX_ENTITIES 256
#define ECS_MAX_COMPONENTS 64

typedef uint32_t Entity;

// Sentinel returned by EcsCreate when the world is full. Never a live handle.
#define ENTITY_INVALID 0xFFFFFFFFu

typedef uint64_t EcsMask;

typedef struct EcsWorld {
    uint16_t generation[ECS_MAX_ENTITIES];
    bool alive[ECS_MAX_ENTITIES];
    EcsMask mask[ECS_MAX_ENTITIES];       // Component bits present on each entity.
    uint16_t componentCount[ECS_MAX_COMPONENTS];
    uint16_t freeList[ECS_MAX_ENTITIES];  // Slots freed by EcsFlush, reused LIFO.
    uint16_t freeCount;
    uint16_t nextFresh;                   // Slots never handed out yet.
    Entity pending[ECS_MAX_ENTITIES];     // Handles queued by EcsDestroy.
    uint16_t pendingCount;
    uint16_t aliveCount;
} EcsWorld;

static inline uint16_t EntityIndex(Entity entity) {
    return (uint16_t)(entity & 0xFFFFu);
}

static inline uint16_t EntityGeneration(Entity entity) {
    return (uint16_t)(entity >> 16);
}

static inline Entity EntityMake(uint16_t index, uint16_t generation) {
    return (Entity)index | ((Entity)generation << 16);
}

static inline EcsMask EcsBit(uint16_t componentId) {
    return (EcsMask)1 << componentId;
}

// Clears every slot. Call once at boot and on a fresh run.
void EcsInit(EcsWorld *world);

// Returns a new entity, or ENTITY_INVALID when all slots are taken. Freed
// slots return in LIFO order with their generation already bumped.
Entity EcsCreate(EcsWorld *world);

// Marks the entity dead and queues its slot for cleanup. Returns false when
// the handle is stale. Queries skip the entity immediately. The slot is not
// reused until EcsFlush runs.
bool EcsDestroy(EcsWorld *world, Entity entity);

// Applies queued destroys: frees slots and bumps their generations. Call once
// per tick after the system list, per ADR-005.
void EcsFlush(EcsWorld *world);

// True when the handle references a live entity. False for stale or
// out-of-range handles.
bool EcsAlive(const EcsWorld *world, Entity entity);

uint16_t EcsAliveCount(const EcsWorld *world);
uint16_t EcsComponentCount(const EcsWorld *world, uint16_t componentId);

// A component pool. The caller owns a fixed rows array with ECS_MAX_ENTITIES
// rows of rowSize bytes. Pools are plain structs with no allocation.
typedef struct EcsPool {
    uint16_t id;
    void *rows;
    size_t rowSize;
} EcsPool;

void EcsPoolInit(EcsPool *pool, uint16_t id, void *rows, size_t rowSize);

// Adds the component to a live entity and returns a zeroed row. Returns NULL
// when the handle is stale. Adding twice returns the existing row unchanged.
void *EcsAdd(EcsWorld *world, Entity entity, EcsPool *pool);

// Returns the row, or NULL when the entity is dead or lacks the component.
void *EcsGet(const EcsWorld *world, Entity entity, const EcsPool *pool);

bool EcsHas(const EcsWorld *world, Entity entity, uint16_t componentId);

// Removes the component when present. Removing twice is a no-op.
void EcsRemove(EcsWorld *world, Entity entity, const EcsPool *pool);

// Iterates live entities whose mask contains every bit in required.
typedef struct EcsQuery {
    EcsMask required;
    uint16_t next;
} EcsQuery;

static inline EcsQuery EcsQueryAll(EcsMask required) {
    EcsQuery query;
    query.required = required;
    query.next = 0;
    return query;
}

// Fills out and advances the query. Returns false when the scan is done.
// Destroying the yielded entity mid-scan is safe.
bool EcsQueryNext(const EcsWorld *world, EcsQuery *query, Entity *out);

#endif
