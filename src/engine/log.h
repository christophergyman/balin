#ifndef LOG_H
#define LOG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    LOG_LEVEL_TRACE = 0,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL,
    LOG_LEVEL_COUNT
} LogLevel;

typedef enum {
    LOG_CAT_BOOT = 0,
    LOG_CAT_ENGINE,
    LOG_CAT_INPUT,
    LOG_CAT_RENDER,
    LOG_CAT_AUDIO,
    LOG_CAT_ASSETS,
    LOG_CAT_GAME,
    LOG_CAT_AI,
    LOG_CAT_COMBAT,
    LOG_CAT_EDITOR,
    LOG_CAT_COUNT
} LogCategory;

#if defined(__GNUC__) || defined(__clang__)
#define LOG_PRINTF(fmtIndex, firstArg) __attribute__((format(printf, fmtIndex, firstArg)))
#else
#define LOG_PRINTF(fmtIndex, firstArg)
#endif

// Opens the session log and loads the threshold table. Exits on failure.
void LogInit(const char *configPath, const char *logDir);

// Writes pending session bytes and checks the config for changes.
// Call once per rendered frame.
void LogFlush(void);

// Writes remaining bytes and closes the session log.
void LogShutdown(void);

// Context stamped on every line.
void LogSetContext(uint64_t tick, uint32_t run);

void LogWrite(LogLevel level, LogCategory category, const char *fmt, ...) LOG_PRINTF(3, 4);

void LogAssertFail(LogCategory category, const char *file, int line,
                   const char *expr, const char *fmt, ...) LOG_PRINTF(5, 6);

// Loads thresholds from path. On failure returns false and fills error.
// The current table is unchanged on failure.
bool LogConfigLoad(const char *path, char *error, size_t errorCap);

// Path of the current session log, or an empty string before LogInit.
const char *LogSessionPath(void);

uint64_t LogFormattedCount(void);
uint64_t LogDroppedCount(void);

#undef LOG_PRINTF

#define LOGT(cat, ...) LogWrite(LOG_LEVEL_TRACE, (cat), __VA_ARGS__)
#define LOGD(cat, ...) LogWrite(LOG_LEVEL_DEBUG, (cat), __VA_ARGS__)
#define LOGI(cat, ...) LogWrite(LOG_LEVEL_INFO, (cat), __VA_ARGS__)
#define LOGW(cat, ...) LogWrite(LOG_LEVEL_WARN, (cat), __VA_ARGS__)
#define LOGE(cat, ...) LogWrite(LOG_LEVEL_ERROR, (cat), __VA_ARGS__)
#define ASSERT(cond, cat, ...)                                               \
    do {                                                                     \
        if (!(cond)) {                                                       \
            LogAssertFail((cat), __FILE__, __LINE__, #cond, "" __VA_ARGS__); \
        }                                                                    \
    } while (0)

#endif
