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


//      ---- Definitions ----
#define DEBUG_GRID true 
#define WINDOW_W 800
#define WINDOW_H 600 
#define TILE_W 40
#define TILE_H 40
#define COLS 20
#define ROWS 15

//      ---- Types ----
struct color{
    int r, g, b;
};

struct color grass_green= {34, 139, 34};
struct color sky_blue = {135, 206, 235};
struct color dark_purple = {75, 0, 130};
struct color grey = {90, 90, 90};

enum Tiles {
    AIR,
    WALL,
};

struct entity_stats{
    float strength;
    float faith;
    float speech;
};

struct entity{
    char name[32];
    int pos_x;
    int pos_y;
    struct entity_stats stats;
};


//      ---- HELPERS ----
void draw_tile(SDL_Renderer *renderer, int c, int r, struct color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
    SDL_FRect rect = {
        .x = c * TILE_W,
        .y = r * TILE_H,
        .w = TILE_W,
        .h = TILE_H
    };
    SDL_RenderFillRect(renderer, &rect);
}


//      ---- Main Program Loop ----

int main (void) {

    //      ----game colors----

    //      ----create window and renderer----
    if(!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_INIT failed: %s", SDL_GetError());
        return 1;
    }
    SDL_Window *window = SDL_CreateWindow("Balin", WINDOW_W, WINDOW_H, 0);
    if (!window){
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer){
        SDL_Log("SDL_Renderer failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }


    //      ---- World Tiles----
    struct entity entity_array[32];
    int world[COLS][ROWS] = {0};

    for (int c=0; c<COLS; c++) {
        for(int r=0; r< ROWS; r++){
            // fill the world with air
            world[c][r] = AIR;
        }
    }
    world[2][2] = WALL;

    //      ---- Entities----
    struct entity player = {
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
                        player.pos_y += 1;
                        break;
                    case SDLK_S:
                        player.pos_y -= 1;
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
      
        // grid lines
        if (DEBUG_GRID == true){
            SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
            for (int i=0; i<=COLS; i++){SDL_RenderLine(renderer, i*TILE_W, 0, i*TILE_W, ROWS*TILE_H);}
            for (int i=0; i<=ROWS; i++){SDL_RenderLine(renderer, 0 , i * TILE_H, COLS * TILE_W, i * TILE_H);}
        }

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
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    return 0;
}