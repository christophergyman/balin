#include "engine/input/input.h"

#include <stddef.h>
#include <string.h>

// The buffer policy: attack and dash only, per ADR-010.
static const Action BUFFERED_ACTIONS[] = {
    ACTION_ATTACK,
    ACTION_DASH,
};

_Static_assert(ACTION_COUNT <= 32, "InputState packs actions into 32 bits");
_Static_assert(INPUT_BUFFER_TICKS == 6, "100 ms at 60 Hz, per ADR-010");

static InputState state;
static uint32_t pendingPressed;
static uint32_t pendingReleased;
static uint32_t sampledHeld;
static uint8_t bufferTicks[ACTION_COUNT];

void InputInit(void) {
    memset(&state, 0, sizeof(state));
    pendingPressed = 0;
    pendingReleased = 0;
    sampledHeld = 0;
    memset(bufferTicks, 0, sizeof(bufferTicks));
}

void InputBeginFrame(const InputFrame *frame) {
    // Edges latch until a tick runs, so no press is lost when a frame owes no
    // tick. Held is the latest sample; edges are the union since the last tick.
    pendingPressed |= frame->pressed;
    pendingReleased |= frame->released;
    sampledHeld = frame->held;
}

void InputBeginTick(void) {
    state.pressed = pendingPressed;
    state.released = pendingReleased;
    state.held = sampledHeld;

    // Age the buffers, then capture this tick's press. The press is valid on
    // this tick and the five that follow.
    for (size_t index = 0; index < sizeof(BUFFERED_ACTIONS) / sizeof(BUFFERED_ACTIONS[0]); index++) {
        Action action = BUFFERED_ACTIONS[index];
        if (bufferTicks[action] > 0) {
            bufferTicks[action]--;
        }
        if (state.pressed & ACTION_BIT(action)) {
            bufferTicks[action] = INPUT_BUFFER_TICKS;
        }
    }

    pendingPressed = 0;
    pendingReleased = 0;
}

const InputState *InputStateGet(void) {
    return &state;
}

// Rejects invalid ids so no accessor can index out of bounds.
static bool ActionInRange(Action action) {
    return (uint32_t)action < (uint32_t)ACTION_COUNT;
}

bool InputHeld(Action action) {
    if (!ActionInRange(action)) {
        return false;
    }
    return (state.held & ACTION_BIT(action)) != 0;
}

bool InputPressed(Action action) {
    if (!ActionInRange(action)) {
        return false;
    }
    return (state.pressed & ACTION_BIT(action)) != 0;
}

bool InputReleased(Action action) {
    if (!ActionInRange(action)) {
        return false;
    }
    return (state.released & ACTION_BIT(action)) != 0;
}

bool InputBuffered(Action action) {
    if (!ActionInRange(action)) {
        return false;
    }
    return bufferTicks[action] > 0;
}

bool InputConsumeBuffered(Action action) {
    if (!ActionInRange(action) || bufferTicks[action] == 0) {
        return false;
    }

    bufferTicks[action] = 0;
    return true;
}

void InputDropPending(void) {
    memset(&state, 0, sizeof(state));
    memset(bufferTicks, 0, sizeof(bufferTicks));
    pendingPressed = 0;
    pendingReleased = 0;
    sampledHeld = 0;
}
