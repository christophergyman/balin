#ifndef TIME_H
#define TIME_H

#include <stdint.h>

// Fixed 60 Hz simulation with render interpolation, per ADR-004.
#define TICK_RATE 60
#define TICK_DT (1.0 / 60.0)
#define MAX_STEPS_PER_FRAME 5

typedef struct Clock {
    double accumulator;   // Seconds owed to the simulation.
    uint64_t tick;        // Ticks completed since boot.
    int stepsLastFrame;   // Steps run on the last ClockBeginFrame, for the overlay.
} Clock;

void ClockInit(Clock *clock);

// Adds the frame time to the accumulator and returns how many fixed steps to
// run this frame (0 to MAX_STEPS_PER_FRAME). Excess time beyond the cap is
// dropped so the simulation never chases a hitch.
int ClockBeginFrame(Clock *clock, double frameSeconds);

// Consumes one step and advances the tick counter.
void ClockBeginTick(Clock *clock);

// Fraction of a tick left in the accumulator, in [0, 1). Used to interpolate
// rendered positions between the previous and current tick.
float ClockAlpha(const Clock *clock);

// Clears owed time. Pause states call this so resume does not catch up.
void ClockDropPending(Clock *clock);

#endif
