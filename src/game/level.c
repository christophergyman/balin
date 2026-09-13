#include "game/level.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// One line holds a full RLE row: 128 tokens of up to 10 characters each.
#define LEVEL_LINE_CAP 2048
#define LEVEL_TOKEN_CAP 64

static const char *const layerNames[LEVEL_LAYER_COUNT] = {
    "floor",
    "wall",
    "decor",
};

static const char *const entityKindNames[LEVEL_ENTITY_KIND_COUNT] = {
    "player",
    "crawler",
    "miner",
    "brute",
    "apple",
    "small_mushroom",
    "big_mushroom",
    "ration",
};

static const char *const shrineKindNames[LEVEL_SHRINE_KIND_COUNT] = {
    "opening",
    "small",
    "final",
};

static char *TrimLeft(char *text) {
    while (*text == ' ' || *text == '\t') {
        text++;
    }
    return text;
}

static void TrimRight(char *text) {
    char *end = text + strlen(text);
    while (end > text && (end[-1] == '\n' || end[-1] == '\r' ||
                          end[-1] == ' ' || end[-1] == '\t')) {
        end--;
        *end = '\0';
    }
}

// Copies the next whitespace-delimited token into out and advances cursor
// past it. Returns false at the end of the line.
static bool NextToken(const char **cursor, char *out, size_t cap) {
    const char *text = *cursor;
    while (*text == ' ' || *text == '\t') {
        text++;
    }
    if (*text == '\0') {
        *cursor = text;
        return false;
    }

    size_t length = 0;
    while (text[length] != '\0' && text[length] != ' ' && text[length] != '\t') {
        length++;
    }
    if (length >= cap) {
        length = cap - 1;
    }
    memcpy(out, text, length);
    out[length] = '\0';
    *cursor = text + length;
    return true;
}

static bool ParseLong(const char *text, long *out) {
    if (text == NULL) {
        return false;
    }

    char *end = NULL;
    long value = strtol(text, &end, 10);
    if (end == text || *end != '\0') {
        return false;
    }
    *out = value;
    return true;
}

static bool ParseUnsigned(const char *text, unsigned long *out) {
    if (text == NULL || text[0] == '\0' || text[0] == '-') {
        return false;
    }

    char *end = NULL;
    unsigned long value = strtoul(text, &end, 10);
    if (end == text || *end != '\0') {
        return false;
    }
    *out = value;
    return true;
}

static int FindName(const char *const *names, size_t count, const char *name) {
    for (size_t index = 0; index < count; index++) {
        if (strcmp(names[index], name) == 0) {
            return (int)index;
        }
    }
    return -1;
}

static bool ParseHeader(const char *text, const char *path, int line,
                        char *error, size_t cap) {
    const char *cursor = text;
    char magic[LEVEL_TOKEN_CAP];

    if (!NextToken(&cursor, magic, sizeof(magic)) || strcmp(magic, "balin-level") != 0) {
        snprintf(error, cap, "%s:%d: expected 'balin-level', got '%s'", path, line, text);
        return false;
    }

    char versionText[LEVEL_TOKEN_CAP];
    long version = 0;
    if (!NextToken(&cursor, versionText, sizeof(versionText)) ||
        !ParseLong(versionText, &version)) {
        snprintf(error, cap, "%s:%d: expected a version number", path, line);
        return false;
    }

    if (version != LEVEL_VERSION) {
        snprintf(error, cap, "%s:%d: version %ld is not supported (expected %d)",
                 path, line, version, LEVEL_VERSION);
        return false;
    }

    char extra[LEVEL_TOKEN_CAP];
    if (NextToken(&cursor, extra, sizeof(extra))) {
        snprintf(error, cap, "%s:%d: unexpected trailing tokens", path, line);
        return false;
    }
    return true;
}

