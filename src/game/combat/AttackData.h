#pragma once
#include "game/Types.h"
#include "game/moves/MoveCurves.h"   // LimbArc (F0.2)

// DF-013 FASE 2 — Attack Data.
// Los ataques dejan de ser ternarios repartidos por Player.cpp y pasan a ser
// datos. Agregar un ataque nuevo = agregar una fila en la tabla; ningun stage
// necesita cambiar.

namespace district_fury {

enum class AttackId {
    None = 0,
    Punch1,
    Punch2,
    Punch3,
    Kick,
    EnergyWave,
    DashAttack,
    RageAttack,
    Finisher,
    Skill,        // habilidad con espera de 15 s: duracion y golpes los marca su animacion
    Count
};

struct AttackDef {
    const char* name;

    int   damage;          // dano base
    float startup;         // segundos hasta que la hitbox activa
    float active;          // duracion de la hitbox activa
    float recovery;        // recuperacion posterior
    float range;           // alcance horizontal
    float depth;           // tolerancia en profundidad (carriles)

    float boxWidth;        // hitbox
    float boxHeight;
    float boxForward;      // desplazamiento frontal del centro
    float boxCenterOffsetY;

    float knockback;
    float hitstun;
    float launch;          // impulso vertical (knockdown/launch)

    int   spCost;
    int   rageGain;
    int   priority;        // gana el ataque de mayor prioridad en trade
    float cooldown;

    bool  canCombo;        // permite encadenar al siguiente
    bool  canCancel;       // cancelable en ventana activa
    bool  breaksGuard;
    bool  heavy;           // feedback pesado (particulas/hitstop/shake)
    bool  spawnsProjectile;

    float hitstop;
    float shake;
    float comboWindow;     // ventana para encadenar tras impactar

    // ---- F0.2 (solo se agregan campos al final; los valores por defecto
    // reproducen el comportamiento de siempre) ----
    // Arco del miembro (V5): forma de la caja "matematica". hasArc=false = se usa
    // la caja historica (boxWidth/boxHeight...) o la del cuadro (KF/BETA).
    LimbArc arc{};
    bool  hasArc = false;
    // Re-golpe del mismo objetivo dentro de una activacion: 0 = un solo golpe.
    float multiHitInterval = 0.0f;
    // Escala del hitstop de V7 (1 = formula tal cual).
    float hitstopScale = 1.0f;
};

// Duracion total de la animacion/estado del ataque.
float AttackTotalDuration(const AttackDef& def);

const AttackDef& GetAttack(AttackId id);

// Cadena de combo de punch: Punch1 -> Punch2 -> Punch3 -> Punch1.
AttackId NextPunchInChain(AttackId current);

// Compatibilidad con el enum historico AttackType de Player.
bool AttackIsEnergy(AttackId id);
bool AttackIsHeavy(AttackId id);

}  // namespace district_fury
