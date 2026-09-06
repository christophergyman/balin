#include "world_file.h"

#include <libgen.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "paths.h"

#define WORLD_FILE_MAGIC 0x54574C42u // "BLWT" little endian
#define WORLD_FILE_VERSION 2

static void WriteU32(FILE *file, uint32_t value) {
    fwrite(&value, sizeof(value), 1, file);
}

static bool ReadU32(FILE *file, uint32_t *value) {
    return fread(value, sizeof(*value), 1, file) == 1;
}

// Demo room: an 8x8 walled room with a doorway and three crates.
// Faces are set only on air-facing sides so adjacent wall tiles never
// draw coplanar quads on top of each other.
static void FillDefaultWorld(World *world) {
    memset(world, 0, sizeof(*world));

    bool isWall[WORLD_COLS][WORLD_COLS] = {0};
    int minX = 1, maxX = 8, minZ = 3, maxZ = 10;

    for (int row = minZ; row <= maxZ; row++) {
        for (int col = minX; col <= maxX; col++) {
            if (col == minX || col == maxX || row == minZ || row == maxZ) {
                isWall[row][col] = true;
            }
        }
    }

    // Crates inside the room (freestanding wall tiles with the crate texture)
    isWall[6][5] = true;
    isWall[4][6] = true;
    isWall[8][3] = true;

    for (int row = 0; row < WORLD_COLS; row++) {
        for (int col = 0; col < WORLD_COLS; col++) {
            if (!isWall[row][col]) {
                continue;
            }
            Tile tile = {
                .textureId = 0,
                .faces = 0,
                .collision = DIR_N | DIR_E | DIR_S | DIR_W,
            };

            // Face every side that borders open space
            if (row == 0 || !isWall[row - 1][col]) tile.faces |= DIR_N;
            if (col == WORLD_COLS - 1 || !isWall[row][col + 1]) tile.faces |= DIR_E;
            if (row == WORLD_COLS - 1 || !isWall[row + 1][col]) tile.faces |= DIR_S;
            if (col == 0 || !isWall[row][col - 1]) tile.faces |= DIR_W;

            world->tiles[row][col] = tile;
        }
    }

    // Crates use the crate texture
    world->tiles[6][5].textureId = 1;
    world->tiles[4][6].textureId = 1;
    world->tiles[8][3].textureId = 1;

    // Doorway: two south wall tiles you can walk through, side faces only
    for (int col = 4; col <= 5; col++) {
        world->tiles[maxZ][col].faces = DIR_E | DIR_W;
        world->tiles[maxZ][col].collision = 0;
    }
}

bool LoadWorldTiles(World *world) {
    char path[PATH_MAX];
    DataFilePath(WORLD_FILE_NAME, path, sizeof(path));

    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        FillDefaultWorld(world);
        SaveWorldTiles(world);
        printf("Created default %s\n", path);
        return true;
    }

    uint32_t magic = 0;
    uint32_t version = 0;
    uint32_t cols = 0;
    uint32_t rows = 0;
    bool ok = ReadU32(file, &magic) && ReadU32(file, &version) &&
              ReadU32(file, &cols) && ReadU32(file, &rows);

    if (!ok || magic != WORLD_FILE_MAGIC || version != WORLD_FILE_VERSION ||
        cols != WORLD_COLS || rows != WORLD_COLS ||
        fread(world->tiles, sizeof(world->tiles), 1, file) != 1) {
        printf("Unreadable or outdated %s, loading defaults\n", path);
        FillDefaultWorld(world);
        fclose(file);
        SaveWorldTiles(world);
        return true;
    }

    fclose(file);
    return true;
}

void SaveWorldTiles(const World *world) {
    EnsureDataDir();

    char path[PATH_MAX];
    DataFilePath(WORLD_FILE_NAME, path, sizeof(path));

    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        printf("Failed to open %s for writing\n", path);
        return;
    }

    WriteU32(file, WORLD_FILE_MAGIC);
    WriteU32(file, WORLD_FILE_VERSION);
    WriteU32(file, WORLD_COLS);
    WriteU32(file, WORLD_COLS);
    fwrite(world->tiles, sizeof(world->tiles), 1, file);
    fclose(file);
}
