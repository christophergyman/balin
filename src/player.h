#ifndef PLAYER_H
#define PLAYER_H

#include <stdbool.h>

#include "raylib.h"

#include "config.h"
#include "world.h"

typedef struct Player {
    Vector3 position; // feet position
    Vector3 velocity;
    float yaw;   // radians, 0 = looking toward -Z
    float pitch; // radians, positive = looking up
    bool onGround;
    float coyoteTimer; // seconds left to jump after leaving ground
} Player;

void InitPlayer(Player *player, Vector3 startPosition);

// Move a Vector2 toward a target at a limited rate (acceleration model)
void MoveVector2Toward(Vector2 *vector, Vector2 target, float maxDelta);
void UpdatePlayer(Player *player, Camera3D *camera, float dt, const World *world);

#endif
