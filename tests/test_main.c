#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "engine/arena.h"
#include "engine/log.h"
#include "game/memory.h"

static void WriteTextFile(const char *path, const char *text) {
    FILE *file = fopen(path, "w");
    assert(file != NULL);
    fputs(text, file);
    fclose(file);
}

static void ReadTextFile(const char *path, char *out, size_t cap) {
    FILE *file = fopen(path, "r");
    assert(file != NULL);
    size_t length = fread(out, 1, cap - 1, file);
    out[length] = '\0';
    fclose(file);
}

// Scaffold smoke test. Real tests land with the engine modules.
static void TestScaffold(void) {
    assert(1 == 1);
}

static void TestLog(void) {
    mkdir("balin_test_tmp", 0755);

    WriteTextFile("balin_test_tmp/log.cfg", "# tests\ndefault = warn\n");
    LogInit("balin_test_tmp/log.cfg", "balin_test_tmp/logs");
    LogSetContext(42, 3);

    LOGD(LOG_CAT_ENGINE, "hidden debug");
    LOGI(LOG_CAT_ENGINE, "hidden info");
    LOGW(LOG_CAT_GAME, "visible %s", "warn");
    assert(LogFormattedCount() == 1);

    LogFlush();

    char contents[4096];
    ReadTextFile(LogSessionPath(), contents, sizeof(contents));
    assert(strstr(contents, "level=warn") != NULL);
    assert(strstr(contents, "cat=game") != NULL);
    assert(strstr(contents, "tick=42") != NULL);
    assert(strstr(contents, "run=3") != NULL);
    assert(strstr(contents, "msg=\"visible warn\"") != NULL);

    char error[256];

    WriteTextFile("balin_test_tmp/log.cfg", "default = trace\n");
    assert(LogConfigLoad("balin_test_tmp/log.cfg", error, sizeof(error)));
    uint64_t before = LogFormattedCount();
    LOGD(LOG_CAT_ENGINE, "now visible");
    assert(LogFormattedCount() == before + 1);

    WriteTextFile("balin_test_tmp/log.cfg", "default = error\nengine = trace\n");
    assert(LogConfigLoad("balin_test_tmp/log.cfg", error, sizeof(error)));
    before = LogFormattedCount();
    LOGT(LOG_CAT_ENGINE, "engine trace");
    LOGT(LOG_CAT_GAME, "game hidden");
    assert(LogFormattedCount() == before + 1);

    WriteTextFile("balin_test_tmp/log.cfg", "default = verbose\n");
    assert(!LogConfigLoad("balin_test_tmp/log.cfg", error, sizeof(error)));
    assert(strstr(error, ":1:") != NULL);
    assert(strstr(error, "unknown level") != NULL);

    WriteTextFile("balin_test_tmp/log.cfg", "default = info\nnoise = warn\n");
    assert(!LogConfigLoad("balin_test_tmp/log.cfg", error, sizeof(error)));
    assert(strstr(error, "unknown key") != NULL);

    WriteTextFile("balin_test_tmp/log.cfg", "engine = warn\n");
    assert(!LogConfigLoad("balin_test_tmp/log.cfg", error, sizeof(error)));
    assert(strstr(error, "missing 'default'") != NULL);

    LogShutdown();
}

static void TestArenaPushAndAlignment(void) {
    // Start one byte into the buffer so the base is deliberately misaligned.
    // ArenaPush must align the absolute address, not only the offset.
    static _Alignas(64) unsigned char storage[1024 + 64];
    Arena arena;
    ArenaInit(&arena, "test", storage + 1, sizeof(storage) - 1);

    assert(ArenaUsed(&arena) == 0);
    assert(ArenaCapacity(&arena) == sizeof(storage) - 1);

    for (size_t alignment = 1; alignment <= 64; alignment *= 2) {
        void *block = ArenaPush(&arena, 3, alignment);
        assert(((uintptr_t)block % alignment) == 0);
        assert((unsigned char *)block >= storage + 1);
        assert((unsigned char *)block + 3 <= storage + sizeof(storage));
    }

    void *first = ArenaPush(&arena, 16, ARENA_DEFAULT_ALIGNMENT);
    size_t afterFirst = ArenaUsed(&arena);
    void *second = ArenaPush(&arena, 16, ARENA_DEFAULT_ALIGNMENT);
    assert(second == (unsigned char *)first + 16);
    assert(ArenaUsed(&arena) == afterFirst + 16);
}

static void TestArenaReset(void) {
    static unsigned char storage[256];
    Arena arena;
    ArenaInit(&arena, "test", storage, sizeof(storage));

    void *first = ArenaPush(&arena, 100, 8);
    assert(ArenaUsed(&arena) > 0);

    ArenaReset(&arena);
    assert(ArenaUsed(&arena) == 0);

    void *again = ArenaPush(&arena, 8, 8);
    assert(again == first);
}

static void TestGameMemory(void) {
    GameMemoryInit();

    assert(ArenaUsed(GameArenaAsset()) == 0);
    assert(ArenaUsed(GameArenaLevel()) == 0);
    assert(ArenaUsed(GameArenaRun()) == 0);
    assert(ArenaUsed(GameArenaFrame()) == 0);

    ArenaPush(GameArenaAsset(), 64, ARENA_DEFAULT_ALIGNMENT);
    ArenaPush(GameArenaLevel(), 64, ARENA_DEFAULT_ALIGNMENT);
    ArenaPush(GameArenaRun(), 64, ARENA_DEFAULT_ALIGNMENT);
    ArenaPush(GameArenaFrame(), 64, ARENA_DEFAULT_ALIGNMENT);

    GameMemoryResetFrame();
    assert(ArenaUsed(GameArenaFrame()) == 0);
    assert(ArenaUsed(GameArenaAsset()) == 64);
    assert(ArenaUsed(GameArenaLevel()) == 64);
    assert(ArenaUsed(GameArenaRun()) == 64);

    GameMemoryResetRun();
    assert(ArenaUsed(GameArenaRun()) == 0);
    assert(ArenaUsed(GameArenaAsset()) == 64);
    assert(ArenaUsed(GameArenaLevel()) == 64);
}

int main(void) {
    TestScaffold();
    TestLog();
    TestArenaPushAndAlignment();
    TestArenaReset();
    TestGameMemory();

    printf("balin_tests: all tests passed\n");
    return 0;
}
