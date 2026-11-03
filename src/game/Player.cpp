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
    abilityHitFlashTimer = 0;

    for (int i = 0; i < static_cast<int>(AttackId::Count); ++i) attackCooldowns[i] = 0.0f;

    comboCount = 0; comboStep = 0; hasHit = false; energyReleased = false;
    chainAttack = AttackId::None; attackPrevElapsed = 0.0f;
    projectileFired = false; projectilePending = false;
    hitTargets.clear(); multiHitClock = 0.0f; attackConnected = false;
    hurtShakeTimer = 0.0f; hurtShakeAmp = 0.0f; hurtShakeDir = 1.0f;
    for (float& cd : skillCooldown) cd = 0.0f;
    abilityCooldown.clear(); skillPage = 0;
    activeSkill = -1; clipDriven = false; multiHitTimer = 0.0f; transformTimer = 0.0f;
    // F0.4
    victoryTimer = 0.0f; jumpCancelTimer = 0.0f; lastHitTargetX = 0.0f; airAttacks = 0;
    chainLinkJ = chainLinkK = -1;
    walkVelX = walkVelY = 0.0f; running = false; tapTimer = 0.0f; lastTapDir = 0; prevHoriz = 0;
    dashDir = 1; dashSpeed = 700.0f; stepCount = 0;
    attackHitsDone = 0; armorAbsorbed = 0; projectileShots = 0; projectileClock = 0.0f; rootAppliedX = 0.0f;
    stompTargetAhead = false;
    attackClipName.clear(); attackClipOrigin = ClipOrigin::Historic;
    effectiveId_ = AttackId::Count;
    inputEnabled = true;
    debugInvulnerable = false;
    animator = Animator{};
    altAnimator = Animator{}; usingAlt = false; altTried = false;
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
    hitTargets.clear();
    // B09: el dash tiene su propia velocidad; no arrastra empujes anteriores.
    if (state == PlayerState::Dash || state == PlayerState::Defeat) velocity.x = 0.0f;
    if (state != PlayerState::Attack) { attackPhase = AttackPhase::None; activeSkill = -1; clipDriven = false; }
    if (state != PlayerState::Walk && state != PlayerState::Idle) { walkVelX = walkVelY = 0.0f; running = false; }
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
        case PlayerState::Jump:
            // Salto propio (F0.4): clip de salto si existe; si no, una pose de
            // impulso (dash). Nunca la de derribado (airborne).
            if (named("jump") || named("jump_rise") || named("air") || named("dash")) break;
            animator.Play(normalized ? AnimationClip{14, 14, 0.08f, false} : AnimationClip{0, 4, 0.08f, false});
            break;
        case PlayerState::Victory:
            if (named("victory")) break;
            [[fallthrough]];
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
            EmitPlayerEvent(*this, GameEventType::PlayerDefeat);
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

