#include "game/game.h"

#include "raylib.h"

#include "engine/time.h"

#define PROBE_SIZE 64
#define PROBE_SPEED 120.0f // Pixels per second, so 2 pixels per tick at 60 Hz.

// Temporary debug probe. Movement runs once per fixed tick. Drawing
// interpolates between ticks with alpha.
static float probeX;
static float probePreviousX;

static EcsWorld ecsWorld;

void GameInit(void) {
    EcsInit(&ecsWorld);

    probeX = 0.0f;
    probePreviousX = probeX;
}

void GameProbeTick(void) {
    probePreviousX = probeX;
    probeX += PROBE_SPEED * (float)TICK_DT;

    float limit = (float)GetScreenWidth() - PROBE_SIZE;
    if (probeX > limit) {
        probeX = 0.0f;
        probePreviousX = probeX; // Do not interpolate across the reset.
    }
}

void GameDraw(float alpha) {
    ClearBackground(BLACK);

    float x = probePreviousX + (probeX - probePreviousX) * alpha;
    float y = ((float)GetScreenHeight() - PROBE_SIZE) / 2.0f;

    DrawRectangle((int)x, (int)y, PROBE_SIZE, PROBE_SIZE, WHITE);
}

void GameShutdown(void) {
}

EcsWorld *GameEcs(void) {
    return &ecsWorld;
}
