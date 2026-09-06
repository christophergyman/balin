#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "raylib.h"

#include "config.h"
#include "combat.h"
#include "entity.h"
#include "input.h"
#include "types.h"
#include "world.h"

// MAIN
int main(void) {
    // World Layer
    WorldTile worldArray[ROWS][COLS] = {0};
    worldArray[0][1].tileType = WALL;

    // Entity Layer
    Entity *entityArray = malloc(MAX_ENTITIES * sizeof(Entity));
    if (entityArray == NULL) {
        printf("Failed to allocate entity array\n");
        return 1;
    }
    int entityCount = InitEntityArray(entityArray);

    // Mouse Layer
    Vector2 mousePos = { -1.0f, -1.0f };
    Position mouseTile = { .posX = -1, .posY = -1 };

    // HUD Layer
    int killMessageTimer = 0;
    bool enemyWasAlive = true;

    // Game loop
    InitWindow(WINDOW_W, WINDOW_H, "Balin");
    SetTargetFPS(120);

    while (!WindowShouldClose()) {
        PlayerMovement(entityArray, entityCount, worldArray);
        TrackMouse(&mousePos, &mouseTile);
        HandleCombat(entityArray, entityCount, mouseTile);

        // HUD: start the message when the enemy dies this frame
        bool enemyAlive = false;
        for (int i = 0; i < entityCount; i++) {
            if (entityArray[i].name != NULL && strcmp(entityArray[i].name, "enemy") == 0 &&
                entityArray[i].entityStats.health > 0) {
                enemyAlive = true;
                break;
            }
        }
        if (enemyWasAlive && !enemyAlive) {
            killMessageTimer = 120; // 1 second at 120 fps
        }
        enemyWasAlive = enemyAlive;
        if (killMessageTimer > 0) {
            killMessageTimer--;
        }

        BeginDrawing();
        ClearBackground(BLACK);

        DrawWorldTiles(worldArray);
        DrawWorldEntities(entityArray, entityCount);
        DrawHud(entityArray, entityCount, killMessageTimer);
        DebugGrid();

        EndDrawing();
    }
    CloseWindow();

    free(entityArray);
    return 0;
}
