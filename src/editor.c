#define RAYGUI_IMPLEMENTATION
#include "editor.h"

#include <math.h>
#include <stdio.h>

#include "entity_file.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "raygui.h"
#pragma GCC diagnostic pop
#include "raymath.h"
#include "textures.h"
#include "world_file.h"

static float Clampf(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

// ---------------------------------------------------------------- picking

// The debug panel rectangle. Picking must not edit tiles behind it.
static const Rectangle PANEL_RECT = {1280.0f - 270.0f, 10.0f, 260.0f, 430.0f};

// True when the mouse ray hits a tile edge. Finds the tile whose face
// we can see: the owner tile is on the same side of the edge as the camera.
static bool PickHover(const Camera3D *camera, Editor *editor) {
    editor->hoverValid = false;
    editor->hoverDir = 0;

    Vector2 mouse = GetMousePosition();
    if (CheckCollisionPointRec(mouse, PANEL_RECT)) {
        return false;
    }

    Ray ray = GetScreenToWorldRay(mouse, *camera);

    float bestT = INFINITY;

    for (int row = 0; row < WORLD_COLS; row++) {
        for (int col = 0; col < WORLD_COLS; col++) {
            float minX = col * TILE_SIZE;
            float minZ = row * TILE_SIZE;
            float maxX = minX + TILE_SIZE;
            float maxZ = minZ + TILE_SIZE;

            // Two vertical planes per axis: the cell's two edges on that axis
            const float planeXs[2] = {minX, maxX};
            const float planeZs[2] = {minZ, maxZ};

            for (int i = 0; i < 2; i++) {
                // Vertical plane x = planeXs[i]
                if (fabsf(ray.direction.x) > 0.0001f) {
                    float t = (planeXs[i] - ray.position.x) / ray.direction.x;
                    if (t > 0.001f && t < bestT) {
                        float hitY = ray.position.y + ray.direction.y * t;
                        float hitZ = ray.position.z + ray.direction.z * t;
                        if (hitY >= 0.0f && hitY <= WALL_HEIGHT &&
                            hitZ >= minZ - 0.01f && hitZ <= maxZ + 0.01f) {
                            bestT = t;
                            editor->hoverValid = true;
                            // Plane x = minX is the west edge of (col,row)
                            if (planeXs[i] == minX) {
                                // camera west of the plane sees (col,row)'s west face
                                if (ray.position.x < planeXs[i]) {
                                    editor->hoverCol = col;
                                    editor->hoverRow = row;
                                    editor->hoverDir = DIR_W;
                                } else { // sees (col-1,row)'s east face
                                    editor->hoverCol = col - 1;
                                    editor->hoverRow = row;
                                    editor->hoverDir = DIR_E;
                                }
                            } else { // plane x = maxX: east edge of (col,row)
                                if (ray.position.x < planeXs[i]) {
                                    editor->hoverCol = col;
                                    editor->hoverRow = row;
                                    editor->hoverDir = DIR_E;
                                } else {
                                    editor->hoverCol = col + 1;
                                    editor->hoverRow = row;
                                    editor->hoverDir = DIR_W;
                                }
                            }
                        }
                    }
                }

                // Vertical plane z = planeZs[i]
                if (fabsf(ray.direction.z) > 0.0001f) {
                    float t = (planeZs[i] - ray.position.z) / ray.direction.z;
                    if (t > 0.001f && t < bestT) {
                        float hitY = ray.position.y + ray.direction.y * t;
                        float hitX = ray.position.x + ray.direction.x * t;
                        if (hitY >= 0.0f && hitY <= WALL_HEIGHT &&
                            hitX >= minX - 0.01f && hitX <= maxX + 0.01f) {
                            bestT = t;
                            editor->hoverValid = true;
                            if (planeZs[i] == minZ) { // north edge of (col,row)
                                if (ray.position.z < planeZs[i]) {
                                    editor->hoverCol = col;
                                    editor->hoverRow = row;
                                    editor->hoverDir = DIR_N;
                                } else {
                                    editor->hoverCol = col;
                                    editor->hoverRow = row - 1;
                                    editor->hoverDir = DIR_S;
                                }
                            } else { // south edge of (col,row)
                                if (ray.position.z < planeZs[i]) {
                                    editor->hoverCol = col;
                                    editor->hoverRow = row;
                                    editor->hoverDir = DIR_S;
                                } else {
                                    editor->hoverCol = col;
                                    editor->hoverRow = row + 1;
                                    editor->hoverDir = DIR_N;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return editor->hoverValid;
}

// ---------------------------------------------------------------- hotkeys

static void PlaceEnemyAtHover(Editor *editor, Entity *entities, int *entityCount, bool *dirty) {
    if (*entityCount >= MAX_ENTITIES) {
        printf("Entity limit reached (%d)\n", MAX_ENTITIES);
        return;
    }
    Vector3 center = TileCenter(editor->hoverCol, editor->hoverRow);
    entities[*entityCount] = (Entity){
        .kind = ENTITY_ENEMY,
        .position = center,
        .health = 100,
    };
    (*entityCount)++;
    *dirty = true;
    printf("Placed enemy at tile (col: %d, row: %d)\n", editor->hoverCol, editor->hoverRow);
}

static void RemoveEntityAtHover(Editor *editor, Entity *entities, int *entityCount, bool *dirty) {
    for (int i = 0; i < *entityCount; i++) {
        if (entities[i].kind == ENTITY_PLAYER) {
            continue;
        }
        int col = (int)(entities[i].position.x / TILE_SIZE);
        int row = (int)(entities[i].position.z / TILE_SIZE);
        if (col != editor->hoverCol || row != editor->hoverRow) {
            continue;
        }

        printf("Removed enemy at tile (col: %d, row: %d)\n", col, row);
        for (int j = i; j < *entityCount - 1; j++) {
            entities[j] = entities[j + 1];
        }
        (*entityCount)--;
        *dirty = true;
        return;
    }
}

static void HandleEditorHotkeys(Editor *editor, World *world,
                                Entity *entities, int *entityCount, bool *dirty) {
    if (!editor->hoverValid) {
        return;
    }
    Tile *tile = &world->tiles[editor->hoverRow][editor->hoverCol];

    // Left click (and drag) toggles the face on the hovered edge, like F
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && editor->hoverDir != 0) {
        int paintKey = ((editor->hoverCol * WORLD_COLS) + editor->hoverRow) * 16 + editor->hoverDir;
        if (paintKey != editor->lastPaint) {
            tile->faces ^= (uint8_t)editor->hoverDir;
            editor->lastPaint = paintKey;
            *dirty = true;
        }
    }

    // Cycle the tile's texture id from the loaded table
    int textureCount = GetTextureCount();
    if (IsKeyPressed(KEY_LEFT_BRACKET)) {
        tile->textureId = (uint8_t)((tile->textureId + textureCount - 1) % textureCount);
        *dirty = true;
    }
    if (IsKeyPressed(KEY_RIGHT_BRACKET)) {
        tile->textureId = (uint8_t)((tile->textureId + 1) % textureCount);
        *dirty = true;
    }

    if (editor->hoverDir != 0) {
        if (IsKeyPressed(KEY_F)) {
            tile->faces ^= (uint8_t)editor->hoverDir;
            *dirty = true;
            printf("Toggled face at tile (%d, %d)\n", editor->hoverCol, editor->hoverRow);
        }
        if (IsKeyPressed(KEY_C)) {
            tile->collision ^= (uint8_t)editor->hoverDir;
            *dirty = true;
            printf("Toggled collision at tile (%d, %d)\n", editor->hoverCol, editor->hoverRow);
        }
    }

    if (IsKeyPressed(KEY_X)) {
        tile->faces = 0;
        tile->collision = 0;
        *dirty = true;
        printf("Cleared tile (%d, %d)\n", editor->hoverCol, editor->hoverRow);
    }

    if (IsKeyPressed(KEY_E)) {
        PlaceEnemyAtHover(editor, entities, entityCount, dirty);
    }
    if (IsKeyPressed(KEY_R)) {
        RemoveEntityAtHover(editor, entities, entityCount, dirty);
    }
}

// ---------------------------------------------------------------- update

void UpdateEditor(Editor *editor, Camera3D *camera, Player *player, World *world,
                  Entity *entities, int *entityCount, bool *dirty) {
    if (!editor->active) {
        return;
    }

    if (IsKeyPressed(KEY_V)) {
        editor->showCollision = !editor->showCollision;
    }

    // Left mouse drag paints faces on every new hovered edge
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        editor->lastPaint = 0;
    }

    // Hold right mouse to orbit the camera while editing
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        Vector2 mouseDelta = GetMouseDelta();
        player->yaw += mouseDelta.x * MOUSE_SENS * DEG2RAD;
        player->pitch -= mouseDelta.y * MOUSE_SENS * DEG2RAD;
        float pitchLimit = PITCH_LIMIT * DEG2RAD;
        player->pitch = Clampf(player->pitch, -pitchLimit, pitchLimit);
        camera->target = Vector3Add(camera->position, PlayerLookDirection(player));
    }

    PickHover(camera, editor);
    HandleEditorHotkeys(editor, world, entities, entityCount, dirty);
}

// ---------------------------------------------------------------- overlays

static void DrawCellHighlight(const Editor *editor, Color color) {
    float y = 0.05f;
    float minX = editor->hoverCol * TILE_SIZE;
    float minZ = editor->hoverRow * TILE_SIZE;
    float maxX = minX + TILE_SIZE;
    float maxZ = minZ + TILE_SIZE;

    DrawLine3D((Vector3){minX, y, minZ}, (Vector3){maxX, y, minZ}, color);
    DrawLine3D((Vector3){maxX, y, minZ}, (Vector3){maxX, y, maxZ}, color);
    DrawLine3D((Vector3){maxX, y, maxZ}, (Vector3){minX, y, maxZ}, color);
    DrawLine3D((Vector3){minX, y, maxZ}, (Vector3){minX, y, minZ}, color);
}

static void DrawEdgeHighlight(const Editor *editor, Color color) {
    if (!editor->hoverValid || editor->hoverDir == 0) {
        return;
    }
    float minX = editor->hoverCol * TILE_SIZE;
    float minZ = editor->hoverRow * TILE_SIZE;
    float maxX = minX + TILE_SIZE;
    float maxZ = minZ + TILE_SIZE;
    float half = WALL_HEIGHT / 2.0f;
    Color translucent = (Color){color.r, color.g, color.b, 110};

    switch (editor->hoverDir) {
    case DIR_N:
        DrawCube((Vector3){(minX + maxX) / 2.0f, half, minZ}, TILE_SIZE, WALL_HEIGHT, 0.06f, translucent);
        break;
    case DIR_S:
        DrawCube((Vector3){(minX + maxX) / 2.0f, half, maxZ}, TILE_SIZE, WALL_HEIGHT, 0.06f, translucent);
        break;
    case DIR_W:
        DrawCube((Vector3){minX, half, (minZ + maxZ) / 2.0f}, 0.06f, WALL_HEIGHT, TILE_SIZE, translucent);
        break;
    case DIR_E:
        DrawCube((Vector3){maxX, half, (minZ + maxZ) / 2.0f}, 0.06f, WALL_HEIGHT, TILE_SIZE, translucent);
        break;
    }
}

// Red direction arrows: one per face bit, pointing from the tile center
// out toward the edge that face sits on
static void DrawFaceDirectionArrows(const World *world, int col, int row, float y, Color color) {
    const Tile *tile = &world->tiles[row][col];
    if (tile->faces == 0) {
        return;
    }

    Vector3 center = TileCenter(col, row);
    center.y = y;
    float minX = col * TILE_SIZE;
    float minZ = row * TILE_SIZE;
    float maxX = minX + TILE_SIZE;
    float maxZ = minZ + TILE_SIZE;
    float head = 0.25f; // arrowhead size

    if (tile->faces & DIR_N) {
        Vector3 tip = {center.x, y, minZ};
        DrawLine3D(center, tip, color);
        DrawLine3D(tip, (Vector3){center.x - head, y, minZ + head}, color);
        DrawLine3D(tip, (Vector3){center.x + head, y, minZ + head}, color);
    }
    if (tile->faces & DIR_S) {
        Vector3 tip = {center.x, y, maxZ};
        DrawLine3D(center, tip, color);
        DrawLine3D(tip, (Vector3){center.x - head, y, maxZ - head}, color);
        DrawLine3D(tip, (Vector3){center.x + head, y, maxZ - head}, color);
    }
    if (tile->faces & DIR_E) {
        Vector3 tip = {maxX, y, center.z};
        DrawLine3D(center, tip, color);
        DrawLine3D(tip, (Vector3){maxX - head, y, center.z - head}, color);
        DrawLine3D(tip, (Vector3){maxX - head, y, center.z + head}, color);
    }
    if (tile->faces & DIR_W) {
        Vector3 tip = {minX, y, center.z};
        DrawLine3D(center, tip, color);
        DrawLine3D(tip, (Vector3){minX + head, y, center.z - head}, color);
        DrawLine3D(tip, (Vector3){minX + head, y, center.z + head}, color);
    }
}

void DrawEditorOverlays(const Editor *editor, const World *world) {
    if (!editor->active) {
        return;
    }

    // Collision debug view: red lines on collision edges, blue on face edges
    if (editor->showCollision) {
        for (int row = 0; row < WORLD_COLS; row++) {
            for (int col = 0; col < WORLD_COLS; col++) {
                const Tile *tile = &world->tiles[row][col];
                float y = 0.1f;
                float minX = col * TILE_SIZE;
                float minZ = row * TILE_SIZE;
                float maxX = minX + TILE_SIZE;
                float maxZ = minZ + TILE_SIZE;

                if (tile->collision & DIR_N) DrawLine3D((Vector3){minX, y, minZ}, (Vector3){maxX, y, minZ}, RED);
                if (tile->collision & DIR_S) DrawLine3D((Vector3){minX, y, maxZ}, (Vector3){maxX, y, maxZ}, RED);
                if (tile->collision & DIR_W) DrawLine3D((Vector3){minX, y, minZ}, (Vector3){minX, y, maxZ}, RED);
                if (tile->collision & DIR_E) DrawLine3D((Vector3){maxX, y, minZ}, (Vector3){maxX, y, maxZ}, RED);

                if (tile->faces & DIR_N) DrawLine3D((Vector3){minX, y + 0.03f, minZ}, (Vector3){maxX, y + 0.03f, minZ}, SKYBLUE);
                if (tile->faces & DIR_S) DrawLine3D((Vector3){minX, y + 0.03f, maxZ}, (Vector3){maxX, y + 0.03f, maxZ}, SKYBLUE);
                if (tile->faces & DIR_W) DrawLine3D((Vector3){minX, y + 0.03f, minZ}, (Vector3){minX, y + 0.03f, maxZ}, SKYBLUE);
                if (tile->faces & DIR_E) DrawLine3D((Vector3){maxX, y + 0.03f, minZ}, (Vector3){maxX, y + 0.03f, maxZ}, SKYBLUE);
            }
        }
    }

    if (editor->hoverValid) {
        // Bright red arrows on the hovered tile show its face directions
        DrawFaceDirectionArrows(world, editor->hoverCol, editor->hoverRow, 0.3f, RED);
        DrawCellHighlight(editor, YELLOW);
        DrawEdgeHighlight(editor, ORANGE);
    }

    // Dim arrows on every faced tile while the debug view is open
    if (editor->showCollision) {
        for (int row = 0; row < WORLD_COLS; row++) {
            for (int col = 0; col < WORLD_COLS; col++) {
                if (world->tiles[row][col].faces != 0) {
                    DrawFaceDirectionArrows(world, col, row, 0.2f, (Color){200, 60, 60, 180});
                }
            }
        }
    }
}

// ---------------------------------------------------------------- panel

static void DirBitString(uint8_t bits, char *out, int outSize) {
    if (bits == 0) {
        snprintf(out, outSize, "-");
        return;
    }
    out[0] = '\0';
    if (bits & DIR_N) strncat(out, "N", outSize - strlen(out) - 1);
    if (bits & DIR_E) strncat(out, "E", outSize - strlen(out) - 1);
    if (bits & DIR_S) strncat(out, "S", outSize - strlen(out) - 1);
    if (bits & DIR_W) strncat(out, "W", outSize - strlen(out) - 1);
}

static void ReloadEverything(Editor *editor, World *world, Entity *entities,
                             int *entityCount, Player *player, bool *dirty) {
    LoadWorldTiles(world);
    *entityCount = LoadEntities(entities);

    Entity *playerEntity = NULL;
    for (int i = 0; i < *entityCount; i++) {
        if (entities[i].kind == ENTITY_PLAYER) {
            playerEntity = &entities[i];
            break;
        }
    }
    if (playerEntity != NULL) {
        InitPlayer(player, playerEntity->position);
    }
    *dirty = false;
    printf("Reloaded world and entities\n");
    (void)editor;
}

void DrawEditorPanel(Editor *editor, World *world, Entity *entities, int *entityCount,
                     Player *player, bool *dirty) {
    if (!editor->active) {
        return;
    }

    Rectangle panel = PANEL_RECT;
    GuiPanel(panel, "EDITOR");

    float x = panel.x + 10.0f;
    float w = panel.width - 20.0f;
    float y = panel.y + 36.0f;

    char line[96];

    snprintf(line, sizeof(line), "hover tile: (%d, %d)  edge: %s",
             editor->hoverValid ? editor->hoverCol : -1,
             editor->hoverValid ? editor->hoverRow : -1,
             editor->hoverDir == DIR_N ? "N" : editor->hoverDir == DIR_E ? "E"
                                       : editor->hoverDir == DIR_S       ? "S"
                                       : editor->hoverDir == DIR_W       ? "W"
                                                                         : "-");
    GuiLabel((Rectangle){x, y, w, 20}, line);
    y += 22;

    if (editor->hoverValid) {
        const Tile *tile = &world->tiles[editor->hoverRow][editor->hoverCol];
        char faces[16];
        char collision[16];
        DirBitString(tile->faces, faces, sizeof(faces));
        DirBitString(tile->collision, collision, sizeof(collision));
        snprintf(line, sizeof(line), "texture: %d / %d   faces: %s   coll: %s",
                 tile->textureId, GetTextureCount(), faces, collision);
        GuiLabel((Rectangle){x, y, w, 20}, line);
        y += 22;

        Vector3 center = TileCenter(editor->hoverCol, editor->hoverRow);
        int tileIndex = editor->hoverRow * WORLD_COLS + editor->hoverCol;
        snprintf(line, sizeof(line), "tile center: (%.1f, %.1f)  file idx: %d",
                 center.x, center.z, tileIndex);
        GuiLabel((Rectangle){x, y, w, 20}, line);
        y += 22;

        int entitiesHere = 0;
        for (int i = 0; i < *entityCount; i++) {
            int col = (int)(entities[i].position.x / TILE_SIZE);
            int row = (int)(entities[i].position.z / TILE_SIZE);
            if (col == editor->hoverCol && row == editor->hoverRow) {
                entitiesHere++;
            }
        }
        snprintf(line, sizeof(line), "entities on tile: %d", entitiesHere);
        GuiLabel((Rectangle){x, y, w, 20}, line);
        y += 22;
    } else {
        GuiLabel((Rectangle){x, y, w, 20}, "no tile under mouse");
        y += 66;
    }

    snprintf(line, sizeof(line), "player: (%.1f, %.1f, %.1f)",
             player->position.x, player->position.y, player->position.z);
    GuiLabel((Rectangle){x, y, w, 20}, line);
    y += 22;

    snprintf(line, sizeof(line), "entities: %d / %d   fps: %d",
             *entityCount, MAX_ENTITIES, GetFPS());
    GuiLabel((Rectangle){x, y, w, 20}, line);
    y += 26;

    GuiLabel((Rectangle){x, y, w, 20}, "TAB resume   RMB+drag look");
    y += 20;
    GuiLabel((Rectangle){x, y, w, 20}, "[ ] texture  F face  C collision");
    y += 20;
    GuiLabel((Rectangle){x, y, w, 20}, "X clear  E place  R remove entity");
    y += 20;
    GuiLabel((Rectangle){x, y, w, 20}, "V collision view");
    y += 26;

    GuiCheckBox((Rectangle){x, y, 20, 20}, "collision debug view", &editor->showCollision);
    y += 30;

    if (GuiButton((Rectangle){x, y, (w - 10) / 2, 26}, "SAVE")) {
        SaveWorldTiles(world);
        SaveEntities(entities, *entityCount);
        *dirty = false;
        printf("Saved world and entities\n");
    }
    if (GuiButton((Rectangle){x + (w - 10) / 2 + 10, y, (w - 10) / 2, 26}, "RELOAD")) {
        ReloadEverything(editor, world, entities, entityCount, player, dirty);
    }
    y += 34;

    if (GuiButton((Rectangle){x, y, w, 26}, "RESUME PLAY (TAB)")) {
        editor->active = false;
        DisableCursor();
    }
    y += 32;

    GuiLabel((Rectangle){x, y, w, 20}, *dirty ? "unsaved changes" : "saved");
}
