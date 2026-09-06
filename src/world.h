#ifndef WORLD_H
#define WORLD_H

#include <stdint.h>

#include "raylib.h"

#include "config.h"

// Direction bitmask: which sides of a tile have faces or collision
typedef enum {
    DIR_N = 1 << 0, // north = -Z
    DIR_E = 1 << 1, // east  = +X
    DIR_S = 1 << 2, // south = +Z
    DIR_W = 1 << 3, // west  = -X
} DirBit;

// Stored as uint8 fields so the raw array can be written to disk directly
typedef struct Tile {
    uint8_t textureId; // index into the texture table, 0 = placeholder
    uint8_t faces;     // DirBit bitmask: sides that render a textured quad
    uint8_t collision; // DirBit bitmask: sides that block movement
} Tile;

typedef struct World {
    Tile tiles[WORLD_COLS][WORLD_COLS];
} World;

Vector3 TileCenter(int col, int row);

// True when the player circle (radius, at x/z) crosses any collision edge
bool WorldCollidesCircle(const World *world, float x, float z, float radius);

#endif
