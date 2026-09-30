#include "game/CharacterVisual.h"
#include "game/Player.h"
#include <array>

namespace district_fury {
namespace {

// name, folder, uniformCanvas, scale, targetHeight, footInset,
// facesRightByDefault, maxHp, damageMultiplier, speedMultiplier
const std::array<CharacterVisual, 6> kCharacters{{
    {"RAYDEN (ORIGINAL)", nullptr,           false, 1.00f,   0.0f, 0.0f, true,  100, 1.00f, 1.00f},
    {"RAYDEN CLON",       "rayder_clone",    false, 1.00f, 118.0f, 0.0f, true,  110, 1.05f, 1.05f},
    {"BRAKK",             "brakk",           true,  175.0f/188.0f, 0.0f, 9.0f, true, 150, 1.25f, 0.85f},
    {"GRINDER",           "grinder",         true,  3.00f,   0.0f, 0.0f, true,  160, 1.30f, 0.80f},
    {"TITAN-X",           "titanx",          false, 1.00f, 170.0f, 0.0f, false, 140, 1.20f, 0.90f},
    {"TITAN-X MEJORADO",  "titanx_mejorado", false, 1.00f, 235.0f, 0.0f, false, 180, 1.40f, 0.75f},
}};

}  // namespace

int CharacterCount() { return static_cast<int>(kCharacters.size()); }

const CharacterVisual& GetCharacterVisual(int id) {
    if (id < 0 || id >= static_cast<int>(kCharacters.size())) return kCharacters[0];
    return kCharacters[static_cast<std::size_t>(id)];
}

const char* CharacterPose(int id, PlayerState state, AttackType attack, bool rage, double time) {
    const int idleCycle = static_cast<int>(time * (state == PlayerState::Walk ? 9.0 : 3.2)) % 4;
    switch (id) {
        case 1:  // Rayden clon
            if (state == PlayerState::Defeat) return "death";
            if (state == PlayerState::Hit || state == PlayerState::Knockdown || state == PlayerState::GuardBreak) return "hurt";
            if (state == PlayerState::Block) return "ready";
            if (state == PlayerState::Dash) return "dash";
            if (state == PlayerState::Attack)
                return attack == AttackType::Energy ? "release_orb" : attack == AttackType::Kick ? "kick" : "punch";
            return idleCycle == 0 ? "idle1" : idleCycle == 1 ? "idle2" : idleCycle == 2 ? "idle3" : "idle4";
        case 2:  // Brakk
            if (state == PlayerState::Defeat) return "death";
            if (state == PlayerState::Hit || state == PlayerState::Knockdown || state == PlayerState::GuardBreak) return "hurt";
            if (state == PlayerState::Dash) return "charge";
            if (state == PlayerState::Attack) {
                if (attack == AttackType::Energy) return "chain_throw";
                if (rage) return "fury";
                return attack == AttackType::Kick ? "heavy" : "basic";
            }
            if (state == PlayerState::Walk) return "walk";
            return "idle";
        case 3:  // Grinder
            if (state == PlayerState::Defeat) return "death";
            if (state == PlayerState::Hit || state == PlayerState::Knockdown || state == PlayerState::GuardBreak) return "hurt";
            if (state == PlayerState::Dash) return "ram";
            if (state == PlayerState::Attack) {
                if (attack == AttackType::Energy || rage) return "overdrive";
                return attack == AttackType::Kick ? "slam" : "saw";
            }
            return "idle";
        case 4:  // Titan-X
            if (state == PlayerState::Defeat) return "death";
            if (state == PlayerState::Hit || state == PlayerState::Knockdown || state == PlayerState::GuardBreak) return "recoil";
            if (state == PlayerState::Dash) return "lean";
            if (state == PlayerState::Attack) {
                if (attack == AttackType::Energy) return "shoot_orb";
                if (attack == AttackType::Kick) return "uppercut";
                return rage ? "slam" : "punch1";
            }
            return idleCycle == 0 ? "idle1" : idleCycle == 1 ? "idle2" : idleCycle == 2 ? "idle3" : "idle4";
        case 5:  // Titan-X Mejorado
            if (state == PlayerState::Defeat) return "death";
            if (state == PlayerState::Hit || state == PlayerState::Knockdown || state == PlayerState::GuardBreak) return "cannon_aim";
            if (state == PlayerState::Dash) return "dash";
            if (state == PlayerState::Attack) {
                if (attack == AttackType::Energy) return "shoot_orb";
                if (rage) return "rage_aura";
                return attack == AttackType::Kick ? "slam" : "charge_orb";
            }
            return idleCycle == 0 ? "idle1" : idleCycle == 1 ? "idle2" : idleCycle == 2 ? "idle3" : "idle4";
        default:
            return "idle1";
    }
}

}  // namespace district_fury
