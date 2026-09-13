#include <assert.h>
#include <dirent.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "engine/arena.h"
#include "engine/input.h"
#include "engine/log.h"
#include "engine/time.h"
#include "game/ecs.h"
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
    LogInit("balin_test_tmp/log.cfg", "balin_test_tmp/logs", "balin_test_tmp/bugs");
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

static void TestSnapshotWriter(FILE *out) {
    fputs("test_marker=1\n", out);
}

static size_t MaxLineLength(const char *text) {
    size_t longest = 0;
    size_t current = 0;
    for (const char *cursor = text; *cursor != '\0'; cursor++) {
        if (*cursor == '\n') {
            longest = (current > longest) ? current : longest;
            current = 0;
        } else {
            current++;
        }
    }
    return longest;
}

static int CountLines(const char *text) {
    int lines = 0;
    for (const char *cursor = text; *cursor != '\0'; cursor++) {
        if (*cursor == '\n') {
            lines++;
        }
    }
    return lines;
}

static void TestLogRing(void) {
    mkdir("balin_test_tmp", 0755);
    WriteTextFile("balin_test_tmp/ring.cfg", "# tests\ndefault = warn\n");

    // A fresh root per run avoids the same-second bundle cap and stale files.
    char rootTemplate[] = "balin_test_tmp/ringbugs-XXXXXX";
    char *root = mkdtemp(rootTemplate);
    assert(root != NULL);

    LogInit("balin_test_tmp/ring.cfg", "balin_test_tmp/ringlogs", root);
    LogSetContext(7, 2);
    LogSetSnapshotWriter(TestSnapshotWriter);

    // Fill the ring and overflow it by one. The oldest event is evicted.
    for (int index = 0; index < LOG_RING_COUNT + 1; index++) {
        LOGT(LOG_CAT_ENGINE, "event-%d", index);
    }
    assert(LogRingCount() == LOG_RING_COUNT);

    // A muted level still lands in the ring, even after it is full, per ADR-020.
    LOGI(LOG_CAT_GAME, "late entry");

    // A long event is truncated to the slot size.
    char longText[400];
    memset(longText, 'x', sizeof(longText) - 1);
    longText[sizeof(longText) - 1] = '\0';
    LOGT(LOG_CAT_ENGINE, "%s", longText);

    // One visible event gives the session copy real content.
    LOGW(LOG_CAT_GAME, "visible warn");

    assert(LogDumpBundle("test"));

    static char contents[512 * 1024 + 1024];
    char path[512];

    snprintf(path, sizeof(path), "%s/ring.log", LogLastBundlePath());
    ReadTextFile(path, contents, sizeof(contents));
    assert(CountLines(contents) == LOG_RING_COUNT);
    assert(MaxLineLength(contents) <= LOG_RING_SLOT_BYTES - 1);
    assert(strstr(contents, "msg=\"late entry\"") != NULL);
    assert(strstr(contents, "level=info") != NULL);
    assert(strstr(contents, "msg=\"event-0\"") == NULL);
    assert(strstr(contents, "msg=\"event-2048\"") != NULL);

    // The session copy matches the live session file.
    char live[8192];
    char copied[8192];
    ReadTextFile(LogSessionPath(), live, sizeof(live));
    snprintf(path, sizeof(path), "%s/session.log", LogLastBundlePath());
    ReadTextFile(path, copied, sizeof(copied));
    assert(strcmp(live, copied) == 0);
    assert(strstr(copied, "msg=\"visible warn\"") != NULL);

    // The state snapshot carries tick, run, reason, and the game section.
    snprintf(path, sizeof(path), "%s/state.txt", LogLastBundlePath());
    ReadTextFile(path, contents, sizeof(contents));
    assert(strstr(contents, "tick=7") != NULL);
    assert(strstr(contents, "run=2") != NULL);
    assert(strstr(contents, "reason=test") != NULL);
    assert(strstr(contents, "test_marker=1") != NULL);

    snprintf(path, sizeof(path), "%s/env.txt", LogLastBundlePath());
    ReadTextFile(path, contents, sizeof(contents));
    assert(strstr(contents, "os=") != NULL);
    assert(strstr(contents, "build=") != NULL);

    LogShutdown();
}

