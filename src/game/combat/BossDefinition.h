#pragma once
#include "game/Types.h"
#include <vector>

// DF-013.2 — Boss Definition (fase 1 de la unificacion Boss/BossDefinition).
//
// Contexto (ver ARCHITECTURE.md, seccion "Decision de arquitectura:
// CombatEntity / EnemyProfile / BossDefinition"): el usuario pidio
// explicitamente reemplazar los 5 structs de boss duplicados (uno por
// stage: StoryBoss en Stage1, Grinder en Stage2, el boss de Stage3,
// TitanXImproved en Stage4, RayderClone en Stage5 — cada uno con su
// propio UpdateBoss/DrawBoss/HandleBossHits copiado a mano) por UNA sola
// clase Boss que interprete datos, igual que EnemyBrain::EnemyProfile ya
// hace para StreetEnemy.
//
// Este archivo es solo el primer paso, deliberadamente de bajo riesgo:
// define las estructuras de datos y aun NO se consume desde ningun
// Stage*Game (los 5 stages siguen usando sus structs propios y su codigo
// verificado). La migracion real (clase Boss + reemplazo stage por stage)
// es la fase 2, descrita en ARCHITECTURE.md, y se hara de forma
// incremental para poder compilar y revisar cada stage por separado en
// vez de arriesgar los 5 a la vez sin poder probar el juego visualmente
// en este entorno (sin GPU/Wine).

namespace district_fury {

enum class BossId {
    Brakk,
    Grinder,
    TitanX,          // forma prototipo/humanoide, Stage 3
    TitanXMejorado,  // forma grande/verde, penultimo jefe, Stage 4
    RayderClone,     // jefe final, Stage 5
    Count
};

// Un ataque de boss, con la misma forma de datos que AttackDef
// (src/game/combat/AttackData.h) mantiene para el jugador, pero
// desacoplado de AttackId porque los ataques de boss no comparten
// semantica 1:1 con los del jugador (Charge, Shockwave, ProjectileBurst,
// Overdrive, DarkWave, MirrorCombo, Teleport... no son "puñetazos").
struct BossAttackDef {
    const char* name;

    int   damage;
    float startup;
    float active;
    float recovery;
    float range;
    float depth;

    float boxWidth;
    float boxHeight;
    float boxForward;

    float knockback;
    float hitstun;

    float cooldown;
    int   priority;

    bool  heavy;
    bool  spawnsProjectile;
    float projectileSpeed;

    float hitstop;
    float shake;

    // Umbral de fase (0..1 de HP) a partir del cual este ataque queda
    // disponible en la seleccion de IA. 0 = disponible desde el inicio.
    float minPhaseHpRatio;
};

// Ajustes que cambian al entrar en una fase (por umbral de HP).
struct BossPhaseDef {
    float hpRatioThreshold;   // fase activa cuando hp/maxHp <= este valor
    float speedMultiplier;
    float attackCooldownMultiplier;  // <1 = ataca mas seguido
    float damageMultiplier;   // escala el dano de los ataques en esta fase
    Color tint;               // tinte adicional sobre el color base (ira/overdrive)
};

struct BossDefinition {
    const char* displayName;
    const char* hudLabel;   // texto corto para la barra de HP del boss

    int   maxHp;
    float moveSpeed;

    float bodyWidth;
    float bodyHeight;
    float visualScale;
    Color baseColor;

    std::vector<BossPhaseDef>  phases;   // ordenadas de mayor a menor hpRatioThreshold
    std::vector<BossAttackDef> attacks;

    // Identidad canonica (GAME_DESIGN.md): Titan-X y Titan-X Mejorado son
    // el MISMO personaje organico-mecanico verde en dos escalas, nunca un
    // robot rojo. Se deja el campo explicito para que cualquier consumidor
    // futuro (IA de arte, tooling) no lo reinvente.
    bool isOrganicMechanical{false};
};

const BossDefinition& GetBossDefinition(BossId id);
const char* BossDisplayName(BossId id);

}  // namespace district_fury
