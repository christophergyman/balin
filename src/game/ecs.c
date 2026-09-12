#include "game/ecs.h"

#include <string.h>

#include "engine/log.h"

// The mask word stores one bit per component id, so the cap is 64.
_Static_assert(ECS_MAX_COMPONENTS <= 64, "EcsMask holds 64 component ids");

static bool SlotInRange(uint16_t index) {
    return index < ECS_MAX_ENTITIES;
}

void EcsInit(EcsWorld *world) {
    ASSERT(world != NULL, LOG_CAT_GAME, "ecs world is null");
    memset(world, 0, sizeof(EcsWorld));
}

Entity EcsCreate(EcsWorld *world) {
    ASSERT(world != NULL, LOG_CAT_GAME, "ecs world is null");

    uint16_t index;
    if (world->freeCount > 0) {
        index = world->freeList[--world->freeCount];
    } else if (world->nextFresh < ECS_MAX_ENTITIES) {
        index = world->nextFresh++;
    } else {
        return ENTITY_INVALID;
    }

    world->alive[index] = true;
    world->mask[index] = 0;
    world->aliveCount++;
    return EntityMake(index, world->generation[index]);
}

bool EcsDestroy(EcsWorld *world, Entity entity) {
    ASSERT(world != NULL, LOG_CAT_GAME, "ecs world is null");

    if (!EcsAlive(world, entity)) {
        return false;
    }

    uint16_t index = EntityIndex(entity);

    // Drop the components now so queries and counts see the death at once.
    // The pool rows stay untouched until the slot is reused.
    for (uint16_t id = 0; id < ECS_MAX_COMPONENTS; id++) {
        if (world->mask[index] & EcsBit(id)) {
            ASSERT(world->componentCount[id] > 0, LOG_CAT_GAME,
                "ecs component %u count underflow", id);
            world->componentCount[id]--;
        }
    }
    world->mask[index] = 0;

    world->alive[index] = false;
    world->aliveCount--;

    ASSERT(world->pendingCount < ECS_MAX_ENTITIES, LOG_CAT_GAME, "ecs pending queue overflow");
    world->pending[world->pendingCount++] = entity;
    return true;
}

void EcsFlush(EcsWorld *world) {
    ASSERT(world != NULL, LOG_CAT_GAME, "ecs world is null");

    for (uint16_t i = 0; i < world->pendingCount; i++) {
        uint16_t index = EntityIndex(world->pending[i]);
        world->generation[index]++;
        world->freeList[world->freeCount++] = index;
    }
    world->pendingCount = 0;
}

bool EcsAlive(const EcsWorld *world, Entity entity) {
    ASSERT(world != NULL, LOG_CAT_GAME, "ecs world is null");

    uint16_t index = EntityIndex(entity);
    if (!SlotInRange(index)) {
        return false;
    }
    return world->alive[index] && world->generation[index] == EntityGeneration(entity);
}

uint16_t EcsAliveCount(const EcsWorld *world) {
    ASSERT(world != NULL, LOG_CAT_GAME, "ecs world is null");
    return world->aliveCount;
}

uint16_t EcsComponentCount(const EcsWorld *world, uint16_t componentId) {
    ASSERT(world != NULL, LOG_CAT_GAME, "ecs world is null");
    ASSERT(componentId < ECS_MAX_COMPONENTS, LOG_CAT_GAME,
        "ecs component id %u out of range", componentId);
    return world->componentCount[componentId];
}

void EcsPoolInit(EcsPool *pool, uint16_t id, void *rows, size_t rowSize) {
    ASSERT(pool != NULL, LOG_CAT_GAME, "ecs pool is null");
    ASSERT(id < ECS_MAX_COMPONENTS, LOG_CAT_GAME, "ecs component id %u out of range", id);
    ASSERT(rows != NULL, LOG_CAT_GAME, "ecs pool %u rows are null", id);
    ASSERT(rowSize > 0, LOG_CAT_GAME, "ecs pool %u row size is zero", id);

    pool->id = id;
    pool->rows = rows;
    pool->rowSize = rowSize;
}

void *EcsAdd(EcsWorld *world, Entity entity, EcsPool *pool) {
    ASSERT(world != NULL, LOG_CAT_GAME, "ecs world is null");
    ASSERT(pool != NULL, LOG_CAT_GAME, "ecs pool is null");
    ASSERT(pool->id < ECS_MAX_COMPONENTS, LOG_CAT_GAME,
        "ecs component id %u out of range", pool->id);

    if (!EcsAlive(world, entity)) {
        return NULL;
    }

    uint16_t index = EntityIndex(entity);
    EcsMask bit = EcsBit(pool->id);
    unsigned char *row = (unsigned char *)pool->rows + (size_t)index * pool->rowSize;

    if ((world->mask[index] & bit) == 0) {
        world->mask[index] |= bit;
        world->componentCount[pool->id]++;
        memset(row, 0, pool->rowSize);
    }
    return row;
}

void *EcsGet(const EcsWorld *world, Entity entity, const EcsPool *pool) {
    ASSERT(world != NULL, LOG_CAT_GAME, "ecs world is null");
    ASSERT(pool != NULL, LOG_CAT_GAME, "ecs pool is null");
    ASSERT(pool->id < ECS_MAX_COMPONENTS, LOG_CAT_GAME,
        "ecs component id %u out of range", pool->id);

    if (!EcsHas(world, entity, pool->id)) {
        return NULL;
    }
    return (unsigned char *)pool->rows + (size_t)EntityIndex(entity) * pool->rowSize;
}

bool EcsHas(const EcsWorld *world, Entity entity, uint16_t componentId) {
    ASSERT(world != NULL, LOG_CAT_GAME, "ecs world is null");
    ASSERT(componentId < ECS_MAX_COMPONENTS, LOG_CAT_GAME,
        "ecs component id %u out of range", componentId);

    if (!EcsAlive(world, entity)) {
        return false;
    }
    return (world->mask[EntityIndex(entity)] & EcsBit(componentId)) != 0;
}

void EcsRemove(EcsWorld *world, Entity entity, const EcsPool *pool) {
    ASSERT(world != NULL, LOG_CAT_GAME, "ecs world is null");
    ASSERT(pool != NULL, LOG_CAT_GAME, "ecs pool is null");
    ASSERT(pool->id < ECS_MAX_COMPONENTS, LOG_CAT_GAME,
        "ecs component id %u out of range", pool->id);

    if (!EcsHas(world, entity, pool->id)) {
        return;
    }

    uint16_t index = EntityIndex(entity);
    world->mask[index] &= ~EcsBit(pool->id);
    ASSERT(world->componentCount[pool->id] > 0, LOG_CAT_GAME,
        "ecs component %u count underflow", pool->id);
    world->componentCount[pool->id]--;
}

bool EcsQueryNext(const EcsWorld *world, EcsQuery *query, Entity *out) {
    ASSERT(world != NULL, LOG_CAT_GAME, "ecs world is null");
    ASSERT(query != NULL, LOG_CAT_GAME, "ecs query is null");
    ASSERT(out != NULL, LOG_CAT_GAME, "ecs query out is null");

    while (query->next < ECS_MAX_ENTITIES) {
        uint16_t index = query->next++;
        if (!world->alive[index]) {
            continue;
        }
        if ((world->mask[index] & query->required) != query->required) {
            continue;
        }
        *out = EntityMake(index, world->generation[index]);
        return true;
    }
    return false;
}
