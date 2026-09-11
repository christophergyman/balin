#include "game/game.h"

#include "raylib.h"

#define SQUARE_SIZE 64

void GameInit(void) {
}

void GameUpdate(void) {
}

void GameDraw(void) {
    ClearBackground(BLACK);

    DrawRectangle(
        (GetScreenWidth() - SQUARE_SIZE) / 2,
        (GetScreenHeight() - SQUARE_SIZE) / 2,
        SQUARE_SIZE,
        SQUARE_SIZE,
        WHITE
    );
}

void GameShutdown(void) {
}
