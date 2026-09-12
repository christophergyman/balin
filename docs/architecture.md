# Balin: Architecture Decision Document

| Field | Value |
|---|---|
| Status | Draft v1.2 |
| Owner | cman |
| Date | 2026-09-11 |
| Scope | Engine and code architecture for the Balin demo |
| Related | `docs/product-requirements-document.md` for product decisions |

This document adapts general ADR practice to a solo game project. There is no strong, game-specific ADR convention in the sources, so the game-specific sections (frame budget, editor workflow, playtest confirmation) are a reasoned adaptation, not an established standard.

## 1. How to use this document

- **One decision per record.** Each ADR is short and answers one question.
- **Significance test.** A decision belongs here if it affects structure, dependencies, interfaces, performance, or the development workflow. Everything smaller lives in code or comments.
- **Lifecycle.** `proposed` -> `accepted` -> `superseded by ADR-XXX` or `deprecated`. An accepted record is never rewritten. A changed decision gets a new record that supersedes the old one.
- **Adding a record.** Copy the template, take the next number, state the decision in active voice ("We will..."), and list real consequences, including negative ones.
- **Confirmation.** Each record ends with how we verify the decision still holds: a test, a profiling check, an editor walkthrough, or a playtest.
- **Sources.** The format follows Nygard, MADR, Fowler, and AWS guidance, listed in section 7.

## 2. System overview

This section is orientation, not a decision. Update it freely when the shape of the system changes.

### 2.1 Layers

```
+-----------------------------------------------------------+
| main.c          composition root, main loop, wiring        |
+-----------------------------------------------------------+
| src/tools/      editor, dev builds only                    |
+-----------------------------------------------------------+
| src/game/       world, ECS, systems, states, HUD, bag      |
+-----------------------------------------------------------+
| src/engine/     time, input, render, audio, assets,        |
|                 arena, math, logging                       |
+-----------------------------------------------------------+
| raylib          GLFW, OpenGL, miniaudio, raymath           |
+-----------------------------------------------------------+
```

Dependency rule: engine never includes game headers. Game may include engine headers. Tools may include both. Only `main.c` wires the layers together.

### 2.2 Runtime flow

```
window event poll (raylib)
      |
      v
input sample (engine) -> InputState snapshot for this frame
      |
      v
fixed tick loop @ 60 Hz (0..N steps, capped at 5)
      |
      ordered systems (ADR-017)
      |
      v
render interpolation (alpha = accumulator / dt)
      |
      v
draw layers -> light mask multiply -> emissive -> HUD
```

### 2.3 Directory map

```
CMakeLists.txt
assets/
  tilesets/          32x32 grid PNG sheets
  manifest.txt       sheets, tiles, animations, items
  levels/            section files, versioned text
  tuning.txt         reloadable tuning values
  log.cfg            reloadable log thresholds
src/
  main.c
  engine/            time, input, render, audio, assets, arena, log, math
  game/              world, ecs, systems/, states/, hud, inventory
  tools/             editor/
tests/
  test_main.c        pure-logic tests
docs/
  product-requirements-document.md
  architecture.md
```

### 2.4 Frame budget

| Budget | Target at 120 FPS | Notes |
|---|---|---|
| Total frame | 8.3 ms | 60 FPS floor means 16.6 ms on low-end |
| Simulation | under 1.5 ms | 60 Hz tick, all systems |
| Render | under 3 ms | base resolution 640x360 |
| Audio | under 0.5 ms | event flush only, mixing is threaded in raylib |
| Headroom | the rest | input latency and spikes |

### 2.5 Conventions

- Base resolution is 640x360 pixels. The window defaults to 1280x720, an exact 2x integer scale.
- Tiles and entity sprites are 32x32 pixels. Entity movement is continuous, per the PRD section 6.
- Coordinates: origin top-left, +x right, +y down, all in base-resolution pixels. No world units.
- Angles: radians in code, degrees in tuning files where it reads more naturally.
- Time: seconds in state and data, ticks in scheduled logic. `dt` is always `1.0f / 60.0f`.
- Entity handles are 32-bit: a 16-bit slot index plus a 16-bit generation counter.

## 3. Decision index

| ID | Title | Status | Date |
|---|---|---|---|
| ADR-001 | C11 with old repo code conventions | accepted | 2026-09-11 |
| ADR-002 | Engine, game, and tools folders | accepted | 2026-09-11 |
| ADR-003 | Scoped arenas for variable-size data | accepted | 2026-09-11 |
| ADR-004 | Fixed 60 Hz simulation with render interpolation | accepted | 2026-09-11 |
| ADR-005 | Small custom ECS with indexed arrays and bitsets | accepted | 2026-09-11 |
| ADR-006 | Layered tile grid per section | accepted | 2026-09-11 |
| ADR-007 | Circle collision against tile edges | accepted | 2026-09-11 |
| ADR-008 | Versioned text content and reloadable tuning | accepted | 2026-09-11 |
| ADR-009 | Light mask rendering, no custom shader | accepted | 2026-09-11 |
| ADR-010 | Action map with input buffers | accepted | 2026-09-11 |
| ADR-011 | Flat state enum with per-state functions | accepted | 2026-09-11 |
| ADR-012 | Editor as a dev state with hand-rolled UI | accepted | 2026-09-11 |
| ADR-013 | Grid PNG sheets with a reloadable manifest | accepted | 2026-09-11 |
| ADR-014 | Audio event queue and crossfaded drone layers | accepted | 2026-09-11 |
| ADR-015 | Pure-logic tests plus an in-house debug overlay | accepted | 2026-09-11 |
| ADR-016 | CMake with Debug sanitizers, Release, and a test target | accepted | 2026-09-11 |
| ADR-017 | Explicit ordered system list | accepted | 2026-09-11 |
| ADR-018 | Assets in repo, copied next to the executable for release | accepted | 2026-09-11 |
| ADR-019 | Fail fast on malformed data | accepted | 2026-09-11 |
| ADR-020 | Structured logfmt events, a flight recorder ring, and bug bundles | accepted | 2026-09-11 |
| ADR-021 | Per-entity component masks in the ECS | accepted | 2026-09-12 |

