#include "world.h"

#include <math.h>

static float Clampf(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

// Circle vs axis-aligned rectangle overlap
static bool CircleHitsAabb(float x, float z, float radius, float minX, float minZ, float maxX, float maxZ) {
    float closestX = Clampf(x, minX, maxX);
    float closestZ = Clampf(z, minZ, maxZ);
    float dx = x - closestX;
    float dz = z - closestZ;
    return dx * dx + dz * dz < radius * radius;
}

// Each collision bit is a thin wall segment on that tile's edge.
// A shared edge blocks when either neighboring tile has its bit set,
// which falls out naturally: each tile emits segments only for its own bits.
bool WorldCollidesCircle(const World *world, float x, float z, float radius) {
    float th = EDGE_THICKNESS;

    int minCol = (int)floorf((x - radius) / TILE_SIZE) - 1;
    int maxCol = (int)floorf((x + radius) / TILE_SIZE) + 1;
    int minRow = (int)floorf((z - radius) / TILE_SIZE) - 1;
    int maxRow = (int)floorf((z + radius) / TILE_SIZE) + 1;

    for (int row = minRow; row <= maxRow; row++) {
        for (int col = minCol; col <= maxCol; col++) {
            if (col < 0 || col >= WORLD_COLS || row < 0 || row >= WORLD_COLS) {
                continue;
            }
            const Tile *tile = &world->tiles[row][col];
            if (tile->collision == 0) {
                continue;
            }

            float minX = col * TILE_SIZE;
            float minZ = row * TILE_SIZE;
            float maxX = minX + TILE_SIZE;
            float maxZ = minZ + TILE_SIZE;

            if (tile->collision & DIR_N &&
                CircleHitsAabb(x, z, radius, minX - th, minZ - th, maxX + th, minZ + th)) {
                return true;
            }
            if (tile->collision & DIR_S &&
                CircleHitsAabb(x, z, radius, minX - th, maxZ - th, maxX + th, maxZ + th)) {
                return true;
            }
            if (tile->collision & DIR_W &&
                CircleHitsAabb(x, z, radius, minX - th, minZ - th, minX + th, maxZ + th)) {
                return true;
            }
            if (tile->collision & DIR_E &&
                CircleHitsAabb(x, z, radius, maxX - th, minZ - th, maxX + th, maxZ + th)) {
                return true;
            }
        }
    }
    return false;
}

// Tile (col, row) fills the cell [col*TILE_SIZE, (col+1)*TILE_SIZE] in x and z
Vector3 TileCenter(int col, int row) {
    return (Vector3){
        (col + 0.5f) * TILE_SIZE,
        0.0f,
        (row + 0.5f) * TILE_SIZE,
    };
}
