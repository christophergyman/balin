#include "paths.h"

#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

// Cached data directory, resolved once: next to the executable, wherever it is run from
static char dataDir[PATH_MAX] = {0};

static const char *GetDataDir(void) {
    if (dataDir[0] != '\0') {
        return dataDir;
    }

#ifdef __APPLE__
    char exePath[PATH_MAX];
    uint32_t size = sizeof(exePath);
    if (_NSGetExecutablePath(exePath, &size) == 0) {
        char exePathCopy[PATH_MAX];
        snprintf(exePathCopy, sizeof(exePathCopy), "%s", exePath);
        char *exeDir = dirname(exePathCopy); // note: dirname may modify its argument
        snprintf(dataDir, sizeof(dataDir), "%s/%s", exeDir, DATA_DIR_NAME);
    }
#endif

    // Fallback for non-macOS builds: relative to the working directory
    if (dataDir[0] == '\0') {
        snprintf(dataDir, sizeof(dataDir), "%s", DATA_DIR_NAME);
    }
    return dataDir;
}

void DataFilePath(const char *fileName, char *out, size_t outSize) {
    snprintf(out, outSize, "%s/%s", GetDataDir(), fileName);
}

void EnsureDataDir(void) {
    mkdir(GetDataDir(), 0755); // fails quietly if it already exists
}
