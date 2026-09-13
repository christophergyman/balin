#ifndef MEMORY_H
#define MEMORY_H

#include "engine/arena.h"

// The four scoped arenas from ADR-003. Accessors return the live arena.
void GameMemoryInit(void);
void GameMemoryResetLevel(void);
void GameMemoryResetRun(void);
void GameMemoryResetFrame(void);

Arena *GameArenaAsset(void);
Arena *GameArenaLevel(void);
Arena *GameArenaRun(void);
Arena *GameArenaFrame(void);

#endif
