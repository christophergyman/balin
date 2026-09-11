# Balin: Demo PRD

| Field | Value |
|---|---|
| Status | Draft v1.0 |
| Owner | cman |
| Date | 2026-09-11 |
| Scope | Single-player 2D demo, one run of about 20 minutes |
| Doc type | Product decisions only. No architecture, no code |

This is a living document. Update it when decisions change. See `docs/game-design-doc-guide.md` for the doc practices behind this format.

## 1. Summary

**Pitch:** Balin is a grim 2D top-down survival action game. A dwarf of faith falls to the bottom of a cursed cave and must climb back to the surface. His faith is his light, his strength, and his dwindling hope. Every unholy threshold he crosses weakens him permanently. The demo is one 20 minute run through four sections. It ends at the surface or in the dark.

**Why this demo exists:** prove that the faith descent is fun, legible, and finishable in one sitting.

**Target player:** players who enjoy tense, dark action with heavy resource pressure and short punishing runs.

**Platform:** native desktop. Keyboard in the world, mouse in the bag.

## 2. Design pillars

1. **The climb weakens you.** Faith is the difficulty curve. The world does not get stronger. Balin gets weaker. Every gate is a permanent step down.
2. **Reward lives off the path.** Food and shrines sit in glowing side pockets. Safety costs risk.
3. **No words, only the cave.** The story is props, idols, and corpses. One line of text exists in the entire demo, at the end.

## 3. Player experience

- **Tone:** grim survival, dread, brief relief at flame, triumph that is earned and quiet.
- **Feel:** deliberate movement, readable fights, darkness pressing in, growing desperation.
- **Fantasy:** a devoted dwarf whose god feels farther away with every step up.
- **Session:** one run of about 20 minutes. Death restarts the whole run.

## 4. Core loop

**Moment to moment:** explore the dark, then fight or avoid, then loot glowing pockets, then manage HP, faith, and bag space, then pass a gate and climb on.

**Run structure:**

```
bottom shrine (full faith)
   -> section 1: cap 100
   -> gate: cap to 70
   -> section 2: cap 70
   -> gate: cap to 40
   -> section 3: cap 40
   -> gate: cap to 10
   -> section 4: cap 10, final shrine, gate brute
   -> surface (win) or death (restart from the bottom)
```

**Failure:** at 0 HP Balin dies and the run resets to the opening shrine. No saves, no checkpoints.

## 5. Systems

### 5.1 Faith (core system)

- Faith is divine power, shown as a flame. It scales axe damage and the light radius.
- Current faith regenerates over time toward the current maximum.
- Enemy hits drain current faith. Getting hit makes Balin weaker in the moment.
- Divine Smite spends current faith.
- Small shrines refill current faith only.
- The maximum is permanently cut at each gate. This is the descent.
- The HUD shows the lost maximum as a dark notch, so the player sees the ceiling fall even without text.
- First pass values: start 100, gates cut to 70, then 40, then 10. Tune in playtests.

**Design rule:** the world never scales up. A crawler in section 4 is the same crawler as in section 1. Balin is what changed.

### 5.2 HP and food

- HP is a separate pool. It does not regenerate over time.
- Only food restores HP. Shrines do not heal.
- Food is carried in the bag. Pickups are never consumed on contact.
- Food appears only in side pockets, never on the main path.
- Glowing pockets signal a resource. One visual rule: glow means reward.
- Design guardrail: each section guarantees at least one reachable food pocket, and the main path is survivable without it. Food is the safety margin, not a hard gate.

### 5.3 Grid bag

- A 4x4 bag. Items occupy multiple cells and rotate.
- Opening the bag pauses the cave.
- Mouse controls in the bag: drag to move, R to rotate, right click to eat, drag out to drop.
- Food items: apple 1x1, small mushroom 1x2, big mushroom 2x2, ration 1x3.
- If the bag is full, a pickup prompts a swap. The replaced item drops on the floor.
- The bag resets to empty on death.

### 5.4 Combat

- **Move:** free 8-direction movement.
- **Axe swing:** short melee arc. The main damage tool.
- **Dash:** short burst with brief invincibility frames. The main defensive tool.
- **Divine Smite:** targets the nearest enemy in front and ignites it with a burning aura that deals damage over time. Costs faith and has a cooldown. No aiming required.
- No block and no ranged weapon.
- Fights must be readable: every enemy attack has a visible wind-up, and enemy silhouettes stay distinct in low light.

