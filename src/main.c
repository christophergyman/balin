#include "raylib.h"

#define WINDOW_W 1280
#define WINDOW_H 720

int main(void) {
    // Camera
    Camera3D camera = {
        .position = {10.0f, 10.0f, 10.0f},
        .target = {0.0f, 0.0f, 0.0f},
        .up = {0.0f, 1.0f, 0.0f},
        .fovy = 45.0f,
        .projection = CAMERA_PERSPECTIVE,
    };

    InitWindow(WINDOW_W, WINDOW_H, "Balin");
    SetTargetFPS(120);

    while (!WindowShouldClose()) {
        UpdateCamera(&camera, CAMERA_ORBITAL);

        BeginDrawing();
        ClearBackground(BLACK);

        BeginMode3D(camera);
        DrawGrid(20, 1.0f);
        DrawCube((Vector3){0.0f, 0.5f, 0.0f}, 1.0f, 1.0f, 1.0f, BLUE);
        DrawCubeWires((Vector3){0.0f, 0.5f, 0.0f}, 1.0f, 1.0f, 1.0f, WHITE);
        EndMode3D();

        DrawText("Balin 3D", 10, 10, 20, WHITE);

        EndDrawing();
    }
    CloseWindow();

    return 0;
}