static bool ParseSize(const char *text, const char *path, int line, Level *level,
                      char *error, size_t cap) {
    const char *cursor = text;
    char keyword[LEVEL_TOKEN_CAP];

    if (!NextToken(&cursor, keyword, sizeof(keyword)) || strcmp(keyword, "size") != 0) {
        snprintf(error, cap, "%s:%d: expected 'size', got '%s'", path, line, text);
        return false;
    }

    char widthText[LEVEL_TOKEN_CAP];
    char heightText[LEVEL_TOKEN_CAP];
    long width = 0;
    long height = 0;
    if (!NextToken(&cursor, widthText, sizeof(widthText)) || !ParseLong(widthText, &width) ||
        !NextToken(&cursor, heightText, sizeof(heightText)) || !ParseLong(heightText, &height)) {
        snprintf(error, cap, "%s:%d: size expects width and height", path, line);
        return false;
    }

    if (width < 1 || width > LEVEL_MAX_SIZE || height < 1 || height > LEVEL_MAX_SIZE) {
        snprintf(error, cap, "%s:%d: size must be 1 to %d, got %ld x %ld",
                 path, line, LEVEL_MAX_SIZE, width, height);
        return false;
    }

    char extra[LEVEL_TOKEN_CAP];
    if (NextToken(&cursor, extra, sizeof(extra))) {
        snprintf(error, cap, "%s:%d: unexpected trailing tokens", path, line);
        return false;
    }

    level->width = (uint16_t)width;
    level->height = (uint16_t)height;
    return true;
}

static bool ParseLayerKeyword(const char *text, const char *path, int line,
                              int layerIndex, char *error, size_t cap) {
    const char *cursor = text;
    char keyword[LEVEL_TOKEN_CAP];

    if (!NextToken(&cursor, keyword, sizeof(keyword)) ||
        strcmp(keyword, layerNames[layerIndex]) != 0) {
        snprintf(error, cap, "%s:%d: expected '%s' layer, got '%s'",
                 path, line, layerNames[layerIndex], text);
        return false;
    }

    char extra[LEVEL_TOKEN_CAP];
    if (NextToken(&cursor, extra, sizeof(extra))) {
        snprintf(error, cap, "%s:%d: unexpected trailing tokens", path, line);
        return false;
    }
    return true;
}

// Parses one row of RLE runs: a run is 'id' or 'id*count'. Runs must fill the
// row exactly. Id 0 is an empty tile. Wall tiles with a nonzero id get
// TILE_FLAG_SOLID; the file stores ids only.
static bool ParseRow(const char *text, const char *path, int line, Level *level,
                     int layerIndex, int row, char *error, size_t cap) {
    Tile tiles[LEVEL_MAX_SIZE];
    uint32_t total = 0;

    const char *cursor = text;
    char token[LEVEL_TOKEN_CAP];
    while (NextToken(&cursor, token, sizeof(token))) {
        unsigned long id = 0;
        unsigned long count = 1;
        char *star = strchr(token, '*');

        if (star == NULL) {
            if (!ParseUnsigned(token, &id) || id > 0xFFFFul) {
                snprintf(error, cap, "%s:%d: bad run '%s'", path, line, token);
                return false;
            }
        } else {
            *star = '\0';
            bool wellFormed = ParseUnsigned(token, &id) && id <= 0xFFFFul &&
                              ParseUnsigned(star + 1, &count);
            *star = '*';
            if (!wellFormed || count == 0) {
                snprintf(error, cap, "%s:%d: bad run '%s'", path, line, token);
                return false;
            }
        }

        if (count > level->width) {
            snprintf(error, cap, "%s:%d: run count %lu is larger than the %u tile width",
                     path, line, count, level->width);
            return false;
        }

        Tile tile;
        tile.id = (uint16_t)id;
        tile.flags = 0;
        if (layerIndex == LEVEL_LAYER_WALL && id != 0) {
            tile.flags = TILE_FLAG_SOLID;
        }

        for (unsigned long index = 0; index < count; index++) {
            if (total < level->width) {
                tiles[total] = tile;
            }
            total++;
        }
    }

    if (total != level->width) {
        snprintf(error, cap, "%s:%d: row has %lu tiles, expected %u",
                 path, line, (unsigned long)total, level->width);
        return false;
    }

    Tile *destination = level->layers[layerIndex].tiles + (size_t)row * level->width;
    memcpy(destination, tiles, (size_t)level->width * sizeof(Tile));
    return true;
}