### 5.5 Enemies

| Enemy | Role | Behavior | First pass |
|---|---|---|---|
| Cave crawler | Bread and butter | Fast, weak, attacks in packs | 20 HP, low damage |
| Fallen miner | Dread and story | Slow, telegraphed heavy swing, high HP | 60 HP, high damage |
| Gate brute | Finale elite | Slow, heavy, guards the last gate | 200 HP, dies to one full Smite burn plus follow-up hits |

- Enemies are hand-placed. They aggro on sight and give up at a leash range, returning home.
- No additional enemy types in the demo.
- Fallen miners are the thematic heart: they are what Balin could become.

### 5.6 Shrines and gates

- **Opening shrine:** sets faith to full and heals. Starts the run.
- **Small shrines:** refill current faith only, one time each, placed in glowing side pockets, one or two per section.
- **Final shrine:** a scripted beat before the gate brute. It grants exactly enough faith for one Divine Smite and resets the Smite cooldown. This is the intended solution to the brute.
- **Gates:** unholy thresholds, visually dark idols or snuffed braziers. Crossing one is one-way and permanently cuts the faith maximum. The cut is the story of the cave killing Balin's belief.
- The final shrine sits after the last regular encounter and within sight of the brute, so the one Smite naturally lands on the brute.

## 6. World structure

Four sections, about 5 minutes each, on a linear climb with side pockets.

| # | Section | Faith cap | Content | Exit |
|---|---|---|---|---|
| 0 | Opening shrine | 100 | Teach move, axe, dash. Crawlers only | Gate 1: cap to 70 |
| 1 | The Fallen Camp | 70 | Fallen miners appear. Environmental story: collapsed camp, dead miners, a broken shrine | Gate 2: cap to 40 |
| 2 | The Dark Chapel | 40 | Mixed enemies, cave hazards, the idol that took his faith, small shrines | Gate 3: cap to 10 |
| 3 | The Climb Out | 10 | Tiny light, few mandatory fights, the final shrine, the gate brute, daylight | Surface and ending overlay |

- One-way gates prevent backtracking and make the descent irreversible.
- Side pockets hold food, small shrines, and story props.
- The player always climbs upward. Level geometry and camera reinforce rising.

### Tile and movement model

- The cave is built on a 32x32 tile grid. Every tile is 32x32 pixels at the base resolution.
- Every entity sprite is also 32x32 pixels, so one entity matches one tile footprint when placed.
- Entities move freely in continuous space. Movement is not grid-locked, not turn-based, and not restricted to tile centers or cardinal directions.
- Walls block movement through tile collision. Entities never snap to the grid while moving.
- Facing is free and continuous, supporting movement in any direction.
- The editor snaps placement to the tile grid. Runtime movement does not snap.
- Collision bodies are smaller than the sprite footprint, tuned so Balin and enemies do not snag on tile corners.
- The camera is free, not tile-locked. It follows smoothly.

## 7. HUD and screens

- **HUD only.** HP bar, faith bar with the lost maximum shown as a dark notch, and a Smite cooldown icon.
- The light radius itself is the strongest faith readout in the world.
- The bag screen pauses the game and shows the 4x4 grid.
- No title screen. The run starts at the opening shrine.
- No pause menu and no settings menu in the MVP.
- No text anywhere except the ending overlay. Proposed line: "You escaped."
- Accessibility defaults: enemy tells use shape and motion, not color alone. Smite and burn effects avoid rapid flashing. Strong contrast between flame light and cave dark.

## 8. Art direction

- 32x32 pixel art on the shared tile grid from section 6. Gothic style, generated through the existing AI tileset pipeline and curated by hand.
- **Palette:** dark desaturated stone and earth, warm flame accents for faith, shrines, and Smite, sickly pale accents for the undead.
- **Symbol language:** faith is flame. Shrines are braziers, the glow is fire, Smite ignites enemies. Dark idols and snuffed flames represent lost faith.
- **Acceptance rules for generated assets:**
  - One shared palette across all tiles and sprites.
  - Consistent top-down light direction. No baked shadows that fight the dynamic light.
  - Tiles must seam cleanly in engine. Check before accepting.
  - Silhouette first. Every enemy must read as a black shape at small size.
  - Manual cleanup pass for stray pixels and style drift.
