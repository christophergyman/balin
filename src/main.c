#include "raylib.h"

#include "engine/input/input.h"
#include "engine/core/log.h"
#include "engine/core/time.h"
#include "engine/core/tuning.h"
#include "game/game.h"
#include "game/memory.h"
#include "game/systems/systems.h"
#include "game/tuning_keys.h"

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
    InputInit();

    InitWindow(WINDOW_W, WINDOW_H, "Balin");

    // ADR-018 lookup order: the executable folder first, then the repo working
    // directory. Values load before GameInit so systems read them at boot.
    TuningInit(TuningSpecs, TUNE_COUNT, "assets/tuning.txt", GetApplicationDirectory());

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

        // One sample per rendered frame, before the fixed tick loop, per
        // ADR-004. Frames that owe no tick still latch their edges.
        InputPollRaylib();

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
        TuningUpdate();
    }

    LOGI(LOG_CAT_BOOT, "Balin stopping");
    LogShutdown();

    GameShutdown();
    CloseWindow();
    return 0;
}
