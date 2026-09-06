#ifndef EDITOR_H
#define EDITOR_H

#include <stdbool.h>

#include "entity.h"
#include "player.h"
#include "world.h"

typedef struct Editor {
    bool active;
    bool showCollision;
    bool freemove; // free-fly camera while editing, off = anchored to player
    Vector3 camPos; // camera position while freemove is on
    bool hoverValid;
    int hoverCol;
    int hoverRow;
    int hoverDir; // DirBit of the hovered edge, 0 = no edge (floor only)
    int lastPaint; // dedupe key for left-drag face painting, 0 = none
} Editor;

// Runs the editor: picking, hotkeys, camera orbit. Only when editor->active.
void UpdateEditor(Editor *editor, Camera3D *camera, Player *player, World *world,
                  Entity *entities, int *entityCount, bool *dirty, float dt);

// 3D overlays: hover highlight and collision debug view. Call inside BeginMode3D.
void DrawEditorOverlays(const Editor *editor, const World *world);

// 2D debug panel. Call inside BeginDrawing, after EndMode3D.
void DrawEditorPanel(Editor *editor, World *world, Entity *entities, int *entityCount,
                     Player *player, bool *dirty);

#endif
