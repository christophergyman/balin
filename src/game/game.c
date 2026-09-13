#include "game/game.h"

#include "raylib.h"

#include "engine/time.h"
#include "game/memory.h"

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

void GameWriteSnapshot(FILE *out) {
    fprintf(out, "ecs_alive=%u\n", (unsigned)EcsAliveCount(&ecsWorld));
    fprintf(out, "ecs_capacity=%d\n", ECS_MAX_ENTITIES);

    const char *names[] = { "asset", "level", "run", "frame" };
    Arena *arenas[] = {
        GameArenaAsset(),
        GameArenaLevel(),
        GameArenaRun(),
        GameArenaFrame(),
    };

    for (size_t index = 0; index < sizeof(arenas) / sizeof(arenas[0]); index++) {
        fprintf(out, "arena_%s_used=%zu\n", names[index], ArenaUsed(arenas[index]));
        fprintf(out, "arena_%s_capacity=%zu\n", names[index], ArenaCapacity(arenas[index]));
    }

    fprintf(out, "player_probe_x=%.2f\n", probeX);
    fprintf(out, "player_probe_previous_x=%.2f\n", probePreviousX);

    // No AI state yet. SystemAI is a stub until BAL-26.
    fprintf(out, "ai_state=absent\n");
}
