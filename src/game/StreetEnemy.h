#pragma once
#include "game/Types.h"
#include "game/Player.h"
#include "game/enemies/EnemyBrain.h"
#include "rendering/Animator.h"
#include <string>

namespace district_fury {

class CombatWorld;

// DF-013: la FSM pasa de 5 a 11 estados (brief 6). Los valores historicos
// conservan su orden para no romper codigo que ya los consultaba.
enum class StreetEnemyState {
    Idle, Chase, Attack, Hit, Defeat,
    Patrol, Position, Block, Stun, Retreat, Special,
    Airborne   // lanzado por un remate (launch > 0); al caer pasa a Hit
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

private:
    void EnterState(StreetEnemyState next);
    void Decide(float dt, const Player& player);
    void RunSpecial(float dt, const Player& player, CombatWorld* world);
};

}  // namespace district_fury