static void TestLogAssertBundle(void) {
    mkdir("balin_test_tmp", 0755);
    WriteTextFile("balin_test_tmp/fatal.cfg", "# tests\ndefault = trace\n");

    // A fresh root per run avoids stale bundles and PID reuse.
    char rootTemplate[] = "balin_test_tmp/fatalbugs-XXXXXX";
    char *root = mkdtemp(rootTemplate);
    assert(root != NULL);

    pid_t pid = fork();
    assert(pid >= 0);

    if (pid == 0) {
        // Child: a failed ASSERT must write a bundle, then exit non-zero.
        // The trace flood fills the session buffer, so the fatal line must
        // force a flush to reach the copied session.log.
        (void)freopen("/dev/null", "w", stdout);
        LogInit("balin_test_tmp/fatal.cfg", "balin_test_tmp/fatallogs", root);
        for (int index = 0; index < 1500; index++) {
            LOGT(LOG_CAT_ENGINE, "filler-%d", index);
        }
        LOGI(LOG_CAT_BOOT, "before the failure");
        ASSERT(false, LOG_CAT_ENGINE, "fatal test");
        _exit(2); // Not reached when ASSERT dumps and exits.
    }

    int status = 0;
    assert(waitpid(pid, &status, 0) == pid);
    assert(WIFEXITED(status));
    assert(WEXITSTATUS(status) == 1);

    // Exactly one bundle directory, auto-written by the fatal path.
    DIR *dir = opendir(root);
    assert(dir != NULL);

    char bundle[512] = "";
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] != '.') {
            assert(bundle[0] == '\0');
            snprintf(bundle, sizeof(bundle), "%s/%s", root, entry->d_name);
        }
    }
    closedir(dir);
    assert(bundle[0] != '\0');

    // The ring dump can reach 512 KB, and the session copy is larger than the
    // session buffer, so the read must cover both.
    static char contents[512 * 1024 + 1024];
    char path[512];
    snprintf(path, sizeof(path), "%s/ring.log", bundle);
    ReadTextFile(path, contents, sizeof(contents));
    assert(strstr(contents, "level=fatal") != NULL);
    assert(strstr(contents, "ASSERT failed") != NULL);
    assert(strstr(contents, "msg=\"before the failure\"") != NULL);

    // The fatal line survives a full session buffer.
    snprintf(path, sizeof(path), "%s/session.log", bundle);
    ReadTextFile(path, contents, sizeof(contents));
    assert(strstr(contents, "level=fatal") != NULL);
    assert(strstr(contents, "ASSERT failed") != NULL);

    snprintf(path, sizeof(path), "%s/state.txt", bundle);
    ReadTextFile(path, contents, sizeof(contents));
    assert(strstr(contents, "reason=assert") != NULL);
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

static void TestInputHeldAndEdges(void) {
    InputInit();

    uint32_t bit = ACTION_BIT(ACTION_MOVE_UP);
    InputFrame press = { bit, bit, 0 };
    InputBeginFrame(&press);
    InputBeginTick();
    assert(InputHeld(ACTION_MOVE_UP));
    assert(InputPressed(ACTION_MOVE_UP));
    assert(!InputReleased(ACTION_MOVE_UP));

    // The press edge does not repeat on the next tick.
    InputFrame hold = { bit, 0, 0 };
    InputBeginFrame(&hold);
    InputBeginTick();
    assert(InputHeld(ACTION_MOVE_UP));
    assert(!InputPressed(ACTION_MOVE_UP));
    assert(!InputReleased(ACTION_MOVE_UP));

    InputFrame release = { 0, 0, bit };
    InputBeginFrame(&release);
    InputBeginTick();
    assert(!InputHeld(ACTION_MOVE_UP));
    assert(!InputPressed(ACTION_MOVE_UP));
    assert(InputReleased(ACTION_MOVE_UP));

    // The release edge does not repeat either.
    InputFrame idle = { 0, 0, 0 };
    InputBeginFrame(&idle);
    InputBeginTick();
    assert(!InputHeld(ACTION_MOVE_UP));
    assert(!InputReleased(ACTION_MOVE_UP));
}

