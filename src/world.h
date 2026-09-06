#ifndef WORLD_H
#define WORLD_H

#include "types.h"
#include "config.h"
#include <stdbool.h>

void DebugGrid(void);
void DrawWorldTiles(WorldTile worldArray[ROWS][COLS]);
bool CanMoveTo(WorldTile worldArray[ROWS][COLS], int col, int row);

#endif
