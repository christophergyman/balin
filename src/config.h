#ifndef CONFIG_H
#define CONFIG_H

// World scale: 1 world unit = 1 meter
#define UNIT_METERS 1.0f

// Level layout
#define TILE_SIZE 2.0f   // one grid tile = 2m x 2m of floor
#define WALL_HEIGHT 2.5f // cave ceiling height

// Player
#define PLAYER_EYE 1.6f    // camera height in meters
#define PLAYER_RADIUS 0.4f // collision radius, for later

// Physics
#define MOVE_SPEED 5.0f     // m/s walk
#define JUMP_SPEED 8.0f     // ~1.3m jump height with GRAVITY 24
#define GRAVITY 24.0f       // m/s^2, snappier than real 9.8 for game feel
#define GROUND_ACCEL 60.0f  // m/s^2, reach full speed in ~0.1s
#define AIR_ACCEL 20.0f     // m/s^2, limited steering mid-air
#define COYOTE_TIME 0.1f    // seconds of jump grace after leaving ground
#define MOUSE_SENS 0.1f     // degrees per pixel of mouse movement
#define PITCH_LIMIT 85.0f   // max look up/down in degrees

// Test world grid (level files come later)
#define GRID_COLS 20

// Window
#define WINDOW_W 1280
#define WINDOW_H 720

#endif