static void TestInputTapWithinOneFrame(void) {
    InputInit();

    // A press and release can land between two samples. Both edges survive
    // even though held stays false.
    uint32_t bit = ACTION_BIT(ACTION_ATTACK);
    InputFrame tap = { 0, bit, bit };
    InputBeginFrame(&tap);
    InputBeginTick();

    assert(!InputHeld(ACTION_ATTACK));
    assert(InputPressed(ACTION_ATTACK));
    assert(InputReleased(ACTION_ATTACK));
    assert(InputBuffered(ACTION_ATTACK));
}

static void TestInputEdgeLatchesAcrossFrames(void) {
    InputInit();

    // 120 FPS: the press frame owes no tick. The latch carries the edge into
    // the next frame's tick.
    uint32_t bit = ACTION_BIT(ACTION_DASH);
    InputFrame press = { bit, bit, 0 };
    InputFrame hold = { bit, 0, 0 };
    InputBeginFrame(&press);
    InputBeginFrame(&hold);
    InputBeginTick();

    assert(InputPressed(ACTION_DASH));
    assert(InputHeld(ACTION_DASH));
    assert(InputBuffered(ACTION_DASH));

    // The latch is consumed: the following tick has no edge.
    InputBeginFrame(&hold);
    InputBeginTick();
    assert(!InputPressed(ACTION_DASH));
    assert(InputHeld(ACTION_DASH));
}

// Simulates main.c: one frame sample, then the fixed steps the clock owes,
// each with an input tick. The action is held from pressFrame to releaseFrame.
static void CountActionEdges(double frameSeconds, int frames, int pressFrame, int releaseFrame,
                             int *pressedTicks, int *releasedTicks) {
    InputInit();
    Clock clock;
    ClockInit(&clock);
    *pressedTicks = 0;
    *releasedTicks = 0;

    for (int frame = 0; frame < frames; frame++) {
        uint32_t bit = ACTION_BIT(ACTION_ATTACK);
        InputFrame sample = { 0, 0, 0 };
        if (frame >= pressFrame && frame < releaseFrame) {
            sample.held |= bit;
        }
        if (frame == pressFrame) {
            sample.pressed |= bit;
        }
        if (frame == releaseFrame) {
            sample.released |= bit;
        }
        InputBeginFrame(&sample);

        int steps = ClockBeginFrame(&clock, frameSeconds);
        for (int step = 0; step < steps; step++) {
            ClockBeginTick(&clock);
            InputBeginTick();
            if (InputPressed(ACTION_ATTACK)) {
                (*pressedTicks)++;
            }
            if (InputReleased(ACTION_ATTACK)) {
                (*releasedTicks)++;
            }
        }
    }
}

static void TestInputEdgesAtAnyRenderRate(void) {
    // A press and release must land on exactly one tick at each render rate,
    // including rates that do not divide the tick.
    int pressed = 0;
    int released = 0;

    CountActionEdges(1.0 / 30.0, 60, 5, 30, &pressed, &released);
    assert(pressed == 1);
    assert(released == 1);

    CountActionEdges(1.0 / 60.0, 120, 10, 60, &pressed, &released);
    assert(pressed == 1);
    assert(released == 1);

    CountActionEdges(1.0 / 120.0, 240, 20, 120, &pressed, &released);
    assert(pressed == 1);
    assert(released == 1);

    CountActionEdges(1.0 / 144.0, 288, 24, 144, &pressed, &released);
    assert(pressed == 1);
    assert(released == 1);
}

