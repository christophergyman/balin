#include "raylib.h"

#include "engine/log.h"
#include "game/game.h"

#define WINDOW_W 1280
#define WINDOW_H 720

int main(void) {
    LogInit("assets/log.cfg", "output/logs");
    LOGI(LOG_CAT_BOOT, "Balin starting");

    InitWindow(WINDOW_W, WINDOW_H, "Balin");
    SetTargetFPS(60);

    GameInit();

    while (!WindowShouldClose()) {
        GameUpdate();

        BeginDrawing();
        GameDraw();
        EndDrawing();

        LogFlush();
    }

    LOGI(LOG_CAT_BOOT, "Balin stopping");
    LogShutdown();

    GameShutdown();
    CloseWindow();
    return 0;
}