static bool ParsePoint(const char *xText, const char *yText, const Level *level,
                       const char *path, int line, float *x, float *y,
                       char *error, size_t cap) {
    long xValue = 0;
    long yValue = 0;

    if (!ParseLong(xText, &xValue)) {
        snprintf(error, cap, "%s:%d: bad coordinate '%s'", path, line, xText);
        return false;
    }
    if (!ParseLong(yText, &yValue)) {
        snprintf(error, cap, "%s:%d: bad coordinate '%s'", path, line, yText);
        return false;
    }

    long maxX = (long)level->width * LEVEL_TILE_PIXELS;
    long maxY = (long)level->height * LEVEL_TILE_PIXELS;
    if (xValue < 0 || xValue >= maxX || yValue < 0 || yValue >= maxY) {
        snprintf(error, cap, "%s:%d: coordinate %ld,%ld is outside the %ldx%ld section",
                 path, line, xValue, yValue, maxX, maxY);
        return false;
    }

    *x = (float)xValue;
    *y = (float)yValue;
    return true;
}

// Points at the first record of each kind pushed into the arena during this
// parse. Records of one kind are contiguous, since nothing else pushes while
// the record stage runs.
typedef struct RecordSink {
    Level *level;
    Arena *arena;
    LevelEntity *firstEntity;
    LevelGate *firstGate;
    LevelShrine *firstShrine;
} RecordSink;

static bool ParseEntity(RecordSink *sink, const char *cursor, const char *path,
                        int line, char *error, size_t cap) {
    char kindText[LEVEL_TOKEN_CAP];
    char xText[LEVEL_TOKEN_CAP];
    char yText[LEVEL_TOKEN_CAP];
    char extra[LEVEL_TOKEN_CAP];
    if (!NextToken(&cursor, kindText, sizeof(kindText)) ||
        !NextToken(&cursor, xText, sizeof(xText)) ||
        !NextToken(&cursor, yText, sizeof(yText)) ||
        NextToken(&cursor, extra, sizeof(extra))) {
        snprintf(error, cap, "%s:%d: entity expects kind x y", path, line);
        return false;
    }

    int kind = FindName(entityKindNames, LEVEL_ENTITY_KIND_COUNT, kindText);
    if (kind < 0) {
        snprintf(error, cap, "%s:%d: unknown entity kind '%s'", path, line, kindText);
        return false;
    }

    float x = 0.0f;
    float y = 0.0f;
    if (!ParsePoint(xText, yText, sink->level, path, line, &x, &y, error, cap)) {
        return false;
    }

    if (sink->level->entityCount == UINT16_MAX) {
        snprintf(error, cap, "%s:%d: too many entity records", path, line);
        return false;
    }

    LevelEntity *entity = ArenaPush(sink->arena, sizeof(LevelEntity), _Alignof(LevelEntity));
    entity->kind = (uint8_t)kind;
    entity->x = x;
    entity->y = y;
    if (sink->firstEntity == NULL) {
        sink->firstEntity = entity;
    }
    sink->level->entityCount++;
    return true;
}

