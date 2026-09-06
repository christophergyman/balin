#include "raylib.h"

#include "config.h"
#include "player.h"

// Tile (col, row) fills the cell [col*TILE_SIZE, (col+1)*TILE_SIZE] in x and z.
static Vector3 TileCenter(int col, int row) {
    return (Vector3){
        (col + 0.5f) * TILE_SIZE,
        0.0f,
        (row + 0.5f) * TILE_SIZE,
    };
}

int main(void) {
    // Test grid: floor is y=0 everywhere, this marks solid wall tiles
    static bool solid[GRID_COLS][GRID_COLS] = {0};
    solid[0][0] = true; // the demo wall tile

    // Player starts three tiles down the corridor, looking toward the wall
    Player player;
    InitPlayer(&player, (Vector3){2.5f * TILE_SIZE, 0.0f, 5.5f * TILE_SIZE});

    Camera3D camera = {
        .up = {0.0f, 1.0f, 0.0f},
        .fovy = 45.0f,
        .projection = CAMERA_PERSPECTIVE,
    };

    InitWindow(WINDOW_W, WINDOW_H, "Balin");
    SetTargetFPS(120);
    DisableCursor(); // lock and hide the mouse for first person look

    while (!WindowShouldClose()) {
        // Q: quit
        if (IsKeyPressed(KEY_Q)) {
            break;
        }

        UpdatePlayer(&player, &camera, GetFrameTime(), solid);

        BeginDrawing();
        ClearBackground(BLACK);

        BeginMode3D(camera);
        DrawGrid(GRID_COLS * 2, (float)TILE_SIZE);

        // Solid wall tile, full cell size
        Vector3 wallCenter = TileCenter(0, 0);
        DrawCube(
            (Vector3){wallCenter.x, WALL_HEIGHT / 2.0f, wallCenter.z},
            TILE_SIZE, WALL_HEIGHT, TILE_SIZE, GRAY);
        DrawCubeWires(
            (Vector3){wallCenter.x, WALL_HEIGHT / 2.0f, wallCenter.z},
            TILE_SIZE, WALL_HEIGHT, TILE_SIZE, WHITE);

        EndMode3D();

        DrawText("Balin 3D - WASD move, mouse look, space jump, Q quit", 10, 10, 20, WHITE);

        EndDrawing();
    }
    CloseWindow();

    return 0;
}
