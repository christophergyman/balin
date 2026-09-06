#include <stdio.h>
#include <stdlib.h>

#include "raylib.h"

#include "config.h"
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

    // Game loop
    InitWindow(WINDOW_W, WINDOW_H, "Balin");
    SetTargetFPS(120);

    while (!WindowShouldClose()) {
        PlayerMovement(entityArray, entityCount, worldArray);
        TrackMouse(&mousePos, &mouseTile);

        BeginDrawing();
        ClearBackground(BLACK);

        DrawWorldTiles(worldArray);
        DrawWorldEntities(entityArray, entityCount);
        DebugGrid();

        EndDrawing();
    }
    CloseWindow();

    free(entityArray);
    return 0;
}
