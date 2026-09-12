#include "tools/overlay.h"

#include <stdio.h>

#include "raylib.h"

#define OVERLAY_MARGIN 10
#define OVERLAY_WIDTH 300
#define OVERLAY_LINE_HEIGHT 24
#define OVERLAY_LINE_COUNT 6
#define OVERLAY_FONT_SIZE 20

// Last entry 0 means uncapped. Vsync is cleared for caps above the 60 FPS floor.
static const int fpsCaps[] = { 30, 60, 120, 0 };
static int fpsCapIndex = 3;
static bool overlayVisible = true;
static bool vsyncCleared = false;

void OverlayHandleKeys(void) {
    if (IsKeyPressed(KEY_F3)) {
        overlayVisible = !overlayVisible;
    }

    if (!IsKeyPressed(KEY_F1)) {
        return;
    }

    fpsCapIndex = (fpsCapIndex + 1) % (int)(sizeof(fpsCaps) / sizeof(fpsCaps[0]));
    int cap = fpsCaps[fpsCapIndex];
    SetTargetFPS(cap);

    if (cap == 0 || cap > TICK_RATE) {
        ClearWindowState(FLAG_VSYNC_HINT);
        vsyncCleared = true;
    } else if (vsyncCleared) {
        SetWindowState(FLAG_VSYNC_HINT);
        vsyncCleared = false;
    }
}

void OverlayDraw(const Clock *clock) {
    if (!overlayVisible) {
        return;
    }

    int cap = fpsCaps[fpsCapIndex];
    char capText[16];
    if (cap == 0) {
        snprintf(capText, sizeof(capText), "uncapped");
    } else {
        snprintf(capText, sizeof(capText), "%d", cap);
    }

    int x = OVERLAY_MARGIN;
    int y = OVERLAY_MARGIN;

    DrawRectangle(
        x - 4,
        y - 4,
        OVERLAY_WIDTH,
        OVERLAY_LINE_COUNT * OVERLAY_LINE_HEIGHT + 8,
        Fade(BLACK, 0.65f)
    );

    DrawText(TextFormat("fps %d  cap %s", GetFPS(), capText), x, y, OVERLAY_FONT_SIZE, GREEN);
    y += OVERLAY_LINE_HEIGHT;

    DrawText(TextFormat("frame %.2f ms", GetFrameTime() * 1000.0f), x, y, OVERLAY_FONT_SIZE, GREEN);
    y += OVERLAY_LINE_HEIGHT;

    DrawText(TextFormat("tick %llu", (unsigned long long)clock->tick), x, y, OVERLAY_FONT_SIZE, GREEN);
    y += OVERLAY_LINE_HEIGHT;

    DrawText(
        TextFormat("acc %.2f ms  steps %d", clock->accumulator * 1000.0, clock->stepsLastFrame),
        x, y, OVERLAY_FONT_SIZE, GREEN
    );
    y += OVERLAY_LINE_HEIGHT;

    DrawText(TextFormat("alpha %.2f", ClockAlpha(clock)), x, y, OVERLAY_FONT_SIZE, GREEN);
    y += OVERLAY_LINE_HEIGHT;

    DrawText("F1 cap  F3 overlay", x, y, OVERLAY_FONT_SIZE, LIGHTGRAY);
}
