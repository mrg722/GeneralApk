// Poses sueltas de TITAN-X (skin 4, assets/bosses/titanx) y TITAN-X MEJORADO
// (skin 5, assets/bosses/titanx_mejorado). Movido tal cual desde CharacterPose
// (F0.1). Lo edita F1.1 (orientacion por pose y movimientos nuevos).
#include "game/visual/PoseTables.h"
#include <cstring>

namespace district_fury {

const char* PoseTitanX(PlayerState state, AttackType attack, bool rage, double time) {
    const int idleCycle = static_cast<int>(time * (state == PlayerState::Walk ? 9.0 : 3.2)) % 4;
    if (state == PlayerState::Defeat) return "death";
    if (state == PlayerState::Hit || state == PlayerState::Knockdown || state == PlayerState::GuardBreak) return "recoil";
    if (state == PlayerState::Dash) return "lean";
    if (state == PlayerState::Attack) {
        if (attack == AttackType::Energy) return "shoot_orb";
        if (attack == AttackType::Kick) return "uppercut";
        return rage ? "slam" : "punch1";
    }
    return idleCycle == 0 ? "idle1" : idleCycle == 1 ? "idle2" : idleCycle == 2 ? "idle3" : "idle4";
}

const char* PoseTitanXM(PlayerState state, AttackType attack, bool rage, double time) {
    const int idleCycle = static_cast<int>(time * (state == PlayerState::Walk ? 9.0 : 3.2)) % 4;
    if (state == PlayerState::Defeat) return "death";
    if (state == PlayerState::Hit || state == PlayerState::Knockdown || state == PlayerState::GuardBreak) return "cannon_aim";
    if (state == PlayerState::Dash) return "dash";
    if (state == PlayerState::Attack) {
        if (attack == AttackType::Energy) return "shoot_orb";
        if (rage) return "rage_aura";
        return attack == AttackType::Kick ? "slam" : "charge_orb";
    }
    return idleCycle == 0 ? "idle1" : idleCycle == 1 ? "idle2" : idleCycle == 2 ? "idle3" : "idle4";
}

int PoseFacesRightTitanX(const char* folder, const char* pose) {
    (void)pose;
    if (folder == nullptr) return -1;
    if (std::strcmp(folder, "titanx") == 0) return 0;            // ficha: facesRightByDefault = false
    if (std::strcmp(folder, "titanx_mejorado") == 0) return 0;   // ficha: facesRightByDefault = false
    return -1;
}

}  // namespace district_fury
