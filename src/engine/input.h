#ifndef INPUT_H
#define INPUT_H

#include <stdbool.h>
#include <stdint.h>

// Action map with input buffers, per ADR-010. The world uses keyboard actions
// only. Mouse input is a separate path for the bag and editor screens.
typedef enum Action {
    ACTION_MOVE_UP = 0,
    ACTION_MOVE_DOWN,
    ACTION_MOVE_LEFT,
    ACTION_MOVE_RIGHT,
    ACTION_ATTACK,
    ACTION_DASH,
    ACTION_SMITE,
    ACTION_BAG,
    ACTION_INTERACT,
    ACTION_COUNT
} Action;

#define ACTION_BIT(action) (1u << (uint32_t)(action))

// 100 ms at 60 Hz, per ADR-010.
#define INPUT_BUFFER_TICKS 6

// Per-tick snapshot. pressed and released are edges: each is true for exactly
// one tick, so a press is never seen twice by the ticks of one frame.
typedef struct InputState {
    uint32_t held;
    uint32_t pressed;
    uint32_t released;
} InputState;

// One rendered frame of mapped actions, sampled from the platform.
typedef struct InputFrame {
    uint32_t held;
    uint32_t pressed;
    uint32_t released;
} InputFrame;

void InputInit(void);

// Latches the frame edges until a tick consumes them. Call once per rendered
// frame, before the fixed tick loop, per ADR-004.
void InputBeginFrame(const InputFrame *frame);

// Builds the tick snapshot and ages the buffers. Call once per tick, step 1 in
// GameTick, per ADR-017.
void InputBeginTick(void);

const InputState *InputStateGet(void);

bool InputHeld(Action action);
bool InputPressed(Action action);
bool InputReleased(Action action);

// Buffered presses for attack and dash, 100 ms, per ADR-010. A fresh press is
// consumable on its own tick. Consume clears the buffer.
bool InputBuffered(Action action);
bool InputConsumeBuffered(Action action);

// Drops latched edges and buffered presses. Pause states call this on enter
// and on exit so resume does not replay stale input.
void InputDropPending(void);

// Fills a frame from raylib keyboard state and latches it. Defined in
// input_raylib.c, which is linked into the game but not into balin_tests.
void InputPollRaylib(void);

#endif