static bool ParseGate(RecordSink *sink, const char *cursor, const char *path,
                      int line, char *error, size_t cap) {
    char xText[LEVEL_TOKEN_CAP];
    char yText[LEVEL_TOKEN_CAP];
    char capKeyword[LEVEL_TOKEN_CAP];
    char capText[LEVEL_TOKEN_CAP];
    char extra[LEVEL_TOKEN_CAP];
    if (!NextToken(&cursor, xText, sizeof(xText)) ||
        !NextToken(&cursor, yText, sizeof(yText)) ||
        !NextToken(&cursor, capKeyword, sizeof(capKeyword)) ||
        !NextToken(&cursor, capText, sizeof(capText)) ||
        NextToken(&cursor, extra, sizeof(extra))) {
        snprintf(error, cap, "%s:%d: gate expects x y cap N", path, line);
        return false;
    }

    if (strcmp(capKeyword, "cap") != 0) {
        snprintf(error, cap, "%s:%d: gate expects 'cap', got '%s'", path, line, capKeyword);
        return false;
    }

    long faithCap = 0;
    if (!ParseLong(capText, &faithCap)) {
        snprintf(error, cap, "%s:%d: gate cap expects an integer, got '%s'",
                 path, line, capText);
        return false;
    }
    if (faithCap < 0 || faithCap > 100) {
        snprintf(error, cap, "%s:%d: gate cap must be 0 to 100, got %ld",
                 path, line, faithCap);
        return false;
    }

    float x = 0.0f;
    float y = 0.0f;
    if (!ParsePoint(xText, yText, sink->level, path, line, &x, &y, error, cap)) {
        return false;
    }

    if (sink->level->gateCount == UINT16_MAX) {
        snprintf(error, cap, "%s:%d: too many gate records", path, line);
        return false;
    }

    LevelGate *gate = ArenaPush(sink->arena, sizeof(LevelGate), _Alignof(LevelGate));
    gate->x = x;
    gate->y = y;
    gate->faithCap = (uint8_t)faithCap;
    if (sink->firstGate == NULL) {
        sink->firstGate = gate;
    }
    sink->level->gateCount++;
    return true;
}

static bool ParseShrine(RecordSink *sink, const char *cursor, const char *path,
                        int line, char *error, size_t cap) {
    char kindText[LEVEL_TOKEN_CAP];
    char xText[LEVEL_TOKEN_CAP];
    char yText[LEVEL_TOKEN_CAP];
    char extra[LEVEL_TOKEN_CAP];
    if (!NextToken(&cursor, kindText, sizeof(kindText)) ||
        !NextToken(&cursor, xText, sizeof(xText)) ||
        !NextToken(&cursor, yText, sizeof(yText)) ||
        NextToken(&cursor, extra, sizeof(extra))) {
        snprintf(error, cap, "%s:%d: shrine expects kind x y", path, line);
        return false;
    }

    int kind = FindName(shrineKindNames, LEVEL_SHRINE_KIND_COUNT, kindText);
    if (kind < 0) {
        snprintf(error, cap, "%s:%d: unknown shrine kind '%s'", path, line, kindText);
        return false;
    }

    float x = 0.0f;
    float y = 0.0f;
    if (!ParsePoint(xText, yText, sink->level, path, line, &x, &y, error, cap)) {
        return false;
    }

    if (sink->level->shrineCount == UINT16_MAX) {
        snprintf(error, cap, "%s:%d: too many shrine records", path, line);
        return false;
    }

    LevelShrine *shrine = ArenaPush(sink->arena, sizeof(LevelShrine), _Alignof(LevelShrine));
    shrine->kind = (uint8_t)kind;
    shrine->x = x;
    shrine->y = y;
    if (sink->firstShrine == NULL) {
        sink->firstShrine = shrine;
    }
    sink->level->shrineCount++;
    return true;
}

static bool ParseRecord(RecordSink *sink, const char *text, const char *path,
                        int line, char *error, size_t cap) {
    const char *cursor = text;
    char keyword[LEVEL_TOKEN_CAP];
    if (!NextToken(&cursor, keyword, sizeof(keyword))) {
        return true; // Unreachable: blank lines are skipped before this stage.
    }

    if (strcmp(keyword, "entity") == 0) {
        return ParseEntity(sink, cursor, path, line, error, cap);
    }
    if (strcmp(keyword, "gate") == 0) {
        return ParseGate(sink, cursor, path, line, error, cap);
    }
    if (strcmp(keyword, "shrine") == 0) {
        return ParseShrine(sink, cursor, path, line, error, cap);
    }

    snprintf(error, cap, "%s:%d: unknown record '%s'", path, line, keyword);
    return false;
}

typedef enum ParseStage {
    STAGE_HEADER = 0,
    STAGE_SIZE,
    STAGE_LAYERS,
    STAGE_RECORDS,
} ParseStage;

