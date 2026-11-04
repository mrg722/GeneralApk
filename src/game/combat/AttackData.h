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
    // ---- F0.4 (V1 del plan maestro): 31 ids nuevos, en ESTE orden exacto ----
    PunchHook,      // 3er golpe de la cadena J por defecto (GANCHO)
    Kick2,          // K K (MEDIA LUNA)
    Kick3,          // K K K (TALON GIRATORIO)
    KickLauncher,   // J J K (PATADA ALTA, lanzador)
    FwdPunch,       // >J
    FwdKick,        // >K
    BackElbow,      // <J (codazo atras sin girar)
    SpinKick,       // <K (golpea a los dos lados)
    Sweep,          // vK (barrida baja)
    Uppercut,       // vJ (gancho ascendente, lanzador)
    Stomp,          // vJ sobre un enemigo en el suelo (OTG)
    DashKick,       // Shift + K (deslizamiento)
    AirPunch,       // J en el aire
    AirPunch2,      // J J en el aire
    AirKick,        // K en el aire
    DiveKick,       // vK en el aire (patada picada)
    AirSpike,       // >J en el aire (martillo)
    RisingUppercut, // DP + K (shoryu)
    Hurricane,      // QCB + K
    DrillKick,      // QCF + K
    Signature1,     // QCB + J   (solo con MoveOverride completo del personaje)
    Signature2,     // >L        (idem)
    Signature3,     // vL        (idem)
    Grab,           // Fase 2 (F2.1): deshabilitados hasta entonces
    GrabKnee,
    ThrowForward,
    ThrowBack,
    GuardCounter,
    WakeupAttack,
    Chain5,         // 5o y 6o eslabon de una cadena J larga (solo con override)
    Chain6,
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

    // ---- F0.4 (solo se agregan campos al final; defaults = comportamiento de
    // siempre en los 9 ataques historicos) ----
    // Desplazamiento real del cuerpo (root motion) durante el ataque. dx/dz = 0:
    // sin root motion de datos (los historicos conservan su paso de siempre).
    RootCurve root{0, 0, 0, 1};
    int   hitCount = 1;            // golpes por activacion (re-golpe cada multiHitInterval)
    float cancelFrom = -1.0f;      // s desde el inicio en que abre el cancel (-1 = regla general)
    unsigned cancelOnHit = 0;      // mascara de InputCommand que cancelan en hit (0 = tabla)
    unsigned cancelOnWhiff = 0;    // idem en fallo (0 = tabla)
    float blockstun = 0.20f;       // aturdimiento al bloquear (lo usa F2.1)
    float pushback = 0.0f;         // empuje extra a la victima en bloqueo (px/s, 0 = el de siempre)
    float selfPushOnBlock = 0.0f;  // retroceso propio al ser bloqueado (px/s)
    int   juggleCost = 1;          // coste del presupuesto de juggle (V8, F1.9)
    bool  otg = false;             // golpea a un enemigo en el suelo (Down)
    bool  launcher = false;        // levanta al enemigo (abre la persecucion)
    bool  jumpCancel = false;      // en hit: salto de persecucion en 10 f
    bool  groundBounce = false;
    bool  wallBounce = false;
    bool  knockdown = false;       // derriba (jugador vs jugador y Fase 1)
    bool  low = false;             // bajo (Fase 2: no lo para el bloqueo alto)
    bool  overhead = false;        // por arriba
    bool  hitsBehind = false;      // golpea a la espalda del atacante
    bool  air = false;             // solo en el aire (estado Jump)
    float armorFrom = -1.0f;       // ventana de superarmadura en s (-1 = sin)
    float armorTo = -1.0f;
    int   armorHits = 0;           // golpes que absorbe la armadura (0 = ilimitado en la ventana)
    float shakeScale = 1.0f;       // escala del temblor de camara
    bool  requiresOverride = false; // sin MoveOverride del personaje = deshabilitado
    float invulnTo = 0.0f;         // invulnerable de 0 a invulnTo s (RisingUppercut f1-5)
    float landRecovery = 0.0f;     // aereos: recuperacion al tocar suelo (s)
};

// Duracion total de la animacion/estado del ataque.
float AttackTotalDuration(const AttackDef& def);

const AttackDef& GetAttack(AttackId id);

// Cadena de combo de punch historica: Punch1 -> Punch2 -> Punch3 -> Punch1.
// (La cadena real por personaje es MoveSet.chainJ: ver NextChainLink en
// player/PlayerCommon.h. Esta se conserva para los KF con su clip.)
AttackId NextPunchInChain(AttackId current);

// Numero de ids validos (sin None).
constexpr int kAttackIdCount = static_cast<int>(AttackId::Count);
// Familias (F0.4): para la tabla de cancelacion, el sonido y el clip historico.
bool AttackIsKickFamily(AttackId id);      // patadas
bool AttackIsSpecialMove(AttackId id);     // EnergyWave, RisingUppercut, Hurricane, DrillKick, Signature1-3
bool AttackIsSuper(AttackId id);           // RageAttack, Finisher
bool AttackIsAir(AttackId id);             // def.air
bool AttackIsFase2(AttackId id);           // Grab..WakeupAttack
// Nombre interno estable (para herramientas/tests): "PunchHook", "Kick2"...
const char* AttackIdName(AttackId id);

// Compatibilidad con el enum historico AttackType de Player.
bool AttackIsEnergy(AttackId id);
bool AttackIsHeavy(AttackId id);

}  // namespace district_fury
