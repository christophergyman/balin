#ifndef TEXTURES_H
#define TEXTURES_H

#include <stdint.h>

#include "raylib.h"

void LoadTextureTable(void);
void UnloadTextureTable(void);
Texture2D *GetTexture(uint8_t id);

#endif