## 4. Decision records

### ADR-001: C11 with old repo code conventions

- Status: accepted
- Date: 2026-09-11

**Context.** The game is C and raylib. The old 3D repo established a consistent style that mirrors raylib naming. Contributors include AI tooling, which follows explicit conventions better than implicit ones.

**Decision drivers**

- Consistency with raylib and the previous codebase.
- Readable diffs and easy review for a solo developer.
- Low ceremony, no new tooling.

**Considered options**

1. Old repo style. Good: matches raylib and history. Bad: diverges from common C style guides.
2. Snake case C style. Good: common in C projects. Bad: inconsistent with raylib calls.
3. Newer C standard. Good: modern features. Bad: tooling risk, little gain here.

**Decision outcome.** We will use C11 with clang: PascalCase types and functions, camelCase fields and locals, ALL_CAPS macros, snake_case file names, 4-space indentation, K&R braces, include guards, and `static` for module-private helpers.

**Consequences**

- Positive: one style across engine, game, tools, and generated code.
- Positive: raylib examples paste in with minimal editing.
- Negative: no formatter enforces it yet. A `.clang-format` is a backlog item.

**Confirmation.** Code review. The next code change following a different style is treated as a bug.

### ADR-002: Engine, game, and tools folders

- Status: accepted
- Date: 2026-09-11

**Context.** The PRD adds an editor, a bag, combat, and world systems. Without a boundary rule, engine code and game code tangle, and the editor duplicates logic.

**Decision drivers**

- Clear direction of dependencies.
- Editor must not fork simulation logic.
- Enough structure for a solo project, no ceremony for its own sake.

**Considered options**

1. Flat `src/` with a dependency rule. Good: no path churn. Bad: boundaries are only in the naming.
2. Folders `src/engine`, `src/game`, `src/tools`. Good: boundaries visible at a glance. Bad: slightly more structure.
3. Single `main.c`. Good: fastest start. Bad: unusable once the editor lands.

**Decision outcome.** We will use three folders with a hard dependency rule: engine never includes game headers, game may include engine headers, tools may include both, and `main.c` is the only composition root.

**Consequences**

- Positive: the editor reuses the same world, ECS, and render code.
- Positive: engine code is testable without the game.
- Negative: shared types like vectors must live in engine, not game.

**Confirmation.** A grep check that no file under `src/engine` includes from `src/game`. Run it before milestone reviews.

### ADR-003: Scoped arenas for variable-size data

- Status: accepted
- Date: 2026-09-11

**Context.** Bump arenas give fast allocation and explicit lifetimes, which suits a fixed-scope demo. The game has four natural lifetimes: assets, level, run, and frame.

**Decision drivers**

- No heap fragmentation or hitches during play.
- Lifetimes tied to game events: level load and run restart.
- Simple ownership and fast reset on death.

**Considered options**

1. Persistent arena plus frame scratch. Good: two scopes. Bad: level and run data share one lifetime.
2. Single persistent arena. Good: simplest. Bad: no clean reset point for restarts.
3. Scoped arenas: asset, level, run, frame. Good: each lifetime is explicit. Bad: slightly more bookkeeping.

**Decision outcome.** We will use four bump arenas:
- **Asset arena:** loaded once at boot, reset only on a full asset reload.
- **Level arena:** reset when a section loads.
- **Run arena:** reset on death and on a fresh run.
- **Frame arena:** reset at the start of every rendered frame for scratch lists.

Persistent objects use fixed arrays and structs, not the arenas. Arenas serve variable-size and file-loaded data. The API is `ArenaPush(arena, size, alignment)` and `ArenaReset(arena)`, with 16-byte default alignment and no free.

**Consequences**

- Positive: death restart is a handful of pointer resets plus struct re-initialization.
- Positive: no `malloc` or `free` in gameplay.
- Negative: no general free. Data that outlives its scope must live in a later-reset arena or a struct field.
- Negative: arena overflow is fatal, by ADR-019.

**Confirmation.** Unit tests for push, alignment, and reset. The debug overlay shows bytes used per arena.

### ADR-004: Fixed 60 Hz simulation with render interpolation

- Status: accepted
- Date: 2026-09-11

**Context.** Physics must not change with frame rate. The PRD targets 120+ FPS on capable hardware and a 60 FPS floor. Dash i-frames and faith regeneration need tick-exact behavior.

**Decision drivers**

- Frame-rate-independent physics and combat.
- Smooth motion on 120+ Hz displays.
- Simple mental model for timing.

**Considered options**