- **Story props:** collapsed camp, broken cart, dead miners, dark idols, charred braziers, cave paintings of the flame deity, mushrooms, bones.
- Base render resolution 640x360, integer scaled to the window. Default window 1280x720.
- The ending overlay uses a simple pixel font. It is the only text in the game.

## 9. Audio direction

- One ambient drone that darkens as faith falls. Warmer and brighter with high faith, hollow and oppressive at 10.
- Sparse SFX over the drone: footsteps, axe swing and hit, dash, Smite ignition, burn loop, shrine activation, gate pass toll, crawler skitter, miner attack and moan, brute stomp, pickup, eat, death, and the surface reveal.
- No composed soundtrack.
- SFX must be clearly audible over the drone. Enemies need distinct sound tells, since text cannot teach.

## 10. Controls

| Action | Key |
|---|---|
| Move | WASD or arrow keys |
| Axe swing | J |
| Dash | Space |
| Divine Smite | K |
| Open bag | TAB, pauses the game |
| Interact or confirm | E or Enter |
| Rotate item in bag | R or right click |
| Move item in bag | Mouse drag |
| Eat item in bag | Right click |

Editor controls live in section 12.

## 11. Platform and performance

- Native desktop. Development on macOS.
- 120+ FPS on capable hardware, with a 60 FPS floor on low-end and integrated graphics. No frame spikes.
- Startup under 2 seconds. Restart after death under 1 second.
- Input latency low enough that dash i-frames feel reliable.
- Fullscreen support later. Default window is 1280x720.

## 12. Editor (dev tool)

A dev-only level editor, hidden in player builds.

- **Terrain mode:** paint floors, walls, cave edges, and props on layers.
- **Entity mode:** place enemy spawns, food, small shrines, the player spawn, and the opening and final shrines.
- **Gate mode:** place gate markers and assign each gate its faith cap cut.
- **Camera:** free camera, mouse-driven tools, grid snap.
- **Persistence:** save and load level files.
- **Playtest:** hot-swap from editing into play with one key, and back.
- **Access:** toggled with F1 in dev builds. Hidden in release builds with a flag.
- **Out of scope for the MVP:** undo and redo, copy and paste, per-room playtesting. Revisit only if authoring becomes painful.

## 13. Content budget

- 4 sections, about 5 minutes each.
- 15 to 18 regular encounters total, plus the gate brute.
- 2 to 3 food pockets per section. Food is placed, not dropped.
- 1 to 2 small shrines per section, plus the opening and final shrines.
- 3 enemy types with idle, move, attack, hurt, and death states. The brute can reuse states with different timing.
- Balin states: idle, walk, attack, dash, hurt, death.
- 4 item icons, a bag grid, 4 story prop sets, one gothic cave tileset.
- VFX: Smite ignition, burn, hit spark, shrine flame, gate drain, dust.
- About 12 SFX plus the faith drone.

## 14. Success criteria

1. At least 3 of 5 first-time testers finish a run within about 30 minutes.
2. Testers describe the faith mechanic in their own words after playing, such as "I got weaker as I climbed." The core idea must land without text.
3. The build meets the performance bar: 120+ FPS on capable hardware, 60 FPS floor on low-end, no crashes.

## 15. Non-goals

- No ranged weapons or throwables.
- No dialogue, NPCs, or cutscenes.
- No saves, meta-progression, or unlocks between runs.
- No gamepad support and no key rebinding. Mouse exists only in the bag.
- No full soundtrack.
- No additional enemy types beyond the three.
- No procedural level generation. All sections are hand-authored in the editor.
- No crafting, shops, or currency. The bag only carries food.
- No difficulty settings, achievements, online features, or localization.
- No editor in player builds.

## 16. Milestones

Five relative milestones, no dates. Order is fixed.

| # | Milestone | Done means |
|---|---|---|
| M1 | Core loop | One test room. Balin moves, swings, dashes. Crawlers fight back. HP and faith bars work. Death restarts. |
| M2 | Full combat kit | Divine Smite with faith cost and cooldown. 4x4 bag with mouse control, food items, eating, and a small shrine. |
| M3 | Editor | Terrain, entity, and gate placement. Save, load, and hot-swap playtest. Levels can be authored. |
| M4 | Vertical slice | Sections 0 and 1 fully built and paced, including the first gate and its faith cut. Placeholder art allowed. |
| M5 | Demo complete | All four sections, all three enemies, the final shrine and brute, the ending overlay, art and audio pass, performance verified, playtests run. |

