#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "engine/log.h"

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

int main(void) {
    TestScaffold();
    TestLog();

    printf("balin_tests: all tests passed\n");
    return 0;
}
