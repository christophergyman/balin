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


// STRUCTS
enum TileType{
    AIR,
    WALL,
};
struct WorldTile{
    enum TileType tileType;
};

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

void DrawWorldTiles(struct WorldTile worldArray[ROWS][COLS]){
    for(int r=0; r < ROWS; r++){
        for(int c=0; c < ROWS; c++){
            if(worldArray[r][c].tileType == WALL){
                DrawRectangle(c*TILE_SIZE, r*TILE_SIZE, TILE_SIZE, TILE_SIZE, GRAY);
            }
        }
    }
}

// MAIN
int main(void){
    struct WorldTile worldArray[ROWS][COLS] = {0};
    worldArray[0][1].tileType = WALL;

    InitWindow(WINDOW_W, WINDOW_H, "Balin");
    SetTargetFPS(120);

    while (!WindowShouldClose()){
        BeginDrawing();
        ClearBackground(BLACK);

        DrawWorldTiles(worldArray);
        DebugGrid();

        EndDrawing();
    }
    CloseWindow();
}