1. Fixed 60 Hz plus interpolation. Good: deterministic, cheap, smooth. Bad: requires previous and current state for visuals.
2. Fixed 120 Hz plus interpolation. Good: tighter feel. Bad: double sim cost, more state.
3. Variable delta. Good: simplest. Bad: physics changes with FPS, explicitly rejected.

**Decision outcome.** We will run simulation at a fixed 60 Hz with an accumulator, cap catch-up at 5 steps per frame to avoid a spiral of death, and interpolate rendered positions with `alpha = accumulator / dt`. Pause states skip ticks but keep rendering.

**Consequences**

- Positive: dash distance, regen, and collision behave identically at any frame rate.
- Positive: deterministic bugs are reproducible.
- Negative: renderable entities carry `position` and `previousPosition`.
- Negative: input must be buffered so edges are not lost between ticks, see ADR-010.

**Confirmation.** A manual check that a dash covers the same distance at 30, 60, 120, and 144 FPS. The overlay shows tick count and accumulator.

### ADR-005: Small custom ECS with indexed arrays and bitsets

- Status: accepted
- Date: 2026-09-11
- Superseded by: ADR-021 for the mask layout only. The rest stands.

**Context.** The PRD has three enemy types, a player, pickups, lights, and burn effects. A full ECS is unnecessary machinery, but ad-hoc structs would branch constantly.

**Decision drivers**

- Data-oriented layout for small counts.
- Explicit component lifetimes.
- No framework, no virtual dispatch, debugger-friendly.

**Considered options**

1. Slots plus per-kind data. Good: simple. Bad: per-kind branching returns as types grow.
2. Small custom ECS. Good: composable and uniform. Bad: more upfront machinery.
3. Off-the-shelf ECS. Good: battle-tested. Bad: external dependency and learning cost.

**Decision outcome.** We will build a small ECS:
- Entity handle: 16-bit index plus 16-bit generation.
- Capacity: 256 entities, fixed.
- Component storage: one fixed array per component indexed by entity slot, plus a presence bitset per component.
- Systems iterate the slot range and test masks.
- Destroy is deferred: entities are marked dead and cleaned after the system list runs.

First component set:

| Component | Fields | Used by |
|---|---|---|
| Transform | position, previousPosition, facing | all |
| Velocity | vx, vy | player, enemies |
| Collider | radius, layer | player, enemies, pickups |
| Health | hp, maxHp, hitFlash, iFrames | player, enemies |
| Melee | phase, timer, damage, range, arc | player, miners, brute |
| AI | state, stateTimer, home, aggroRange, leash | enemies |
| Burn | dps, remaining, tickTimer | enemies hit by Smite |
| Sprite | sheet, animation, frame, frameTimer, flip | visible entities |
| Light | radius, color, intensity | player, shrines, burn |
| Pickup | itemKind | floor items |
| Tags | Player, Crawler, Miner, Brute, Shrine, Gate | filtering |

Player state that is not per-frame ECS data (faith, bag, run stats) lives in a single `PlayerState` struct owned by the game.

**Consequences**

- Positive: adding a fourth enemy type is data, not new architecture.
- Positive: a debug overlay can print component counts trivially.
- Negative: component arrays are fixed at 256 slots and waste some memory.
- Negative: bitset masks cap components at 64 until a second word is added.

**Confirmation.** Unit tests for create, destroy, generation reuse, and mask iteration.

### ADR-006: Layered tile grid per section

- Status: accepted
- Date: 2026-09-11

**Context.** The editor paints terrain, collision reads walls, and the light system shades around them. Sections are small and hand-authored.

**Decision drivers**

- Simple editor tooling: pick a layer, paint tiles.
- Collision and rendering read the same data.
- No streaming or chunking needed for four short sections.

**Considered options**

1. Layered grid: floor, wall and collision, decor. Good: flexible, editor-friendly. Bad: three arrays to keep in sync.
2. Single-layer grid with packed flags. Good: less memory. Bad: cramped decor and editor modes.
3. Chunked world. Good: scales to open worlds. Bad: machinery the demo never uses.

**Decision outcome.** We will store each section as layers of fixed-capacity grids, capped at 128x128 tiles:
- Floor: visual tiles, no collision.
- Wall: visual and collision tiles.
- Decor: visual only, drawn after entities where needed.

Tiles store a tile ID plus flags. The active section lives in the level arena. Entity positions are independent pixel coordinates inside the section.

**Consequences**

- Positive: the editor and runtime share one representation.
- Positive: collision queries are simple grid lookups.
- Negative: a 128x128 cap must be asserted at load time.
- Negative: decor over entities needs a draw-order rule in ADR-009.

**Confirmation.** Level serialization round-trip test. The overlay draws the collision layer.

### ADR-007: Circle collision against tile edges

- Status: accepted
- Date: 2026-09-11

**Context.** Entities move freely in continuous space over a 32x32 tile grid. Circle bodies slide around corners more naturally than boxes in top-down movement. The old 3D repo already used a circle-vs-edge model.

**Decision drivers**

- Movement feel: sliding around corners and diagonals.
- Dash speed is about 7 pixels per tick, far below a tile, so discrete checks cannot tunnel.
- Entity counts are in the dozens, so all-pairs checks are free.

**Considered options**

1. AABB plus brute-force pairs. Good: simplest to debug. Bad: boxy corners.
2. Circles against tile edges. Good: natural sliding, familiar from the old repo. Bad: more math.
3. Spatial hash broadphase. Good: scales to hundreds. Bad: unnecessary now.

