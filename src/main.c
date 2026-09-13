#include "raylib.h"

#include "engine/log.h"
#include "engine/time.h"
#include "game/game.h"
#include "game/memory.h"
#include "game/systems.h"

#if BALIN_DEV
#include "tools/overlay.h"
#endif

#define WINDOW_W 1280
#define WINDOW_H 720
#define GAME_RUN_ID 1

// Frame-rate independent loop, per ADR-004. Simulation runs in fixed 1/60 s
// ticks; rendering interpolates with the leftover accumulator fraction.
int main(void) {
    LogInit("assets/log.cfg", "output/logs", "output/bugs");
    LOGI(LOG_CAT_BOOT, "Balin starting");

    GameMemoryInit();

    InitWindow(WINDOW_W, WINDOW_H, "Balin");

    GameInit();
    LogSetSnapshotWriter(GameWriteSnapshot);

    Clock clock;
    ClockInit(&clock);

    while (!WindowShouldClose()) {
#if BALIN_DEV
        OverlayHandleKeys();

        // Dev-only for now. The fatal path still bundles in every build.
        if (IsKeyPressed(KEY_F9)) {
            LogDumpBundle("f9");
        }
#endif

        GameMemoryResetFrame();

        int steps = ClockBeginFrame(&clock, GetFrameTime());
        for (int i = 0; i < steps; i++) {
            ClockBeginTick(&clock);
            LogSetContext(clock.tick, GAME_RUN_ID);
            GameTick();
        }

        BeginDrawing();
        GameDraw(ClockAlpha(&clock));
#if BALIN_DEV
        OverlayDraw(&clock);
#endif
        EndDrawing();

        LogFlush();
    }

    LOGI(LOG_CAT_BOOT, "Balin stopping");
    LogShutdown();

    GameShutdown();
    CloseWindow();
    return 0;
}
