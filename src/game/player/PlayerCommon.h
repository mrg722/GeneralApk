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
#include "game/events/GameEvents.h"
#include "game/combat/CombatMath.h"
#include "game/moves/MoveRegistry.h"
#include <algorithm>
#include <cmath>

namespace district_fury {
namespace {

// Evento de juego originado por el jugador (el audio lo consume en AudioDirector).
inline void EmitPlayerEvent(const Player& p, GameEventType type, bool heavy = false, int value = 0,
                            AttackId attack = AttackId::None, int weight = 0) {
    GameEvent e;
    e.type = type;
    e.x = p.position.x;
    e.y = p.position.y;
    e.skin = p.skin;
    e.heavy = heavy;
    e.weight = weight > 0 ? weight : (heavy ? 3 : 1);
    e.value = value;
    e.attack = static_cast<int>(attack == AttackId::None ? p.currentAttack : attack);
    EmitGameEvent(e);
}

constexpr float kRageStartDuration = 0.45f;
constexpr float kRageEndDuration = 0.55f;
constexpr float kRageMaxDuration = 9.0f;

// Proyeccion al enum historico AttackType (los stages y las tablas de poses lo
// consultan): patadas -> Kick, punos -> Punch, proyectiles -> Energy (F0.4).
inline AttackType LegacyType(AttackId id) {
    switch (id) {
        case AttackId::EnergyWave: return AttackType::Energy;
        case AttackId::DashAttack: return AttackType::Dash;
        case AttackId::RageAttack: return AttackType::Rage;
        case AttackId::Finisher:   return AttackType::Finisher;
        case AttackId::None:
        case AttackId::Skill:
        case AttackId::Count:      return AttackType::None;
        default: break;
    }
    if (AttackIsKickFamily(id)) return AttackType::Kick;
    return AttackType::Punch;
}
// Con el ataque efectivo: un override con proyectil se proyecta como Energy.
inline AttackType LegacyType(AttackId id, const AttackDef& def) {
    if (def.spawnsProjectile && id != AttackId::DashAttack) return AttackType::Energy;
    return LegacyType(id);
}

constexpr float kRecoveryLight = 0.08f;   // ~5 frames
constexpr float kRecoveryHeavy = 0.14f;   // ~8 frames

// Sub-estado AttackPhase (F0.4): patadas -> Kick; punos normales -> Punch3 (la
// pose/clip "fuerte" de la familia); especiales, supers, dash y Fase 2 ->
// Special. Los 9 historicos no cambian.
inline AttackPhase PhaseFor(AttackId id) {
    switch (id) {
        case AttackId::Punch1: return AttackPhase::Punch1;
        case AttackId::Punch2: return AttackPhase::Punch2;
        case AttackId::Punch3: return AttackPhase::Punch3;
        case AttackId::Kick:   return AttackPhase::Kick;
        case AttackId::None:   return AttackPhase::None;
        case AttackId::EnergyWave: case AttackId::DashAttack: case AttackId::RageAttack: case AttackId::Finisher:
        case AttackId::Skill:  return AttackPhase::Special;
        default: break;
    }
    if (AttackIsSpecialMove(id) || AttackIsFase2(id)) return AttackPhase::Special;
    if (AttackIsKickFamily(id)) return AttackPhase::Kick;
    return AttackPhase::Punch3;
}

// Nombre canonico del clip de cada ataque (B18: ningun id 1..40 es "idle").
inline const char* ClipNameFor(AttackId id) {
    switch (id) {
        case AttackId::Punch1:         return "punch1";
        case AttackId::Punch2:         return "punch2";
        case AttackId::Punch3:         return "punch3";
        case AttackId::Kick:           return "kick";
        case AttackId::EnergyWave:     return "energy";
        case AttackId::DashAttack:     return "dash_attack";
        case AttackId::RageAttack:     return "rage_attack";
        case AttackId::Finisher:       return "finisher";
        case AttackId::Skill:          return "special";
        case AttackId::PunchHook:      return "punch_hook";
        case AttackId::Kick2:          return "kick2";
        case AttackId::Kick3:          return "kick3";
        case AttackId::KickLauncher:   return "kick_launcher";
        case AttackId::FwdPunch:       return "fwd_punch";
        case AttackId::FwdKick:        return "fwd_kick";
        case AttackId::BackElbow:      return "back_elbow";
        case AttackId::SpinKick:       return "spin_kick";
        case AttackId::Sweep:          return "sweep";
        case AttackId::Uppercut:       return "uppercut";
        case AttackId::Stomp:          return "stomp";
        case AttackId::DashKick:       return "dash_kick";
        case AttackId::AirPunch:       return "air_punch";
        case AttackId::AirPunch2:      return "air_punch2";
        case AttackId::AirKick:        return "air_kick";
        case AttackId::DiveKick:       return "dive_kick";
        case AttackId::AirSpike:       return "air_spike";
        case AttackId::RisingUppercut: return "rising_uppercut";
        case AttackId::Hurricane:      return "hurricane";
        case AttackId::DrillKick:      return "drill_kick";
        case AttackId::Signature1:     return "signature1";
        case AttackId::Signature2:     return "signature2";
        case AttackId::Signature3:     return "signature3";
        case AttackId::Grab:           return "grab";
        case AttackId::GrabKnee:       return "grab_knee";
        case AttackId::ThrowForward:   return "throw_forward";
        case AttackId::ThrowBack:      return "throw_back";
        case AttackId::GuardCounter:   return "guard_counter";
        case AttackId::WakeupAttack:   return "wakeup_attack";
        case AttackId::Chain5:         return "chain5";
        case AttackId::Chain6:         return "chain6";
        case AttackId::None:
        case AttackId::Count:          return "idle";
    }
    return "idle";
}

// Clips historicos de la familia (respaldo de B18, en orden) para cuando el
// personaje no tiene el clip canonico. nullptr termina la lista.
inline const char* const* HistoricClipsFor(AttackId id) {
    static const char* const kP1[] = {"punch1", "punch2", nullptr};
    static const char* const kP2[] = {"punch2", "punch1", nullptr};
    static const char* const kP3[] = {"punch3", "punch2", "punch1", nullptr};
    static const char* const kK[] = {"kick", "punch3", nullptr};
    static const char* const kE[] = {"energy", "special", "punch3", nullptr};
    static const char* const kD[] = {"dash_attack", "punch2", nullptr};
    static const char* const kDK[] = {"dash_attack", "kick", nullptr};
    static const char* const kR[] = {"rage_attack", "super", "special", "punch3", nullptr};
    static const char* const kF[] = {"finisher", "super", "special", "punch3", nullptr};
    static const char* const kSp[] = {"special", "energy", "punch3", nullptr};
    static const char* const kSpK[] = {"kick", "special", nullptr};
    static const char* const kUp[] = {"punch3", "special", "punch2", nullptr};
    switch (id) {
        case AttackId::Punch1: case AttackId::BackElbow: case AttackId::AirPunch: case AttackId::Grab:
        case AttackId::GrabKnee: case AttackId::GuardCounter:
            return kP1;
        case AttackId::Punch2: case AttackId::PunchHook: case AttackId::AirPunch2: case AttackId::Chain5:
            return kP2;
        case AttackId::Punch3: case AttackId::FwdPunch: case AttackId::AirSpike: case AttackId::ThrowForward:
        case AttackId::ThrowBack: case AttackId::Chain6:
            return kP3;
        case AttackId::Uppercut: case AttackId::RisingUppercut: return kUp;
        case AttackId::EnergyWave: return kE;
        case AttackId::DashAttack: return kD;
        case AttackId::DashKick: return kDK;
        case AttackId::RageAttack: return kR;
        case AttackId::Finisher: return kF;
        case AttackId::Signature1: case AttackId::Signature2: case AttackId::Signature3: case AttackId::Skill:
            return kSp;
        case AttackId::Hurricane: case AttackId::DrillKick: return kSpK;
        default: break;
    }
    return AttackIsKickFamily(id) ? kK : kP1;
}
// Ultimo recurso antes de no ofrecer el movimiento: cualquier clip de ataque real.
inline const char* const* AnyAttackClips() {
    static const char* const kAny[] = {"punch1", "punch2", "punch3", "kick", "special", "energy", "atk1", "atk2",
                                       nullptr};
    return kAny;
}

struct ClipRange { int cleanStart; int cleanEnd; int legacyStart; int legacyEnd; float frameTime; };

// Rango de indices historico (atlas sin clips con nombre).
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
        case AttackId::None: case AttackId::Count: return {0, 3, 0, 4, 0.120f};
        default: break;
    }
    // F0.4: familia (nunca el rango de reposo 0..3).
    if (AttackIsSpecialMove(id)) return {12, 13, 5, 9, 0.130f};
    if (AttackIsKickFamily(id)) return {10, 11, 10, 14, 0.150f};
    return {8, 9, 5, 9, 0.140f};
}

