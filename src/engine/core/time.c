#include "engine/core/time.h"

void ClockInit(Clock *clock) {
    clock->accumulator = 0.0;
    clock->tick = 0;
    clock->stepsLastFrame = 0;
}

int ClockBeginFrame(Clock *clock, double frameSeconds) {
    if (frameSeconds < 0.0) {
        frameSeconds = 0.0;
    }

    clock->accumulator += frameSeconds;

    // ADR-004: cap catch-up at five steps. The excess is dropped.
    double maxOwed = (double)MAX_STEPS_PER_FRAME * TICK_DT;
    if (clock->accumulator > maxOwed) {
        clock->accumulator = maxOwed;
    }

    // The epsilon absorbs double rounding at exact tick boundaries.
    clock->stepsLastFrame = (int)((clock->accumulator / TICK_DT) + 1e-9);
    return clock->stepsLastFrame;
}

void ClockBeginTick(Clock *clock) {
    clock->accumulator -= TICK_DT;
    if (clock->accumulator < 0.0) {
        clock->accumulator = 0.0;
    }

    clock->tick++;
}

float ClockAlpha(const Clock *clock) {
    return (float)(clock->accumulator / TICK_DT);
}

void ClockDropPending(Clock *clock) {
    clock->accumulator = 0.0;
}
