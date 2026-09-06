#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// DEFINITIONS
#define DEBUG_GRID true
#define WINDOW_W 1280
#define WINDOW_H 720
#define TILE_SIZE 32
#define COLS 60
#define ROWS 34

#define MAX_ENTITIES 5

// STRUCTS
enum TileType {
    AIR,
    WALL,
};

typedef struct WorldTile {
    enum TileType tileType;
} WorldTile;

typedef struct Position {
    int posX;
    int posY;
} Position;

typedef struct EntityStats {
    int health;
    int strength;
    int faith;
    int money;
} EntityStats;

typedef struct Entity {
    char* name;
    struct Position position;
    struct EntityStats entityStats;
} Entity;

// HELPER
void DebugGrid() {
    // Draw grid
    for (int r = 0; r < ROWS; r++) {
        int y = r * TILE_SIZE;
        DrawLine(0, y, WINDOW_W, y, WHITE);
    }
    for (int c = 0; c < COLS; c++) {
        int x = c * TILE_SIZE;
        DrawLine(x, 0, x, WINDOW_H, WHITE);
    }
}

void DrawWorldTiles(WorldTile worldArray[ROWS][COLS]) {
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            if (worldArray[r][c].tileType == WALL) {
                DrawRectangle(c * TILE_SIZE, r * TILE_SIZE, TILE_SIZE, TILE_SIZE, GRAY);
            }
        }
    }
}

int InitEntityArray(Entity *entityArray) {
    entityArray[0] = (Entity){
        .name = "player",
        .position = {
            .posX = 2,
            .posY = 2,
        },
        .entityStats = {
            .health = 100,
            .strength = 99,
            .faith = 99,
            .money = 999
        }
    };

    entityArray[1] = (Entity){
        .name = "enemy",
        .position = {
            .posX = 5,
            .posY = 2,
        },
        .entityStats = {
            .health = 100,
            .strength = 99,
            .faith = 99,
            .money = 999
        }
    };

    return 2;
}

void DrawWorldEntities(Entity *entityArray, int entityCount) {
    for (int i = 0; i < entityCount; i++) {
        if(strcmp(entityArray[i].name, "player")){
            DrawRectangle(
                entityArray[i].position.posX * TILE_SIZE,
                entityArray[i].position.posY * TILE_SIZE,
                TILE_SIZE,
                TILE_SIZE,
                BLUE
            );
        } else {
            DrawRectangle(
                entityArray[i].position.posX * TILE_SIZE,
                entityArray[i].position.posY * TILE_SIZE,
                TILE_SIZE,
                TILE_SIZE,
                PURPLE
            );
        }
    }
}

// player movement
void PlayerMovement(Entity *entityArray, int entityCount) {
    for (int i = 0; i < entityCount; i++) {
        if (entityArray[i].name == NULL || strcmp(entityArray[i].name, "player") != 0) {
            continue;
        }

        if (IsKeyPressed(KEY_W)) {
            entityArray[i].position.posY -= 1;
        }
        if (IsKeyPressed(KEY_S)) {
            entityArray[i].position.posY += 1;
        }
        if (IsKeyPressed(KEY_A)) {
            entityArray[i].position.posX -= 1;
        }
        if (IsKeyPressed(KEY_D)) {
            entityArray[i].position.posX += 1;
        }
    }
}

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

    // Game loop
    InitWindow(WINDOW_W, WINDOW_H, "Balin");
    SetTargetFPS(120);

    while (!WindowShouldClose()) {
        PlayerMovement(entityArray, entityCount);

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