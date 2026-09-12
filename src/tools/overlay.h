#ifndef OVERLAY_H
#define OVERLAY_H

#include "engine/time.h"

// Dev-only debug overlay, per ADR-015. F3 toggles it. F1 cycles the FPS cap
// through 30, 60, 120, and uncapped for fixed timestep checks.
void OverlayHandleKeys(void);
void OverlayDraw(const Clock *clock);

#endif