## 17. Risks and open questions

**Risks**

1. **The finale hinges on one scripted Smite.** If the player misses or wastes it, the finale can become unwinnable. Mitigation: the final shrine sits after the last regular enemy, within sight of the brute, and the brute is the only valid target.
2. **Food only in side pockets can confuse or starve players.** Mitigation: every section guarantees one reachable glowing food pocket, and the main path stays survivable without food. Verify in every playtest.
3. **The grid bag is a large system.** Mitigation: keep it to food only, and never let it delay the core loop.
4. **The editor can become a project of its own.** Mitigation: no undo or redo in the MVP.
5. **AI tileset drift.** Palette and seam consistency can break between generation batches. Mitigation: shared palette, in-engine seam checks, and a manual cleanup pass.
6. **The faith cut must teach itself without text.** Mitigation: the HUD notch, the shrinking light, and the drone shift all communicate it. Test whether players notice without prompting.
7. **Name check.** Balin is also a dwarf in Tolkien's work. Confirm the name is acceptable before any commercial release.
8. **Scope.** Editor, grid inventory, and four authored sections together are ambitious. The milestone order keeps the game playable from M1 onward.

**Open questions**

- Is a Windows build required at M5, or is macOS enough for now?
- Is "You escaped." the final line, or should the wording change?
- Are the 70, 40, and 10 faith caps the right descent, or should the cuts be steeper or gentler?
- Does the final shrine grant a fixed Smite charge, or temporarily raise the cap? Product intent is "exactly one Smite is available." Engineering and feel will decide the cleanest expression.
- Should the bag auto-consume a food item on pickup when HP is full? Current answer: no, it prompts a swap instead.

## 18. Appendix: first pass tuning values

All values are starting points for playtests, not final numbers.

### Faith

| Value | First pass | Notes |
|---|---|---|
| Starting maximum | 100 | Set at the opening shrine |
| Maximum after gate 1 | 70 | Section 2 |
| Maximum after gate 2 | 40 | Section 3 |
| Maximum after gate 3 | 10 | Section 4 finale |
| Regeneration | 4 per second | Starts 1.5 seconds after the last hit taken |
| Faith lost per hit | 6 | Scale per enemy |
| Smite cost | 25 | Spent on cast |
| Smite cooldown | 6 seconds | Shown on the HUD |
| Smite burn | 8 damage per second for 5 seconds | Applies once, no stacking |
| Axe damage | 20 at full faith, 10 at 10 faith | Tune for a fair finale |
| Light radius | 200 px at 100 faith, 48 px at 10 faith | At the 640x360 base resolution |

### HP and food

| Value | First pass |
|---|---|
| Maximum HP | 100 |
| Crawler hit | 8 |
| Fallen miner hit | 20 |
| Gate brute hit | 30 |
| Apple (1x1) | +15 HP |
| Small mushroom (1x2) | +25 HP |
| Big mushroom (2x2) | +40 HP |
| Ration (1x3) | +60 HP |
| Bag size | 4x4 cells |

## 19. Decision log

- 2026-09-11: Concept locked. Top-down 2D action, Balin climbs out of a cave.
- 2026-09-11: Tone locked. Grim survival, no comedy.
- 2026-09-11: Core mechanic locked. Faith scales power and light, hits drain it, gates cut its maximum permanently.
- 2026-09-11: Survival model locked. Separate HP, food-only healing, food in side pockets, shrines refill faith only.
- 2026-09-11: Combat kit locked. Move, axe, dash with i-frames, Divine Smite burn with faith cost and cooldown.
- 2026-09-11: Roster locked. Crawlers, fallen miners, one gate brute.
- 2026-09-11: Structure locked. Four sections, about 20 minutes, restart the run on death, one final Smite for the finale.
- 2026-09-11: Scope additions locked. 4x4 grid bag in scope, dev-only level editor in scope.
- 2026-09-11: Presentation locked. 32x32 gothic AI-generated pixel art, flame as the faith symbol, ambient drone plus SFX, HUD only, no text until the ending.
- 2026-09-11: Success criteria locked. 3 of 5 testers finish, the faith mechanic lands unprompted, performance bar met.
- 2026-09-11: Grid model locked. The world is a 32x32 tile grid, entity sprites are 32x32, and entities move freely in continuous space. Only editor placement snaps to the grid.