**Decision outcome.** We will use circle bodies. Each collider has a radius. For tile collision, resolve against the closest point on nearby tile edges, repeating per axis of travel. For entity pairs, check circles directly and separate overlaps. After entity separation, re-resolve against tiles so nothing is pushed into a wall.

**Consequences**

- Positive: one collision shape for player, enemies, and pickups.
- Positive: debug overlay draws circles directly.
- Negative: closest-point math has sign and normal edge cases. Tests will cover corners and tile seams.
- Negative: entity separation can jitter in dense packs. Tune the separation rate.

**Confirmation.** Pure-logic tests for edge, corner, and seam cases. Playtest if corners feel sticky or seams catch.

### ADR-008: Versioned text content and reloadable tuning

- Status: accepted
- Date: 2026-09-11

**Context.** The editor writes levels, and tuning will change constantly. Git diffs and fast iteration both matter more than file size or parse speed at this scale.

**Decision drivers**

- Levels must diff cleanly in version control.
- Tuning changes should apply without a rebuild.
- Formats must survive code changes gracefully.

**Considered options**

1. Versioned text. Good: diffable, debuggable, robust. Bad: parser code to write.
2. Binary with a version header. Good: compact. Bad: opaque, brittle across code changes.
3. JSON. Good: standard. Bad: parser dependency, verbose.

**Decision outcome.** We will use two text formats plus a manifest:
- **Level format:** a version header, then layers as rows of tile IDs with run-length compression, then entity records (kind, x, y), gate records (x, y, faith cap), and shrine records. One file per section.
- **Tuning format:** `key = value` lines with units in comments, grouped by system: player, faith, enemies, food, light, audio. The file is re-read when its modification time changes, in dev builds and release.
- **Version mismatch:** fatal, with the file name and version numbers logged, per ADR-019.

**Consequences**

- Positive: content changes produce readable diffs.
- Positive: tuning sessions do not need rebuilds.
- Negative: parser and validation code must exist and be tested.
- Negative: two sources of truth do not exist; the tuning file is required, there are no compiled defaults.

**Confirmation.** Round-trip serialization tests. Malformed-file tests that expect a clean error. A tuning change applied while the game runs.

### ADR-009: Light mask rendering, no custom shader

- Status: accepted
- Date: 2026-09-11

**Context.** Light is the game's signature: it scales with faith, shrines and burns glow, and enemy eyes must stay visible at 10% faith. The PRD targets 120+ FPS.

**Decision drivers**

- Multiple light sources without custom shader code.
- Emissive elements readable outside the player's radius.
- Crisp 32x32 pixel art at any window size.

**Considered options**

1. Light mask texture with multiply blending. Good: no shader, multiple lights add naturally. Bad: no shadows or occlusion.
2. Custom GLSL light shader. Good: flicker, color, shadows. Bad: more code and risk.
3. No lighting. Good: fastest. Bad: loses the core visual.

**Decision outcome.** We will render to a 640x360 render target with point filtering, integer-scale it to the window with letterboxing, and use this draw order:

1. Floor tiles
2. Floor decor
3. Pickups
4. Entities, y-sorted by feet position
5. Wall tiles and overhead decor
6. Non-emissive VFX
7. Light mask, multiplied over the scene
8. Emissive layer: Balin's aura edge, shrine flames, Smite burn, enemy eyes
9. HUD, in screen space

The light mask is a 640x360 render target: black base, one additive radial gradient per light. Balin's radius maps from current faith through the tuning table. There are no shadows in the MVP.

**Consequences**

- Positive: the 10% faith finale stays visually readable because emissive things draw after the mask.
- Positive: the camera can follow and clamp without affecting pixel scale.
- Negative: lights pass through walls, which is acceptable for the MVP but limits atmosphere.
- Negative: multiple overlapping lights wash out; tune the gradients.

**Confirmation.** Visual playtest at 100%, 40%, and 10% faith. Profiling of the mask pass in the overlay.

### ADR-010: Action map with input buffers

- Status: accepted
- Date: 2026-09-11

**Context.** Simulation runs at 60 Hz while rendering runs higher. Key presses can fall between ticks and be lost. The PRD excludes rebinding UI but hardcoding keys everywhere would block a later feature.

**Decision drivers**

- No dropped attack or dash presses.
- One place lists all bindings.
- The bag needs mouse input without polluting the world input path.

**Considered options**

1. Action map plus buffers. Good: robust and centralized. Bad: one layer of indirection.
2. Direct key polling. Good: fewest lines. Bad: hardcoded keys and dropped presses.
3. Full remappable stack. Good: future-proof. Bad: out of PRD scope.

**Decision outcome.** We will use an `Action` enum (move directions, attack, dash, smite, bag, interact) with one binding table. Each frame produces an `InputState` snapshot: held, pressed, and released per action. The player controller buffers attack and dash for 100 ms (6 ticks). Mouse position and buttons are a separate path used only by the bag and editor screens.

**Consequences**

- Positive: adding rebinding later means editing one table.
- Positive: buffered dash and attack feel responsive at any frame rate.
- Negative: buffered actions can fire shortly after the player changed their mind. Keep buffers short.

**Confirmation.** Playtest dash and attack responsiveness. The overlay shows buffered actions.

### ADR-011: Flat state enum with per-state functions

- Status: accepted
- Date: 2026-09-11

**Context.** The PRD defines play, paused bag, death beat, ending overlay, and a dev editor. There is no title screen and no menu stack.

