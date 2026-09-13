#ifndef GAME_H
#define GAME_H

#include <stdio.h>

#include "game/ecs.h"

void GameInit(void);

// Draws one rendered frame. alpha in [0, 1) interpolates between the previous
// and current tick positions, per ADR-004.
void GameDraw(float alpha);

void GameShutdown(void);

// Temporary moving probe for the fixed timestep check. It stands in for Balin
// until the ECS (BAL-11) and player movement (BAL-20) land.
void GameProbeTick(void);

// The single ECS world. Systems run against it, and GameTick flushes the
// deferred destroys after the system list.
EcsWorld *GameEcs(void);

// Appends the game half of a bug bundle state snapshot: ECS counts, arena
// bytes, and player state. Later systems add their own sections. Registered
// by main.c as the logger snapshot writer.
void GameWriteSnapshot(FILE *out);

#endif
