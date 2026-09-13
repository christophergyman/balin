#include "engine/core/tuning.h"

#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "engine/core/log.h"

static const TuningSpec *specs;
static size_t specCount;
static double values[TUNING_MAX_KEYS];
static char watchedPath[TUNING_PATH_CAP];
static int64_t watchedMtime;

static void TuningFatal(const char *fmt, ...) {
    char message[TUNING_ERROR_CAP];

    va_list args;
    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

    LOGE(LOG_CAT_ASSETS, "tuning: %s", message);
    LogShutdown();
    exit(1);
}

// Nanosecond mtime where the platform has it, so a save within the same
// second still reloads. Seconds on platforms without it.
static int64_t MtimeOf(const struct stat *info) {
#if defined(__APPLE__)
    return (int64_t)info->st_mtimespec.tv_sec * 1000000000LL +
           (int64_t)info->st_mtimespec.tv_nsec;
#else
    return (int64_t)info->st_mtime * 1000000000LL;
#endif
}

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

static int FindSpec(const TuningSpec *table, size_t count, const char *name) {
    for (size_t index = 0; index < count; index++) {
        if (strcmp(table[index].name, name) == 0) {
            return (int)index;
        }
    }
    return -1;
}

static bool ParseValue(const TuningSpec *spec, const char *value, double *out,
                       const char *path, int line, char *error, size_t cap) {
    char *end = NULL;

    if (spec->type == TUNING_BOOL) {
        if (strcmp(value, "true") == 0) {
            *out = 1.0;
            return true;
        }
        if (strcmp(value, "false") == 0) {
            *out = 0.0;
            return true;
        }
        snprintf(error, cap, "%s:%d: '%s' expects true or false, got '%s'",
                 path, line, spec->name, value);
        return false;
    }

    if (spec->type == TUNING_INT) {
        long parsed = strtol(value, &end, 10);
        if (end == value || *end != '\0' || parsed < INT_MIN || parsed > INT_MAX) {
            snprintf(error, cap, "%s:%d: '%s' expects an integer, got '%s'",
                     path, line, spec->name, value);
            return false;
        }
        *out = (double)parsed;
        return true;
    }

    double parsed = strtod(value, &end);
    if (end == value || *end != '\0' || !isfinite(parsed)) {
        snprintf(error, cap, "%s:%d: '%s' expects a number, got '%s'",
                 path, line, spec->name, value);
        return false;
    }
    *out = parsed;
    return true;
}

bool TuningResolve(const char *relativePath, const char *exeDir, char *out, size_t cap) {
    struct stat info;

    if (exeDir != NULL && exeDir[0] != '\0') {
        size_t length = strlen(exeDir);
        if (exeDir[length - 1] == '/') {
            snprintf(out, cap, "%s%s", exeDir, relativePath);
        } else {
            snprintf(out, cap, "%s/%s", exeDir, relativePath);
        }
        if (stat(out, &info) == 0) {
            return true;
        }
    }

    snprintf(out, cap, "%s", relativePath);
    if (stat(out, &info) == 0) {
        return true;
    }

    out[0] = '\0';
    return false;
}

bool TuningParse(const TuningSpec *table, size_t count, const char *path,
                 double *out, char *error, size_t cap) {
    if (count > TUNING_MAX_KEYS) {
        snprintf(error, cap, "%s: too many tuning keys (%zu)", path, count);
        return false;
    }

    FILE *file = fopen(path, "r");
    if (file == NULL) {
        snprintf(error, cap, "%s: cannot open: %s", path, strerror(errno));
        return false;
    }

    double loaded[TUNING_MAX_KEYS];
    bool seen[TUNING_MAX_KEYS];
    memset(seen, 0, sizeof(seen));

    bool ok = true;
    char line[256];
    int lineNumber = 0;

    while (ok && fgets(line, sizeof(line), file) != NULL) {
        lineNumber++;

        char *text = TrimLeft(line);

        char *comment = strchr(text, '#');
        if (comment != NULL) {
            *comment = '\0';
        }

        TrimRight(text);
        if (*text == '\0') {
            continue;
        }

        char *equals = strchr(text, '=');
        if (equals == NULL) {
            snprintf(error, cap, "%s:%d: expected key = value", path, lineNumber);
            ok = false;
            break;
        }

        *equals = '\0';
        char *key = TrimLeft(text);
        TrimRight(key);
        char *value = TrimLeft(equals + 1);
        TrimRight(value);

        int index = FindSpec(table, count, key);
        if (index < 0) {
            snprintf(error, cap, "%s:%d: unknown key '%s'", path, lineNumber, key);
            ok = false;
            break;
        }

        if (seen[index]) {
            snprintf(error, cap, "%s:%d: duplicate key '%s'", path, lineNumber, key);
            ok = false;
            break;
        }

        if (!ParseValue(&table[index], value, &loaded[index], path, lineNumber, error, cap)) {
            ok = false;
            break;
        }

        seen[index] = true;
    }

    fclose(file);

    if (!ok) {
        return false;
    }

    for (size_t index = 0; index < count; index++) {
        if (!seen[index]) {
            snprintf(error, cap, "%s: missing key '%s'", path, table[index].name);
            return false;
        }
    }

    memcpy(out, loaded, count * sizeof(double));
    return true;
}

const char *TuningPath(void) {
    return watchedPath;
}

void TuningInit(const TuningSpec *table, size_t count,
                const char *relativePath, const char *exeDir) {
    specs = table;
    specCount = count;
    watchedPath[0] = '\0';
    watchedMtime = 0;

    if (count > TUNING_MAX_KEYS) {
        TuningFatal("too many tuning keys (%zu)", count);
    }

    if (!TuningResolve(relativePath, exeDir, watchedPath, sizeof(watchedPath))) {
        TuningFatal("cannot find %s (executable folder '%s', then working directory)",
                    relativePath, exeDir != NULL ? exeDir : "");
    }

    char error[TUNING_ERROR_CAP];
    if (!TuningParse(specs, count, watchedPath, values, error, sizeof(error))) {
        TuningFatal("%s", error);
    }

    struct stat info;
    if (stat(watchedPath, &info) != 0) {
        TuningFatal("cannot read %s: %s", watchedPath, strerror(errno));
    }
    watchedMtime = MtimeOf(&info);

    LOGI(LOG_CAT_ASSETS, "tuning loaded keys=%zu path=%s", count, watchedPath);
}

void TuningUpdate(void) {
    if (specCount == 0 || watchedPath[0] == '\0') {
        return;
    }

    struct stat info;
    if (stat(watchedPath, &info) != 0) {
        TuningFatal("cannot read %s: %s", watchedPath, strerror(errno));
    }

    int64_t mtime = MtimeOf(&info);
    if (mtime == watchedMtime) {
        return;
    }

    // Parse into scratch first, so a late failure cannot leave half the
    // values from the new file and half from the old one.
    double reloaded[TUNING_MAX_KEYS];
    char error[TUNING_ERROR_CAP];
    if (!TuningParse(specs, specCount, watchedPath, reloaded, error, sizeof(error))) {
        TuningFatal("%s", error);
    }

    memcpy(values, reloaded, specCount * sizeof(double));
    watchedMtime = mtime;
    LOGI(LOG_CAT_ASSETS, "tuning reloaded keys=%zu", specCount);
}

double TuningNumber(size_t index) {
    if (index >= specCount) {
        TuningFatal("key index %zu is out of range", index);
    }
    return values[index];
}

int TuningInt(size_t index) {
    return (int)TuningNumber(index);
}

bool TuningBool(size_t index) {
    return TuningNumber(index) != 0.0;
}
