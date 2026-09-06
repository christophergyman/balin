#ifndef ENTITY_H
#define ENTITY_H

#include "types.h"

int InitEntityArray(Entity *entityArray);
void DrawWorldEntities(Entity *entityArray, int entityCount);
void DrawHud(Entity *entityArray, int entityCount, int killMessageTimer);

#endif
