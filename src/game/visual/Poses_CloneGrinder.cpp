// Poses sueltas del CLON ANTIGUO (skin 1, assets/bosses/rayder_clone) y de
// GRINDER (skin 3, assets/bosses/grinder). Movido tal cual desde CharacterPose
// (F0.1). Lo edita F1.2.
#include "game/visual/PoseTables.h"
#include <cmath>
#include <cstring>

namespace district_fury {

const char* PoseClone(PlayerState state, AttackType attack, bool rage, double time) {
    (void)rage;
    const int idleCycle = static_cast<int>(time * (state == PlayerState::Walk ? 9.0 : 3.2)) % 4;
    // Rayden clon — usa SOLO poses existentes del atlas del clon.
    // La mejora es de secuenciacion/timing, no crea arte nuevo.
    if (state == PlayerState::Defeat) return "death";
    if (state == PlayerState::Hit || state == PlayerState::Knockdown || state == PlayerState::GuardBreak) return "hurt";
    if (state == PlayerState::Block) return "ready";
    if (state == PlayerState::Dash) return "dash";
    if (state == PlayerState::Attack) {
        const float phase = std::fmod(static_cast<float>(time), 0.56f);
        if (attack == AttackType::Energy) {
            return phase < 0.16f ? "ready" : phase < 0.40f ? "release_orb" : "idle2";
        }
        if (attack == AttackType::Kick) {
            return phase < 0.10f ? "ready" : phase < 0.32f ? "kick" : "idle3";
        }
        return phase < 0.09f ? "ready" : phase < 0.28f ? "punch" : phase < 0.44f ? "idle2" : "idle1";
    }
    if (state == PlayerState::Walk) {
        const int walkCycle = static_cast<int>(time * 9.0) % 4;
        return walkCycle == 0 ? "idle1" : walkCycle == 1 ? "idle2" : walkCycle == 2 ? "idle3" : "idle4";
    }
    return idleCycle == 0 ? "idle1" : idleCycle == 1 ? "idle2" : idleCycle == 2 ? "idle3" : "idle4";
}

const char* PoseGrinder(PlayerState state, AttackType attack, bool rage, double time) {
    (void)time;
    if (state == PlayerState::Defeat) return "death";
    if (state == PlayerState::Hit || state == PlayerState::Knockdown || state == PlayerState::GuardBreak) return "hurt";
    if (state == PlayerState::Dash) return "ram";
    if (state == PlayerState::Attack) {
        if (attack == AttackType::Energy || rage) return "overdrive";
        return attack == AttackType::Kick ? "slam" : "saw";
    }
    return "idle";
}

int PoseFacesRightCloneGrinder(const char* folder, const char* pose) {
    (void)pose;
    if (folder == nullptr) return -1;
    if (std::strcmp(folder, "rayder_clone") == 0) return 1;   // ficha: facesRightByDefault = true
    if (std::strcmp(folder, "grinder") == 0) return 1;        // ficha: facesRightByDefault = true
    return -1;
}

}  // namespace district_fury
