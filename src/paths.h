#ifndef PATHS_H
#define PATHS_H

#include <stddef.h>

// File names inside the data directory, which lives next to the executable
#define DATA_DIR_NAME "data"
#define WORLD_FILE_NAME "world_tiles.bin"
#define ENTITY_FILE_NAME "entities.bin"
#define TEXTURES_DIR_NAME "textures"

// Fill out with the full path to fileName inside the data directory
void DataFilePath(const char *fileName, char *out, size_t outSize);

// Create the data directory if it is missing
void EnsureDataDir(void);

#endif
