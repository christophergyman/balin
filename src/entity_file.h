#ifndef ENTITY_FILE_H
#define ENTITY_FILE_H

#include "entity.h"

// Binary format:
// "BLEN" | u32 version | u32 count
// then per entity: u8 kind | f32 x | f32 y | f32 z | i32 health
int LoadEntities(Entity *entities);
void SaveEntities(const Entity *entities, int entityCount);

#endif
