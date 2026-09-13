#include "game/systems/systems.h"

#include "engine/input/input.h"
#include "game/game.h"

// Empty stubs mark each ADR-017 slot. The owning ticket fills the body.
// The call order in GameTick is the contract.

static void SystemInput(void) {
    // 1. Input snapshot applied, buffers ticked. The player controller reads
    // the snapshot in BAL-20 and consumes the dash and attack buffers later.
    InputBeginTick();
}

static void SystemAI(void) {
    // 2. AI decisions: aggro, states, wind-ups, leashes. TODO(BAL-26)
}

static void SystemMovement(void) {
    // 3. Movement and dash, i-frame timers. TODO(BAL-20, BAL-23)
    GameProbeTick();
}

static void SystemTileCollision(void) {
    // 4. Tile collision resolution. TODO(BAL-19)
}

static void SystemSeparation(void) {
    // 5. Entity separation. TODO(BAL-27)
}

static void SystemMelee(void) {
    // 6. Melee hit resolution and damage queuing. TODO(BAL-22)
}

static void SystemBurn(void) {
    // 7. Burn and damage over time. TODO(BAL-25)
}

static void SystemFaith(void) {
    // 8. Faith regeneration and hit-faith drain. TODO(BAL-25)
}

static void SystemHealth(void) {
    // 9. Health, death, and destruction queue. TODO(BAL-21)
}

static void SystemCleanup(void) {
    // 10. Lifetime cleanup: despawn, corpses, pickups consumed. TODO(BAL-21)
}

static void SystemAudio(void) {
    // 11. Audio event flush. TODO(audio milestone)
}

static void SystemLightGather(void) {
    // 12. Light source gather and render prep, y-sort. TODO(BAL-18)
}

void GameTick(void) {
    SystemInput();          // 1
    SystemAI();             // 2
    SystemMovement();       // 3
    SystemTileCollision();  // 4
    SystemSeparation();     // 5
    SystemMelee();          // 6
    SystemBurn();           // 7
    SystemFaith();          // 8
    SystemHealth();         // 9
    SystemCleanup();        // 10
    SystemAudio();          // 11
    SystemLightGather();    // 12

    // Deferred destroys apply once the system list has run, per ADR-005.
    // Queries skip dead entities before this, so no system can touch them.
    EcsFlush(GameEcs());
}
