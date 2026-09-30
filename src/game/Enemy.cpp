#include "game/Enemy.h"
#include "rendering/AssetManager.h"
#include <algorithm>
#include <cmath>

namespace district_fury {

namespace {
constexpr float kMinX = 50.0f;
constexpr float kMaxX = 1230.0f;
constexpr float kMinLane = 450.0f;
constexpr float kMaxLane = 700.0f;

float DepthScale(float laneY) {
    const float t = std::clamp((laneY - kMinLane) / (kMaxLane - kMinLane), 0.0f, 1.0f);
    return 0.86f + 0.26f * t;
}
}

Enemy::Enemy() {
    Init({800.0f, 500.0f, 0.0f});
}

void Enemy::Init(Vector3D startPos) {
    position = startPos;
    velocity = {0.0f, 0.0f, 0.0f};
    facing = Facing::Left;
    state = EnemyState::Idle;
    hp = 70;
    maxHp = 70;
    stateTimer = 0.0f;
    attackElapsed = 0.0f;
    hasHit = false;
    animator = Animator{};
}

static void EnsureEnemyAnimator(Animator& animator) {
    if (animator.texture.id != 0) return;

    Texture2D clean = AssetManager::Get().GetTexture("grinder_clean");
    if (clean.id != 0) {
        animator.Init(clean, 4, 3, true);
        animator.Play({0, 3, 0.12f, true});
        return;
    }

    Texture2D legacy = AssetManager::Get().GetTexture("grinder_sheet");
    if (legacy.id != 0) {
        animator.Init(legacy, 4, 3, false);
        animator.Play({0, 3, 0.12f, true});
    }
}

void Enemy::Update(float dt, const Player& player) {
    EnsureEnemyAnimator(animator);
    animator.Update(dt);

    if (state == EnemyState::Defeat) {
        stateTimer -= dt;
        return;
    }

    if (state == EnemyState::Hit) {
        stateTimer -= dt;
        position.x += velocity.x * dt;
        position.y += velocity.y * dt;
        velocity.x *= 0.87f;
        velocity.y *= 0.87f;
        if (stateTimer <= 0.0f) state = EnemyState::Idle;
        position.x = std::clamp(position.x, kMinX, kMaxX);
        position.y = std::clamp(position.y, kMinLane, kMaxLane);
        return;
    }

    if (state == EnemyState::Attack) {
        stateTimer -= dt;
        attackElapsed += dt;
        if (stateTimer <= 0.0f || animator.isFinished) state = EnemyState::Idle;
        return;
    }

    if (player.state == PlayerState::Defeat) {
        state = EnemyState::Idle;
        return;
    }

    const float dx = player.position.x - position.x;
    const float dy = player.position.y - position.y;
    const float horizontal = std::abs(dx);
    const float depth = std::abs(dy);
    const float dist = std::sqrt(dx * dx + dy * dy);

    facing = dx >= 0.0f ? Facing::Right : Facing::Left;

    if (horizontal < 105.0f && depth < 28.0f) {
        state = EnemyState::Attack;
        stateTimer = 0.62f;
        attackElapsed = 0.0f;
        hasHit = false;
        animator.Play({4, 6, 0.10f, false});
        return;
    }

    if (dist < 520.0f) {
        state = EnemyState::Chase;
        const float speed = 145.0f;
        if (dist > 0.001f) {
            position.x += (dx / dist) * speed * dt;
            position.y += (dy / dist) * speed * 0.72f * dt;
        }
        if (animator.isFinished || !animator.isPlaying) animator.Play({0, 3, 0.12f, true});
    } else {
        state = EnemyState::Idle;
        if (animator.currentFrame < 0 || animator.currentFrame > 3 || animator.isFinished) {
            animator.Play({0, 3, 0.12f, true});
        }
    }

    position.x = std::clamp(position.x, kMinX, kMaxX);
    position.y = std::clamp(position.y, kMinLane, kMaxLane);
}

bool Enemy::AttackIsActive() const {
    return state == EnemyState::Attack && attackElapsed >= 0.24f && attackElapsed <= 0.46f;
}

void Enemy::TakeDamage(int damage, Vector3D knockback) {
    if (state == EnemyState::Defeat) return;
    hp = std::max(0, hp - damage);
    velocity = knockback;

    if (hp == 0) {
        state = EnemyState::Defeat;
        stateTimer = 0.85f;
        animator.Play({8, 10, 0.11f, false});
    } else {
        state = EnemyState::Hit;
        stateTimer = 0.42f;
        animator.Play({8, 10, 0.09f, false});
    }
}

void Enemy::Draw() const {
    const Vector2 screenPos = position.ToScreen();
    const float depthScale = DepthScale(position.y);
    const float visualScale = animator.normalizedAtlas ? 2.02f * depthScale : 0.86f * depthScale;
    DrawEllipse(static_cast<int>(screenPos.x), static_cast<int>(screenPos.y),
                38.0f * depthScale, 11.0f * depthScale, {0, 0, 0, 145});

    if (animator.texture.id != 0) {
        Color tint = WHITE;
        if (state == EnemyState::Hit) tint = {255, 215, 215, 255};
        if (state == EnemyState::Defeat) tint = {180, 180, 180, 255};
        animator.Draw(screenPos, visualScale, facing == Facing::Left, tint);
    } else {
        Color c = state == EnemyState::Hit ? WHITE : state == EnemyState::Attack ? ORANGE : state == EnemyState::Defeat ? DARKGRAY : PURPLE;
        DrawRectangle(static_cast<int>(screenPos.x - 25 * depthScale), static_cast<int>(screenPos.y - 90 * depthScale),
                      static_cast<int>(50 * depthScale), static_cast<int>(90 * depthScale), c);
    }

    if (state != EnemyState::Defeat && hp < maxHp) {
        const int width = 64;
        const int x = static_cast<int>(screenPos.x - width / 2.0f);
        const int y = static_cast<int>(screenPos.y - 110.0f * depthScale);
        DrawRectangle(x, y, width, 7, {20, 20, 20, 220});
        DrawRectangle(x, y, static_cast<int>(width * (static_cast<float>(hp) / maxHp)), 7, GREEN);
    }
}

}
