#ifndef SYSTEMS_H
#define SYSTEMS_H

// Runs one fixed 60 Hz tick through the explicit ordered system list from
// ADR-017. Order is the architecture: new systems slot in at an explicit
// position in GameTick.
void GameTick(void);

#endif
