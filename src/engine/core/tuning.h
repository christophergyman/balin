#ifndef TUNING_H
#define TUNING_H

#include <stdbool.h>
#include <stddef.h>

// Reloadable tuning values, per ADR-008. The file is the only source of
// truth. There are no compiled defaults; every spec key is required.

#define TUNING_PATH_CAP 512
#define TUNING_MAX_KEYS 64
#define TUNING_ERROR_CAP 256

typedef enum TuningType {
    TUNING_NUMBER = 0, // double
    TUNING_INT,
    TUNING_BOOL,
} TuningType;

typedef struct TuningSpec {
    const char *name;
    TuningType type;
} TuningSpec;

// Resolves relativePath: exeDir first, then the working directory. On success
// writes the found path to out and returns true. On failure out is empty.
bool TuningResolve(const char *relativePath, const char *exeDir, char *out, size_t cap);

// Parses and validates path against the spec table. Values land in out, one
// per spec. On failure returns false and fills error with path:line and the
// expectation. out is untouched on failure.
bool TuningParse(const TuningSpec *specs, size_t count, const char *path,
                 double *out, char *error, size_t cap);

// Resolves, loads, and watches the file. Fatal on any failure, per ADR-019.
// exeDir may be NULL; main.c passes GetApplicationDirectory().
void TuningInit(const TuningSpec *specs, size_t count,
                const char *relativePath, const char *exeDir);

// Reloads when the resolved file's mtime changes. Call once per frame.
void TuningUpdate(void);

// Typed reads by spec index. Only valid after TuningInit.
double TuningNumber(size_t index);
int TuningInt(size_t index);
bool TuningBool(size_t index);

// Resolved path of the watched file, or an empty string before TuningInit.
const char *TuningPath(void);

#endif
