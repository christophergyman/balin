#include "engine/log.h"

#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
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

static void LogVWrite(LogLevel level, LogCategory category, const char *fmt, va_list args) {
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

    formattedCount++;

    FILE *console = (level >= LOG_LEVEL_WARN) ? stderr : stdout;
    fputs(lineBuffer, console);

    length = strlen(lineBuffer);
    if (fileLength + length <= sizeof(fileBuffer)) {
        memcpy(fileBuffer + fileLength, lineBuffer, length);
        fileLength += length;
    } else {
        droppedCount++;
    }
}

void LogWrite(LogLevel level, LogCategory category, const char *fmt, ...) {
    // BAL-12 copies the event into the flight recorder ring before this check.
    if (!initialized || !LogEnabled(level, category)) {
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

void LogInit(const char *configFile, const char *logDir) {
    initialized = false;
    fileLength = 0;
    formattedCount = 0;
    droppedCount = 0;
    currentTick = 0;
    currentRun = 0;
    configMtime = 0;

    for (int index = 0; index < LOG_CAT_COUNT; index++) {
        thresholds[index] = LOG_LEVEL_INFO;
    }

    snprintf(configPath, sizeof(configPath), "%s", configFile);
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

    if (fileLength > 0) {
        fwrite(fileBuffer, 1, fileLength, sessionFile);
        fileLength = 0;
    }

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
