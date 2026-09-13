#include "game/tuning_keys.h"

// The one schema table. Every key is required in assets/tuning.txt, per
// ADR-008. Add a row with the system that reads the value.
const TuningSpec TuningSpecs[] = {
    { "player.moveSpeed", TUNING_NUMBER },
    { "player.radius", TUNING_NUMBER },
};

_Static_assert(sizeof(TuningSpecs) / sizeof(TuningSpecs[0]) == TUNE_COUNT,
               "TuningSpecs must have one row per TuneKey");
