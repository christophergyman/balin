#include "raylib.h"

#include "game/game.h"

#define WINDOW_W 1280
#define WINDOW_H 720

int main(void) {
    InitWindow(WINDOW_W, WINDOW_H, "Balin");
    SetTargetFPS(60);

    GameInit();

    while (!WindowShouldClose()) {
        GameUpdate();

        BeginDrawing();
        GameDraw();
        EndDrawing();
    }

    GameShutdown();
    CloseWindow();
    return 0;
}