**Decision drivers**

- Six states do not justify a framework.
- Transitions must be obvious in one place.
- Pause behavior must be explicit per state.

**Considered options**

1. Flat enum with enter, update, draw, exit per state. Good: traceable. Bad: manual transition calls.
2. State stack. Good: overlays and return-to-previous. Bad: indirection for no current need.
3. Boolean flags. Good: quick. Bad: untraceable combinations.

**Decision outcome.** We will use `State` enum values `Boot`, `Playing`, `Bag`, `Death`, `Ending`, `Editor`, each with `Enter`, `Update`, `Draw`, and `Exit` functions, and one `ChangeState` transition. Bag and Death skip simulation ticks while continuing to render. Death runs a 2 second beat, then resets the run arena and re-enters `Playing`. Ending shows the overlay until a key press starts a fresh run.

**Consequences**

- Positive: pause and reset logic is explicit and testable.
- Positive: the editor is just another state, matching ADR-012.
- Negative: cross-state data (for example the bag) lives in `PlayerState`, not the state itself.

**Confirmation.** Pure-logic tests for transition validity. Playtest the death-to-restart flow for feel.

### ADR-012: Editor as a dev state with hand-rolled UI

- Status: accepted
- Date: 2026-09-11

**Context.** The PRD requires a dev-only editor that builds the four sections and stays hidden in player builds. Hand-rolled UI keeps dependencies minimal and gives full control.

**Decision drivers**

- Editor and game must never diverge in behavior.
- No new UI dependency.
- Hidden in release builds by construction.

**Considered options**

1. Dev state sharing the runtime. Good: one simulation, one build. Bad: editor code lives in the same binary in dev.
2. Separate editor binary. Good: clean separation. Bad: build complexity and drift risk.
3. Mode flags in the game loop. Good: quick. Bad: muddies ADR-011.

**Decision outcome.** We will implement the editor as an `Editor` state compiled only when the CMake option `BALIN_DEV` is on (default on in Debug). Features:
- Modes: terrain, entity, gate, spawn. Mouse tools with grid snap; free camera pan and zoom.
- Terrain paints the three layers. Entity mode places enemies, food, small shrines, and the player spawn. Gate mode places gates and edits each faith cap.
- Save and load use the ADR-008 text format. F5 rebuilds the run from the edit buffer and switches to `Playing`. F6 returns to the editor.
- UI is a hand-rolled immediate-mode panel: buttons, lists, and property fields drawn with raylib primitives in screen space.

Out of scope: undo and redo, copy and paste, per-room playtest.

**Consequences**

- Positive: no editor and game divergence, because the same systems run in both.
- Positive: release builds contain no editor code.
- Negative: hand-rolled widgets need hit-testing and focus handling that a toolkit would provide.
- Negative: there is no undo, so destructive edits can lose work. Save often.

**Confirmation.** Build both configurations and confirm the release build has no editor symbols or hotkeys. Author one full section only in the editor.

### ADR-013: Grid PNG sheets with a reloadable manifest

- Status: accepted
- Date: 2026-09-11

**Context.** The AI pipeline emits 32x32 gothic tilesets as PNG sheets on a regular grid. Sprites and animations need names, frame rects, and origins. The PRD allows placeholder art early, so the pipeline must tolerate regeneration.

**Decision drivers**

- Regenerating a sheet should not require code changes.
- One texture per category keeps draw batching simple.
- Art tweaks should not need a rebuild.

**Considered options**

1. Grid sheets plus a text manifest. Good: tool-friendly, reloadable. Bad: custom manifest to maintain.
2. Separate PNGs. Good: trivial mapping. Bad: many files, no batching.
3. Tool JSON schema. Good: no custom parser. Bad: tied to the tool's format.

**Decision outcome.** We will use grid PNG sheets with a text manifest. Sheet cells are 32x32 with a fixed column count. The manifest declares:
- `sheet` entries: name, columns, rows.
- `tile` entries: tile ID, sheet, column, row.
- `anim` entries: name, sheet, frame list, frame duration, loop flag, origin offset.
- `item` entries: kind, icon frame, bag size, heal amount.

Manifests and images reload when their modification times change in dev builds. Loading is synchronous at boot; total asset size is small. Malformed entries fail fast per ADR-019.

**Consequences**

- Positive: regenerating art is drop-in. Sheets keep one texture bound per category.
- Positive: animations are data, so timing tweaks need no rebuild.
- Negative: no atlas packing tool yet; sheets are the packing unit.
- Negative: frame origins must be declared per animation for foot alignment.

**Confirmation.** A manifest parse test. Building and loading a sheet sliced by index, with a debug mode drawing frame rects.

### ADR-014: Audio event queue and crossfaded drone layers

- Status: accepted
- Date: 2026-09-11

**Context.** The PRD wants an ambient drone that darkens with faith plus about 12 SFX. Combat can produce many simultaneous hit sounds.

**Decision drivers**

- No audio calls scattered through gameplay code.
- Cooldowns and a voice cap prevent noise stacking.
- The drone must respond continuously to faith without new assets.

**Considered options**

1. Event queue plus two drone layers. Good: decoupled, controlled, expressive. Bad: small queue system to build.
2. Direct `PlaySound` calls. Good: simplest. Bad: no central control, untestable headless.
3. Queue plus single drone. Good: one asset. Bad: less expressive transition.

