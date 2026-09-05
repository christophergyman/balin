#pragma once

enum Tiles {
    AIR,
    WALL,
};

struct Entity_stats{
    float strength;
    float faith;
    float speech;
};

struct Entity{
    char name[32];
    int pos_x;
    int pos_y;
    struct Entity_stats stats;
};