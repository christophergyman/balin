#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "engine/arena.h"
#include "engine/log.h"
#include "engine/time.h"
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

static int TicksFor(int frames, double frameSeconds) {
    Clock clock;
    ClockInit(&clock);

    for (int frame = 0; frame < frames; frame++) {
        int steps = ClockBeginFrame(&clock, frameSeconds);
        for (int step = 0; step < steps; step++) {
            ClockBeginTick(&clock);
        }
    }

    return (int)clock.tick;
}

static void TestClockStepsPerFrame(void) {
    Clock clock;
    ClockInit(&clock);

    // 30 FPS needs two steps per frame, 60 FPS needs one.
    assert(ClockBeginFrame(&clock, 1.0 / 30.0) == 2);
    ClockDropPending(&clock);
    assert(ClockBeginFrame(&clock, 1.0 / 60.0) == 1);
    ClockDropPending(&clock);

    // 120 FPS is half a tick per frame, so a step every other frame.
    assert(ClockBeginFrame(&clock, 1.0 / 120.0) == 0);
    assert(ClockBeginFrame(&clock, 1.0 / 120.0) == 1);
}

static void TestClockAlpha(void) {
    Clock clock;
    ClockInit(&clock);

    ClockBeginFrame(&clock, 0.5 * TICK_DT);
    assert(ClockAlpha(&clock) > 0.49f && ClockAlpha(&clock) < 0.51f);

    ClockBeginFrame(&clock, 0.5 * TICK_DT);
    ClockBeginTick(&clock);
    assert(ClockAlpha(&clock) < 0.001f);

    ClockDropPending(&clock);
    assert(ClockAlpha(&clock) == 0.0f);
}

static void TestClockFrameRateIndependence(void) {
    // Two seconds of frames. All three rates must run the same tick count,
    // per the ADR-004 confirmation.
    assert(TicksFor(60, 1.0 / 30.0) == 2 * TICK_RATE);
    assert(TicksFor(120, 1.0 / 60.0) == 2 * TICK_RATE);
    assert(TicksFor(240, 1.0 / 120.0) == 2 * TICK_RATE);

    // 144 FPS does not divide the tick. The remainder carries in the
    // accumulator, so the count stays within one tick of two seconds.
    int at144 = TicksFor(288, 1.0 / 144.0);
    assert(at144 >= 2 * TICK_RATE - 1 && at144 <= 2 * TICK_RATE);

    // Dash stand-in: distance is speed * ticks * dt. Equal ticks give equal
    // distance at every frame rate. The real dash re-checks this in BAL-23.
    double speed = 120.0; // Pixels per second.
    double at30 = speed * TICK_DT * (double)TicksFor(60, 1.0 / 30.0);
    double at60 = speed * TICK_DT * (double)TicksFor(120, 1.0 / 60.0);
    double at120 = speed * TICK_DT * (double)TicksFor(240, 1.0 / 120.0);
    assert(at30 == at60);
    assert(at60 == at120);
}

static void TestClockCatchUpCap(void) {
    Clock clock;
    ClockInit(&clock);

    // A one second hitch runs at most five steps and drops the debt.
    int steps = ClockBeginFrame(&clock, 1.0);
    assert(steps == MAX_STEPS_PER_FRAME);
    for (int step = 0; step < steps; step++) {
        ClockBeginTick(&clock);
    }
    assert(clock.tick == MAX_STEPS_PER_FRAME);

    // The next frame does not try to catch up.
    assert(ClockBeginFrame(&clock, 1.0 / 60.0) == 1);
}

int main(void) {
    TestScaffold();
    TestLog();
    TestArenaPushAndAlignment();
    TestArenaReset();
    TestGameMemory();
    TestClockStepsPerFrame();
    TestClockAlpha();
    TestClockFrameRateIndependence();
    TestClockCatchUpCap();

    printf("balin_tests: all tests passed\n");
    return 0;
}
