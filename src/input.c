#include "input.h"

#include <raylib.h>
#include <stdio.h>
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

// mouse position tracking and debug printing
void TrackMouse(void) {
    static Vector2 lastMouse = { -1.0f, -1.0f };

    Vector2 mouse = GetMousePosition();

    // Print only when the mouse moves so the terminal stays readable
    if (mouse.x != lastMouse.x || mouse.y != lastMouse.y) {
        int tileCol = (int)(mouse.x / TILE_SIZE);
        int tileRow = (int)(mouse.y / TILE_SIZE);
        printf("Mouse: pixels (x: %.1f, y: %.1f) tile (col: %d, row: %d)\n",
               mouse.x, mouse.y, tileCol, tileRow);
        lastMouse = mouse;
    }
}
