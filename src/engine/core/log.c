#include "engine/core/log.h"

#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <time.h>

#define LOG_MESSAGE_CAP 512
#define LOG_LINE_CAP 1024
#define LOG_FILE_BUFFER_CAP (64 * 1024)
#define LOG_FLUSH_BUDGET (16 * 1024)
#define LOG_PATH_CAP 512

static const char *levelNames[LOG_LEVEL_COUNT] = {
    "trace",
    "debug",
    "info",
    "warn",
    "error",
    "fatal",
};

static const char *categoryNames[LOG_CAT_COUNT] = {
    "boot",
    "engine",
    "input",
    "render",
    "audio",
    "assets",
    "game",
    "ai",
    "combat",
    "editor",
};

static LogLevel thresholds[LOG_CAT_COUNT];
static bool initialized = false;

static char configPath[LOG_PATH_CAP] = "";
static time_t configMtime = 0;

static FILE *sessionFile = NULL;
static char sessionPath[LOG_PATH_CAP] = "";

static uint64_t currentTick = 0;
static uint32_t currentRun = 0;

// Flight recorder ring. Static storage, no allocation after init.
static char ringSlots[LOG_RING_COUNT][LOG_RING_SLOT_BYTES];
static uint32_t ringHead = 0;  // Next slot to overwrite.
static uint32_t ringCount = 0; // Filled slots, capped at LOG_RING_COUNT.

static char bundleRoot[LOG_PATH_CAP] = "";
static char lastBundlePath[LOG_PATH_CAP] = "";
static LogSnapshotWriter snapshotWriter = NULL;

static char messageBuffer[LOG_MESSAGE_CAP];
static char lineBuffer[LOG_LINE_CAP];
static char fileBuffer[LOG_FILE_BUFFER_CAP];
static size_t fileLength = 0;

static uint64_t formattedCount = 0;
static uint64_t droppedCount = 0;

static bool LogEnabled(LogLevel level, LogCategory category) {
    if (level == LOG_LEVEL_FATAL) {
        return true;
    }
    if (category < 0 || category >= LOG_CAT_COUNT) {
        return false;
    }
    return level >= thresholds[category];
}

static void LogTimestamp(char *out, size_t cap) {
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);

    struct tm utc;
    gmtime_r(&now.tv_sec, &utc);

    size_t length = strftime(out, cap, "%Y-%m-%dT%H:%M:%S", &utc);
    snprintf(out + length, cap - length, ".%03dZ", (int)(now.tv_nsec / 1000000));
}

static void LogAppendEscaped(const char *text) {
    size_t length = strlen(lineBuffer);

    for (const char *cursor = text; *cursor != '\0'; cursor++) {
        const char *escape = NULL;
        switch (*cursor) {
            case '"': escape = "\\\""; break;
            case '\\': escape = "\\\\"; break;
            case '\n': escape = "\\n"; break;
            case '\r': escape = "\\r"; break;
            case '\t': escape = "\\t"; break;
            default: break;
        }

        size_t needed = (escape != NULL) ? 2 : 1;
        if (length + needed + 1 > LOG_LINE_CAP) {
            break;
        }

        if (escape != NULL) {
            lineBuffer[length++] = escape[0];
            lineBuffer[length++] = escape[1];
        } else {
            lineBuffer[length++] = *cursor;
        }
    }

    lineBuffer[length] = '\0';
}

static void LogFlushAllPendingBytes(void);

// Formats one event into lineBuffer, with a trailing newline.
static void LogFormatLine(LogLevel level, LogCategory category, const char *fmt, va_list args) {
    vsnprintf(messageBuffer, sizeof(messageBuffer), fmt, args);

    char timestamp[32];
    LogTimestamp(timestamp, sizeof(timestamp));

    snprintf(lineBuffer, sizeof(lineBuffer),
        "ts=%s level=%s cat=%s tick=%" PRIu64 " run=%" PRIu32 " msg=\"",
        timestamp, levelNames[level], categoryNames[category],
        currentTick, currentRun);

    LogAppendEscaped(messageBuffer);

    size_t length = strlen(lineBuffer);
    if (length + 2 < LOG_LINE_CAP) {
        lineBuffer[length] = '"';
        lineBuffer[length + 1] = '\n';
        lineBuffer[length + 2] = '\0';
    }
}

