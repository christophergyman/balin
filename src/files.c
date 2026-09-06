#include "files.h"

#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#include "entity.h"

// Cached data directory, resolved once: next to the executable, wherever it is run from
static char dataDir[PATH_MAX] = {0};

static const char *GetDataDir(void) {
    if (dataDir[0] != '\0') {
        return dataDir;
    }

#ifdef __APPLE__
    char exePath[PATH_MAX];
    uint32_t size = sizeof(exePath);
    if (_NSGetExecutablePath(exePath, &size) == 0) {
        char exePathCopy[PATH_MAX];
        snprintf(exePathCopy, sizeof(exePathCopy), "%s", exePath);
        char *exeDir = dirname(exePathCopy); // note: dirname may modify its argument
        snprintf(dataDir, sizeof(dataDir), "%s/%s", exeDir, DATA_DIR_NAME);
    }
#endif

    // Fallback for non-macOS builds: relative to the working directory
    if (dataDir[0] == '\0') {
        snprintf(dataDir, sizeof(dataDir), "%s", DATA_DIR_NAME);
    }
    return dataDir;
}

static void BuildDataPath(const char *fileName, char *out, size_t outSize) {
    snprintf(out, outSize, "%s/%s", GetDataDir(), fileName);
}

// Create the data directory if it is missing
static void EnsureDataDir(void) {
    mkdir(GetDataDir(), 0755); // fails quietly if it already exists
}

static char TileToChar(WorldTile tile) {
    return tile.tileType == WALL ? '#' : '.';
}

static WorldTile CharToTile(char c) {
    WorldTile tile = { .tileType = AIR };
    if (c == '#') {
        tile.tileType = WALL;
    }
    return tile;
}

// Format: one character per tile, ROWS lines of COLS characters. '#' = WALL, '.' = AIR
void LoadWorld(WorldTile worldArray[ROWS][COLS]) {
    char path[PATH_MAX];
    BuildDataPath(WORLD_FILE, path, sizeof(path));

    FILE *file = fopen(path, "r");
    if (file == NULL) {
        // No world file yet: start from the default and write it out
        memset(worldArray, 0, ROWS * COLS * sizeof(WorldTile));
        worldArray[0][1].tileType = WALL;
        SaveWorld(worldArray);
        printf("Created default %s\n", path);
        return;
    }

    char line[COLS + 8];
    for (int r = 0; r < ROWS; r++) {
        if (fgets(line, sizeof line, file) == NULL) {
            break;
        }
        for (int c = 0; c < COLS; c++) {
            worldArray[r][c] = CharToTile(line[c]);
        }
    }
    fclose(file);
}

void SaveWorld(WorldTile worldArray[ROWS][COLS]) {
    EnsureDataDir();

    char path[PATH_MAX];
    BuildDataPath(WORLD_FILE, path, sizeof(path));

    FILE *file = fopen(path, "w");
    if (file == NULL) {
        printf("Failed to open %s for writing\n", path);
        return;
    }

    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            fputc(TileToChar(worldArray[r][c]), file);
        }
        fputc('\n', file);
    }
    fclose(file);
}

// Format: one entity per line: name x y health strength faith money
int LoadEntities(Entity *entityArray) {
    char path[PATH_MAX];
    BuildDataPath(ENTITY_FILE, path, sizeof(path));

    FILE *file = fopen(path, "r");
    if (file == NULL) {
        // No entity file yet: start from the defaults and write them out
        int count = InitEntityArray(entityArray);
        SaveEntities(entityArray, count);
        printf("Created default %s\n", path);
        return count;
    }

    char name[32];
    int count = 0;
    int x, y, health, strength, faith, money;

    while (count < MAX_ENTITIES &&
           fscanf(file, "%31s %d %d %d %d %d %d",
                  name, &x, &y, &health, &strength, &faith, &money) == 7) {
        size_t nameLen = strlen(name) + 1;
        char *nameCopy = malloc(nameLen);
        if (nameCopy == NULL) {
            printf("Failed to allocate entity name\n");
            break;
        }
        memcpy(nameCopy, name, nameLen);

        entityArray[count] = (Entity){
            .name = nameCopy,
            .position = { .posX = x, .posY = y },
            .entityStats = {
                .health = health,
                .strength = strength,
                .faith = faith,
                .money = money,
            },
        };
        count++;
    }
    fclose(file);
    return count;
}

void SaveEntities(Entity *entityArray, int entityCount) {
    EnsureDataDir();

    char path[PATH_MAX];
    BuildDataPath(ENTITY_FILE, path, sizeof(path));

    FILE *file = fopen(path, "w");
    if (file == NULL) {
        printf("Failed to open %s for writing\n", path);
        return;
    }

    for (int i = 0; i < entityCount; i++) {
        Entity *entity = &entityArray[i];
        fprintf(file, "%s %d %d %d %d %d %d\n",
                entity->name,
                entity->position.posX,
                entity->position.posY,
                entity->entityStats.health,
                entity->entityStats.strength,
                entity->entityStats.faith,
                entity->entityStats.money);
    }
    fclose(file);
}
