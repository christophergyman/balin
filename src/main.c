#include "SDL3/SDL_error.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gamepad.h"
#include "SDL3/SDL_keyboard.h"
#include "SDL3/SDL_keycode.h"
#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_timer.h"
#include "SDL3/SDL_video.h"
#include <stdio.h>
#include <string.h>
#include <SDL3/SDL.h>

#include "colors.h"
#include "entity.h"
#include "sdl.h"

// DEFINITIONS
#define DEBUG_GRID true 
#define WINDOW_W 800
#define WINDOW_H 600 
#define COLS 20
#define ROWS 15

// HELPER

// MAIN
int main (void) {

    // Window
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = init_sdl(&window, WINDOW_W, WINDOW_H);
    if (!renderer) return 1;

    // WORLD TILES
    int world[COLS][ROWS] = {0};
    for (int c=0; c<COLS; c++) {
        for(int r=0; r< ROWS; r++){
            // fill the world with air
            world[c][r] = AIR;
        }
    }
    world[2][2] = WALL;

    //      ---- Entities----
    struct Entity entity_array[32];
    struct Entity player = {
        .name = "balin",
        .pos_x = 1,
        .pos_y = 1,
        .stats.strength = 99,
        .stats.faith = 99,
        .stats.speech = 99,
    };
    entity_array[0] = player;

    //      ----main event loop----
    int running = 1;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)){
            if(event.type == SDL_EVENT_QUIT){
                running = 0;
            }
            if (event.type == SDL_EVENT_KEY_DOWN){
                switch (event.key.key){
                    case SDLK_ESCAPE:
                        running = 0;
                        break;
                    case SDLK_W:
                        player.pos_y -= 1;
                        break;
                    case SDLK_S:
                        player.pos_y += 1;
                        break;
                    case SDLK_A:
                        player.pos_x -= 1;
                        break;
                    case SDLK_D:
                        player.pos_x += 1;
                        break;
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
      
        draw_grid(renderer, COLS, ROWS);

        // draw world tiles
        for (int c=0; c<COLS; c++) {
            for(int r=0; r<ROWS; r++){
                switch(world[c][r]){
                    case AIR:
                        break;
                    case WALL:
                        draw_tile(renderer, c, r, grey);
                }
            }
        }

        // size of entity_array
        for (int i=0; i < (sizeof(entity_array) / sizeof(entity_array[0]) ); i++ ){
            if(strcmp(entity_array[i].name, "balin")){
                draw_tile(renderer, player.pos_x, player.pos_y, sky_blue);
            }
        }
        // update render buffer
        SDL_RenderPresent(renderer);

    }
    shutdown_sdl(window, renderer);
    return 0;
}