// Copies one event into the ring, dropping the newline and truncating to the
// slot size. Overwrites the oldest event when the ring is full.
static void LogRingPush(const char *line) {
    size_t length = strlen(line);
    while (length > 0 && (line[length - 1] == '\n' || line[length - 1] == '\r')) {
        length--;
    }
    if (length > LOG_RING_SLOT_BYTES - 1) {
        length = LOG_RING_SLOT_BYTES - 1;
    }

    memcpy(ringSlots[ringHead], line, length);
    ringSlots[ringHead][length] = '\0';

    ringHead = (ringHead + 1) % LOG_RING_COUNT;
    if (ringCount < LOG_RING_COUNT) {
        ringCount++;
    }
}

// Sends one enabled event to the console and the session buffer.
static void LogEmitLine(LogLevel level) {
    formattedCount++;

    // A fatal line must reach the session file even when the buffer is full.
    if (level >= LOG_LEVEL_FATAL) {
        LogFlushAllPendingBytes();
    }

    FILE *console = (level >= LOG_LEVEL_WARN) ? stderr : stdout;
    fputs(lineBuffer, console);

    size_t length = strlen(lineBuffer);
    if (fileLength + length <= sizeof(fileBuffer)) {
        memcpy(fileBuffer + fileLength, lineBuffer, length);
        fileLength += length;
    } else {
        droppedCount++;
    }
}

static void LogVWrite(LogLevel level, LogCategory category, const char *fmt, va_list args) {
    // Every event is formatted and ringed. The threshold only gates the
    // console and session file, so a muted category keeps full detail for a
    // bundle, per ADR-020.
    LogFormatLine(level, category, fmt, args);
    LogRingPush(lineBuffer);

    if (LogEnabled(level, category)) {
        LogEmitLine(level);
    }
}

void LogWrite(LogLevel level, LogCategory category, const char *fmt, ...) {
    if (!initialized || category < 0 || category >= LOG_CAT_COUNT ||
        level < 0 || level >= LOG_LEVEL_COUNT) {
        return;
    }

    va_list args;
    va_start(args, fmt);
    LogVWrite(level, category, fmt, args);
    va_end(args);
}

void LogAssertFail(LogCategory category, const char *file, int line,
                   const char *expr, const char *fmt, ...) {
    char detail[LOG_MESSAGE_CAP];
    va_list args;
    va_start(args, fmt);
    vsnprintf(detail, sizeof(detail), fmt, args);
    va_end(args);

    if (initialized) {
        if (detail[0] != '\0') {
            LogWrite(LOG_LEVEL_FATAL, category, "ASSERT failed: %s at %s:%d %s", expr, file, line, detail);
        } else {
            LogWrite(LOG_LEVEL_FATAL, category, "ASSERT failed: %s at %s:%d", expr, file, line);
        }

        // Capture the seconds before the exit, per ADR-020.
        LogDumpBundle("assert");
    } else if (detail[0] != '\0') {
        fprintf(stderr, "ASSERT failed: %s at %s:%d %s\n", expr, file, line, detail);
    } else {
        fprintf(stderr, "ASSERT failed: %s at %s:%d\n", expr, file, line);
    }

    LogShutdown();
    exit(1);
}

static int LogLevelFromName(const char *name) {
    for (int index = 0; index < LOG_LEVEL_COUNT; index++) {
        if (strcmp(name, levelNames[index]) == 0) {
            return index;
        }
    }
    return -1;
}

static int LogCategoryFromName(const char *name) {
    for (int index = 0; index < LOG_CAT_COUNT; index++) {
        if (strcmp(name, categoryNames[index]) == 0) {
            return index;
        }
    }
    return -1;
}

