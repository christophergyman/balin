# Design Decision: 3D world with 2D sprite visuals

Date: 2026-09-06

## Decision

Balin renders a 3D world using flat 2D texture sprites instead of 3D models.
The world is real 3D geometry (positions, camera, depth), but everything visible
is a textured flat quad.

## Rules by object type

| Object type       | Technique                                    |
|-------------------|----------------------------------------------|
| Trees, big props  | Cross billboard: two quads crossed at 90 deg |
| NPCs, creatures   | Camera-facing billboard (rotates to camera)  |
| Walls, floors     | Fixed flat quads, static textures            |
| Player (Balin)    | Camera-facing billboard, same as NPCs        |

## World data model

Hybrid placement, the classic pattern:

- **World tiles live on a grid.** Integer tile coordinates, one entry per
  tile in the level file. Each tile stores:
  - its type (wall, floor, air, door, ...)
  - a facing direction, so the renderer knows how to orient the flat
    texture on the quad and which way a thin wall runs
- **Objects that move live at free float positions** (x, y, z), not tile
  slots. NPCs, Balin, and pickups slide smoothly without snapping.
- Trees and props are grid-placed in the editor but rendered at the tile
  center plus a small random offset, so forests do not look stamped.
- Collision is one simple test: does this float position overlap a solid
  grid tile? Same wall logic as the 2D prototype, with floats.
- The grid is for level data and collision. The free floats are for
  motion. Rendering style is independent of both.

Precedent: Doom (grid blockmap, float entity positions), Daggerfall
(grid dungeon tiles, free action objects), Don't Starve (grid biome
tiles, free object positions).

## Why

- No 3D modelling or sprite rotations needed. One flat image per object.
- Proven style: Doom, Daggerfall, Don't Starve.
- Matches the pixel-art dwarf-in-a-cave look.
- Keeps the engine simple: one quad type, one texture type.

## Constraints and implications

- Sprite textures need alpha transparency (.png).
- Wall face quads render double-sided: backface culling is disabled while
  drawing tiles, so one quad is visible from both directions. This is a
  deliberate design choice (2026-09-06): tiles do not need opposite
  face pairs to look solid.
- Transparent sprites must draw back to front for correct blending.
- Camera stays near horizontal; extreme pitch breaks the illusion.
- Textures load once and are reused by name, so the level format
  references sprites by name rather than embedding images.
- First-person or low-angle third-person camera fits this style best.

## DrawMesh transform composition order

Date: 2026-09-06

With raylib 6 `DrawMesh`, compose a transform as
`MatrixMultiply(rotation, translation)`. That means "rotate the quad
around its own center, then place it at the position."

The reverse order, `MatrixMultiply(translation, rotation)`, rotates the
already-placed quad around the world origin. Every face with a non-identity
rotation lands mirrored through (0, 0), far from its tile. Identity-rotation
faces look correct either way, which hides the bug in translate-only tests.

Proven with an isolated framebuffer test (one red quad, known transform,
pixel bounding box read back), not from raymath docs. The rule lives in a
comment at `WallFaceMatrix` in `src/main.c`.
