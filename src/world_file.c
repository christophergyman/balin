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

// One wall tile with faces and collision on all four sides
static void FillDefaultWorld(World *world) {
    memset(world, 0, sizeof(*world));
    world->tiles[0][0] = (Tile){
        .textureId = 0,
        .faces = DIR_N | DIR_E | DIR_S | DIR_W,
        .collision = DIR_N | DIR_E | DIR_S | DIR_W,
    };
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
