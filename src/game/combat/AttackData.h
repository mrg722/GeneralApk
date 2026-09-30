#pragma once
#include "game/Types.h"

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
