#include "input.h"

#include <raylib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "files.h"
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
void TrackMouse(Vector2 *mousePos, Position *mouseTile) {
    static Vector2 lastMouse = { -1.0f, -1.0f };

    Vector2 mouse = GetMousePosition();

    // Store latest mouse state for main
    mousePos->x = mouse.x;
    mousePos->y = mouse.y;
    mouseTile->posX = (int)(mouse.x / TILE_SIZE);
    mouseTile->posY = (int)(mouse.y / TILE_SIZE);

    // Print only when the mouse moves so the terminal stays readable
    if (mouse.x != lastMouse.x || mouse.y != lastMouse.y) {
        printf("Mouse: pixels (x: %.1f, y: %.1f) tile (col: %d, row: %d)\n",
               mouse.x, mouse.y, mouseTile->posX, mouseTile->posY);
        lastMouse = mouse;
    }
}

// Editor mode: number keys place or remove things at the cursor tile
void HandleEditor(Entity *entityArray, int *entityCount, WorldTile worldArray[ROWS][COLS], Position mouseTile) {
    bool inBounds = mouseTile.posX >= 0 && mouseTile.posX < COLS &&
                    mouseTile.posY >= 0 && mouseTile.posY < ROWS;

    // 1: place wall
    if (IsKeyPressed(KEY_ONE) && inBounds) {
        worldArray[mouseTile.posY][mouseTile.posX].tileType = WALL;
        SaveWorld(worldArray);
        printf("Placed WALL at tile (col: %d, row: %d)\n", mouseTile.posX, mouseTile.posY);
    }

    // 2: erase tile to air
    if (IsKeyPressed(KEY_TWO) && inBounds) {
        worldArray[mouseTile.posY][mouseTile.posX].tileType = AIR;
        SaveWorld(worldArray);
        printf("Placed AIR at tile (col: %d, row: %d)\n", mouseTile.posX, mouseTile.posY);
    }

    // 3: place enemy entity at cursor
    if (IsKeyPressed(KEY_THREE) && inBounds) {
        if (*entityCount >= MAX_ENTITIES) {
            printf("Entity limit reached (%d)\n", MAX_ENTITIES);
            return;
        }

        Entity *entity = &entityArray[*entityCount];
        entity->name = "enemy";
        entity->position = mouseTile;
        entity->entityStats = (EntityStats){
            .health = 100,
            .strength = 5,
            .faith = 5,
            .money = 0,
        };
        (*entityCount)++;
        SaveEntities(entityArray, *entityCount);
        printf("Placed enemy at tile (col: %d, row: %d)\n", mouseTile.posX, mouseTile.posY);
    }

    // 4: remove non-player entity at cursor
    if (IsKeyPressed(KEY_FOUR) && inBounds) {
        for (int i = 0; i < *entityCount; i++) {
            if (entityArray[i].name == NULL ||
                strcmp(entityArray[i].name, "player") == 0 ||
                entityArray[i].position.posX != mouseTile.posX ||
                entityArray[i].position.posY != mouseTile.posY) {
                continue;
            }

            printf("Removed %s at tile (col: %d, row: %d)\n",
                   entityArray[i].name, mouseTile.posX, mouseTile.posY);

            // Shift the remaining entities down one slot
            for (int j = i; j < *entityCount - 1; j++) {
                entityArray[j] = entityArray[j + 1];
            }
            (*entityCount)--;
            SaveEntities(entityArray, *entityCount);
            return;
        }
    }
}