**Decision outcome.** We will use:
- An `AudioEvent` queue filled during the tick and flushed after systems run: sound ID, world position, volume, priority.
- A voice cap (16 concurrent) with per-sound cooldowns, for example footsteps and hit sounds.
- Two streamed drone layers, warm and hollow, crossfaded by current faith: warm at 100, hollow at 10.
- raylib's built-in audio only. No new audio dependency.

**Consequences**

- Positive: muting audio in tests is one flag.
- Positive: the drone communicates faith without UI.
- Negative: streamed layer crossfade needs testing on the target machine.
- Negative: positional panning is optional and deferred; sounds currently play centered.

**Confirmation.** Playtest at 100 and 10 faith to judge the crossfade. The overlay shows active voice count.

### ADR-015: Pure-logic tests plus an in-house debug overlay

- Status: accepted
- Date: 2026-09-11
- Superseded by: ADR-020 for the logging part only. Tests and overlay stay here.

**Context.** Game feel needs human playtests, but parsing, arenas, ECS, and collision math are cheap to verify automatically. Performance and AI state need visibility beyond printf.

**Decision drivers**

- Catch parser and serialization regressions early.
- See frame cost and ECS state without a debugger.
- No test framework or profiler dependency.

**Considered options**

1. Plain-assert test binary plus in-house overlay. Good: zero dependencies. Bad: hand-rolled harness.
2. No tests. Good: fastest. Bad: silent regressions.
3. Test framework plus Tracy. Good: powerful. Bad: setup cost, new dependencies.

**Decision outcome.** We will build:
- A `balin_tests` target using plain `assert`: arena push and reset, ECS create and destroy and generation reuse, circle-edge collision cases, tuning parsing, level serialization round-trip, manifest parsing.
- `LOG_ERROR`, `LOG_WARN`, `LOG_INFO`, `LOG_DEBUG`, and `ASSERT` macros wrapping raylib `TraceLog`, compiled out at Debug level in release.
- An F3 debug overlay: FPS, frame milliseconds, per-system timings, ECS component counts, arena usage, current state, player and AI state, collision circles, light radii, and gate values.

**Consequences**

- Positive: regression safety for the formats and math most likely to break.
- Positive: the overlay doubles as a profiling tool for the frame budget.
- Negative: gameplay logic remains untested and relies on playtests.
- Negative: macros must be used consistently or logging stays ad hoc.

**Confirmation.** `ctest` passes. The overlay appears in Debug and disappears in Release.

### ADR-016: CMake with Debug sanitizers, Release, and a test target

- Status: accepted
- Date: 2026-09-11

**Context.** The reset left a working CMake setup with vendored raylib. Sanitizers and tests need to fit without disrupting iteration.

**Decision drivers**

- Fast Debug cycle with memory and UB checking.
- Optimized Release for performance validation.
- One command builds everything, including tests.

**Considered options**

1. CMake with two build types plus a test target. Good: simple, standard. Bad: manual CI later.
2. Add CI from day one. Good: early break detection. Bad: setup and maintenance.
3. Makefile. Good: fewer layers. Bad: loses raylib CMake integration.

**Decision outcome.** We will keep CMake:
- Targets: `balin` and `balin_tests`.
- Debug: `-O0 -g`, AddressSanitizer and UBSan, `BALIN_DEV=ON`.
- Release: `-O2`, `BALIN_DEV=OFF`.
- Warnings: `-Wall -Wextra`, `-Werror` off.
- Vendored raylib untouched. No CI yet.

**Consequences**

- Positive: sanitizers catch arena and ECS memory bugs early.
- Positive: release validation is one flag change.
- Negative: sanitizer builds run slower, so performance checks use Release.

**Confirmation.** `cmake -S . -B build && cmake --build build && ctest --test-dir build` passes. Profiling uses a Release build.

### ADR-017: Explicit ordered system list

- Status: accepted
- Date: 2026-09-11

**Context.** Faith regeneration, burn damage, i-frames, and collision resolution all depend on update order. With a custom ECS, order is the architecture.

**Decision drivers**

- Deterministic, reproducible ticks.
- Order visible in one place for debugging.
- No registration or priority machinery.

**Considered options**

1. Explicit ordered list. Good: traceable, deterministic. Bad: edit one function when adding a system.
2. Self-registering systems with priorities. Good: plugin-like. Bad: effective order hidden.
3. Single update function. Good: quick. Bad: tangles as systems grow.

**Decision outcome.** We will run ticks through one explicit list, in this order:

1. Input snapshot applied, buffers ticked
2. AI decisions (aggro, states, wind-ups, leashes)
3. Movement and dash (velocity integration, i-frame timers)
4. Tile collision resolution
5. Entity separation
6. Melee hit resolution and damage queuing
7. Burn and damage over time
8. Faith regeneration and hit-faith drain
9. Health, death, and destruction queue
10. Lifetime cleanup (despawn, corpses, pickups consumed)
11. Audio event flush
12. Light source gather and render prep (y-sort)

**Consequences**

- Positive: order-dependent bugs are reproducible and reviewable.
- Positive: new systems slot in with an explicit position.
- Negative: the list is a hot spot for merge conflicts and must be reviewed when changed.

**Confirmation.** The system list lives in one function. Order-dependent behavior is covered by the collision and combat tests where possible, and by the debug overlay.

### ADR-018: Assets in repo, copied next to the executable for release

- Status: accepted
- Date: 2026-09-11