static void TestInputBufferWindow(void) {
    InputInit();

    uint32_t bit = ACTION_BIT(ACTION_ATTACK);
    InputFrame press = { bit, bit, 0 };
    InputFrame hold = { bit, 0, 0 };

    InputBeginFrame(&press);
    InputBeginTick();
    assert(InputBuffered(ACTION_ATTACK));

    // Valid through tick six: the press tick plus five.
    for (int tick = 1; tick < INPUT_BUFFER_TICKS; tick++) {
        InputBeginFrame(&hold);
        InputBeginTick();
        assert(InputBuffered(ACTION_ATTACK));
    }

    // Expired on tick seven.
    InputBeginFrame(&hold);
    InputBeginTick();
    assert(!InputBuffered(ACTION_ATTACK));
    assert(!InputConsumeBuffered(ACTION_ATTACK));
}

static void TestInputBufferConsume(void) {
    InputInit();

    uint32_t bit = ACTION_BIT(ACTION_DASH);
    InputFrame press = { bit, bit, 0 };
    InputFrame release = { 0, 0, bit };
    InputFrame idle = { 0, 0, 0 };

    InputBeginFrame(&press);
    InputBeginTick();

    // A fresh press is consumable on its own tick, and only once.
    assert(InputConsumeBuffered(ACTION_DASH));
    assert(!InputBuffered(ACTION_DASH));
    assert(!InputConsumeBuffered(ACTION_DASH));

    // Wait out the release, then press again to re-arm.
    InputBeginFrame(&release);
    InputBeginTick();
    InputBeginFrame(&idle);
    InputBeginTick();
    InputBeginFrame(&press);
    InputBeginTick();
    assert(InputConsumeBuffered(ACTION_DASH));
    assert(!InputConsumeBuffered(ACTION_DASH));
}

static void TestInputBufferOnlyAttackAndDash(void) {
    Action others[] = {
        ACTION_MOVE_UP,
        ACTION_MOVE_DOWN,
        ACTION_MOVE_LEFT,
        ACTION_MOVE_RIGHT,
        ACTION_SMITE,
        ACTION_BAG,
        ACTION_INTERACT,
    };

    for (size_t index = 0; index < sizeof(others) / sizeof(others[0]); index++) {
        InputInit();

        uint32_t bit = ACTION_BIT(others[index]);
        InputFrame press = { bit, bit, 0 };
        InputBeginFrame(&press);
        InputBeginTick();

        assert(!InputBuffered(others[index]));
        assert(!InputConsumeBuffered(others[index]));
    }
}

static void TestInputInvalidAction(void) {
    InputInit();

    // Out-of-range ids return false instead of indexing out of bounds.
    assert(!InputHeld((Action)ACTION_COUNT));
    assert(!InputPressed((Action)ACTION_COUNT));
    assert(!InputReleased((Action)ACTION_COUNT));
    assert(!InputBuffered((Action)ACTION_COUNT));
    assert(!InputConsumeBuffered((Action)ACTION_COUNT));

    assert(!InputHeld((Action)-1));
    assert(!InputBuffered((Action)-1));
    assert(!InputConsumeBuffered((Action)-1));
}

static void TestInputDropPending(void) {
    InputInit();

    uint32_t bit = ACTION_BIT(ACTION_ATTACK);
    InputFrame press = { bit, bit, 0 };
    InputBeginFrame(&press);
    InputBeginTick();
    assert(InputBuffered(ACTION_ATTACK));

    // Pause enter: a latched edge and a live buffer are dropped, so resume
    // does not replay them.
    InputBeginFrame(&press);
    InputDropPending();
    InputBeginTick();
    assert(!InputPressed(ACTION_ATTACK));
    assert(!InputHeld(ACTION_ATTACK));
    assert(!InputBuffered(ACTION_ATTACK));
}

// ECS test components. Real component ids land with their system tickets.
typedef enum {
    TEST_COMPONENT_POS = 0,
    TEST_COMPONENT_VEL = 1,
} TestComponentId;

typedef struct TestPos {
    float x;
    float y;
} TestPos;

