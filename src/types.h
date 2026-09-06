#ifndef TYPES_H
#define TYPES_H

// STRUCTS
enum TileType {
    AIR,
    WALL,
};

typedef struct WorldTile {
    enum TileType tileType;
} WorldTile;

typedef struct Position {
    int posX;
    int posY;
} Position;

typedef struct EntityStats {
    int health;
    int strength;
    int faith;
    int money;
} EntityStats;

typedef struct Entity {
    char* name;
    struct Position position;
    struct EntityStats entityStats;
} Entity;

#endif
