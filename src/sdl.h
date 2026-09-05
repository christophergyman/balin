#pragma once

#include <SDL3/SDL_video.h>
#include <SDL3/SDL_render.h>

SDL_Renderer *init_sdl(SDL_Window **window, int width, int height);
void shutdown_sdl(SDL_Window *window, SDL_Renderer *renderer);