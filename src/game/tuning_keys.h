#ifndef TUNING_KEYS_H
#define TUNING_KEYS_H

#include "engine/core/tuning.h"

// One enum value per key in assets/tuning.txt. The order must match the
// TuningSpecs table in tuning_keys.c.
typedef enum TuneKey {
    TUNE_PLAYER_MOVE_SPEED = 0,
    TUNE_PLAYER_RADIUS,
    TUNE_COUNT
} TuneKey;

extern const TuningSpec TuningSpecs[TUNE_COUNT];

#endif
