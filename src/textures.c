#include "textures.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "paths.h"

static Texture2D textures[MAX_TEXTURES];
static int textureCount = 0;

// Procedural placeholder so the game runs with zero art assets
static Texture2D LoadPlaceholderWallTexture(void) {
    Image image = GenImageColor(64, 64, GRAY);
    ImageDrawRectangle(&image, 4, 4, 56, 56, LIGHTGRAY);
    ImageDrawRectangle(&image, 12, 12, 40, 40, GRAY);
    Texture2D texture = LoadTextureFromImage(image);
    UnloadImage(image);
    return texture;
}

static Texture2D LoadPlaceholderCrateTexture(void) {
    Image image = GenImageColor(64, 64, BROWN);
    ImageDrawRectangle(&image, 2, 2, 60, 60, DARKBROWN);
    ImageDrawRectangle(&image, 6, 6, 52, 52, BROWN);
    ImageDrawRectangle(&image, 6, 28, 52, 8, DARKBROWN);
    ImageDrawRectangle(&image, 28, 6, 8, 52, DARKBROWN);
    Texture2D texture = LoadTextureFromImage(image);
    UnloadImage(image);
    return texture;
}

static int ComparePaths(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

void LoadTextureTable(void) {
    textureCount = 0;

    char dirPath[PATH_MAX];
    DataFilePath(TEXTURES_DIR_NAME, dirPath, sizeof(dirPath));

    if (DirectoryExists(dirPath)) {
        FilePathList files = LoadDirectoryFiles(dirPath);
        qsort(files.paths, files.count, sizeof(char *), ComparePaths);

        for (unsigned int i = 0; i < files.count && textureCount < MAX_TEXTURES; i++) {
            if (!IsFileExtension(files.paths[i], ".png")) {
                continue;
            }
            textures[textureCount] = LoadTexture(files.paths[i]);
            textureCount++;
        }
        UnloadDirectoryFiles(files);
    }

    if (textureCount == 0) {
        textures[0] = LoadPlaceholderWallTexture();
        textures[1] = LoadPlaceholderCrateTexture();
        textureCount = 2;
        printf("No textures in %s, using generated placeholders (0 wall, 1 crate)\n", dirPath);
    }
}

void UnloadTextureTable(void) {
    for (int i = 0; i < textureCount; i++) {
        UnloadTexture(textures[i]);
    }
    textureCount = 0;
}

Texture2D *GetTexture(uint8_t id) {
    if (id >= textureCount) {
        return NULL;
    }
    return &textures[id];
}

int GetTextureCount(void) {
    return textureCount;
}
