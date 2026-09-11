#include "raylib.h"

#define WINDOW_W 1280
#define WINDOW_H 720
#define SQUARE_SIZE 64

int main(void) {
    InitWindow(WINDOW_W, WINDOW_H, "Balin");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);

        DrawRectangle(
            (WINDOW_W - SQUARE_SIZE) / 2,
            (WINDOW_H - SQUARE_SIZE) / 2,
            SQUARE_SIZE,
            SQUARE_SIZE,
            WHITE
        );

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
