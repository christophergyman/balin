#ifndef WORLD_FILE_H
#define WORLD_FILE_H

#include "world.h"

// Binary format: "BLWT" | u32 version | u32 cols | u32 rows | raw Tile grid
bool LoadWorldTiles(World *world);
void SaveWorldTiles(const World *world);

#endif
