// Jugador: estado, maquina de estados (FSM), fisica de aire/derribo, dano y personaje elegido.
#include "game/player/PlayerCommon.h"

namespace district_fury {

Player::Player() { Reset(); }

void Player::Reset() {
    position = {180, 565, 0};
    velocity = {0, 0, 0};
    facing = Facing::Right;
    state = PlayerState::Idle;
    attackType = AttackType::None;
    currentAttack = AttackId::None;
    attackPhase = AttackPhase::None;
    inputBuffer.Clear();
    recoveryTimer = 0.0f;

    maxHp = 100 + upgrades.bonusMaxHp;
    hp = maxHp;
    maxSp = 100 + upgrades.bonusMaxSp;
    sp = maxSp;
    maxRage = static_cast<int>(std::round(100.0f * upgrades.rageCapacityScale));
    rage = 0;
    isRageMode = false;
    maxShield = 100 + upgrades.bonusMaxShield;
    shield = maxShield;
    shieldRegenTimer = 0;
    shieldRegenRate = 34.0f;

    stateTimer = 0; attackElapsed = 0; attackDuration = 0;
    dashTimer = 0; dashInvulnerability = 0; dashCooldown = 0;
    comboWindow = 0; spRegenAccumulator = 0; rageDrainAccumulator = 0;
    blockDamageReduction = 0.78f; blockTimer = 0;

    rageState = RageState::Normal;
    rageStateTimer = 0;
    rageAuraPhase = 0;

    damageBuffTimer = 0;
    speedBuffTimer = 0;
    hitstunTimer = 0;
    knockdownTimer = 0;

    for (int i = 0; i < static_cast<int>(AttackId::Count); ++i) attackCooldowns[i] = 0.0f;

    comboCount = 0; comboStep = 0; hasHit = false; energyReleased = false;
    inputEnabled = true;
    debugInvulnerable = false;
    animator = Animator{};
}

void Player::ApplyUpgrades(const PlayerUpgrades& newUpgrades) {
    upgrades = newUpgrades;
    maxHp = 100 + upgrades.bonusMaxHp;
    maxSp = 100 + upgrades.bonusMaxSp;
    maxShield = 100 + upgrades.bonusMaxShield;
    maxRage = static_cast<int>(std::round(100.0f * upgrades.rageCapacityScale));
    hp = std::min(hp <= 0 ? maxHp : hp, maxHp);
    sp = std::min(sp, maxSp);
    shield = std::min(shield, maxShield);
    rage = std::min(rage, maxRage);
}

const AttackDef& Player::CurrentAttackDef() const { return GetAttack(currentAttack); }

const char* Player::RageStateName() const {
    switch (rageState) {
        case RageState::Starting: return "FURIA // ACTIVANDO";
        case RageState::Active:   return "FURIA // ACTIVA";
        case RageState::Ending:   return "FURIA // AGOTANDOSE";
        default:                  return "NORMAL";
    }
}

void Player::SetState(PlayerState next) {
    if (state == next && next != PlayerState::Attack) return;
    state = next;
    hasHit = false;
    if (state != PlayerState::Attack) attackPhase = AttackPhase::None;
    if (state == PlayerState::Hit || state == PlayerState::GuardBreak ||
        state == PlayerState::Knockdown || state == PlayerState::Defeat ||
        state == PlayerState::Airborne) {
        inputBuffer.Clear();   // un golpe recibido invalida lo que estaba en cola
    }
    const bool normalized = animator.normalizedAtlas;
    // Con manifiesto dinamico se reproduce por nombre; si falta el clip, cae al
    // rango de indices historico.
    auto named = [&](const char* clip) { return animator.PlayNamed(clip); };
    switch (state) {
        case PlayerState::Idle:
            if (named("idle")) break;
            animator.Play(normalized ? AnimationClip{0, 3, 0.12f, true}
                                     : AnimationClip{0, 4, 0.12f, true});
            break;
        case PlayerState::Walk:
            if (named("walk")) break;
            animator.Play(normalized ? AnimationClip{4, 7, 0.105f, true}
                                     : AnimationClip{0, 4, 0.10f, true});
            break;
        case PlayerState::Dash:
            if (named("dash")) break;
            animator.Play(normalized ? AnimationClip{14, 14, 0.08f, false}
                                     : AnimationClip{0, 4, 0.08f, false});
            break;
        case PlayerState::Block:
            blockTimer = 0;
            if (named("block")) break;
            animator.Play(normalized ? AnimationClip{3, 3, 0.10f, true}
                                     : AnimationClip{0, 4, 0.10f, true});
            blockTimer = 0;
            break;
        case PlayerState::Hit:
            if (!named(nextHitLow ? "hit_low" : "hit_high"))
                animator.Play(normalized ? AnimationClip{13, 13, 0.08f, false}
                                         : AnimationClip{0, 4, 0.08f, false});
            nextHitLow = !nextHitLow;
            if (stateTimer <= 0.0f) stateTimer = 0.13f;
            break;
        case PlayerState::GuardBreak:
            if (!named("hit_high")) animator.Play(normalized ? AnimationClip{13, 13, 0.08f, false}
                                     : AnimationClip{0, 4, 0.08f, false});
            stateTimer = 0.42f;
            break;
        case PlayerState::Knockdown:
            if (!named("knockdown")) animator.Play(normalized ? AnimationClip{15, 15, 0.10f, false}
                                     : AnimationClip{14, 14, 0.10f, false});
            break;
        case PlayerState::Defeat:
            if (!named("defeat")) animator.Play(normalized ? AnimationClip{15, 15, 0.10f, false}
                                     : AnimationClip{14, 14, 0.10f, false});
            AudioSystem::Get().Play(Sfx::GameOver);
            break;
        case PlayerState::Airborne:
            if (!named("airborne")) animator.Play(normalized ? AnimationClip{13, 13, 0.10f, false}
                                                             : AnimationClip{14, 14, 0.10f, false});
            break;
        case PlayerState::Recovery:
            // Pose de guardia: primer cuadro de idle, sin ciclo.
            if (!named("recovery")) animator.Play(AnimationClip{0, 0, 0.10f, false});
            break;
        case PlayerState::Attack:
            break;
    }
}

static void EnsurePlayerAnimator(Animator& animator, int skin) {
    if (animator.texture.id != 0) return;
    // Perfil dinamico (celda/pivote/clips desde data/sprite_manifest.json).
    const CharacterVisual& cv = GetCharacterVisual(skin);
    if (cv.kfRoster >= 0) {                          // Laboratorio: personaje de la APK
        const KfReference& kf = GetKfCharacter(cv.kfRoster);
        if (kf.loaded) { animator = kf.templ; animator.PlayNamed("idle"); return; }
    }
    if (cv.atlasId != nullptr) {
        const AtlasProfile* profile = SpriteManifest::Get().FindAtlas(cv.atlasId);
        if (profile && animator.InitFromManifest(cv.atlasId, AssetManager::Get().GetTextureByPath(profile->path))) {
            animator.PlayNamed("idle");
            return;
        }
    }
    if (animator.InitFromManifest("rayden", AssetManager::Get().GetTexture("rayden_128"))) {
        animator.PlayNamed("idle");
        return;
    }
    Texture2D clean = AssetManager::Get().GetTexture("rayden_clean");
    if (clean.id != 0) { animator.Init(clean, 4, 4, true); animator.Play({0, 3, 0.12f, true}); return; }
    Texture2D legacy = AssetManager::Get().GetTexture("rayden_sheet");
    if (legacy.id != 0) { animator.Init(legacy, 5, 3, false); animator.Play({0, 4, 0.12f, true}); }
}

void Player::UpdateRage(float dt) {
    rageAuraPhase += dt;

    switch (rageState) {
        case RageState::Starting:
            rageStateTimer -= dt;
            if (rageStateTimer <= 0.0f) {
                rageState = RageState::Active;
                rageStateTimer = kRageMaxDuration;
                isRageMode = true;
            }
            break;
        case RageState::Active: {
            isRageMode = true;
            rageStateTimer -= dt;
            rageDrainAccumulator += dt;
            if (rageDrainAccumulator >= 0.05f) {
                const int drain = std::max(1, static_cast<int>(rageDrainAccumulator * 11.0f));
                rage = std::max(0, rage - drain);
                rageDrainAccumulator = 0;
            }
            if (rage <= 0 || rageStateTimer <= 0.0f) {
                rageState = RageState::Ending;
                rageStateTimer = kRageEndDuration;
            }
            break;
        }
        case RageState::Ending:
            rageStateTimer -= dt;
            isRageMode = false;
            if (rageStateTimer <= 0.0f) {
                rageState = RageState::Normal;
                rage = 0;
            }
            break;
        case RageState::Normal:
        default:
            isRageMode = false;
            break;
    }
}

bool Player::CanAct() const {
    return state != PlayerState::Defeat && state != PlayerState::GuardBreak &&
           state != PlayerState::Knockdown && state != PlayerState::Hit &&
           state != PlayerState::Attack && state != PlayerState::Recovery &&
           state != PlayerState::Airborne;
}

bool Player::IsKnockedDown() const {
    return state == PlayerState::Knockdown || state == PlayerState::Airborne;
}

bool Player::IsInvulnerable() const {
    return debugInvulnerable || dashInvulnerability > 0.0f;
}

float Player::GetMoveSpeed() const {
    float speed = 245.0f * upgrades.moveSpeedScale;
    if (isRageMode) speed *= 1.16f;
    if (speedBuffTimer > 0.0f) speed *= 1.22f;
    return speed;
}

void Player::Update(float dt) {
    EnsurePlayerAnimator(animator, skin);
    animator.Update(dt);

    dashInvulnerability = std::max(0.0f, dashInvulnerability - dt);
    dashCooldown = std::max(0.0f, dashCooldown - dt);
    damageBuffTimer = std::max(0.0f, damageBuffTimer - dt);
    speedBuffTimer = std::max(0.0f, speedBuffTimer - dt);
    for (int i = 0; i < static_cast<int>(AttackId::Count); ++i) {
        attackCooldowns[i] = std::max(0.0f, attackCooldowns[i] - dt);
    }

    UpdateRage(dt);

    if (!inputPumped) PumpInput(dt);
    inputPumped = false;           // el proximo frame vuelve a leer

    if (shield < maxShield && !IsBlocking() && state != PlayerState::Hit &&
        state != PlayerState::GuardBreak && state != PlayerState::Knockdown) {
        shieldRegenTimer -= dt;
        if (shieldRegenTimer <= 0) {
            shield = std::min(maxShield, shield + static_cast<int>(std::ceil(shieldRegenRate * dt)));
        }
    }

    if (comboWindow > 0 && (comboWindow -= dt) <= 0) {
        comboWindow = 0;
        comboCount = 0;
        comboStep = 0;
        currentAttack = AttackId::None;
    }

    // Altura (launch/knockdown): gravedad simple sobre position.z.
    if (position.z > 0.0f || velocity.z != 0.0f) {
        velocity.z -= 1500.0f * dt;
        position.z += velocity.z * dt;
        if (position.z <= 0.0f) { position.z = 0.0f; velocity.z = 0.0f; }
    }

    if (state == PlayerState::Defeat) return;

    if (state == PlayerState::Airborne) {
        position.x += velocity.x * dt;
        velocity.x *= 0.94f;
        position.x = std::clamp(position.x, kStageStartX, kStageEndX - 90.f);
        if (position.z <= 0.0f && velocity.z == 0.0f) SetState(PlayerState::Knockdown);
        return;
    }

    if (state == PlayerState::Knockdown) {
        knockdownTimer -= dt;
        position.x += velocity.x * dt;
        velocity.x *= 0.86f;
        position.x = std::clamp(position.x, kStageStartX, kStageEndX - 90.f);
        position.y = std::clamp(position.y, kLaneMinY, kLaneMaxY);
        if (knockdownTimer <= 0 && position.z <= 0.0f) {
            dashInvulnerability = std::max(dashInvulnerability, 0.55f);
            SetState(PlayerState::Recovery);           // levantarse
            animator.PlayNamed("getup");
            recoveryTimer = 0.32f;
        }
        return;
    }

    if (state == PlayerState::GuardBreak) {
        if ((stateTimer -= dt) <= 0) SetState(PlayerState::Idle);
        return;
    }

    if (state == PlayerState::Hit) {
        stateTimer -= dt;
        hitstunTimer = std::max(0.0f, hitstunTimer - dt);
        position.x += velocity.x * dt;
        position.y += velocity.y * dt;
        velocity.x *= 0.80f;
        velocity.y *= 0.80f;
        if (stateTimer <= 0) SetState(PlayerState::Idle);
        position.x = std::clamp(position.x, kStageStartX, kStageEndX - 90.f);
        position.y = std::clamp(position.y, kLaneMinY, kLaneMaxY);
        return;
    }

    if (state == PlayerState::Dash) {
        dashTimer -= dt;
        position.x += (facing == Facing::Right ? 1.f : -1.f) * 700.f * dt;
        position.x = std::clamp(position.x, kStageStartX, kStageEndX - 90.f);
        // DASH ATTACK: J durante el dash.
        const InputBuffer::Entry* queued = inputBuffer.Peek();
        if (inputEnabled && queued && queued->command == InputCommand::Punch &&
            attackCooldowns[static_cast<int>(AttackId::DashAttack)] <= 0.0f) {
            InputCommand consumed;
            inputBuffer.Pop(consumed);
            BeginAttack(AttackId::DashAttack);
            return;
        }
        if (dashTimer <= 0) SetState(PlayerState::Idle);
        return;
    }

    if (state == PlayerState::Attack) {
        attackElapsed += dt;
        stateTimer -= dt;
        const AttackDef& def = GetAttack(currentAttack);
        if (def.spawnsProjectile && !energyReleased && attackElapsed >= def.startup) {
            energyReleased = true;
        }
        // Animation cancel: con un comando valido en el buffer y el ataque en su
        // ventana final, se corta el winddown y se encadena el siguiente golpe.
        if (inputEnabled && InCancelWindow() && TryStartBufferedAttack(true)) return;
        if (stateTimer <= 0 || (animator.isFinished && attackElapsed >= attackDuration)) {
            EndAttack();
        }
        return;
    }

    if (state == PlayerState::Recovery) {
        // Recuperacion fisica obligatoria: sin moverse ni empezar ataques
        // hasta que termine. Lo pulsado durante este tiempo queda en el buffer.
        recoveryTimer -= dt;
        if (recoveryTimer <= 0.0f) {
            recoveryTimer = 0.0f;
            if (!inputEnabled || !TryStartBufferedAttack(false)) SetState(PlayerState::Idle);
        }
        return;
    }

    HandleInput(dt);
}

void Player::AddRage(int amount) {
    if (amount > 0 && rageState == RageState::Normal) rage = std::min(maxRage, rage + amount);
}

bool Player::IsBlocking() const { return state == PlayerState::Block; }

bool Player::IsGuardBroken() const { return state == PlayerState::GuardBreak; }

CombatBox Player::GetHurtbox() const {
    if (state == PlayerState::Knockdown) {
        return {position.x - 38.f, position.y - 40.f, 76.f, 36.f};
    }
    return {position.x - 24.f, position.y - 112.f + -position.z, 48.f, 105.f};
}

void Player::TakeDamage(int damage) {
    if (damage <= 0 || state == PlayerState::Defeat) return;
    if (debugInvulnerable || dashInvulnerability > 0) return;

    shieldRegenTimer = 1.25f;

    if (IsBlocking()) {
        const int reduced = std::max(1, static_cast<int>(std::round(damage * (1.f - blockDamageReduction))));
        const int absorbed = std::min(shield, reduced);
        shield -= absorbed;
        hp = std::max(0, hp - reduced + absorbed);
        AddRage(12);
        velocity.x += facing == Facing::Right ? -55.f : 55.f;
        dashInvulnerability = 0.12f;
        AudioSystem::Get().Play(Sfx::Hit);
        if (hp == 0) SetState(PlayerState::Defeat);
        else if (shield == 0) { SetState(PlayerState::GuardBreak); AudioSystem::Get().Play(Sfx::HeavyHit); }
        return;
    }

    if (state == PlayerState::Hit && stateTimer > 0) return;
    if (state == PlayerState::Knockdown || state == PlayerState::Airborne) return;

    hp = std::max(0, hp - damage);
    AddRage(15);

    if (hp == 0) {
        SetState(PlayerState::Defeat);
        return;
    }

    // Golpes fuertes derriban: knockdown real en vez de un unico estado Hit.
    if (damage >= 28) {
        knockdownTimer = 0.85f;
        velocity.x = facing == Facing::Right ? -260.f : 260.f;
        velocity.z = 380.f;
        SetState(PlayerState::Airborne);
        AudioSystem::Get().Play(Sfx::HeavyHit);
        return;
    }

    stateTimer = 0.13f;
    hitstunTimer = 0.18f;
    SetState(PlayerState::Hit);
    dashInvulnerability = 0.28f;
}

void Player::ApplyCharacter(int id) {
    skin = id;
    animator = Animator{};                     // se reinicia con el atlas del personaje
    if (id == 0) return;                       // Rayden original: intacto
    const CharacterVisual& cv = GetCharacterVisual(id);
    // Los personajes con hoja completa (Rayder) conservan las mejoras de campana.
    maxHp = cv.maxHp + (cv.atlasId != nullptr ? upgrades.bonusMaxHp : 0);
    hp = maxHp;
}

}  // namespace district_fury