bool LogConfigLoad(const char *path, char *error, size_t errorCap) {
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        snprintf(error, errorCap, "%s: cannot open: %s", path, strerror(errno));
        return false;
    }

    LogLevel loaded[LOG_CAT_COUNT];
    bool overridden[LOG_CAT_COUNT] = { false };
    bool defaultSeen = false;
    LogLevel defaultLevel = LOG_LEVEL_INFO;
    bool ok = true;

    char line[256];
    int lineNumber = 0;

    while (ok && fgets(line, sizeof(line), file) != NULL) {
        lineNumber++;

        char *text = line;
        while (*text == ' ' || *text == '\t') {
            text++;
        }

        char *end = text + strlen(text);
        while (end > text && (end[-1] == '\n' || end[-1] == '\r' || end[-1] == ' ' || end[-1] == '\t')) {
            end--;
            *end = '\0';
        }

        if (*text == '\0' || *text == '#') {
            continue;
        }

        char *equals = strchr(text, '=');
        if (equals == NULL) {
            snprintf(error, errorCap, "%s:%d: expected key = value", path, lineNumber);
            ok = false;
            break;
        }

        *equals = '\0';
        char *key = text;
        char *value = equals + 1;

        char *keyEnd = key + strlen(key);
        while (keyEnd > key && (keyEnd[-1] == ' ' || keyEnd[-1] == '\t')) {
            keyEnd--;
            *keyEnd = '\0';
        }

        while (*value == ' ' || *value == '\t') {
            value++;
        }

        int level = LogLevelFromName(value);
        if (level < 0) {
            snprintf(error, errorCap, "%s:%d: unknown level '%s'", path, lineNumber, value);
            ok = false;
            break;
        }

        if (strcmp(key, "default") == 0) {
            defaultLevel = (LogLevel)level;
            defaultSeen = true;
            continue;
        }

        int category = LogCategoryFromName(key);
        if (category < 0) {
            snprintf(error, errorCap, "%s:%d: unknown key '%s'", path, lineNumber, key);
            ok = false;
            break;
        }

        loaded[category] = (LogLevel)level;
        overridden[category] = true;
    }

    fclose(file);

    if (!ok) {
        return false;
    }

    if (!defaultSeen) {
        snprintf(error, errorCap, "%s: missing 'default' entry", path);
        return false;
    }

    for (int index = 0; index < LOG_CAT_COUNT; index++) {
        thresholds[index] = overridden[index] ? loaded[index] : defaultLevel;
    }

    struct stat info;
    if (stat(path, &info) == 0) {
        configMtime = info.st_mtime;
    }

    return true;
}

static void LogCheckConfigReload(void) {
    struct stat info;
    if (stat(configPath, &info) != 0) {
        fprintf(stderr, "log: cannot read %s: %s\n", configPath, strerror(errno));
        exit(1);
    }

    if (info.st_mtime == configMtime) {
        return;
    }

    char error[256];
    if (!LogConfigLoad(configPath, error, sizeof(error))) {
        fprintf(stderr, "log: %s\n", error);
        exit(1);
    }

    LOGI(LOG_CAT_BOOT, "log.cfg reloaded");
}

static void LogWritePendingBytes(void) {
    if (sessionFile == NULL || fileLength == 0) {
        return;
    }

    size_t budget = (fileLength < LOG_FLUSH_BUDGET) ? fileLength : LOG_FLUSH_BUDGET;
    if (fwrite(fileBuffer, 1, budget, sessionFile) != budget) {
        fprintf(stderr, "log: session write failed\n");
        droppedCount++;
    } else {
        fflush(sessionFile);
    }

    if (budget < fileLength) {
        memmove(fileBuffer, fileBuffer + budget, fileLength - budget);
    }
    fileLength -= budget;
}

// Writes every pending session byte. The bundle dump uses this so the copied
// session log ends with the event that triggered the dump.
static void LogFlushAllPendingBytes(void) {
    if (sessionFile == NULL || fileLength == 0) {
        return;
    }

    if (fwrite(fileBuffer, 1, fileLength, sessionFile) != fileLength) {
        fprintf(stderr, "log: session write failed\n");
        droppedCount++;
    }
    fflush(sessionFile);
    fileLength = 0;
}

