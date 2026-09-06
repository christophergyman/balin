#ifndef INPUT_H
#define INPUT_H

#include <raylib.h>

#include "types.h"
#include "config.h"

void PlayerMovement(Entity *entityArray, int entityCount, WorldTile worldArray[ROWS][COLS]);
void TrackMouse(Vector2 *mousePos, Position *mouseTile);

#endif
