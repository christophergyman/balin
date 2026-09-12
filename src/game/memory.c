#include "game/memory.h"

#define GAME_ASSET_ARENA_BYTES (4 * 1024 * 1024)
#define GAME_LEVEL_ARENA_BYTES (2 * 1024 * 1024)
#define GAME_RUN_ARENA_BYTES (1 * 1024 * 1024)
#define GAME_FRAME_ARENA_BYTES (256 * 1024)

static unsigned char assetMemory[GAME_ASSET_ARENA_BYTES];
static unsigned char levelMemory[GAME_LEVEL_ARENA_BYTES];
static unsigned char runMemory[GAME_RUN_ARENA_BYTES];
static unsigned char frameMemory[GAME_FRAME_ARENA_BYTES];

static Arena assetArena;
static Arena levelArena;
static Arena runArena;
static Arena frameArena;

void GameMemoryInit(void) {
    ArenaInit(&assetArena, "asset", assetMemory, sizeof(assetMemory));
    ArenaInit(&levelArena, "level", levelMemory, sizeof(levelMemory));
    ArenaInit(&runArena, "run", runMemory, sizeof(runMemory));
    ArenaInit(&frameArena, "frame", frameMemory, sizeof(frameMemory));
}

void GameMemoryResetRun(void) {
    ArenaReset(&runArena);
}

void GameMemoryResetFrame(void) {
    ArenaReset(&frameArena);
}

Arena *GameArenaAsset(void) {
    return &assetArena;
}

Arena *GameArenaLevel(void) {
    return &levelArena;
}

Arena *GameArenaRun(void) {
    return &runArena;
}

Arena *GameArenaFrame(void) {
    return &frameArena;
}