void LogInit(const char *configFile, const char *logDir, const char *bugDir) {
    initialized = false;
    fileLength = 0;
    formattedCount = 0;
    droppedCount = 0;
    currentTick = 0;
    currentRun = 0;
    configMtime = 0;
    ringHead = 0;
    ringCount = 0;
    lastBundlePath[0] = '\0';
    snapshotWriter = NULL;

    for (int index = 0; index < LOG_CAT_COUNT; index++) {
        thresholds[index] = LOG_LEVEL_INFO;
    }

    snprintf(configPath, sizeof(configPath), "%s", configFile);
    snprintf(bundleRoot, sizeof(bundleRoot), "%s", bugDir);
    sessionPath[0] = '\0';

    if (mkdir(logDir, 0755) != 0 && errno != EEXIST) {
        fprintf(stderr, "log: cannot create %s: %s\n", logDir, strerror(errno));
        exit(1);
    }

    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    struct tm utc;
    gmtime_r(&now.tv_sec, &utc);

    char stamp[32];
    strftime(stamp, sizeof(stamp), "%Y%m%d-%H%M%S", &utc);
    snprintf(sessionPath, sizeof(sessionPath), "%s/session-%s.log", logDir, stamp);

    sessionFile = fopen(sessionPath, "w");
    if (sessionFile == NULL) {
        fprintf(stderr, "log: cannot open %s: %s\n", sessionPath, strerror(errno));
        exit(1);
    }

    char error[256];
    if (!LogConfigLoad(configPath, error, sizeof(error))) {
        fprintf(stderr, "log: %s\n", error);
        exit(1);
    }

    initialized = true;
}

void LogFlush(void) {
    if (!initialized) {
        return;
    }
    LogCheckConfigReload();
    LogWritePendingBytes();
}

void LogShutdown(void) {
    if (sessionFile == NULL) {
        return;
    }

    LogFlushAllPendingBytes();

    fclose(sessionFile);
    sessionFile = NULL;
    initialized = false;
}

void LogSetContext(uint64_t tick, uint32_t run) {
    currentTick = tick;
    currentRun = run;
}

const char *LogSessionPath(void) {
    return sessionPath;
}

uint64_t LogFormattedCount(void) {
    return formattedCount;
}

uint64_t LogDroppedCount(void) {
    return droppedCount;
}

static bool LogCopyFile(const char *from, const char *to) {
    FILE *input = fopen(from, "rb");
    if (input == NULL) {
        return false;
    }

    FILE *output = fopen(to, "wb");
    if (output == NULL) {
        fclose(input);
        return false;
    }

    char buffer[8192];
    bool ok = true;
    size_t count;
    while ((count = fread(buffer, 1, sizeof(buffer), input)) > 0) {
        if (fwrite(buffer, 1, count, output) != count) {
            ok = false;
            break;
        }
    }
    if (ferror(input)) {
        ok = false;
    }

    fclose(input);
    if (fclose(output) != 0) {
        ok = false;
    }
    return ok;
}

static bool LogWriteRingFile(const char *path) {
    FILE *file = fopen(path, "w");
    if (file == NULL) {
        return false;
    }

    // Oldest event first. Before the ring fills, the oldest slot is index 0.
    bool ok = true;
    uint32_t start = (ringCount < LOG_RING_COUNT) ? 0 : ringHead;
    for (uint32_t index = 0; index < ringCount; index++) {
        uint32_t slot = (start + index) % LOG_RING_COUNT;
        if (fprintf(file, "%s\n", ringSlots[slot]) < 0) {
            ok = false;
            break;
        }
    }

    if (fclose(file) != 0) {
        ok = false;
    }
    return ok;
}

static bool LogWriteStateFile(const char *path, const char *reason) {
    FILE *file = fopen(path, "w");
    if (file == NULL) {
        return false;
    }

    fprintf(file, "tick=%" PRIu64 "\n", currentTick);
    fprintf(file, "run=%" PRIu32 "\n", currentRun);
    fprintf(file, "reason=%s\n", reason);

    if (snapshotWriter != NULL) {
        snapshotWriter(file);
    } else {
        fprintf(file, "snapshot=none\n");
    }

    bool ok = !ferror(file);
    if (fclose(file) != 0) {
        ok = false;
    }
    return ok;
}

