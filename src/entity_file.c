#include "entity_file.h"

#include <limits.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "paths.h"

#define ENTITY_FILE_MAGIC 0x4E454C42u // "BLEN" little endian
#define ENTITY_FILE_VERSION 1

static void WriteU32(FILE *file, uint32_t value) {
    fwrite(&value, sizeof(value), 1, file);
}

static bool ReadU32(FILE *file, uint32_t *value) {
    return fread(value, sizeof(*value), 1, file) == 1;
}

static void WriteEntity(FILE *file, const Entity *entity) {
    uint8_t kind = (uint8_t)entity->kind;
    fwrite(&kind, sizeof(kind), 1, file);
    fwrite(&entity->position.x, sizeof(float), 1, file);
    fwrite(&entity->position.y, sizeof(float), 1, file);
    fwrite(&entity->position.z, sizeof(float), 1, file);
    int32_t health = entity->health;
    fwrite(&health, sizeof(health), 1, file);
}

static bool ReadEntity(FILE *file, Entity *entity) {
    uint8_t kind = 0;
    float x = 0;
    float y = 0;
    float z = 0;
    int32_t health = 0;

    if (fread(&kind, sizeof(kind), 1, file) != 1 ||
        fread(&x, sizeof(x), 1, file) != 1 ||
        fread(&y, sizeof(y), 1, file) != 1 ||
        fread(&z, sizeof(z), 1, file) != 1 ||
        fread(&health, sizeof(health), 1, file) != 1) {
        return false;
    }

    entity->kind = (EntityKind)kind;
    entity->position = (Vector3){x, y, z};
    entity->health = health;
    return true;
}

// Just the player, standing where the demo starts
static int FillDefaultEntities(Entity *entities) {
    entities[0] = (Entity){
        .kind = ENTITY_PLAYER,
        .position = {2.5f * TILE_SIZE, 0.0f, 5.5f * TILE_SIZE},
        .health = 100,
    };
    return 1;
}

int LoadEntities(Entity *entities) {
    char path[PATH_MAX];
    DataFilePath(ENTITY_FILE_NAME, path, sizeof(path));

    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        int count = FillDefaultEntities(entities);
        SaveEntities(entities, count);
        printf("Created default %s\n", path);
        return count;
    }

    uint32_t magic = 0;
    uint32_t version = 0;
    uint32_t count = 0;
    bool headerOk = ReadU32(file, &magic) && ReadU32(file, &version) &&
                    ReadU32(file, &count);

    if (!headerOk || magic != ENTITY_FILE_MAGIC || version != ENTITY_FILE_VERSION ||
        count > MAX_ENTITIES) {
        printf("Unreadable or outdated %s, loading defaults\n", path);
        fclose(file);
        int defaultCount = FillDefaultEntities(entities);
        SaveEntities(entities, defaultCount);
        return defaultCount;
    }

    int loaded = 0;
    while ((uint32_t)loaded < count && loaded < MAX_ENTITIES) {
        if (!ReadEntity(file, &entities[loaded])) {
            break;
        }
        loaded++;
    }
    fclose(file);
    return loaded;
}

void SaveEntities(const Entity *entities, int entityCount) {
    EnsureDataDir();

    char path[PATH_MAX];
    DataFilePath(ENTITY_FILE_NAME, path, sizeof(path));

    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        printf("Failed to open %s for writing\n", path);
        return;
    }

    WriteU32(file, ENTITY_FILE_MAGIC);
    WriteU32(file, ENTITY_FILE_VERSION);
    WriteU32(file, (uint32_t)entityCount);
    for (int i = 0; i < entityCount; i++) {
        WriteEntity(file, &entities[i]);
    }
    fclose(file);
}