void Player::UpdateTransformForm() {
    const CharacterVisual& cv = GetCharacterVisual(skin);
    if (cv.transformAtlasId == nullptr && cv.transformKfRoster < 0) return;
    // Cambia de forma al terminar la animacion de transformar y vuelve al acabar
    // los 12 s; nunca a mitad de un ataque.
    const bool want = IsTransformed() && !(state == PlayerState::Attack && activeSkill == kSkillCount - 1);
    if (want == usingAlt || state == PlayerState::Attack) return;
    if (!altTried) {
        altTried = true;
        if (cv.transformAtlasId) {
            const AtlasProfile* prof = SpriteManifest::Get().FindAtlas(cv.transformAtlasId);
            if (prof) altAnimator.InitFromManifest(cv.transformAtlasId, AssetManager::Get().GetTextureByPath(prof->path));
        } else {
            const KfReference& k = GetKfCharacter(cv.transformKfRoster);
            if (k.loaded) altAnimator = k.templ;
        }
    }
    if (altAnimator.texture.id == 0) return;
    std::swap(animator, altAnimator);
    usingAlt = want;
    if (want) EmitPlayerEvent(*this, GameEventType::TransformStart);   // cambio de forma
    const char* clip = "idle";
    switch (state) {
        case PlayerState::Walk: clip = "walk"; break;
        case PlayerState::Dash: clip = "dash"; break;
        case PlayerState::Block: clip = "block"; break;
        case PlayerState::Recovery: clip = "recovery"; break;
        case PlayerState::Hit: case PlayerState::GuardBreak: clip = "hit_high"; break;
        case PlayerState::Knockdown: clip = "knockdown"; break;
        case PlayerState::Airborne: clip = "airborne"; break;
        case PlayerState::Defeat: clip = "defeat"; break;
        case PlayerState::Jump: clip = animator.HasClip("jump") ? "jump" : "dash"; break;
        case PlayerState::Victory: clip = animator.HasClip("victory") ? "victory" : "idle"; break;
        default: break;
    }
    if (!animator.PlayNamed(clip)) animator.PlayNamed("idle");
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
    // Jump: en el aire solo se atacan aereos (no camina ni bloquea); Victory: nada.
    return state != PlayerState::Defeat && state != PlayerState::GuardBreak &&
           state != PlayerState::Knockdown && state != PlayerState::Hit &&
           state != PlayerState::Attack && state != PlayerState::Recovery &&
           state != PlayerState::Airborne && state != PlayerState::Jump && state != PlayerState::Victory;
}

bool Player::IsKnockedDown() const {
    // Jump NO es derribo (B22): Airborne sigue siendo "derribado en el aire".
    return state == PlayerState::Knockdown || state == PlayerState::Airborne;
}

bool Player::IsInvulnerable() const {
    return debugInvulnerable || dashInvulnerability > 0.0f;
}

float Player::GetMoveSpeed() const {
    float speed = 245.0f * upgrades.moveSpeedScale;
    if (isRageMode) speed *= 1.16f;
    if (speedBuffTimer > 0.0f) speed *= 1.22f;
    if (transformTimer > 0.0f) speed *= 1.15f;
    return speed;
}

