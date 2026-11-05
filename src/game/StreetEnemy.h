#pragma once
#include "game/Types.h"
#include "game/Player.h"
#include "game/enemies/EnemyBrain.h"
#include "game/combat/HitTypes.h"
#include "game/combat/CombatMath.h"
#include "rendering/Animator.h"
#include <cstdint>
#include <string>
#include <vector>

namespace district_fury {

class CombatWorld;

// DF-013: la FSM pasa de 5 a 11 estados (brief 6). Los valores historicos
// conservan su orden para no romper codigo que ya los consultaba.
enum class StreetEnemyState {
    Idle, Chase, Attack, Hit, Defeat,
    Patrol, Position, Block, Stun, Retreat, Special,
    Launched,    // F1.9: en el aire (lanzador / juggle, V8); al caer -> Down
    Down,        // en el suelo 0.55 s: solo lo golpea un ataque otg (una vez por derribo)
    WakeUp,      // levantandose 0.35 s, invulnerable
    WallBounce,  // rebote contra el borde de la arena (0.25 s, sigue en el aire)
    Grabbed,     // agarrado (F2.1): anclado a la mano del agarrador
    Thrown,      // lanzado por un agarre: proyectil-cuerpo que golpea a otros
    Airborne = Launched   // nombre historico (antes: lanzado por un remate)
};

enum class StreetEnemyType { Punk, Brute, Charger, Enforcer, ChemicalSoldier, UrbanNinja, Mutant, ArmoredGuard };

class StreetEnemy {
public:
    Vector3D position;
    Vector3D velocity;
    Facing facing;
    StreetEnemyState state;
    StreetEnemyType type;
    bool active;

    int hp;
    int maxHp;
    float moveSpeed;
    float attackDamage;
    float attackRange;
    float attackDepth;
    float attackDuration;
    float stateTimer;
    float attackElapsed;
    float attackCooldown;
    bool hasHit;
    Animator animator;

    // Laboratorio: visual externo (bot de referencia). Solo cambia el dibujo;
    // IA, hitboxes y dano siguen siendo los del tipo de enemigo.
    bool referenceSkin{false};
    Animator skinAnimator;
    std::string skinClip;
    float skinScale{1.8f};
    int skinAttackVariant{0};
    StreetEnemyState skinState{StreetEnemyState::Defeat};
    const char* skinLabel{nullptr};
    void UseReferenceSkin(const Animator& templ, float scale, const char* label);
    // Prepara un animador con el atlas medido de un tipo de enemigo (lo usa el
    // jefe de Stage 1 como cuerpo provisional). Devuelve false si no hay textura.
    static bool PrepareAtlasAnimator(Animator& animator, StreetEnemyType type);

    // --- DF-013: estado de IA ---
    float decisionTimer;
    float specialTimer;
    float guardHealth;
    float maxGuardHealth;
    float hitstunTimer;
    float telegraphTimer;
    float flankSide;
    int   comboHits;
    bool  telegraphing;
    EnemySpecial pendingSpecial;

    StreetEnemy();
    void Init(Vector3D startPos, StreetEnemyType enemyType);
    void Activate();

    // world es opcional: permite a los specials generar hazards/proyectiles.
    void Update(float dt, const Player& player, CombatWorld* world = nullptr);
    void Draw() const;
    void TakeDamage(int damage, Vector3D knockback);
    // F0.2: recepcion de un golpe con toda su informacion (direccion, hitstun
    // de la tabla, launch, guardia, rompe-guardia). TakeDamage es el envoltorio
    // historico (stun fijo 0.30 s, sin guardia).
    HitResult ApplyHit(const HitInfo& info);
    // Arranque de su ataque (telegrafia o antes del fin del startup de su
    // EnemyAttackDef): counter-hit.
    bool InAttackStartup() const;
    bool InStartup() const { return InAttackStartup(); }
    // Prioridad de su ataque en un trade (B16): especial 3, con armadura 2, resto 1.
    int AttackPriority() const;
    // Temblor de victima (V7) que aplica Draw.
    float hurtShakeT{1.0f};
    float hurtShakeAmp{0.0f};

    bool AttackIsActive() const;
    bool IsDefeated() const;
    bool IsGuarding() const;
    bool IsTelegraphing() const;
    void AbsorbGuard(int damage);
    void BreakGuard();
    void ApplyHitstun(float duration);

    CombatBox GetHurtbox() const;
    CombatBox GetAttackHitbox() const;
    const char* GetTypeName() const;
    const char* GetStateName() const;
    const EnemyProfile& Profile() const;

    // ---- F1.9: ataques de datos ----
    int attackIndex{0};              // indice en GetEnemyMoveSet(type).attacks
    float attackPrevElapsed{0.0f};   // attackElapsed del frame anterior (barrido del arco)
    float lungeApplied{0.0f};        // root motion ya aplicado del ataque en curso
    int attackHitsDone{0};           // golpes ya habilitados del ataque (multi-golpe)
    float telegraphFlash{0.0f};      // destello rojo al empezar a telegrafiar (0.15 s)
    float footstepTimer{0.0f};
    const EnemyAttackDef& CurrentAttack() const;
    // Empieza el ataque de datos `index` (sin pasar por la IA). Lo usan la IA,
    // los tests y las herramientas.
    void StartAttack(int index);
    float AttackScale() const;       // escala del arco (visualScale)
    // Forma de su ataque activo contra una hurtbox del plano (X, altura): arco
    // del EnemyAttackDef (escalado), area a ras de suelo o la caja de los specials.
    bool AttackShapeHits(const Rect2& hurt, Vector2* contact) const;

    // ---- F1.9: juggle / suelo (V8) ----
    int juggleUsed{0};               // coste de juggle consumido desde el lanzamiento
    int airHits{0};                  // golpes recibidos en el aire desde el lanzamiento
    int hitCombo{0};                 // golpes recibidos seguidos (escalado de dano/hitstun)
    bool juggleExhausted{false};     // presupuesto agotado: cae sin relanzar
    bool otgUsed{false};             // ya recibio su golpe otg en este derribo
    bool pendingGroundBounce{false}, groundBounced{false};
    bool pendingWallBounce{false}, wallBounced{false};
    float arenaMinX{kStageStartX};   // bordes de la arena para el wallBounce
    float arenaMaxX{kStageEndX - 90.0f};
    int JuggleBudget() const;
    bool IsDown() const { return state == StreetEnemyState::Down; }
    bool InAir() const;

    // ---- F1.9: API de agarres (la usa F2.1) ----
    float grabEscapeTimer{0.0f};
    float throwTimer{0.0f}, throwDuration{0.6f};
    int throwDamage{8};
    int throwerSkin{-1};
    std::vector<std::uintptr_t> bodyHits;   // a quien golpeo ya como proyectil-cuerpo
    bool CanBeGrabbed() const;
    void SetGrabbed(bool on, Vector3D hand);
    void Throw(const HitInfo& info, Vector3D vel);
    // Cuerpo que vuela golpeando a otros (Thrown, o Launched con abs(vx) >= 420: rebote de pared).
    bool IsFlyingBody() const;

private:
    void EnterDown();
    void UpdateAirborne(float dt);
    void EnterState(StreetEnemyState next);
    void Decide(float dt, const Player& player);
    void RunSpecial(float dt, const Player& player, CombatWorld* world);
};

}  // namespace district_fury
