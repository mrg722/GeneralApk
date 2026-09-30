#pragma once
#include "game/Types.h"

// DF-013 FASE 3 — IA diferenciada.
// Antes existia un unico cuerpo de Update para los 8 tipos de enemigo y la
// "diferenciacion" eran dos multiplicadores de velocidad. Aqui cada tipo
// declara un perfil de comportamiento y la FSM lo interpreta.

namespace district_fury {

enum class StreetEnemyType;  // definido en StreetEnemy.h

enum class EnemySpecial {
    None,
    Charge,          // se lanza en linea recta tras telegrafiar
    ChemicalCloud,   // deja una zona quimica peligrosa
    Flank,           // rodea y ataca desde el otro lado
    GuardCounter,    // bloquea y contraataca
    HeavySlam,       // golpe de area lento con gran knockback
    Frenzy           // rafaga de golpes encadenados
};

struct EnemyProfile {
    const char* displayName;

    int   hp;
    float moveSpeed;
    float attackDamage;
    float attackRange;
    float attackDepth;
    float attackDuration;

    float visualScale;
    Color fallbackTint;
    float bodyWidth;
    float bodyHeight;

    // --- comportamiento ---
    float aggression;          // 0..1: cuanto presiona en vez de reposicionar
    float guardChance;         // 0..1: probabilidad de bloquear ante amenaza
    float guardStrength;       // vida de la guardia antes del guard break
    float preferredDistance;   // distancia que intenta mantener
    float repositionSpeed;     // velocidad al reposicionar/retroceder
    float retreatHpRatio;      // por debajo de esto tiende a retroceder
    float reactionTime;        // latencia antes de decidir
    float attackCooldown;      // espera entre ataques
    float flankBias;           // 0..1: tendencia a rodear por profundidad
    bool  hasArmor;            // ignora hitstun de golpes ligeros
    int   armorThreshold;      // dano minimo que rompe el armor

    EnemySpecial special;
    float specialCooldown;
    float specialRange;
    int   specialDamage;

    // Frases cortas para el HUD de VS/debug.
    const char* tacticHint;
};

const EnemyProfile& GetEnemyProfile(StreetEnemyType type);
const char* EnemySpecialName(EnemySpecial special);

}  // namespace district_fury
