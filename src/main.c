#include "raylib.h"
#include <stdio.h>
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
enum TileType{
    AIR,
    WALL,
};

typedef struct WorldTile{
    enum TileType tileType;
} WorldTile;

typedef struct Position {
    int posX;
    int posY;
} Position;

typedef struct EntityStats{
    int health;
    int strength;
    int faith;
    int money;
} EntityStats;

typedef struct Entity{
    char* name;
    struct Position positon;
    struct EntityStats entityStats;
} Entity;


// HELPER
void DebugGrid(){
        // Draw grid
        for (int r=0; r<ROWS; r++){
            int y = r * TILE_SIZE;
            DrawLine(0, y, WINDOW_W, y, WHITE);
        }
        for (int c = 0; c < COLS; c++){
            int x = c * TILE_SIZE;
            DrawLine(x, 0, x, WINDOW_H, WHITE);
        }
}

void DrawWorldTiles(WorldTile worldArray[ROWS][COLS]){
    for(int r=0; r < ROWS; r++){
        for(int c=0; c < ROWS; c++){
            if(worldArray[r][c].tileType == WALL){
                DrawRectangle(c*TILE_SIZE, r*TILE_SIZE, TILE_SIZE, TILE_SIZE, GRAY);
            }
        }
    }
}

void InitEntityArray(Entity * entityArray[MAX_ENTITIES], int entityCount){
    Entity player = {
        .name = "player",
        .positon = {
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
    entityArray[entityCount++] = &player;

    Entity enemy = {
        .name = "enemy",
        .positon = {
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
    entityArray[entityCount++] = &enemy;
}

void DrawWorldEntities(Entity * entityArray){
    for(int i=0; i<MAX_ENTITIES; i++){
        DrawRectangle(
            entityArray[i].positon.posX,
            entityArray[i].positon.posY,
            TILE_SIZE,
            TILE_SIZE,
            PURPLE
        );
    }
}

// player movement
void PlayerMovement(){
    if(IsKeyPressed(KEY_W)){
    }
}


// MAIN
int main(void){
    // World Layer
    WorldTile worldArray[ROWS][COLS] = {0};
    worldArray[0][1].tileType = WALL;

    // Entity Layer
    Entity * entityArray[MAX_ENTITIES] = {0};
    int entityCount = 0;
    InitEntityArray(entityArray, entityCount);


    // Game loop
    InitWindow(WINDOW_W, WINDOW_H, "Balin");
    SetTargetFPS(120);

    while (!WindowShouldClose()){
        BeginDrawing();
        ClearBackground(BLACK);

        DrawWorldTiles(worldArray);
        DrawWorldEntities(entityArray);
        DebugGrid();

        EndDrawing();
    }
    CloseWindow();
}