typedef struct TestVel {
    float vx;
    float vy;
} TestVel;

static int CountQuery(const EcsWorld *world, EcsMask required) {
    EcsQuery query = EcsQueryAll(required);
    Entity entity;
    int count = 0;
    while (EcsQueryNext(world, &query, &entity)) {
        count++;
    }
    return count;
}

static bool QueryHas(const EcsWorld *world, EcsMask required, Entity wanted) {
    EcsQuery query = EcsQueryAll(required);
    Entity entity;
    while (EcsQueryNext(world, &query, &entity)) {
        if (entity == wanted) {
            return true;
        }
    }
    return false;
}

static void TestEcsCreate(void) {
    EcsWorld world;
    EcsInit(&world);

    Entity first = EcsCreate(&world);
    assert(first != ENTITY_INVALID);
    assert(EntityIndex(first) == 0);
    assert(EntityGeneration(first) == 0);
    assert(EcsAlive(&world, first));
    assert(EcsAliveCount(&world) == 1);

    // Fill the world. Fresh slots come in index order.
    for (uint16_t i = 1; i < ECS_MAX_ENTITIES; i++) {
        Entity entity = EcsCreate(&world);
        assert(entity != ENTITY_INVALID);
        assert(EntityIndex(entity) == i);
    }
    assert(EcsAliveCount(&world) == ECS_MAX_ENTITIES);

    // Slot 257 does not exist.
    assert(EcsCreate(&world) == ENTITY_INVALID);
}

static void TestEcsDestroyDeferred(void) {
    EcsWorld world;
    EcsInit(&world);

    Entity a = EcsCreate(&world);
    Entity b = EcsCreate(&world);

    assert(EcsDestroy(&world, a));
    assert(!EcsAlive(&world, a));
    assert(EcsAlive(&world, b));
    assert(EcsAliveCount(&world) == 1);

    // Double destroy and stale destroys are no-ops.
    assert(!EcsDestroy(&world, a));

    // The dead slot is not reused until the flush.
    Entity c = EcsCreate(&world);
    assert(EntityIndex(c) == 2);

    EcsFlush(&world);

    // The freed slot returns with a bumped generation.
    Entity d = EcsCreate(&world);
    assert(EntityIndex(d) == EntityIndex(a));
    assert(EntityGeneration(d) == EntityGeneration(a) + 1);
    assert(EcsAlive(&world, d));
    assert(!EcsAlive(&world, a));
}

static void TestEcsGenerationReuse(void) {
    EcsWorld world;
    EcsInit(&world);

    Entity first = EcsCreate(&world);
    assert(EcsDestroy(&world, first));
    EcsFlush(&world);

    Entity second = EcsCreate(&world);
    assert(EntityIndex(second) == EntityIndex(first));
    assert(EntityGeneration(second) == EntityGeneration(first) + 1);

    // A second reuse cycle bumps the generation again.
    assert(EcsDestroy(&world, second));
    EcsFlush(&world);
    Entity third = EcsCreate(&world);
    assert(EntityIndex(third) == EntityIndex(first));
    assert(EntityGeneration(third) == EntityGeneration(first) + 2);
}

static void TestEcsStaleHandles(void) {
    EcsWorld world;
    EcsInit(&world);

    static TestPos positions[ECS_MAX_ENTITIES];
    EcsPool posPool;
    EcsPoolInit(&posPool, TEST_COMPONENT_POS, positions, sizeof(TestPos));

    Entity first = EcsCreate(&world);
    assert(EcsAdd(&world, first, &posPool) != NULL);
    assert(EcsDestroy(&world, first));
    EcsFlush(&world);

    Entity second = EcsCreate(&world);
    assert(EntityIndex(second) == EntityIndex(first));

    // The old handle cannot see, touch, or kill the new entity.
    assert(!EcsAlive(&world, first));
    assert(EcsGet(&world, first, &posPool) == NULL);
    assert(EcsAdd(&world, first, &posPool) == NULL);
    assert(!EcsDestroy(&world, first));

    // A fabricated handle with an out-of-range index is rejected.
    assert(!EcsAlive(&world, EntityMake(700, 0)));

    // The new handle works.
    TestPos *pos = EcsAdd(&world, second, &posPool);
    assert(pos != NULL);
    pos->x = 3.5f;
    TestPos *reread = EcsGet(&world, second, &posPool);
    assert(reread != NULL);
    assert(reread->x == 3.5f);
}