**Context.** Development runs from the repo, while a shared demo build should be a folder that runs anywhere. Hot reload reads source files during development.

**Decision drivers**

- Development convenience with live file edits.
- A portable release folder.
- One resolution rule, not two code paths that drift.

**Considered options**

1. Repo assets, copied for release. Good: works in both modes. Bad: lookup order logic.
2. Assets next to the executable only. Good: one rule. Bad: copy or symlink before every run.
3. Embedded assets. Good: one file. Bad: no hot reload, larger binary.

**Decision outcome.** We will keep assets in `assets/` in the repo. The runtime looks next to the executable first, then falls back to the project working directory. CMake copies `assets/` next to the binary for release builds. Missing files fail fast per ADR-019.

**Consequences**

- Positive: `./output/balin` and a release folder both work with no setup.
- Positive: the editor saves into the repo assets tree during development.
- Negative: two lookup locations can mask a missing release copy. The confirmation check covers it.

**Confirmation.** Run from the repo root and from the release folder. Both must load, and the release folder must contain `assets/`.

### ADR-019: Fail fast on malformed data

- Status: accepted
- Date: 2026-09-11

**Context.** Levels, tuning, and manifests are authored by hand and by tooling. Silent fallbacks hide content bugs until playtests, which is the slowest place to find them.

**Decision drivers**

- Content errors surface immediately with location.
- No hidden defaults creating two sources of truth.
- Simple mental model: the data is either correct or the game stops.

**Considered options**

1. Fail fast everywhere. Good: honest, fast to diagnose. Bad: a shared demo build stops on bad content.
2. Clamp in Release. Good: survives bad content. Bad: subtle wrong behavior.
3. Ignore bad entries. Good: most forgiving. Bad: hides bugs.

**Decision outcome.** We will fail fast on malformed data in every build. Parsers return an error with file name, line number, and expectation. Callers log it with `LOG_ERROR` and exit with a non-zero code. This applies to levels, tuning, manifests, and asset lookups.

**Consequences**

- Positive: every content bug produces one clear message with a location.
- Positive: no fallback values exist, so there is exactly one source of truth.
- Negative: a typo in a shared build stops the demo. Acceptable for a hand-authored MVP.

**Confirmation.** Malformed-file tests call the parsers directly and assert the error result. A manual test corrupts one line of each format.

### ADR-020: Structured logfmt events, a flight recorder ring, and bug bundles

- Status: accepted
- Date: 2026-09-11

**Context.** ADR-015 left logging to raylib TraceLog wrappers. That gives no categories, no thresholds, no history, and nothing to attach to a bug report. The flight recorder, the debug overlay, and shared demo builds all need one event shape. Rare state bugs are easiest to fix from the seconds before they happen.

**Decision drivers**

- One shape for console, file, ring, overlay, and bug bundles.
- No formatting cost when a level is off.
- No allocation in the log path; all buffers are static.
- Bounded memory for the flight recorder.
- Config changes apply without a rebuild.

**Considered options**

1. Structured logfmt events with a ring and bundles. Good: uniform, greppable, tool-friendly, bundle-ready. Bad: a custom logger to write and test.
2. raylib TraceLog wrappers. Good: no new code. Bad: no categories, thresholds, or history.
3. A third-party logging library. Good: features out of the box. Bad: a new dependency and no bundle concept.

**Decision outcome.** We will replace the logging part of ADR-015 with a structured logger:

- **Event shape.** One event per line in logfmt order: `ts`, `level`, `cat`, `tick`, `run`, `msg`, then caller key=value pairs. Example: `ts=2026-09-11T17:46:51.123Z level=info cat=game tick=1842 run=1 msg="section loaded" section=0`. Values with spaces or special characters are quoted and escaped.
- **Levels.** Six levels: `trace`, `debug`, `info`, `warn`, `error`, `fatal`. Macros `LOGT`, `LOGD`, `LOGI`, `LOGW`, and `LOGE` take a category and a format string. `ASSERT` logs `fatal` and exits non-zero, by ADR-019.
- **Categories.** A fixed enum: `boot`, `engine`, `input`, `render`, `audio`, `assets`, `game`, `ai`, `combat`, `editor`.
- **Thresholds.** `assets/log.cfg` sets a default threshold and one threshold per category. It is re-read when its modification time changes, the same rule as tuning in ADR-008. A missing file or malformed line is fatal; the message goes to stderr because the logger is not configured yet.
- **Sinks.** The console writes enabled lines; `warn` and above go to stderr. A session log is written under `output/logs/` and flushed with a per-frame byte budget, so a burst cannot stall a frame.
- **Formatting.** One static buffer formats a line at a time. The call site checks the threshold first, so a disabled level does no formatting. Nothing allocates, ever.
- **Flight recorder.** A fixed ring keeps the last 2048 events at `trace` detail, independent of thresholds. Each slot holds up to 256 bytes. The ring is a static array.
- **Bug bundle.** F9 writes a bundle folder under `output/bugs/<timestamp>/` with `session.log`, `ring.log`, `state.txt`, and `env.txt`. A fatal event writes the same bundle before exit in every build.
- **Snapshot.** `state.txt` starts with ECS counts, arena bytes, tick, run, and player and AI state. Later systems append sections of their own.

**Consequences**

