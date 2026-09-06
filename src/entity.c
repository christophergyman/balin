#include "entity.h"

#include <raylib.h>
#include <string.h>

#include "config.h"

int InitEntityArray(Entity *entityArray) {
    entityArray[0] = (Entity){
        .name = "player",
        .position = {
            .posX = 2,
            .posY = 2,
        },
        .entityStats = {
            .health = 100,
            .strength = 99,
            .faith = 99,
            .money = 999
        }
    };

    entityArray[1] = (Entity){
        .name = "enemy",
        .position = {
            .posX = 5,
            .posY = 2,
        },
        .entityStats = {
            .health = 100,
            .strength = 99,
            .faith = 99,
            .money = 999
        }
    };

    return 2;
}

void DrawWorldEntities(Entity *entityArray, int entityCount) {
    for (int i = 0; i < entityCount; i++) {
        // Dead entities are not drawn
        if (entityArray[i].entityStats.health <= 0) {
            continue;
        }
        // strcmp returns 0 on match, so compare against 0
        if(strcmp(entityArray[i].name, "player") == 0){
            DrawRectangle(
                entityArray[i].position.posX * TILE_SIZE,
                entityArray[i].position.posY * TILE_SIZE,
                TILE_SIZE,
                TILE_SIZE,
                BLUE
            );
        } else {
            DrawRectangle(
                entityArray[i].position.posX * TILE_SIZE,
                entityArray[i].position.posY * TILE_SIZE,
                TILE_SIZE,
                TILE_SIZE,
                PURPLE
            );
        }
    }
}
