#include "engine/input/input.h"

#include <stddef.h>

#include "raylib.h"

// The one binding table, per ADR-010. Every action lists at most two keys;
// KEY_NULL marks an unused slot. Rebind by editing this table.
typedef struct InputBinding {
    Action action;
    KeyboardKey keyA;
    KeyboardKey keyB;
} InputBinding;

static const InputBinding BINDINGS[] = {
    { ACTION_MOVE_UP, KEY_W, KEY_UP },
    { ACTION_MOVE_DOWN, KEY_S, KEY_DOWN },
    { ACTION_MOVE_LEFT, KEY_A, KEY_LEFT },
    { ACTION_MOVE_RIGHT, KEY_D, KEY_RIGHT },
    { ACTION_ATTACK, KEY_J, KEY_NULL },
    { ACTION_DASH, KEY_SPACE, KEY_NULL },
    { ACTION_SMITE, KEY_K, KEY_NULL },
    { ACTION_BAG, KEY_TAB, KEY_NULL },
    { ACTION_INTERACT, KEY_E, KEY_ENTER },
};

void InputPollRaylib(void) {
    InputFrame frame = { 0 };
    size_t count = sizeof(BINDINGS) / sizeof(BINDINGS[0]);

    // Presses come from the raylib key queue, not IsKeyPressed. A tap that
    // starts and ends between two polls keeps its press here, per ADR-010.
    // GLFW queues one entry per press and does not queue key repeats.
    int queued = 0;
    while ((queued = GetKeyPressed()) != 0) {
        for (size_t index = 0; index < count; index++) {
            const InputBinding *binding = &BINDINGS[index];
            if (queued != (int)binding->keyA && queued != (int)binding->keyB) {
                continue;
            }

            frame.pressed |= ACTION_BIT(binding->action);

            // The key came back up within this poll, so IsKeyReleased cannot
            // see the release edge. Report it from here.
            if (!IsKeyDown(queued)) {
                frame.released |= ACTION_BIT(binding->action);
            }
        }
    }

    for (size_t index = 0; index < count; index++) {
        const InputBinding *binding = &BINDINGS[index];
        KeyboardKey keys[2] = { binding->keyA, binding->keyB };
        uint32_t bit = ACTION_BIT(binding->action);

        for (int slot = 0; slot < 2; slot++) {
            KeyboardKey key = keys[slot];
            if (key == KEY_NULL) {
                continue;
            }
            if (IsKeyDown(key)) {
                frame.held |= bit;
            }
            if (IsKeyReleased(key)) {
                frame.released |= bit;
            }
        }
    }

    InputBeginFrame(&frame);
}
