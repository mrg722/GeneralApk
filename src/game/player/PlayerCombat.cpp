// Jugador: entrada (teclado/tactil), Input Buffer, comandos especiales, combos y cancelaciones.
#include "game/player/PlayerCommon.h"

namespace district_fury {

void Player::BeginAttack(AttackId id) {
    const AttackDef& def = GetAttack(id);
    currentAttack = id;
    attackType = LegacyType(id);
    attackPhase = PhaseFor(id);
    recoveryTimer = 0.0f;
    state = PlayerState::Attack;
    attackElapsed = 0;
    attackDuration = AttackTotalDuration(def);
    stateTimer = attackDuration;
    hasHit = false;
    energyReleased = false;
    if (def.cooldown > 0.0f) attackCooldowns[static_cast<int>(id)] = def.cooldown;
    if (def.spCost > 0) sp = std::max(0, sp - def.spCost);

    if (animator.PlayNamed(ClipNameFor(id))) {
        // Clip dinamico: el manifiesto manda la duracion por cuadro.
    } else {
    const ClipRange clip = ClipFor(id);
    const int start = animator.normalizedAtlas ? clip.cleanStart : clip.legacyStart;
    const int end = animator.normalizedAtlas ? clip.cleanEnd : clip.legacyEnd;
    animator.Play({start, end, clip.frameTime, false});
    }

    switch (id) {
        case AttackId::EnergyWave: AudioSystem::Get().Play(Sfx::EnergyCharge); break;
        case AttackId::Kick:
        case AttackId::Finisher:   AudioSystem::Get().Play(Sfx::Kick); break;
        case AttackId::RageAttack: AudioSystem::Get().Play(Sfx::Rage); break;
        default:                   AudioSystem::Get().Play(Sfx::Punch); break;
    }
}

void Player::OnAttackConnected(const AttackDef& def) {
    if (def.canCombo) {
        comboWindow = def.comboWindow + upgrades.comboWindowBonus;
        ++comboCount;
    } else {
        comboWindow = std::max(comboWindow, 0.18f);
    }
    AddRage(def.rageGain);
}

void Player::HandleInput(float dt) {
    if (!inputEnabled) return;

    const PlayerInput& in = frameInput;
    if (in.block) {
        attackType = AttackType::None;
        currentAttack = AttackId::None;
        if (state != PlayerState::Block) SetState(PlayerState::Block);
        blockTimer += dt;
        return;
    }
    if (state == PlayerState::Block) SetState(PlayerState::Idle);

    Vector2 move = {in.moveX, in.moveY};

    const bool moving = move.x != 0 || move.y != 0;
    const float speed = GetMoveSpeed();
    if (moving) {
        const float length = std::sqrt(move.x * move.x + move.y * move.y);
        position.x += move.x / length * speed * dt;
        position.y += move.y / length * speed * 0.70f * dt;
        if (move.x < 0) facing = Facing::Left;
        if (move.x > 0) facing = Facing::Right;
        if (state != PlayerState::Walk) SetState(PlayerState::Walk);
    } else if (state == PlayerState::Walk) {
        SetState(PlayerState::Idle);
    }

    // Los carriles se acotan aqui, no en cada stage. Stage2 no lo hacia (DF-013 D3).
    position.y = std::clamp(position.y, kLaneMinY, kLaneMaxY);

    if (in.dash && dashCooldown <= 0.0f) {
        dashTimer = 0.12f;
        dashInvulnerability = 0.20f;
        dashCooldown = 0.38f * upgrades.dashCooldownScale;
        SetState(PlayerState::Dash);
        AudioSystem::Get().Play(Sfx::Dash);
        return;
    }

    // J/K llegan por el Input Buffer (ver PollAttackInput): un comando pulsado
    // hasta 15 frames antes sigue siendo valido.
    if (TryStartBufferedAttack(false)) return;

    if (in.energy && sp >= GetAttack(AttackId::EnergyWave).spCost &&
        attackCooldowns[static_cast<int>(AttackId::EnergyWave)] <= 0.0f) {
        BeginAttack(AttackId::EnergyWave);
        return;
    }

    if (in.rage) {
        if (rageState == RageState::Normal && rage >= maxRage) {
            rageState = RageState::Starting;
            rageStateTimer = kRageStartDuration;
            rageDrainAccumulator = 0;
            AudioSystem::Get().Play(Sfx::Rage);
            return;
        }
        if (rageState == RageState::Active &&
            attackCooldowns[static_cast<int>(AttackId::RageAttack)] <= 0.0f) {
            BeginAttack(AttackId::RageAttack);
            return;
        }
    }

    spRegenAccumulator += dt;
    while (spRegenAccumulator >= 0.2f && sp < maxSp) { ++sp; spRegenAccumulator -= 0.2f; }
}

void Player::PollAttackInput() {
    if (!inputEnabled) return;
    if (state == PlayerState::Defeat || state == PlayerState::Knockdown ||
        state == PlayerState::GuardBreak || state == PlayerState::Hit) return;
    // Historial de direcciones para los comandos especiales.
    const int h = frameInput.moveX > 0.5f ? 1 : (frameInput.moveX < -0.5f ? -1 : 0);
    motion.Record(frameInput.moveY > 0.5f, h, inputBuffer.Frame());
    const int facingDir = facing == Facing::Right ? 1 : -1;
    if (frameInput.specialWave) inputBuffer.Push(InputCommand::SpecialWave, h != 0 ? h : facingDir);
    if (frameInput.specialRise) inputBuffer.Push(InputCommand::SpecialRise, h != 0 ? h : facingDir);
    if (frameInput.punch) {
        const int dir = motion.QuarterCircle(inputBuffer.Frame());
        if (dir != 0) { inputBuffer.Push(InputCommand::SpecialWave, dir); motion.Clear(); }
        else inputBuffer.Push(InputCommand::Punch);
    }
    if (frameInput.kick) {
        const int dir = motion.DragonPunch(inputBuffer.Frame());
        if (dir != 0) { inputBuffer.Push(InputCommand::SpecialRise, dir); motion.Clear(); }
        else inputBuffer.Push(InputCommand::Kick);
    }
}

PlayerInput Player::ReadInput() const {
    if (scriptedInput) return *scriptedInput;
    PlayerInput in;
    if (input::Down(KEY_W)) in.moveY -= 1;
    if (input::Down(KEY_S)) in.moveY += 1;
    if (input::Down(KEY_A)) in.moveX -= 1;
    if (input::Down(KEY_D)) in.moveX += 1;
    in.block = input::Down(KEY_B);
    in.dash = input::Pressed(KEY_LEFT_SHIFT);
    in.punch = input::Pressed(KEY_J);
    in.kick = input::Pressed(KEY_K);
    in.energy = input::Pressed(KEY_L);
    in.rage = input::Pressed(KEY_SPACE);
    in.specialWave = input::Pressed(input::kVirtualSpecialWave);
    in.specialRise = input::Pressed(input::kVirtualSpecialRise);
    return in;
}

void Player::PumpInput(float dt) {
    inputPumped = true;
    frameInput = inputEnabled ? ReadInput() : PlayerInput{};
    inputBuffer.Tick(dt);          // avanza frames y elimina comandos > 15 frames
    PollAttackInput();
}

bool Player::InCancelWindow() const {
    if (state != PlayerState::Attack) return false;
    if (attackPhase == AttackPhase::None || attackPhase == AttackPhase::Special) return false;
    const AttackDef& def = GetAttack(currentAttack);
    return attackElapsed >= def.startup + def.active;   // hitbox ya termino: winddown
}

// Consume el comando mas antiguo del buffer y arranca el ataque que le
// corresponde. fromCancel=true aplica la tabla de cancelacion sobre el ataque en
// curso; false resuelve un inicio normal desde Idle/Walk/Recovery.
bool Player::TryStartBufferedAttack(bool fromCancel) {
    inputBuffer.PruneExpired();
    const InputBuffer::Entry* front = inputBuffer.Peek();
    if (!front) return false;
    const InputCommand cmd = front->command;
    const int dir = front->dir;

    AttackId next = AttackId::None;
    if (fromCancel) {
        next = ResolveCancel(currentAttack, cmd);
    } else if (cmd == InputCommand::SpecialWave) {
        next = AttackId::EnergyWave;
    } else if (cmd == InputCommand::SpecialRise) {
        next = AttackId::Punch3;
    } else if (cmd == InputCommand::Kick) {
        const bool finisher = isRageMode && comboCount >= 4 &&
            sp >= GetAttack(AttackId::Finisher).spCost &&
            attackCooldowns[static_cast<int>(AttackId::Finisher)] <= 0.0f;
        next = finisher ? AttackId::Finisher : AttackId::Kick;
    } else {
        next = comboWindow > 0.0f ? NextPunchInChain(currentAttack) : AttackId::Punch1;
    }
    if (next == AttackId::None) return false;
    if (attackCooldowns[static_cast<int>(next)] > 0.0f) return false;   // queda en cola
    // Sin energia suficiente, el comando de onda sale como golpe normal.
    if (next == AttackId::EnergyWave && sp < GetAttack(AttackId::EnergyWave).spCost) {
        if (fromCancel) return false;
        next = comboWindow > 0.0f ? NextPunchInChain(currentAttack) : AttackId::Punch1;
    }
    if (dir != 0) facing = dir > 0 ? Facing::Right : Facing::Left;

    InputCommand consumed;
    inputBuffer.Pop(consumed);
    comboStep = next == AttackId::Punch1 ? 0 : next == AttackId::Punch2 ? 1
              : next == AttackId::Punch3 ? 2 : 3;
    BeginAttack(next);
    return true;
}

// Fin natural del ataque: si hay comando en cola se encadena; si no, el
// personaje pasa por Recovery antes de volver a Idle/Walk.
void Player::EndAttack() {
    const bool heavy = GetAttack(currentAttack).heavy;
    attackType = AttackType::None;
    if (inputEnabled && TryStartBufferedAttack(false)) return;
    SetState(PlayerState::Recovery);
    recoveryTimer = heavy ? kRecoveryHeavy : kRecoveryLight;
}

const char* Player::AttackPhaseName() const {
    switch (attackPhase) {
        case AttackPhase::Punch1:  return "ATTACK_PUNCH_1";
        case AttackPhase::Punch2:  return "ATTACK_PUNCH_2";
        case AttackPhase::Punch3:  return "ATTACK_PUNCH_3";
        case AttackPhase::Kick:    return "ATTACK_KICK";
        case AttackPhase::Special: return "ATTACK_SPECIAL";
        default:                   return "NONE";
    }
}

bool Player::AttackIsActive() const {
    if (state != PlayerState::Attack) return false;
    const AttackDef& def = GetAttack(currentAttack);
    return attackElapsed >= def.startup && attackElapsed <= def.startup + def.active;
}

int Player::GetAttackDamage() const {
    const AttackDef& def = GetAttack(currentAttack);
    float damage = static_cast<float>(def.damage);
    damage *= def.spawnsProjectile ? upgrades.energyDamageScale : upgrades.attackDamageScale;
    if (isRageMode) damage *= 1.35f;
    if (damageBuffTimer > 0.0f) damage *= 1.25f;
    return std::max(1, static_cast<int>(std::round(damage)));
}

float Player::GetAttackRange() const { return GetAttack(currentAttack).range; }

float Player::GetAttackDepthRange() const { return GetAttack(currentAttack).depth; }

float Player::GetAttackKnockback() const { return GetAttack(currentAttack).knockback; }

CombatBox Player::GetAttackHitbox() const {
    if (!AttackIsActive()) return {};
    const AttackDef& def = GetAttack(currentAttack);
    const float direction = facing == Facing::Right ? 1.f : -1.f;
    const float centerX = position.x + direction * def.boxForward;
    const float centerY = position.y + def.boxCenterOffsetY;
    return {centerX - def.boxWidth * 0.5f, centerY - def.boxHeight * 0.5f,
            def.boxWidth, def.boxHeight};
}

}  // namespace district_fury