static void TestEcsQueryMasks(void) {
    EcsWorld world;
    EcsInit(&world);

    static TestPos positions[ECS_MAX_ENTITIES];
    static TestVel velocities[ECS_MAX_ENTITIES];
    EcsPool posPool;
    EcsPool velPool;
    EcsPoolInit(&posPool, TEST_COMPONENT_POS, positions, sizeof(TestPos));
    EcsPoolInit(&velPool, TEST_COMPONENT_VEL, velocities, sizeof(TestVel));

    Entity e0 = EcsCreate(&world);
    Entity e1 = EcsCreate(&world);
    Entity e2 = EcsCreate(&world);
    EcsCreate(&world); // e3 stays empty, for the empty-mask query.

    EcsAdd(&world, e0, &posPool);
    EcsAdd(&world, e1, &posPool);
    EcsAdd(&world, e1, &velPool);
    EcsAdd(&world, e2, &velPool);

    EcsMask posMask = EcsBit(TEST_COMPONENT_POS);
    EcsMask moveMask = posMask | EcsBit(TEST_COMPONENT_VEL);

    // Single component and AND queries.
    assert(CountQuery(&world, posMask) == 2);
    assert(QueryHas(&world, posMask, e0));
    assert(QueryHas(&world, posMask, e1));
    assert(!QueryHas(&world, posMask, e2));
    assert(CountQuery(&world, moveMask) == 1);
    assert(QueryHas(&world, moveMask, e1));

    // An empty required mask matches every live entity.
    assert(CountQuery(&world, 0) == 4);

    // Removing a component drops the entity from the query.
    EcsRemove(&world, e1, &posPool);
    assert(CountQuery(&world, posMask) == 1);
    assert(!QueryHas(&world, posMask, e1));

    // Nobody has both components now, but each keeps its velocity.
    EcsMask velMask = EcsBit(TEST_COMPONENT_VEL);
    assert(CountQuery(&world, moveMask) == 0);
    assert(QueryHas(&world, velMask, e1));
    assert(QueryHas(&world, velMask, e2));

    // Destroying drops the entity before the flush runs.
    assert(EcsDestroy(&world, e0));
    assert(CountQuery(&world, posMask) == 0);
    assert(CountQuery(&world, 0) == 3);
}

static void TestEcsDestroyDuringQuery(void) {
    EcsWorld world;
    EcsInit(&world);

    Entity entities[4];
    for (int i = 0; i < 4; i++) {
        entities[i] = EcsCreate(&world);
    }

    // Destroying mid-scan must not disturb the scan.
    EcsQuery query = EcsQueryAll(0);
    Entity entity;
    int visited = 0;
    while (EcsQueryNext(&world, &query, &entity)) {
        visited++;
        if (entity == entities[0] || entity == entities[1]) {
            assert(EcsDestroy(&world, entity));
        }
    }
    assert(visited == 4);
    assert(EcsAliveCount(&world) == 2);

    EcsFlush(&world);
    assert(EcsAliveCount(&world) == 2);

    // Freed slots return in LIFO order: the last destroyed pops first.
    Entity reused = EcsCreate(&world);
    assert(EntityIndex(reused) == EntityIndex(entities[1]));
    assert(EntityGeneration(reused) == EntityGeneration(entities[1]) + 1);
}

