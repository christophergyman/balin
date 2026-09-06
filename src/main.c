#include "raylib.h"

#include "config.h"

int main(void) {
    // Camera: first person, eye height, standing at a tile corner offset
    Camera3D camera = {
        .position = {0.0f, PLAYER_EYE, TILE_SIZE * 2.0f},
        .target = {0.0f, PLAYER_EYE, 0.0f},
        .up = {0.0f, 1.0f, 0.0f},
        .fovy = 45.0f,
        .projection = CAMERA_PERSPECTIVE,
    };

    InitWindow(WINDOW_W, WINDOW_H, "Balin");
    SetTargetFPS(120);
    DisableCursor(); // lock and hide the mouse for first person look

    // Demo tile: fills grid cell (col: 0, row: 0), with a wall on its far edge.
    // Convention: tile (col, row) fills the cell between grid lines.
    int tileCol = 0;
    int tileRow = 0;
    Vector3 tileCenter = {
        (tileCol + 0.5f) * TILE_SIZE,
        0.0f,
        (tileRow + 0.5f) * TILE_SIZE,
    };

    while (!WindowShouldClose()) {
        // Q: quit
        if (IsKeyPressed(KEY_Q)) {
            break;
        }

        UpdateCamera(&camera, CAMERA_FIRST_PERSON);

        BeginDrawing();
        ClearBackground(BLACK);

        BeginMode3D(camera);
        DrawGrid(20, (float)TILE_SIZE);

        // Floor slab of one tile
        DrawCube(
            (Vector3){tileCenter.x, 0.0f, tileCenter.z},
            TILE_SIZE, 0.1f, TILE_SIZE, DARKBROWN);
        DrawCubeWires(
            (Vector3){tileCenter.x, 0.0f, tileCenter.z},
            TILE_SIZE, 0.1f, TILE_SIZE, WHITE);

        // Wall on the far edge of the tile, one tile wide, WALL_HEIGHT tall
        DrawCube(
            (Vector3){tileCenter.x, WALL_HEIGHT / 2.0f, tileCenter.z - TILE_SIZE / 2.0f},
            TILE_SIZE, WALL_HEIGHT, 0.2f, GRAY);
        DrawCubeWires(
            (Vector3){tileCenter.x, WALL_HEIGHT / 2.0f, tileCenter.z - TILE_SIZE / 2.0f},
            TILE_SIZE, WALL_HEIGHT, 0.2f, WHITE);

        EndMode3D();

        DrawText("Balin 3D - WASD move, mouse look, space jump", 10, 10, 20, WHITE);

        EndDrawing();
    }
    CloseWindow();

    return 0;
}