// ---- Cancelaciones (F0.4, tarea 4) ----
// Tabla generica; MoveSet.cancels se prueba antes (o en su lugar con
// replaceCancels). Ver CancelRule en moves/MoveTypes.h.
inline const CancelRule* DefaultCancelRules(int& count) {
    constexpr int P = static_cast<int>(InputCommand::Punch), K = static_cast<int>(InputCommand::Kick);
    static const CancelRule kRules[] = {
        // Especiales cancelan normales (en hit desde startup+1, en fallo desde recovery).
        {kCancelAnyNormal, kCmdAnySpecial, CancelDir::Any, true, true, kCancelRequested},
        // <J J: el 2o gira.
        {static_cast<int>(AttackId::BackElbow), P, CancelDir::Any, true, true, kCancelTurnChainJ2},
        // Desde el 1er golpe J, en hit: >J, vJ, vK.
        {kCancelChainJ1, P, CancelDir::Fwd, true, false, static_cast<int>(AttackId::FwdPunch)},
        {kCancelChainJ1, P, CancelDir::Down, true, false, kCancelRequested},
        {kCancelChainJ1, K, CancelDir::Down, true, false, static_cast<int>(AttackId::Sweep)},
        // J J K -> lanzador.
        {kCancelChainJ2, K, CancelDir::Any, true, true, static_cast<int>(AttackId::KickLauncher)},
        // Cadenas.
        {kCancelChainJ, P, CancelDir::Any, true, true, kCancelChainJ},
        {kCancelChainJ, K, CancelDir::Any, true, true, kCancelChainKStart},
        {kCancelChainK, K, CancelDir::Any, true, true, kCancelChainK},
        {kCancelChainK, P, CancelDir::Any, true, true, kCancelChainJStart},
        // Embestida -> MEDIA LUNA / gancho ascendente (en hit).
        {static_cast<int>(AttackId::DashAttack), K, CancelDir::Any, true, false, static_cast<int>(AttackId::Kick2)},
        {static_cast<int>(AttackId::DashAttack), P, CancelDir::Down, true, false, static_cast<int>(AttackId::Uppercut)},
        // Aire.
        {static_cast<int>(AttackId::AirPunch), P, CancelDir::Fwd, true, true, static_cast<int>(AttackId::AirSpike)},
        {static_cast<int>(AttackId::AirPunch), P, CancelDir::Any, true, true, static_cast<int>(AttackId::AirPunch2)},
        {static_cast<int>(AttackId::AirPunch), K, CancelDir::Down, true, true, static_cast<int>(AttackId::DiveKick)},
        {static_cast<int>(AttackId::AirPunch), K, CancelDir::Any, true, true, static_cast<int>(AttackId::AirKick)},
        {static_cast<int>(AttackId::AirPunch2), P, CancelDir::Any, true, true, static_cast<int>(AttackId::AirSpike)},
        {static_cast<int>(AttackId::AirPunch2), K, CancelDir::Down, true, true, static_cast<int>(AttackId::DiveKick)},
        {static_cast<int>(AttackId::AirPunch2), K, CancelDir::Any, true, true, static_cast<int>(AttackId::AirKick)},
    };
    count = static_cast<int>(sizeof(kRules) / sizeof(kRules[0]));
    return kRules;
}