- Positive: one event shape feeds every consumer, and a bundle is one folder to share.
- Positive: noisy categories drop to `warn` while the game runs, and the ring still keeps full detail.
- Positive: bounded memory and no allocation keep the frame budget safe.
- Negative: a custom logger needs tests that TraceLog would not need.
- Negative: a malformed `log.cfg` stops the game, by ADR-019.
- Negative: the ring reserves 2048 fixed slots even in a quiet session.

**Confirmation.** Tests for the logfmt shape and for a malformed `log.cfg`. A test that a disabled level does no formatting. A manual check that a threshold edit applies while the game runs and that F9 writes a complete bundle. ADR-015 carries a supersession note.

### ADR-021: Per-entity component masks in the ECS

- Status: accepted
- Date: 2026-09-12

**Context.** ADR-005 chose a small ECS with one indexed array and one presence bitset per component. That sentence has two readings: a 256-bit presence bitset per component over entity slots, or one 64-bit component mask per entity. ADR-005's own consequence, "bitset masks cap components at 64", only holds for the per-entity reading. BAL-11 implemented the storage and needed one reading fixed.

**Decision drivers**

- Query cost and code size at 256 fixed entities.
- An entity's component list should be readable in the debugger as one value.
- The first component set has 11 components, so a 64-type cap has headroom.

**Considered options**

1. One mask per entity. Good: one word test per slot, entity state in one value, simplest code. Bad: caps component types at 64.
2. One presence bitset per component. Good: no component cap, literal ADR-005 wording. Bad: four words per component and more bookkeeping per query at a scale where scanning 256 slots is already trivial.

**Decision outcome.** We will store one `uint64_t` component mask per entity slot in `EcsWorld`, with one bit per component id. Component data stays in one fixed array per component, with a per-component count for the overlay. Queries scan the slot range and test the mask. This supersedes the mask layout of ADR-005 only; the rest of ADR-005 stands.

**Consequences**

- Positive: a query is one AND compare per slot.
- Positive: component counts feed the F3 overlay for free.
- Negative: component ids cap at 64. `_Static_assert(ECS_MAX_COMPONENTS <= 64)` in `ecs.c` fails the build past the cap.
- Negative: the mask and the pool rows are two sources that must stay in sync. Only `EcsAdd`, `EcsRemove`, and `EcsDestroy` touch both.

**Confirmation.** The mask iteration, stale handle, and destroy tests in `balin_tests`. The static assert fires if `ECS_MAX_COMPONENTS` grows past 64.

## 5. Decision backlog

Items not yet decided or deliberately deferred. Each can become an ADR when it is promoted.

| Candidate | Note |
|---|---|
| `.clang-format` adoption | Enforce ADR-001 automatically |
| Windows build and CI | The PRD lists this as an open question |
| Spatial broadphase | Only if entity counts grow well past 256 |
| Custom light shader | Only if the mask limits atmosphere |
| Editor undo and redo | Deferred in ADR-012 |
| Positional audio panning | Deferred in ADR-014 |
| Hot reload of tuning in Release | Currently allowed; verify it earns its place |
| Key rebinding UI | PRD non-goal, made cheap by ADR-010 |
| Job system or threading | Out of scope; the game is single-threaded |
| Save games and meta-progression | PRD non-goal, run resets only |

## 6. Glossary

- **Arena:** a bump allocator. Allocation moves a pointer forward; reset restores it. No individual free.
- **AABB:** axis-aligned bounding box, a rectangle that never rotates.
- **Circle vs tile edges:** collision by finding the closest point on a wall edge to a circle center and pushing out along the normal.
- **ECS:** entity component system. Entities are IDs, components are data arrays, systems are functions over those arrays.
- **Fixed timestep:** simulation advances in constant increments, independent of render frame rate.
- **Generation counter:** a number bumped when an entity slot is reused, so stale handles cannot target new entities.
- **Hot reload:** re-reading a file while the game runs, usually triggered by a modification time change.
- **Light mask:** a grayscale texture where white is lit and black is dark, multiplied over the scene.
- **RLE:** run-length encoding, storing repeated values as a count plus value.
- **Y-sort:** drawing entities in order of their feet position so lower entities overlap higher ones.
- **Render target:** an off-screen texture the game draws into, then scales to the window.
- **Scoped arena:** an arena whose reset point matches a lifetime: asset, level, run, or frame.

## 7. Document sources

- Michael Nygard, "Documenting Architecture Decisions", 2011. The original short ADR format: context, decision, status, consequences.
- Olaf Zimmermann, "The Markdown ADR (MADR) Template Explained and Distilled", updated 2026. Decision drivers, considered options, decision outcome, confirmation.
- Martin Fowler, "Architecture Decision Record", 2026. Brevity, inverted pyramid, supersession, confidence and revisit triggers.
- AWS Architecture Blog, "Master architecture decision records (ADRs)", 2025. One decision per record, separate design from decision, timely decisions.
- adr.github.io. Definitions of architectural decisions and decision logs.
- Robert Nystrom, "Game Programming Patterns", chapter "Architecture, Performance, and Games". Simplicity, decoupling, and the cost of speculative abstraction in games.

## 8. Revision history

| Date | Change |
|---|---|
| 2026-09-11 | v1.0 draft. ADR-001 through ADR-019 recorded from the architecture interview. |
| 2026-09-11 | v1.1. ADR-020 recorded. Logging part of ADR-015 superseded. |
| 2026-09-12 | v1.2. ADR-021 recorded. Mask layout of ADR-005 superseded. |
