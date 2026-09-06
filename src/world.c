#include "world.h"

#include "raylib.h"

#include "config.h"

void DebugGrid(void) {
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

bool CanMoveTo(WorldTile worldArray[ROWS][COLS], int col, int row) {
    // Block movement outside the grid
    if (col < 0 || col >= COLS || row < 0 || row >= ROWS) {
        return false;
    }
    // Block movement into walls
    if (worldArray[row][col].tileType == WALL) {
        return false;
    }
    return true;
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
