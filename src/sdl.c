#include "sdl.h"
#include "SDL3/SDL_render.h"
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_log.h>

#define TILE_W 40
#define TILE_H 40

SDL_Renderer *init_sdl(SDL_Window **window, int width, int height) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return NULL;
    }

    *window = SDL_CreateWindow("Balin", width, height, 0);
    if (!*window) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return NULL;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(*window, NULL);
    if (!renderer) {
        SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
        SDL_DestroyWindow(*window);
        SDL_Quit();
        return NULL;
    }

    return renderer;
}

void draw_grid(SDL_Renderer *renderer, int c, int r){
        // grid lines
        SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
        for (int i=0; i<=c; i++){SDL_RenderLine(renderer, i*TILE_W, 0, i*TILE_W, r*TILE_H);}
        for (int i=0; i<=r; i++){SDL_RenderLine(renderer, 0 , i * TILE_H, c * TILE_W, i * TILE_H);}
}

void draw_tile(SDL_Renderer *renderer, int c, int r, struct Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
    SDL_FRect rect = {
        .x = c * TILE_W,
        .y = r * TILE_H,
        .w = TILE_W,
        .h = TILE_H
    };
    SDL_RenderFillRect(renderer, &rect);
}

void shutdown_sdl(SDL_Window *window, SDL_Renderer *renderer) {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}