#include "player.h"

#include <math.h>

#include "raymath.h"
#include "world.h"

static float Clampf(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

// Direction the camera looks, built from yaw and pitch
static Vector3 LookDirection(float yaw, float pitch) {
    float cosPitch = cosf(pitch);
    return (Vector3){
        sinf(yaw) * cosPitch,
        sinf(pitch),
        -cosf(yaw) * cosPitch,
    };
}

Vector3 PlayerLookDirection(const Player *player) {
    return LookDirection(player->yaw, player->pitch);
}

void InitPlayer(Player *player, Vector3 startPosition) {
    player->position = startPosition;
    player->velocity = (Vector3){0};
    player->yaw = 0.0f;
    player->pitch = 0.0f;
    player->onGround = true;
    player->coyoteTimer = 0.0f;
}

// Move a Vector2 toward a target at a limited rate
void MoveVector2Toward(Vector2 *vector, Vector2 target, float maxDelta) {
    Vector2 delta = Vector2Subtract(target, *vector);
    float distance = Vector2Length(delta);
    if (distance <= maxDelta || distance == 0.0f) {
        *vector = target;
        return;
    }
    delta = Vector2Scale(Vector2Normalize(delta), maxDelta);
    *vector = Vector2Add(*vector, delta);
}

void UpdatePlayer(Player *player, Camera3D *camera, float dt, const World *world) {
    // Mouse look
    Vector2 mouseDelta = GetMouseDelta();
    player->yaw += mouseDelta.x * MOUSE_SENS * DEG2RAD;
    player->pitch -= mouseDelta.y * MOUSE_SENS * DEG2RAD;
    float pitchLimit = PITCH_LIMIT * DEG2RAD;
    player->pitch = Clampf(player->pitch, -pitchLimit, pitchLimit);

    // Walk input, relative to camera yaw. Builds the wish velocity, then
    // accelerates toward it for smooth start, stop, and air control.
    Vector2 moveInput = {0};
    if (IsKeyDown(KEY_W)) moveInput.y += 1;
    if (IsKeyDown(KEY_S)) moveInput.y -= 1;
    if (IsKeyDown(KEY_D)) moveInput.x += 1;
    if (IsKeyDown(KEY_A)) moveInput.x -= 1;

    Vector2 wishVel = {0};
    if (moveInput.x != 0 || moveInput.y != 0) {
        moveInput = Vector2Normalize(moveInput);
        Vector3 forward = {sinf(player->yaw), 0.0f, -cosf(player->yaw)};
        Vector3 right = {cosf(player->yaw), 0.0f, sinf(player->yaw)};
        wishVel.x = (forward.x * moveInput.y + right.x * moveInput.x) * MOVE_SPEED;
        wishVel.y = (forward.z * moveInput.y + right.z * moveInput.x) * MOVE_SPEED;
    }

    Vector2 velH = {player->velocity.x, player->velocity.z};
    float accel = player->onGround ? GROUND_ACCEL : AIR_ACCEL;
    MoveVector2Toward(&velH, wishVel, accel * dt);
    player->velocity.x = velH.x;
    player->velocity.z = velH.y;

    // Jump, with coyote time grace after leaving the ground
    bool canJump = player->onGround || player->coyoteTimer > 0.0f;
    if (canJump && IsKeyPressed(KEY_SPACE)) {
        player->velocity.y = JUMP_SPEED;
        player->coyoteTimer = 0.0f;
        player->onGround = false;
    }

    // Gravity
    player->velocity.y -= GRAVITY * dt;

    // Integrate and collide, one axis at a time
    float newX = player->position.x + player->velocity.x * dt;
    if (!WorldCollidesCircle(world, newX, player->position.z, PLAYER_RADIUS)) {
        player->position.x = newX;
    }

    float newZ = player->position.z + player->velocity.z * dt;
    if (!WorldCollidesCircle(world, player->position.x, newZ, PLAYER_RADIUS)) {
        player->position.z = newZ;
    }

    float newY = player->position.y + player->velocity.y * dt;
    if (newY <= 0.0f) {
        newY = 0.0f;
        player->velocity.y = 0.0f;
        player->onGround = true;
        player->coyoteTimer = COYOTE_TIME;
    } else {
        player->onGround = false;
        if (player->coyoteTimer > 0.0f) {
            player->coyoteTimer -= dt;
        }
    }
    player->position.y = newY;

    // Camera follows the body
    camera->position = (Vector3){
        player->position.x,
        player->position.y + PLAYER_EYE,
        player->position.z,
    };
    Vector3 look = LookDirection(player->yaw, player->pitch);
    camera->target = Vector3Add(camera->position, look);
}