bool LevelParse(Level *out, const char *path, Arena *arena, char *error, size_t cap) {
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        snprintf(error, cap, "%s: cannot open: %s", path, strerror(errno));
        return false;
    }

    Level parsed;
    memset(&parsed, 0, sizeof(parsed));
    RecordSink sink = { &parsed, arena, NULL, NULL, NULL };

    ParseStage stage = STAGE_HEADER;
    int layerIndex = 0;
    int layerRow = 0;
    bool layerStarted = false;
    bool ok = true;
    char line[LEVEL_LINE_CAP];
    int lineNumber = 0;

    while (ok && fgets(line, sizeof(line), file) != NULL) {
        lineNumber++;

        size_t length = strlen(line);
        if (length == sizeof(line) - 1 && line[length - 1] != '\n') {
            snprintf(error, cap, "%s:%d: line is too long", path, lineNumber);
            ok = false;
            break;
        }

        char *text = TrimLeft(line);
        char *comment = strchr(text, '#');
        if (comment != NULL) {
            *comment = '\0';
        }
        TrimRight(text);
        if (*text == '\0') {
            continue;
        }

        switch (stage) {
        case STAGE_HEADER:
            ok = ParseHeader(text, path, lineNumber, error, cap);
            if (ok) {
                stage = STAGE_SIZE;
            }
            break;

        case STAGE_SIZE:
            ok = ParseSize(text, path, lineNumber, &parsed, error, cap);
            if (ok) {
                stage = STAGE_LAYERS;
            }
            break;

        case STAGE_LAYERS:
            if (!layerStarted) {
                ok = ParseLayerKeyword(text, path, lineNumber, layerIndex, error, cap);
                if (ok) {
                    size_t tileCount = (size_t)parsed.width * parsed.height;
                    parsed.layers[layerIndex].tiles =
                        ArenaPush(arena, tileCount * sizeof(Tile), ARENA_DEFAULT_ALIGNMENT);
                    layerStarted = true;
                }
                break;
            }

            ok = ParseRow(text, path, lineNumber, &parsed, layerIndex, layerRow, error, cap);
            if (ok) {
                layerRow++;
                if (layerRow == parsed.height) {
                    layerIndex++;
                    layerRow = 0;
                    layerStarted = false;
                    if (layerIndex == LEVEL_LAYER_COUNT) {
                        stage = STAGE_RECORDS;
                    }
                }
            }
            break;

        case STAGE_RECORDS:
            ok = ParseRecord(&sink, text, path, lineNumber, error, cap);
            break;
        }
    }

    if (ok && stage != STAGE_RECORDS) {
        if (stage == STAGE_HEADER) {
            snprintf(error, cap, "%s: unexpected end of file, expected 'balin-level' header", path);
        } else if (stage == STAGE_SIZE) {
            snprintf(error, cap, "%s: unexpected end of file, expected 'size'", path);
        } else if (!layerStarted) {
            snprintf(error, cap, "%s: unexpected end of file, expected '%s' layer",
                     path, layerNames[layerIndex]);
        } else {
            snprintf(error, cap, "%s: unexpected end of file, expected '%s' layer row %d",
                     path, layerNames[layerIndex], layerRow + 1);
        }
        ok = false;
    }

    fclose(file);

    if (!ok) {
        return false;
    }

    parsed.entities = sink.firstEntity;
    parsed.gates = sink.firstGate;
    parsed.shrines = sink.firstShrine;
    *out = parsed;
    return true;
}

// Text output shared by LevelSerialize (buffer) and LevelSave (file). The
// first failure sticks, so the caller checks failed once at the end.
typedef struct TextSink {
    char *buffer;
    size_t cap;
    size_t used;
    FILE *file;
    bool failed;
} TextSink;

