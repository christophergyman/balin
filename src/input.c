#include "input.h"

#include <raylib.h>
#include <string.h>

#include "world.h"

// player movement
void PlayerMovement(Entity *entityArray, int entityCount, WorldTile worldArray[ROWS][COLS]) {
    for (int i = 0; i < entityCount; i++) {
        if (entityArray[i].name == NULL || strcmp(entityArray[i].name, "player") != 0) {
            continue;
        }

        Position *pos = &entityArray[i].position;

        if (IsKeyPressed(KEY_W) && CanMoveTo(worldArray, pos->posX, pos->posY - 1)) {
            pos->posY -= 1;
        }
        if (IsKeyPressed(KEY_S) && CanMoveTo(worldArray, pos->posX, pos->posY + 1)) {
            pos->posY += 1;
        }
        if (IsKeyPressed(KEY_A) && CanMoveTo(worldArray, pos->posX - 1, pos->posY)) {
            pos->posX -= 1;
        }
        if (IsKeyPressed(KEY_D) && CanMoveTo(worldArray, pos->posX + 1, pos->posY)) {
            pos->posX += 1;
        }
    }
}