// Cadena de cancelacion historica (KF/BETA con su clip, y envoltorio para tests):
// J/K sobre el golpe actual -> siguiente ataque, con el MoveSet GENERICO.
// Devuelve AttackId::None si ese comando no puede cancelar el ataque actual.
inline AttackId ResolveCancelLegacy(AttackId current, InputCommand cmd) {
    const bool normal = current == AttackId::Punch1 || current == AttackId::Punch2 ||
                        current == AttackId::Punch3 || current == AttackId::Kick;
    if (cmd == InputCommand::SpecialWave) return normal ? AttackId::EnergyWave : AttackId::None;
    if (cmd == InputCommand::SpecialRise) return normal && current != AttackId::Punch3 ? AttackId::Punch3 : AttackId::None;
    if (IsSpecialCommand(cmd)) return AttackId::None;
    const bool punch = cmd == InputCommand::Punch;
    switch (current) {
        case AttackId::Punch1: return punch ? AttackId::Punch2 : AttackId::Kick;
        case AttackId::Punch2: return punch ? AttackId::Punch3 : AttackId::Kick;
        case AttackId::Punch3: return punch ? AttackId::None   : AttackId::Kick;
        case AttackId::Kick:   return punch ? AttackId::Punch1 : AttackId::None;
        default:               return AttackId::None;
    }
}

// Envoltorio conservado (tarea 4): cancel de `current` con `cmd` sin direccion,
// cadena por defecto, como si el golpe hubiera conectado. La resolucion completa
// (direcciones, MoveSet, ventanas) la hace Player::ResolveCommand.
inline AttackId ResolveCancel(AttackId current, InputCommand cmd) {
    Player p;
    p.currentAttack = current;
    p.state = PlayerState::Attack;
    const MoveSet& ms = GenericMoveSet();
    for (int i = 0; i < ms.chainJCount; ++i)
        if (ms.chainJ[i] == static_cast<int>(current)) { p.chainLinkJ = i; break; }
    for (int i = 0; i < ms.chainKCount; ++i)
        if (ms.chainK[i] == static_cast<int>(current)) { p.chainLinkK = i; break; }
    InputBuffer::Entry e;
    e.command = cmd;
    p.attackConnected = true;
    return p.ResolveCommand(e, true, nullptr);
}

}  // namespace
}  // namespace district_fury
