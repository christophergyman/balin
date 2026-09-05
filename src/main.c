#include "SDL3/SDL_error.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_video.h"
#include <stdio.h>
#include <SDL3/SDL.h>

int main (void) {
    // init SDL
    if(!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_INIT failed: %s", SDL_GetError());
        return 1;
    }

    // init window
    SDL_Window *window = SDL_CreateWindow("Balin", 800, 600, 0);
    if (!window){
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
    }

    // init renderer
    SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer){
        SDL_Log("SDL_Renderer failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    int running = 1;
    SDL_Event event;

    // main drawing loop
    while (running) {
        while (SDL_PollEvent(&event)){
            if(event.type == SDL_EVENT_QUIT)
            running = 0;
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 225);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    return 0;
}