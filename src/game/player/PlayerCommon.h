// Constantes y helpers internos del jugador (compartidos por sus .cpp).
#pragma once
#include "game/Player.h"
#include "core/InputMap.h"
#include "rendering/AssetManager.h"
#include "rendering/BossSprite.h"
#include "game/CharacterVisual.h"
#include "game/lab/KfReference.h"
#include <string>
#include "audio/AudioSystem.h"
#include <algorithm>
#include <cmath>

namespace district_fury {
namespace {

constexpr float kRageStartDuration = 0.45f;
constexpr float kRageEndDuration = 0.55f;
constexpr float kRageMaxDuration = 9.0f;

inline AttackType LegacyType(AttackId id) {
    switch (id) {
        case AttackId::Punch1:
        case AttackId::Punch2:
        case AttackId::Punch3:     return AttackType::Punch;
        case AttackId::Kick:       return AttackType::Kick;
        case AttackId::EnergyWave: return AttackType::Energy;
        case AttackId::DashAttack: return AttackType::Dash;
        case AttackId::RageAttack: return AttackType::Rage;
        case AttackId::Finisher:   return AttackType::Finisher;
        default:                   return AttackType::None;
    }
}

constexpr float kRecoveryLight = 0.08f;   // ~5 frames
constexpr float kRecoveryHeavy = 0.14f;   // ~8 frames

inline AttackPhase PhaseFor(AttackId id) {
    switch (id) {
        case AttackId::Punch1: return AttackPhase::Punch1;
        case AttackId::Punch2: return AttackPhase::Punch2;
        case AttackId::Punch3: return AttackPhase::Punch3;
        case AttackId::Kick:   return AttackPhase::Kick;
        case AttackId::None:   return AttackPhase::None;
        default:               return AttackPhase::Special;
    }
}

// Cadena de cancelacion arcade: J/K sobre el golpe actual -> siguiente ataque.
// Devuelve AttackId::None si ese comando no puede cancelar el ataque actual.
inline AttackId ResolveCancel(AttackId current, InputCommand cmd) {
    // Special cancel (King Fighter): cualquier normal se corta en un especial.
    const bool normal = current == AttackId::Punch1 || current == AttackId::Punch2 ||
                        current == AttackId::Punch3 || current == AttackId::Kick;
    if (cmd == InputCommand::SpecialWave) return normal ? AttackId::EnergyWave : AttackId::None;
    if (cmd == InputCommand::SpecialRise) return normal && current != AttackId::Punch3 ? AttackId::Punch3 : AttackId::None;
    const bool punch = cmd == InputCommand::Punch;
    switch (current) {
        case AttackId::Punch1: return punch ? AttackId::Punch2 : AttackId::Kick;
        case AttackId::Punch2: return punch ? AttackId::Punch3 : AttackId::Kick;
        case AttackId::Punch3: return punch ? AttackId::None   : AttackId::Kick;
        case AttackId::Kick:   return punch ? AttackId::Punch1 : AttackId::None;
        default:               return AttackId::None;
    }
}

inline const char* ClipNameFor(AttackId id) {
    switch (id) {
        case AttackId::Punch1:     return "punch1";
        case AttackId::Punch2:     return "punch2";
        case AttackId::Punch3:     return "punch3";
        case AttackId::Kick:       return "kick";
        case AttackId::EnergyWave: return "energy";
        case AttackId::DashAttack: return "dash_attack";
        case AttackId::RageAttack: return "rage_attack";
        case AttackId::Finisher:   return "finisher";
        default:                   return "idle";
    }
}

struct ClipRange { int cleanStart; int cleanEnd; int legacyStart; int legacyEnd; float frameTime; };

inline ClipRange ClipFor(AttackId id) {
    switch (id) {
        case AttackId::Punch1:     return {8, 9, 5, 9, 0.150f};
        case AttackId::Punch2:     return {8, 9, 5, 9, 0.135f};
        case AttackId::Punch3:     return {10, 11, 10, 14, 0.150f};
        case AttackId::Kick:       return {10, 11, 10, 14, 0.160f};
        case AttackId::EnergyWave: return {12, 13, 5, 9, 0.130f};
        case AttackId::DashAttack: return {8, 9, 5, 9, 0.120f};
        case AttackId::RageAttack: return {10, 11, 10, 14, 0.140f};
        case AttackId::Finisher:   return {12, 13, 10, 14, 0.150f};
        default:                   return {0, 3, 0, 4, 0.120f};
    }
}

}  // namespace
}  // namespace district_fury
