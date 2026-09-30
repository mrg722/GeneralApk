#pragma once
#include "game/Types.h"
#include "game/Player.h"
#include "rendering/Animator.h"

namespace district_fury {

enum class EnemyState {
    Idle,
    Chase,
    Attack,
    Hit,
    Defeat
};

class Enemy {
public:
    Vector3D position;
    Vector3D velocity;
    Facing facing;
    EnemyState state;

    int hp;
    int maxHp;
    float stateTimer;
    float attackElapsed;
    bool hasHit;
    Animator animator;

    Enemy();

    void Init(Vector3D startPos);
    void Update(float dt, const Player& player);
    void Draw() const;
    void TakeDamage(int damage, Vector3D knockback);
    bool AttackIsActive() const;
};

}
