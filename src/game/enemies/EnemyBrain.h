#pragma once
#include "game/Types.h"
#include "game/moves/MoveCurves.h"   // LimbArc, Ease
#include <vector>

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

// ---------------------------------------------------------------------------
// F1.9: ataques de datos por tipo (EnemyAttackDef) y presupuesto de juggle.
// Cada tipo tiene 2-3 ataques que reutilizan los cuadros 4-7 del atlas con otra
// velocidad, root motion (lunge) y aplastado vertical (sy). Tiempos en segundos.
// El arco (V5) esta en px "nominales": se escala con EnemyProfile::visualScale
// (B06e: antes la caja era fija en y-66 para todos los tamanos).
// ---------------------------------------------------------------------------
enum class EnemyMoveKind { Strike, Roar };

struct EnemyAttackDef {
    const char* name = "";
    EnemyMoveKind kind = EnemyMoveKind::Strike;
    float startup = 0.2f, active = 0.15f, recovery = 0.15f;
    LimbArc arc{};
    float damageScale = 1.0f;      // x EnemyProfile::attackDamage
    float knockback = 120.0f;      // px/s sobre el jugador
    float launch = 0.0f;           // px/s (el jugador lo trata como derribo)
    float hitstun = 0.13f;
    bool low = false, overhead = false, knockdown = false, unblockable = false, heavy = false;
    bool armor = false;            // superarmadura durante arranque + activo
    float lunge = 0.0f;            // root motion hacia el frente en S+A (px, outCubic)
    float areaRadius = 0.0f;       // > 0: golpe de area a ras de suelo, a los dos lados
    int hits = 1;                  // golpes del ataque (re-golpe a mitad del activo)
    float sy = 1.0f;               // aplastado vertical del dibujo (patada baja .85)
    float minDist = 0.0f;          // distancia horizontal minima para elegirlo
    float maxDist = 99999.0f;      // y maxima (ademas del rango del perfil)
    float weight = 1.0f;           // peso de la eleccion
    bool vsGuard = false;          // preferido (x3) si el jugador esta en guardia
    bool afterBlock = false;       // solo como contra al salir de la guardia
    bool counterSpecial = false;   // contra en guardia (EnemySpecial::GuardCounter)
    float Total() const { return startup + active + recovery; }
};

struct EnemyMoveSet {
    std::vector<EnemyAttackDef> attacks;
    int juggleBudget = 6;          // V8: 6 (Brute/Mutant/Armored 4)
};

const EnemyMoveSet& GetEnemyMoveSet(StreetEnemyType type);
// Escala del arco de un tipo (visualScale del perfil).
float EnemyAttackScale(StreetEnemyType type);
// Alcance horizontal del ataque (punta del arco al final del activo, ya escalado).
float EnemyAttackReach(const EnemyAttackDef& a, float scale);

}  // namespace district_fury