static void TestEcsPool(void) {
    EcsWorld world;
    EcsInit(&world);

    static TestPos positions[ECS_MAX_ENTITIES];
    EcsPool posPool;
    EcsPoolInit(&posPool, TEST_COMPONENT_POS, positions, sizeof(TestPos));

    Entity entity = EcsCreate(&world);

    TestPos *pos = EcsAdd(&world, entity, &posPool);
    assert(pos == &positions[EntityIndex(entity)]);
    assert(pos->x == 0.0f && pos->y == 0.0f);
    assert(EcsHas(&world, entity, TEST_COMPONENT_POS));
    assert(EcsComponentCount(&world, TEST_COMPONENT_POS) == 1);

    pos->x = 10.0f;

    // Adding twice returns the same row and keeps the count.
    TestPos *again = EcsAdd(&world, entity, &posPool);
    assert(again == pos);
    assert(again->x == 10.0f);
    assert(EcsComponentCount(&world, TEST_COMPONENT_POS) == 1);

    EcsRemove(&world, entity, &posPool);
    assert(!EcsHas(&world, entity, TEST_COMPONENT_POS));
    assert(EcsGet(&world, entity, &posPool) == NULL);
    assert(EcsComponentCount(&world, TEST_COMPONENT_POS) == 0);

    // Removing twice is a no-op.
    EcsRemove(&world, entity, &posPool);
    assert(EcsComponentCount(&world, TEST_COMPONENT_POS) == 0);

    // A stale handle cannot add.
    assert(EcsDestroy(&world, entity));
    EcsFlush(&world);
    assert(EcsAdd(&world, entity, &posPool) == NULL);
}

static void TestEcsCounts(void) {
    EcsWorld world;
    EcsInit(&world);

    static TestPos positions[ECS_MAX_ENTITIES];
    static TestVel velocities[ECS_MAX_ENTITIES];
    EcsPool posPool;
    EcsPool velPool;
    EcsPoolInit(&posPool, TEST_COMPONENT_POS, positions, sizeof(TestPos));
    EcsPoolInit(&velPool, TEST_COMPONENT_VEL, velocities, sizeof(TestVel));

    Entity a = EcsCreate(&world);
    Entity b = EcsCreate(&world);
    EcsAdd(&world, a, &posPool);
    EcsAdd(&world, a, &velPool);
    EcsAdd(&world, b, &posPool);

    assert(EcsAliveCount(&world) == 2);
    assert(EcsComponentCount(&world, TEST_COMPONENT_POS) == 2);
    assert(EcsComponentCount(&world, TEST_COMPONENT_VEL) == 1);

    // Destroy drops the counts at once; the flush keeps them.
    assert(EcsDestroy(&world, a));
    assert(EcsAliveCount(&world) == 1);
    assert(EcsComponentCount(&world, TEST_COMPONENT_POS) == 1);
    assert(EcsComponentCount(&world, TEST_COMPONENT_VEL) == 0);

    EcsFlush(&world);
    assert(EcsAliveCount(&world) == 1);
    assert(EcsComponentCount(&world, TEST_COMPONENT_POS) == 1);
}

int main(void) {
    TestScaffold();
    TestLog();
    TestLogRing();
    TestLogAssertBundle();
    TestArenaPushAndAlignment();
    TestArenaReset();
    TestGameMemory();
    TestClockStepsPerFrame();
    TestClockAlpha();
    TestClockFrameRateIndependence();
    TestClockCatchUpCap();

    TestInputHeldAndEdges();
    TestInputTapWithinOneFrame();
    TestInputEdgeLatchesAcrossFrames();
    TestInputEdgesAtAnyRenderRate();
    TestInputBufferWindow();
    TestInputBufferConsume();
    TestInputBufferOnlyAttackAndDash();
    TestInputInvalidAction();
    TestInputDropPending();

    TestEcsCreate();
    TestEcsDestroyDeferred();
    TestEcsGenerationReuse();
    TestEcsStaleHandles();
    TestEcsQueryMasks();
    TestEcsDestroyDuringQuery();
    TestEcsPool();
    TestEcsCounts();

    printf("balin_tests: all tests passed\n");
    return 0;
}