static void SinkPrintf(TextSink *sink, const char *format, ...) {
    if (sink->failed) {
        return;
    }

    char text[128];
    va_list args;
    va_start(args, format);
    int length = vsnprintf(text, sizeof(text), format, args);
    va_end(args);

    if (length < 0 || (size_t)length >= sizeof(text)) {
        sink->failed = true;
        return;
    }

    if (sink->file != NULL) {
        if (fwrite(text, 1, (size_t)length, sink->file) != (size_t)length) {
            sink->failed = true;
        }
        return;
    }

    if (sink->used + (size_t)length + 1 > sink->cap) {
        sink->failed = true;
        return;
    }

    memcpy(sink->buffer + sink->used, text, (size_t)length);
    sink->used += (size_t)length;
    sink->buffer[sink->used] = '\0';
}

static void WriteRow(TextSink *sink, const Tile *tiles, uint16_t width) {
    uint16_t runStart = 0;

    for (uint16_t column = 1; column <= width; column++) {
        if (column < width && tiles[column].id == tiles[runStart].id) {
            continue;
        }

        uint16_t count = (uint16_t)(column - runStart);
        if (count == 1) {
            SinkPrintf(sink, "%u", tiles[runStart].id);
        } else {
            SinkPrintf(sink, "%u*%u", tiles[runStart].id, count);
        }
        if (column < width) {
            SinkPrintf(sink, " ");
        }
        runStart = column;
    }
    SinkPrintf(sink, "\n");
}

static void WriteLevel(TextSink *sink, const Level *level) {
    SinkPrintf(sink, "balin-level %d\n", LEVEL_VERSION);
    SinkPrintf(sink, "size %u %u\n\n", level->width, level->height);

    for (int layer = 0; layer < LEVEL_LAYER_COUNT; layer++) {
        SinkPrintf(sink, "%s\n", layerNames[layer]);
        for (uint16_t row = 0; row < level->height; row++) {
            WriteRow(sink, level->layers[layer].tiles + (size_t)row * level->width, level->width);
        }
        SinkPrintf(sink, "\n");
    }

    for (uint16_t index = 0; index < level->entityCount; index++) {
        const LevelEntity *entity = &level->entities[index];
        if (entity->kind >= LEVEL_ENTITY_KIND_COUNT) {
            sink->failed = true;
            return;
        }
        SinkPrintf(sink, "entity %s %.0f %.0f\n",
                   entityKindNames[entity->kind], entity->x, entity->y);
    }

    for (uint16_t index = 0; index < level->gateCount; index++) {
        const LevelGate *gate = &level->gates[index];
        SinkPrintf(sink, "gate %.0f %.0f cap %u\n", gate->x, gate->y, gate->faithCap);
    }

    for (uint16_t index = 0; index < level->shrineCount; index++) {
        const LevelShrine *shrine = &level->shrines[index];
        if (shrine->kind >= LEVEL_SHRINE_KIND_COUNT) {
            sink->failed = true;
            return;
        }
        SinkPrintf(sink, "shrine %s %.0f %.0f\n",
                   shrineKindNames[shrine->kind], shrine->x, shrine->y);
    }
}

bool LevelSerialize(const Level *level, char *out, size_t cap, char *error, size_t errorCap) {
    if (out == NULL || cap == 0) {
        snprintf(error, errorCap, "level text buffer is too small");
        return false;
    }

    out[0] = '\0';
    TextSink sink = { .buffer = out, .cap = cap, .used = 0, .file = NULL, .failed = false };
    WriteLevel(&sink, level);

    if (sink.failed) {
        snprintf(error, errorCap, "level text needs more than %zu bytes", cap);
        return false;
    }
    return true;
}

bool LevelSave(const Level *level, const char *path, char *error, size_t errorCap) {
    FILE *file = fopen(path, "w");
    if (file == NULL) {
        snprintf(error, errorCap, "%s: cannot open: %s", path, strerror(errno));
        return false;
    }

    TextSink sink = { .buffer = NULL, .cap = 0, .used = 0, .file = file, .failed = false };
    WriteLevel(&sink, level);

    if (sink.failed) {
        fclose(file);
        snprintf(error, errorCap, "%s: cannot write level", path);
        return false;
    }

    if (fclose(file) != 0) {
        snprintf(error, errorCap, "%s: cannot close: %s", path, strerror(errno));
        return false;
    }
    return true;
}