static bool LogWriteEnvFile(const char *path) {
    FILE *file = fopen(path, "w");
    if (file == NULL) {
        return false;
    }

    char timestamp[32];
    LogTimestamp(timestamp, sizeof(timestamp));

    struct utsname info;
    if (uname(&info) == 0) {
        fprintf(file, "utc=%s\n", timestamp);
        fprintf(file, "os=%s\n", info.sysname);
        fprintf(file, "kernel=%s\n", info.release);
        fprintf(file, "machine=%s\n", info.machine);
    } else {
        fprintf(file, "utc=%s\n", timestamp);
        fprintf(file, "os=unknown\n");
        fprintf(file, "kernel=unknown\n");
        fprintf(file, "machine=unknown\n");
    }

#if defined(__clang__)
    fprintf(file, "compiler=%s\n", __clang_version__);
#elif defined(__GNUC__)
    fprintf(file, "compiler=%s\n", __VERSION__);
#else
    fprintf(file, "compiler=unknown\n");
#endif

#ifdef NDEBUG
    fprintf(file, "build=release\n");
#else
    fprintf(file, "build=debug\n");
#endif

#if defined(BALIN_DEV) && BALIN_DEV
    fprintf(file, "dev=1\n");
#else
    fprintf(file, "dev=0\n");
#endif

    bool ok = !ferror(file);
    if (fclose(file) != 0) {
        ok = false;
    }
    return ok;
}

bool LogDumpBundle(const char *reason) {
    if (!initialized || bundleRoot[0] == '\0') {
        return false;
    }
    if (reason == NULL) {
        reason = "unknown";
    }

    if (mkdir(bundleRoot, 0755) != 0 && errno != EEXIST) {
        fprintf(stderr, "bundle: cannot create %s: %s\n", bundleRoot, strerror(errno));
        return false;
    }

    // The session copy must include the final events, so flush first.
    LogFlushAllPendingBytes();

    char stamp[32];
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    struct tm utc;
    gmtime_r(&now.tv_sec, &utc);
    strftime(stamp, sizeof(stamp), "%Y%m%d-%H%M%S", &utc);

    // A bundle directory per dump. Same-second dumps get a numeric suffix.
    char path[LOG_PATH_CAP];
    bool created = false;
    for (int attempt = 0; attempt < 10 && !created; attempt++) {
        if (attempt == 0) {
            snprintf(path, sizeof(path), "%s/%s", bundleRoot, stamp);
        } else {
            snprintf(path, sizeof(path), "%s/%s-%d", bundleRoot, stamp, attempt);
        }

        if (mkdir(path, 0755) == 0) {
            created = true;
        } else if (errno != EEXIST) {
            fprintf(stderr, "bundle: cannot create %s: %s\n", path, strerror(errno));
            return false;
        }
    }

    if (!created) {
        fprintf(stderr, "bundle: too many bundles in %s\n", bundleRoot);
        return false;
    }

    bool ok = true;
    char filePath[LOG_PATH_CAP];

    if (sessionPath[0] != '\0') {
        snprintf(filePath, sizeof(filePath), "%s/session.log", path);
        if (!LogCopyFile(sessionPath, filePath)) {
            fprintf(stderr, "bundle: cannot write %s\n", filePath);
            ok = false;
        }
    }

    snprintf(filePath, sizeof(filePath), "%s/ring.log", path);
    if (!LogWriteRingFile(filePath)) {
        fprintf(stderr, "bundle: cannot write %s\n", filePath);
        ok = false;
    }

    snprintf(filePath, sizeof(filePath), "%s/state.txt", path);
    if (!LogWriteStateFile(filePath, reason)) {
        fprintf(stderr, "bundle: cannot write %s\n", filePath);
        ok = false;
    }

    snprintf(filePath, sizeof(filePath), "%s/env.txt", path);
    if (!LogWriteEnvFile(filePath)) {
        fprintf(stderr, "bundle: cannot write %s\n", filePath);
        ok = false;
    }

    if (ok) {
        snprintf(lastBundlePath, sizeof(lastBundlePath), "%s", path);
        fprintf(stderr, "bundle: wrote %s (%s)\n", path, reason);
    }
    return ok;
}

void LogSetSnapshotWriter(LogSnapshotWriter writer) {
    snapshotWriter = writer;
}

const char *LogLastBundlePath(void) {
    return lastBundlePath;
}

uint32_t LogRingCount(void) {
    return ringCount;
}
