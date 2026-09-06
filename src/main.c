#include <stdio.h>
#include <stdlib.h>

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#include "config.h"
#include "editor.h"
#include "entity.h"
#include "entity_file.h"
#include "player.h"
#include "textures.h"
#include "world.h"
#include "world_file.h"

// One reusable textured quad: width TILE_SIZE, height WALL_HEIGHT, facing +Z
static Mesh BuildWallQuadMesh(void) {
    Mesh mesh = {0};
    mesh.vertexCount = 4;
    mesh.triangleCount = 2;

    float halfWidth = TILE_SIZE / 2.0f;
    mesh.vertices = MemAlloc(4 * 3 * sizeof(float));
    mesh.vertices[0] = -halfWidth; mesh.vertices[1]  = 0.0f;        mesh.vertices[2]  = 0.0f;
    mesh.vertices[3] = halfWidth;  mesh.vertices[4]  = 0.0f;        mesh.vertices[5]  = 0.0f;
    mesh.vertices[6] = halfWidth;  mesh.vertices[7]  = WALL_HEIGHT; mesh.vertices[8]  = 0.0f;
    mesh.vertices[9] = -halfWidth; mesh.vertices[10] = WALL_HEIGHT; mesh.vertices[11] = 0.0f;

    mesh.texcoords = MemAlloc(4 * 2 * sizeof(float));
    mesh.texcoords[0] = 0.0f; mesh.texcoords[1] = 1.0f; // bottom left
    mesh.texcoords[2] = 1.0f; mesh.texcoords[3] = 1.0f; // bottom right
    mesh.texcoords[4] = 1.0f; mesh.texcoords[5] = 0.0f; // top right
    mesh.texcoords[6] = 0.0f; mesh.texcoords[7] = 0.0f; // top left

    mesh.indices = MemAlloc(6 * sizeof(unsigned short));
    mesh.indices[0] = 0; mesh.indices[1] = 1; mesh.indices[2] = 2;
    mesh.indices[3] = 0; mesh.indices[4] = 2; mesh.indices[5] = 3;

    UploadMesh(&mesh, false);
    return mesh;
}

// Wall face quad for a tile edge: rotated to point outward, sitting on the edge
static Matrix WallFaceMatrix(int col, int row, int dirBit) {
    Vector3 center = TileCenter(col, row);
    float minX = col * TILE_SIZE;
    float minZ = row * TILE_SIZE;
    float maxX = minX + TILE_SIZE;
    float maxZ = minZ + TILE_SIZE;

    Matrix rotate;
    Vector3 position;
    switch (dirBit) {
    case DIR_N: // face points -Z
        rotate = MatrixRotateY(PI);
        position = (Vector3){center.x, 0.0f, minZ};
        break;
    case DIR_S: // face points +Z
        rotate = MatrixIdentity();
        position = (Vector3){center.x, 0.0f, maxZ};
        break;
    case DIR_E: // face points +X
        rotate = MatrixRotateY(PI / 2.0f);
        position = (Vector3){maxX, 0.0f, center.z};
        break;
    case DIR_W: // face points -X
        rotate = MatrixRotateY(-PI / 2.0f);
        position = (Vector3){minX, 0.0f, center.z};
        break;
    default:
        return MatrixIdentity();
    }
    return MatrixMultiply(MatrixTranslate(position.x, position.y, position.z), rotate);
}

int main(void) {
    // World Layer
    World world;
    LoadWorldTiles(&world);

    // Entity Layer
    Entity *entities = malloc(MAX_ENTITIES * sizeof(Entity));
    if (entities == NULL) {
        printf("Failed to allocate entity array\n");
        return 1;
    }
    int entityCount = LoadEntities(entities);

    // Find the player entity and give the body its saved position
    Entity *playerEntity = NULL;
    for (int i = 0; i < entityCount; i++) {
        if (entities[i].kind == ENTITY_PLAYER) {
            playerEntity = &entities[i];
            break;
        }
    }
    if (playerEntity == NULL) {
        printf("No player entity in save, cannot start\n");
        free(entities);
        return 1;
    }

    Player player;
    InitPlayer(&player, playerEntity->position);

    // Rendering resources, created after the window so the GPU context exists
    Camera3D camera = {
        .up = {0.0f, 1.0f, 0.0f},
        .fovy = 45.0f,
        .projection = CAMERA_PERSPECTIVE,
    };

    InitWindow(WINDOW_W, WINDOW_H, "Balin");
    SetTargetFPS(120);
    DisableCursor(); // lock and hide the mouse for first person look

    // Aim the camera along the player's look direction before the first frame
    camera.position = (Vector3){player.position.x, player.position.y + PLAYER_EYE, player.position.z};
    camera.target = Vector3Add(camera.position, PlayerLookDirection(&player));

    LoadTextureTable();
    Mesh wallQuad = BuildWallQuadMesh();
    Material wallMaterial = LoadMaterialDefault();

    Editor editor = {0};

    while (!WindowShouldClose()) {
        // TAB: toggle play mode and edit mode
        if (IsKeyPressed(KEY_TAB)) {
            editor.active = !editor.active;
            if (editor.active) {
                EnableCursor();
            } else {
                DisableCursor();
            }
        }

        // Q: save and quit
        if (IsKeyPressed(KEY_Q)) {
            playerEntity->position = player.position;
            SaveWorldTiles(&world);
            SaveEntities(entities, entityCount);
            printf("Saved and quitting\n");
            break;
        }

        bool dirty = false;
        if (editor.active) {
            UpdateEditor(&editor, &camera, &player, &world, entities, &entityCount, &dirty);
        } else {
            UpdatePlayer(&player, &camera, GetFrameTime(), &world);
        }

        BeginDrawing();
        ClearBackground(BLACK);

        BeginMode3D(camera);
        DrawGrid(WORLD_COLS * 2, (float)TILE_SIZE);

        // Wall faces render double-sided: no backface culling while drawing tiles
        rlDisableBackfaceCulling();

        // One textured quad per tile face
        for (int row = 0; row < WORLD_COLS; row++) {
            for (int col = 0; col < WORLD_COLS; col++) {
                const Tile *tile = &world.tiles[row][col];
                if (tile->faces == 0) {
                    continue;
                }
                Texture2D *texture = GetTexture(tile->textureId);
                if (texture == NULL) {
                    continue;
                }
                wallMaterial.maps[MATERIAL_MAP_ALBEDO].texture = *texture;

                static const int dirBits[4] = {DIR_N, DIR_E, DIR_S, DIR_W};
                for (int i = 0; i < 4; i++) {
                    if (tile->faces & dirBits[i]) {
                        DrawMesh(wallQuad, wallMaterial, WallFaceMatrix(col, row, dirBits[i]));
                    }
                }
            }
        }

        rlEnableBackfaceCulling();

        EndMode3D();

        DrawEditorOverlays(&editor, &world);

        DrawText("Balin 3D - WASD move, mouse look, space jump, Q quit", 10, 10, 20, WHITE);
        DrawEditorPanel(&editor, &world, entities, &entityCount, &player, &dirty);

        EndDrawing();
    }
    CloseWindow();

    UnloadMesh(wallQuad);
    UnloadMaterial(wallMaterial);
    UnloadTextureTable();
    free(entities);
    return 0;
}
