#ifndef ENTITY_H
#define ENTITY_H

#include "raylib.h"

typedef enum {
    ENTITY_PLAYER = 0,
    ENTITY_ENEMY = 1,
} EntityKind;

typedef struct Entity {
    EntityKind kind;
    Vector3 position; // free float position, y = feet height
    int health;
} Entity;

#endif
