#ifndef FILES_H
#define FILES_H

#include "config.h"
#include "types.h"

// File names inside the data directory, which lives next to the executable
#define DATA_DIR_NAME "data"
#define WORLD_FILE "world.txt"
#define ENTITY_FILE "entities.txt"

void LoadWorld(WorldTile worldArray[ROWS][COLS]);
void SaveWorld(WorldTile worldArray[ROWS][COLS]);
int LoadEntities(Entity *entityArray);
void SaveEntities(Entity *entityArray, int entityCount);

#endif
