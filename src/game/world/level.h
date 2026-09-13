#ifndef LEVEL_H
#define LEVEL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "engine/core/arena.h"

// Versioned text level format, per ADR-006 and ADR-008. One file per section:
// a version header, a size, three tile layers as RLE rows, then records
// grouped by kind: entities, then gates, then shrines. A section caps at
// 128x128 tiles of 32x32 pixels.
//
// The file carries tile ids only. Id 0 is an empty tile, and wall tiles with
// a nonzero id get TILE_FLAG_SOLID at load. LevelParse pushes tile rows and
// record arrays into the caller's arena. Records of each kind are contiguous,
// because the format groups them. A failed parse may leave bytes in the
// arena, but out is untouched. Reset the level arena on every section load,
// per ADR-003. Malformed input fails fast with path and line, per ADR-019.
#define LEVEL_VERSION 1
#define LEVEL_MAX_SIZE 128
#define LEVEL_TILE_PIXELS 32
#define LEVEL_ERROR_CAP 256
#define LEVEL_PATH_CAP 1024

typedef enum LevelLayerId {
    LEVEL_LAYER_FLOOR = 0,
    LEVEL_LAYER_WALL,
    LEVEL_LAYER_DECOR,
    LEVEL_LAYER_COUNT,
} LevelLayerId;

typedef enum TileFlags {
    TILE_FLAG_SOLID = 1 << 0,
} TileFlags;

typedef struct Tile {
    uint16_t id; // Tile id from the file. Meaning lands with the manifest.
    uint8_t flags;
} Tile;

typedef enum LevelEntityKind {
    LEVEL_ENTITY_PLAYER = 0,
    LEVEL_ENTITY_CRAWLER,
    LEVEL_ENTITY_MINER,
    LEVEL_ENTITY_BRUTE,
    LEVEL_ENTITY_APPLE,
    LEVEL_ENTITY_SMALL_MUSHROOM,
    LEVEL_ENTITY_BIG_MUSHROOM,
    LEVEL_ENTITY_RATION,
    LEVEL_ENTITY_KIND_COUNT,
} LevelEntityKind;

typedef enum LevelShrineKind {
    LEVEL_SHRINE_OPENING = 0,
    LEVEL_SHRINE_SMALL,
    LEVEL_SHRINE_FINAL,
    LEVEL_SHRINE_KIND_COUNT,
} LevelShrineKind;

typedef struct LevelLayer {
    Tile *tiles; // width * height, row-major, in the level arena.
} LevelLayer;

typedef struct LevelEntity {
    uint8_t kind; // LevelEntityKind.
    float x;
    float y;
} LevelEntity;

typedef struct LevelGate {
    float x;
    float y;
    uint8_t faithCap; // New faith maximum after crossing, 0 to 100.
} LevelGate;

typedef struct LevelShrine {
    uint8_t kind; // LevelShrineKind.
    float x;
    float y;
} LevelShrine;

// All pointers reference the arena passed to LevelParse. The struct itself is
// caller-owned, like EcsWorld.
typedef struct Level {
    uint16_t width;
    uint16_t height;
    LevelLayer layers[LEVEL_LAYER_COUNT];
    LevelEntity *entities;
    uint16_t entityCount;
    LevelGate *gates;
    uint16_t gateCount;
    LevelShrine *shrines;
    uint16_t shrineCount;
} Level;

// Parses path into out. Tile rows and records are pushed into arena. On
// failure returns false, fills error with path:line and the expectation, and
// leaves out untouched.
bool LevelParse(Level *out, const char *path, Arena *arena, char *error, size_t cap);

// Writes the canonical text form to out. Fails when cap is too small, when
// the level fails validation, or when a kind is out of range.
bool LevelSerialize(const Level *level, char *out, size_t cap, char *error, size_t errorCap);

// Serializes level and writes it to path. Writes a temporary file next to
// path and renames it into place, so a failed save leaves the old file
// intact. Returns false with the reason on failure.
bool LevelSave(const Level *level, const char *path, char *error, size_t errorCap);

#endif
