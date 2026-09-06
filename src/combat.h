#ifndef COMBAT_H
#define COMBAT_H

#include <stdbool.h>

#include "types.h"

bool IsAdjacent(Position a, Position b);
void HandleCombat(Entity *entityArray, int entityCount, Position mouseTile);

#endif
