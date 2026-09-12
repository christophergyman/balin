#ifndef GAME_H
#define GAME_H

void GameInit(void);

// Draws one rendered frame. alpha in [0, 1) interpolates between the previous
// and current tick positions, per ADR-004.
void GameDraw(float alpha);

void GameShutdown(void);

// Temporary moving probe for the fixed timestep check. It stands in for Balin
// until the ECS (BAL-11) and player movement (BAL-20) land.
void GameProbeTick(void);

#endif