void Player::Update(float dt) {
    EnsurePlayerAnimator(animator, skin); UpdateTransformForm(); if(IsKeyPressed(KEY_F9)) animationDebug=!animationDebug; /* F9: F1-F5 cambian de stage */ animator.Update(dt); UpdateMotionFeel(dt);
    // F0.4: Footstep en cada cruce de walkPhase por 0 y pi (peso 3 si es grande).
    {
        const int steps = static_cast<int>(std::floor(walkPhase / 3.14159265f));
        if (steps != stepCount) {
            if (state == PlayerState::Walk)
                EmitPlayerEvent(*this, GameEventType::Footstep, false, running ? 1 : 0, AttackId::None,
                                BodyHeight() / kStandardBodyHeight >= 1.4f ? 3 : 1);
            stepCount = steps;
        }
    }
    jumpCancelTimer = std::max(0.0f, jumpCancelTimer - dt);

    dashInvulnerability = std::max(0.0f, dashInvulnerability - dt);
    abilityHitFlashTimer = std::max(0.0f, abilityHitFlashTimer - dt);
    dashCooldown = std::max(0.0f, dashCooldown - dt);
    damageBuffTimer = std::max(0.0f, damageBuffTimer - dt);
    speedBuffTimer = std::max(0.0f, speedBuffTimer - dt);
    for (int i = 0; i < static_cast<int>(AttackId::Count); ++i) {
        attackCooldowns[i] = std::max(0.0f, attackCooldowns[i] - dt);
    }
    for (float& cd : skillCooldown) cd = std::max(0.0f, cd - dt);
    for (float& cd : abilityCooldown) cd = std::max(0.0f, cd - dt);
    transformTimer = std::max(0.0f, transformTimer - dt);
    hurtShakeTimer = std::max(0.0f, hurtShakeTimer - dt);

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

    // Ventana de combo: cuenta el tiempo ENTRE ataques (no corre mientras el
    // ataque en curso se ejecuta). Al expirar se pierde la cadena, pero nunca se
    // borra el ataque en curso (B04).
    if (comboWindow > 0 && state != PlayerState::Attack && (comboWindow -= dt) <= 0) {
        comboWindow = 0;
        comboCount = 0;
        comboStep = 0;
        chainAttack = AttackId::None;
        chainLinkJ = chainLinkK = -1;
        if (state != PlayerState::Attack) currentAttack = AttackId::None;
    }

    // Altura (launch/knockdown): gravedad simple sobre position.z. El salto y los
    // aereos (F0.4) usan su propia gravedad (UpdateJump / UpdateAttackMotion).
    const bool ownGravity = state == PlayerState::Jump || (state == PlayerState::Attack && (CurrentAttackDef().air ||
                            CurrentAttackDef().root.dz != 0.0f));
    if (!ownGravity && (position.z > 0.0f || velocity.z != 0.0f)) {
        velocity.z -= 1500.0f * dt;
        position.z += velocity.z * dt;
        if (position.z <= 0.0f) { position.z = 0.0f; velocity.z = 0.0f; }
    }

    if (state == PlayerState::Defeat) return;

    if (state == PlayerState::Victory) {
        // No actua ni recibe golpes; 1.6 s y vuelve a Idle.
        velocity = {0, 0, 0};
        if ((victoryTimer -= dt) <= 0.0f) { victoryTimer = 0.0f; SetState(PlayerState::Idle); }
        return;
    }

    if (state == PlayerState::Jump) {
        UpdateJump(dt);
        return;
    }

    if (state == PlayerState::Airborne) {
        DampedStep(position.x, velocity.x, 0.94f, dt);   // B11: por dt, no por frame
        position.x = std::clamp(position.x, kStageStartX, kStageEndX - 90.f);
        if (position.z <= 0.0f && velocity.z == 0.0f) SetState(PlayerState::Knockdown);
        return;
    }

    if (state == PlayerState::Knockdown) {
        knockdownTimer -= dt;
        DampedStep(position.x, velocity.x, 0.86f, dt);
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
        DampedStep(position.x, velocity.x, 0.80f, dt);
        DampedStep(position.y, velocity.y, 0.80f, dt);
        if (stateTimer <= 0) SetState(PlayerState::Idle);
        position.x = std::clamp(position.x, kStageStartX, kStageEndX - 90.f);
        position.y = std::clamp(position.y, kLaneMinY, kLaneMaxY);
        return;
    }

    // B09: empujes (bloqueo, choque contra un jefe en guardia, retroceso leve de
    // la superarmadura...) se integran tambien en los estados de suelo con
    // v *= exp(-12 dt), en vez de acumularse sin moverse.
    if (velocity.x != 0.0f) {
        DampedStep(position.x, velocity.x, std::exp(-12.0f / 60.0f), dt);
        if (std::fabs(velocity.x) < 1.0f) velocity.x = 0.0f;
        position.x = std::clamp(position.x, kStageStartX, kStageEndX - 90.f);
    }

    if (state == PlayerState::Dash) {
        dashTimer -= dt;
        position.x += (facing == Facing::Right ? 1.f : -1.f) * static_cast<float>(dashDir) * dashSpeed * dt;
        position.x = std::clamp(position.x, kStageStartX, kStageEndX - 90.f);
        // DASH ATTACK: J durante el dash; F0.4: K = DESLIZAMIENTO (DashKick).
        const InputBuffer::Entry* queued = inputBuffer.Peek();
        if (inputEnabled && queued && dashDir > 0) {
            if (queued->command == InputCommand::Punch &&
                attackCooldowns[static_cast<int>(AttackId::DashAttack)] <= 0.0f) {
                InputCommand consumed;
                inputBuffer.Pop(consumed);
                BeginAttack(AttackId::DashAttack);
                chainLinkJ = chainLinkK = -1;
                return;
            }
            if (queued->command == InputCommand::Kick && MoveEnabled(AttackId::DashKick) &&
                attackCooldowns[static_cast<int>(AttackId::DashKick)] <= 0.0f) {
                InputCommand consumed;
                inputBuffer.Pop(consumed);
                BeginAttack(AttackId::DashKick);
                chainLinkJ = chainLinkK = -1;
                return;
            }
        }
        if (dashTimer <= 0) SetState(PlayerState::Idle);
        return;
    }

    if (state == PlayerState::Attack) {
        const AttackDef& def = CurrentAttackDef();
        const float previousAttackElapsed=attackElapsed; attackPrevElapsed=attackElapsed; attackElapsed+=dt; stateTimer-=dt;
        if(!attackImpactTriggered&&previousAttackElapsed<def.startup&&attackElapsed>=def.startup)attackImpactTriggered=true;
        // B01: el proyectil sale UNA vez por activacion (projectileFired solo lo
        // reinicia BeginAttack); el stage lo recoge con ConsumeProjectileEvent().
        // F0.4: ProjectileSpec.count > 1 = un disparo mas cada `interval` s.
        if (def.spawnsProjectile && !projectileFired && attackElapsed >= def.startup) {
            projectileFired = true;
            projectilePending = true;
            projectileShots = 1;
            projectileClock = 0.0f;
            energyReleased = true;   // legado: ya no se usa para disparar
        } else if (projectileFired && !projectilePending) {
            const ProjectileSpec spec = CurrentProjectile();
            if (projectileShots < spec.count && (projectileClock += dt) >= spec.interval) {
                projectileClock = 0.0f;
                projectilePending = true;
                ++projectileShots;
            }
        }
        // Root motion de datos, salto propio de los aereos, aterrizaje.
        UpdateAttackMotion(dt, def);
        if (state != PlayerState::Attack) return;
        // Persecucion tras un lanzador que conecto: SALTO / J / K en 10 f.
        if (inputEnabled && jumpCancelTimer > 0.0f && def.jumpCancel && attackConnected) {
            const InputBuffer::Entry* q = inputBuffer.Peek();
            const bool jk = q && (q->command == InputCommand::Punch || q->command == InputCommand::Kick);
            if ((frameInput.jump || jk) && StartJump(true)) {
                if (jk) { InputCommand c; inputBuffer.Pop(c); }
                return;
            }
        }
        // Super desde un especial (furia activa): Espacio cancela en RageAttack.
        if (inputEnabled && frameInput.rage && rageState == RageState::Active && AttackIsSpecialMove(currentAttack) &&
            InCancelWindow() && attackCooldowns[static_cast<int>(AttackId::RageAttack)] <= 0.0f) {
            BeginAttack(AttackId::RageAttack);
            chainLinkJ = chainLinkK = -1;
            return;
        }
        if (clipDriven && (multiHitTimer -= dt) <= 0.0f) {
            hasHit = false;   // la animacion larga vuelve a golpear
            hitTargets.clear();
            multiHitTimer = currentAttack == AttackId::Skill ? 0.26f : 0.30f;
        }
        if (!clipDriven && def.multiHitInterval > 0.0f && (multiHitClock += dt) >= def.multiHitInterval) {
            multiHitClock = 0.0f;
            // golpe multiple controlado (F0.4: como mucho hitCount golpes; hitCount 1 = sin limite, como F0.2)
            if (def.hitCount <= 1 || attackHitsDone + 1 < def.hitCount) { hitTargets.clear(); ++attackHitsDone; }
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
        // Persecucion: tambien en la recuperacion del lanzador que conecto.
        if (inputEnabled && jumpCancelTimer > 0.0f && frameInput.jump && StartJump(true)) return;
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
    // BETA: caja de cuerpo del cuadro actual (la del juego original).
    if (const SpriteFrame* f = animator.CurrentFrameData(); f && !f->pieces.empty() && f->hurtbox) {
        const float sc = SpriteScale();
        const Rectangle& b = *f->hurtbox;
        const float x0 = facing == Facing::Right ? position.x + b.x * sc : position.x - (b.x + b.width) * sc;
        return {x0, position.y - position.z + b.y * sc, b.width * sc, b.height * sc};
    }
    if (state == PlayerState::Knockdown) {
        return {position.x - 38.f, position.y - 40.f, 76.f, 36.f};
    }
    // F0.2 (B06b): cuerpo derivado del alto real del sprite (Rayden: 112 px ->
    // ~48 x 104 como antes; Titan-X x1.5, Titan-X Mejorado x2.1) y de la escala
    // de carril, en vez de 48 x 105 fijos para todos.
    const float s = BodyScale();
    const float w = 48.0f * s;
    const float h = 105.0f * s;
    return {position.x - w * 0.5f, position.y - position.z - 7.0f * s - h, w, h};
}

float Player::BodyHeight() const {
    const CharacterVisual& cv = GetCharacterVisual(skin);
    if (cv.folder != nullptr && !cv.uniformCanvas && cv.targetHeight > 0.0f) return cv.targetHeight;
    return kStandardBodyHeight;
}

float Player::BodyScale() const { return BodyHeight() / kStandardBodyHeight * DepthScaleFor(position.y); }

bool Player::AlreadyHit(std::uintptr_t id) const {
    return std::find(hitTargets.begin(), hitTargets.end(), id) != hitTargets.end();
}

void Player::MarkHit(std::uintptr_t id) {
    if (!AlreadyHit(id)) hitTargets.push_back(id);
    hasHit = true;
}

bool Player::InAttackStartup() const {
    if (state != PlayerState::Attack) return false;
    if (clipDriven) return attackElapsed < attackDuration * 0.12f;
    return attackElapsed < CurrentAttackDef().startup;
}

// ---------------------------------------------------------------------------
// F0.4: salto, aereos y root motion de datos
// ---------------------------------------------------------------------------

void Player::FinishLanding(float recovery) {
    position.z = 0.0f;
    velocity.z = 0.0f;
    velocity.x = 0.0f;
    airAttacks = 0;
    SetState(PlayerState::Recovery);
    recoveryTimer = std::max(InputBuffer::kFrameSeconds, recovery);
}

void Player::UpdateJump(float dt) {
    // Control aereo 60 % de caminar; el carril no cambia en el aire (V9).
    const float target = frameInput.moveX * GetMoveSpeed() * 0.6f;
    velocity.x += (target - velocity.x) * (1.0f - std::exp(-6.0f * dt));
    if (frameInput.moveX > 0.5f) facing = Facing::Right;
    else if (frameInput.moveX < -0.5f) facing = Facing::Left;
    position.x = std::clamp(position.x + velocity.x * dt, kStageStartX, kStageEndX - 90.f);
    velocity.z -= (velocity.z > 0.0f ? kJumpRiseGravity : kJumpFallGravity) * dt;
    position.z += velocity.z * dt;
    if (position.z <= 0.0f) { FinishLanding(4.0f / 60.0f); return; }   // aterrizaje 4 f
    // Aereos: J/K del buffer.
    if (inputEnabled) TryStartBufferedAttack(false);
}

void Player::UpdateAttackMotion(float dt, const AttackDef& def) {
    const float dir = facing == Facing::Right ? 1.0f : -1.0f;
    const float total = std::max(1e-4f, AttackTotalDuration(def));
    const float s = BodyHeight() / kStandardBodyHeight;
    // Root motion de datos (RootCurve): x real, frena x0.3 al conectar.
    if (!clipDriven && def.root.dx != 0.0f) {
        const float span = std::max(1e-4f, def.root.t1 - def.root.t0);
        const float u = std::clamp((attackElapsed / total - def.root.t0) / span, 0.0f, 1.0f);
        const float target = def.root.dx * s * CombatEase(def.root.ease, u);
        float delta = target - rootAppliedX;
        rootAppliedX = target;
        if (attackConnected && def.root.slowOnHit) delta *= 0.3f;
        position.x = std::clamp(position.x + dir * delta, kStageStartX, kStageEndX - 90.f);
    }
    if (def.air) {
        // Aereo: sigue la parabola del salto (DiveKick: picada a 45 grados, 700 px/s).
        if (currentAttack == AttackId::DiveKick &&
            attackElapsed >= def.startup && attackElapsed <= def.startup + def.active) {
            velocity.x = dir * 700.0f * 0.7071f;
            velocity.z = -700.0f * 0.7071f;
        } else {
            velocity.z -= (velocity.z > 0.0f ? kJumpRiseGravity : kJumpFallGravity) * dt;
        }
        position.x = std::clamp(position.x + velocity.x * dt, kStageStartX, kStageEndX - 90.f);
        position.z += velocity.z * dt;
        if (position.z <= 0.0f) FinishLanding(def.landRecovery > 0.0f ? def.landRecovery : 4.0f / 60.0f);
        return;
    }
    if (!clipDriven && def.root.dz != 0.0f) {
        // Salto propio del golpe (Uppercut hop 10, Shoryu 90): sin(pi u') en [t0, t1].
        const float span = std::max(1e-4f, def.root.t1 - def.root.t0);
        const float u = std::clamp((attackElapsed / total - def.root.t0) / span, 0.0f, 1.0f);
        const float up = (attackElapsed / total) <= def.root.t1 ? std::sin(3.14159265f * CombatEase(def.root.ease, u) * 0.5f)
                                                                : 1.0f - std::clamp((attackElapsed / total - def.root.t1) /
                                                                                    std::max(1e-4f, 1.0f - def.root.t1), 0.0f, 1.0f);
        position.z = std::max(0.0f, def.root.dz * s * up);
        velocity.z = 0.0f;
    }
}

bool Player::ConsumeProjectileEvent() {
    if (!projectilePending) return false;
    projectilePending = false;
    EmitPlayerEvent(*this, GameEventType::ProjectileSpawn);
    return true;
}

void Player::TakeDamage(int damage) {
    // Envoltorio historico: sin direccion (empuja hacia atras de SU facing), sin
    // knockback en golpes normales, hitstun 0.13 s y derribo con dano >= 28.
    HitInfo info;
    info.damage = damage;
    info.dirX = facing == Facing::Right ? -1.0f : 1.0f;
    info.knockback = 0.0f;
    info.hitstun = 0.13f;
    info.knockdown = damage >= 28;
    info.heavy = damage >= 28;
    ApplyHit(info);
}

HitResult Player::ApplyHit(const HitInfo& info) {
    const int damage = info.damage;
    if (damage <= 0 || state == PlayerState::Defeat || state == PlayerState::Victory) return HitResult::Ignored;
    if (debugInvulnerable || dashInvulnerability > 0) return HitResult::Ignored;
    const float dir = info.dirX >= 0.0f ? 1.0f : -1.0f;   // hacia donde empuja el golpe

    shieldRegenTimer = 1.25f;

    if (IsBlocking() && !info.unblockable) {
        const int reduced = std::max(1, static_cast<int>(std::round(damage * (1.f - blockDamageReduction))));
        const int absorbed = std::min(shield, reduced);
        shield -= absorbed;
        hp = std::max(0, hp - reduced + absorbed);
        AddRage(12);
        velocity.x += dir * info.pushOnBlock;   // empuje de bloqueo (se integra: B09)
        dashInvulnerability = 0.12f;
        EmitPlayerEvent(*this, GameEventType::PlayerBlocked, info.heavy, damage);
        if (hp == 0) { SetState(PlayerState::Defeat); return HitResult::Killed; }
        if (shield == 0) {
            SetState(PlayerState::GuardBreak);
            EmitPlayerEvent(*this, GameEventType::PlayerGuardBreak, true, damage);
        }
        return HitResult::Blocked;
    }

    if (state == PlayerState::Hit && stateTimer > 0) return HitResult::Ignored;
    if (state == PlayerState::Knockdown || state == PlayerState::Airborne) return HitResult::Ignored;

    hp = std::max(0, hp - damage);
    AddRage(15);
    hurtShakeTimer = 0.24f;
    hurtShakeAmp = VictimShakeAmplitude(damage);
    hurtShakeDir = dir;

    if (hp == 0) {
        EmitPlayerEvent(*this, GameEventType::PlayerHurt, true, damage);
        SetState(PlayerState::Defeat);
        return HitResult::Killed;
    }

    // Habilidad en curso (activeSkill >= 0): SUPERARMADURA total (regla del
    // usuario): el golpe quita vida pero no la cancela ni la derriba; solo tinte
    // rojo breve y un retroceso leve en la direccion del golpe.
    // Especiales sin ser habilidad (onda, furia, finisher, especial KF): armadura
    // solo durante arranque + activo, no en la recuperacion (B19).
    bool armored = false;
    if (state == PlayerState::Attack) {
        if (activeSkill >= 0) {
            armored = true;
        } else if (currentAttack == AttackId::Skill || currentAttack == AttackId::EnergyWave ||
                   currentAttack == AttackId::RageAttack || currentAttack == AttackId::Finisher) {
            const AttackDef& def = CurrentAttackDef();
            armored = clipDriven ? attackElapsed <= attackDuration * 0.92f
                                 : attackElapsed <= def.startup + def.active;
        } else if (!clipDriven) {
            // F0.4: ventana de armadura de datos (armorFrom..armorTo, armorHits golpes).
            const AttackDef& def = CurrentAttackDef();
            if (def.armorFrom >= 0.0f && attackElapsed >= def.armorFrom && attackElapsed <= def.armorTo &&
                (def.armorHits <= 0 || armorAbsorbed < def.armorHits)) {
                armored = true;
                ++armorAbsorbed;
            }
        }
    }
    if (armored) {
        // En Attack la posicion la mueve el root motion; el retroceso leve es un
        // desplazamiento directo y pequeno.
        abilityHitFlashTimer = 0.16f;
        position.x += dir * 10.f;
        position.x = std::clamp(position.x, kStageStartX, kStageEndX - 90.f);
        EmitPlayerEvent(*this, GameEventType::PlayerHurt, false, damage);
        return HitResult::Armored;
    }

    // Derribo por propiedad del golpe (B17): el envoltorio TakeDamage(int)
    // conserva el umbral historico de dano >= 28. F0.4: golpeado en el aire
    // (salto o aereo) siempre cae derribado.
    if (info.knockdown || position.z > 1.0f) {
        knockdownTimer = 0.85f;
        velocity.x = dir * 260.f;   // en la direccion del golpe (B10)
        velocity.z = 380.f;
        SetState(PlayerState::Airborne);
        EmitPlayerEvent(*this, GameEventType::PlayerHurt, true, damage);
        EmitPlayerEvent(*this, GameEventType::PlayerKnockdown, true, damage);
        return HitResult::Hit;
    }

    stateTimer = std::max(0.13f, info.hitstun);
    hitstunTimer = std::max(0.18f, info.hitstun);
    velocity.x = dir * info.knockback;   // retroceso del golpe (0 en el envoltorio)
    SetState(PlayerState::Hit);
    dashInvulnerability = 0.28f;
    EmitPlayerEvent(*this, GameEventType::PlayerHurt, false, damage);
    return HitResult::Hit;
}

void Player::ApplyCharacter(int id) {
    skin = id;
    animator = Animator{};                     // se reinicia con el atlas del personaje
    altAnimator = Animator{}; usingAlt = false; altTried = false;
    if (id == 0) return;                       // Rayden original: intacto
    const CharacterVisual& cv = GetCharacterVisual(id);
    // Los personajes con hoja completa (Rayder) conservan las mejoras de campana.
    maxHp = cv.maxHp + (cv.atlasId != nullptr ? upgrades.bonusMaxHp : 0);
    hp = maxHp;
}

}  // namespace district_fury
