// Contratos de F0.1 (plan maestro de movimientos): cola de eventos, registro de
// movesets, ids historicos y la geometria basica. Sin ventana.
#include "game/events/GameEvents.h"
#include "game/combat/HitTypes.h"
#include "game/combat/CombatMath.h"
#include "game/moves/MoveRegistry.h"
#include "game/visual/PoseTables.h"
#include "game/beta/BetaDerive.h"
#include "game/CharacterVisual.h"
#include "game/Player.h"
#include "audio/AudioSystem.h"
#include "audio/AudioDirector.h"
#include "audio/MusicPlayer.h"
#include <cassert>
#include <cstdio>
#include <cstring>

using namespace district_fury;

static void TestEventQueue() {
    ClearGameEvents();
    for (int i = 0; i < 300; ++i) {
        GameEvent e;
        e.type = GameEventType::Footstep;
        e.value = i;
        EmitGameEvent(e);
    }
    assert(PendingGameEvents() == 256);
    int n = 0, first = -1, last = -1;
    DrainGameEvents([&](const GameEvent& e) {
        if (n == 0) first = e.value;
        last = e.value;
        ++n;
    });
    assert(n == 256);
    assert(DroppedGameEvents() == 44);
    assert(first == 44 && last == 299);   // se descartan los mas viejos, en orden
    assert(PendingGameEvents() == 0);
    int again = 0;
    DrainGameEvents([&](const GameEvent&) { ++again; });
    assert(again == 0);
    ClearGameEvents();
    assert(DroppedGameEvents() == 0);
}

static void TestMoveRegistry() {
    const MoveSet& generic = GenericMoveSet();
    for (int skin = 0; skin <= 41; ++skin) {
        const MoveSet& ms = GetMoveSet(skin);
        if (skin == 0) {   // F1.3: RAYDEN (ORIGINAL) declara su MoveSet (sin overrides historicos)
            assert(&ms != &generic && FindOverride(ms, static_cast<int>(AttackId::Punch1)) == nullptr);
            continue;
        }
        assert(&ms == &generic);
        assert(std::strcmp(ms.name, "generico") == 0);
        assert(ms.caps == CapAll && ms.feel == FeelLevel::Full && ms.overrides == nullptr);
        assert(FindOverride(ms, static_cast<int>(AttackId::Punch1)) == nullptr);
    }
    assert(GetMoveSet(-1).name == generic.name && GetMoveSet(999).name == generic.name);
    // FindOverride encuentra por id.
    static const MoveOverride ov[2] = {[] { MoveOverride o; o.id = 3; return o; }(),
                                       [] { MoveOverride o; o.id = 4; o.damageScale = 2; return o; }()};
    MoveSet test;
    test.overrides = ov;
    test.overrideCount = 2;
    assert(FindOverride(test, 4) == &ov[1]);
    assert(FindOverride(test, 5) == nullptr);
}

static void TestHistoricIds() {
    // F0.4 agrego los 31 ids de V1 al final (Count = 41); los historicos 0-9 no se mueven.
    static_assert(static_cast<int>(AttackId::Count) == 41, "AttackId::Count = 41 tras F0.4");
    assert(static_cast<int>(AttackId::Count) == 41);
    assert(static_cast<int>(AttackId::Skill) == 9);
    assert(static_cast<int>(AttackId::PunchHook) == 10 && static_cast<int>(AttackId::Chain6) == 40);
    // Estados nuevos al final, sin mover los historicos.
    assert(static_cast<int>(PlayerState::Airborne) == 10);
    assert(static_cast<int>(PlayerState::Jump) == 11 && static_cast<int>(PlayerState::Victory) == 12);
    PlayerInput in;
    assert(!in.jump);
}

static void TestPoseTables() {
    // Mismas poses que antes de partir CharacterPose (muestras).
    assert(std::strcmp(CharacterPose(1, PlayerState::Defeat, AttackType::None, false, 0.0), "death") == 0);
    assert(std::strcmp(CharacterPose(4, PlayerState::Airborne, AttackType::None, false, 0.0), "recoil") == 0);
    assert(std::strcmp(CharacterPose(5, PlayerState::Attack, AttackType::Energy, false, 0.0), "shoot_orb") == 0);
    assert(std::strcmp(CharacterPose(2, PlayerState::Walk, AttackType::None, false, 0.0), "walk") == 0);
    assert(std::strcmp(CharacterPose(3, PlayerState::Attack, AttackType::Kick, false, 0.0), "slam") == 0);
    assert(std::strcmp(CharacterPose(0, PlayerState::Idle, AttackType::None, false, 0.0), "idle1") == 0);
    // PoseFacesRight = facesRightByDefault de la ficha (hoy).
    for (int id = 0; id < CharacterCount(); ++id) {
        const CharacterVisual& cv = GetCharacterVisual(id);
        if (cv.folder == nullptr) continue;
        assert(PoseFacesRight(cv.folder, "idle1") == cv.facesRightByDefault);
    }
}

static void TestCombatMath() {
    const Rect2 r{0, 0, 10, 10};
    assert(CapsuleRect({{5, 5}, {6, 6}, 1}, r));            // dentro
    assert(CapsuleRect({{-5, 5}, {15, 5}, 0.5f}, r));       // lo cruza de lado a lado
    assert(CapsuleRect({{-2, -2}, {-2, -2}, 3}, r));        // esquina por radio
    assert(!CapsuleRect({{20, 20}, {30, 30}, 2}, r));       // fuera
    assert(DistPointSegment2({0, 5}, {-5, 0}, {5, 0}) == 5.0f);
}

static void TestDefaults() {
    const HitInfo h;
    assert(h.damage == 0 && h.juggleCost == 1 && h.attackerSkin == -1);
    const StageHitStyle s;
    assert(s.scorePerDamage == 10 && s.rageBonus == 0);
    assert(AudioSystem::Get().BusVolume(Bus::Music) == 1.0f);
    AudioDirectorUpdate(0.016f);
    MusicPlayer::Get().Update(0.016f);
    assert(BetaDeriveEnabled());
    SetBetaDeriveEnabled(false);
    assert(!BetaDeriveEnabled());
    SetBetaDeriveEnabled(true);
}

int main() {
    TestEventQueue();
    TestMoveRegistry();
    TestHistoricIds();
    TestPoseTables();
    TestCombatMath();
    TestDefaults();
    std::printf("ContractsTests OK\n");
    return 0;
}
