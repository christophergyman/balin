#include "combat.h"

#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// True when the two tiles are orthogonally next to each other
bool IsAdjacent(Position a, Position b) {
    int dx = a.posX - b.posX;
    int dy = a.posY - b.posY;
    return abs(dx) + abs(dy) == 1;
}

// Left click attack: click an adjacent entity to reduce its health to zero
void HandleCombat(Entity *entityArray, int entityCount, Position mouseTile) {
    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        return;
    }

    // Find the entity under the cursor
    Entity *target = NULL;
    for (int i = 0; i < entityCount; i++) {
        if (entityArray[i].name == NULL || entityArray[i].entityStats.health <= 0) {
            continue;
        }
        if (entityArray[i].position.posX == mouseTile.posX &&
            entityArray[i].position.posY == mouseTile.posY) {
            target = &entityArray[i];
            break;
        }
    }
    if (target == NULL) {
        return;
    }

    // Never attack the player
    if (strcmp(target->name, "player") == 0) {
        return;
    }

    // Find the player
    Entity *player = NULL;
    for (int i = 0; i < entityCount; i++) {
        if (entityArray[i].name == NULL || strcmp(entityArray[i].name, "player") != 0) {
            continue;
        }
        player = &entityArray[i];
        break;
    }
    if (player == NULL) {
        return;
    }

    // Only adjacent targets can be attacked
    if (!IsAdjacent(player->position, target->position)) {
        printf("Target out of range\n");
        return;
    }

    target->entityStats.health = 0;
    printf("%s slain\n", target->name);
}
