// Jugador: entrada (teclado/tactil), Input Buffer, comandos especiales, combos y cancelaciones.
#include "game/player/PlayerCommon.h"

namespace district_fury {

bool Player::IsKfCharacter() const {
    const CharacterVisual& cv = GetCharacterVisual(skin);
    return cv.kfRoster >= 0 || cv.kfMoves;
}

float Player::ClipSeconds(const char* clip) const { return animator.ClipSeconds(clip); }

namespace {
const KfReference* CurrentKf(const Player& p) {
    const CharacterVisual& cv = GetCharacterVisual(p.skin);
    const int roster = (p.usingAlt && cv.transformKfRoster >= 0) ? cv.transformKfRoster : cv.kfRoster;
    if (roster < 0) return nullptr;
    const KfReference& k = GetKfCharacter(roster);
    return k.loaded ? &k : nullptr;
}
}  // namespace

int Player::AbilityCount() const {
    const KfReference* k = CurrentKf(*this);
    return k ? static_cast<int>(k->abilityClips.size()) : 0;
}

int Player::SkillPageCount() const {
    const int n = AbilityCount();
    return n > 0 ? (n + 4) / 5 : 1;
}

int Player::AbilityForSlot(int i) const {
    if (i < 0 || i >= kSkillCount - 1) return -1;
    const int n = AbilityCount();
    if (n <= 0) return -1;
    const int idx = (skillPage % SkillPageCount()) * 5 + i;
    return idx < n ? idx : -1;
}

float Player::SkillCooldownLeft(int i) const {
    if (i < 0 || i >= kSkillCount) return 1.0f;
    if (i == kSkillCount - 1 || !IsKfCharacter()) return skillCooldown[i];
    const int a = AbilityForSlot(i);
    if (a < 0) return 1.0f;                                   // boton sin habilidad en esta pagina
    return a < static_cast<int>(abilityCooldown.size()) ? abilityCooldown[static_cast<size_t>(a)] : 0.0f;
}

const char* Player::SkillName(int i) const {
    // Nombres generales por ahora: todos los personajes tienen 6 habilidades.
    static const char* kOurs[kSkillCount] = {"ONDA", "GANCHO", "TORBELLINO", "EMBESTIDA", "REMATE", "TRANSFORMAR"};
    static const char* kKfHero[kSkillCount] = {"SOMBRAS", "ESTALLIDO", "PILAR", "LLAMARADA", "FENIX", "TRANSFORMAR"};
    static const char* kKfOther[kSkillCount] = {"TECNICA 1", "TECNICA 2", "TECNICA 3", "TECNICA 4", "TECNICA 5", "TRANSFORMAR"};
    if (i < 0 || i >= kSkillCount) return "";
    if (!IsKfCharacter()) return kOurs[i];
    if (i < kSkillCount - 1) {
        const KfReference* k = CurrentKf(*this);
        const int a = AbilityForSlot(i);
        if (k && a >= 0 && a < static_cast<int>(k->abilityNames.size())) return k->abilityNames[static_cast<size_t>(a)].c_str();
        if (k && a < 0) return "-";
    }
    if (GetCharacterVisual(skin).kfMoves) return kKfHero[i];   // copia del heroe KF
    const int sprite = KfRoster(GetCharacterVisual(skin).kfRoster).sprite;
    return (sprite == 0 || sprite == 2) ? kKfHero[i] : kKfOther[i];   // los dos heroes traen 5 habilidades propias
}

// Ataque cuya duracion y golpes los marca la animacion (habilidades y
// personajes KF): se reproduce completa, con el fuego/estela de su propio arte.
void Player::BeginClipAttack(const char* clip, int skillIndex) {
    currentAttack = AttackId::Skill;
    attackType = AttackType::None;
    attackPhase = AttackPhase::Special;
    recoveryTimer = 0.0f;
    state = PlayerState::Attack;
    attackElapsed = 0;
    hasHit = false;
    energyReleased = false;
    activeSkill = skillIndex;
    clipDriven = true;
    multiHitTimer = 0.0f;
    if (!animator.PlayNamed(clip) && !animator.PlayNamed("special")) animator.PlayNamed("idle");
    attackDuration = std::max(0.45f, animator.ClipSeconds(animator.currentClipName));
    stateTimer = attackDuration;
    AudioSystem::Get().Play(skillIndex == kSkillCount - 1 ? Sfx::FuryCharge : Sfx::EnergyShot);
}

bool Player::TryStartSkill(int i) {
    if (!inputEnabled || i < 0 || i >= kSkillCount || !SkillReady(i)) return false;
    switch (state) {
        case PlayerState::Idle: case PlayerState::Walk: case PlayerState::Recovery:
        case PlayerState::Block: case PlayerState::Dash: break;
        case PlayerState::Attack:
            // Como en King Fighter, una habilidad corta un golpe normal (no otra habilidad).
            if (activeSkill >= 0 || currentAttack == AttackId::Skill) return false;
            break;
        default: return false;
    }
    const int ability = IsKfCharacter() ? AbilityForSlot(i) : -1;
    if (ability >= 0) {
        if (abilityCooldown.size() < static_cast<size_t>(AbilityCount())) abilityCooldown.resize(static_cast<size_t>(AbilityCount()), 0.0f);
        abilityCooldown[static_cast<size_t>(ability)] = kSkillCooldown;
    } else {
        skillCooldown[i] = kSkillCooldown;
    }
    inputBuffer.Clear();
    if (i == kSkillCount - 1) {   // transformacion: aura dorada, mas dano y velocidad
        transformTimer = kTransformDuration;
        if (!IsKfCharacter()) {   // estallido azul y onda en el suelo
            SpawnFx(24, 8, {position.x, position.y - 60.0f}, 0.9f, 1.6f);
            SpawnFx(23, 1, {position.x, position.y - 4.0f}, 0.5f, 1.4f);
        }
        BeginClipAttack(animator.HasClip("transform") ? "transform" : animator.HasClip("victory") ? "victory" : "energy", i);
        if (!animator.HasClip("transform")) { attackDuration = std::min(attackDuration, 0.8f); stateTimer = attackDuration; }
        return true;
    }
    if (IsKfCharacter()) {
        const KfReference* k = CurrentKf(*this);
        static const char* kClips[] = {"skill1", "skill2", "skill3", "skill4", "skill5"};
        BeginClipAttack(k && ability >= 0 ? k->abilityClips[static_cast<size_t>(ability)].c_str() : kClips[i], i);
        return true;
    }
    // Nuestros personajes: sus propios movimientos, sin gastar energia.
    static const AttackId kOurSkills[] = {AttackId::EnergyWave, AttackId::Punch3, AttackId::RageAttack,
                                          AttackId::DashAttack, AttackId::Finisher};
    const int spBefore = sp;
    BeginAttack(kOurSkills[i]);
    sp = spBefore;
    attackCooldowns[static_cast<int>(kOurSkills[i])] = 0.0f;
    activeSkill = i;
    return true;
}

void Player::BeginAttack(AttackId id) {
    if (IsKfCharacter() && (id == AttackId::EnergyWave || id == AttackId::RageAttack || id == AttackId::Finisher)) {
        // Un personaje KF lanza SU especial/super (fuego propio), no nuestra onda azul.
        const AttackDef& orig = GetAttack(id);
        if (orig.cooldown > 0.0f) attackCooldowns[static_cast<int>(id)] = orig.cooldown;
        if (orig.spCost > 0) sp = std::max(0, sp - orig.spCost);
        BeginClipAttack(id == AttackId::RageAttack ? "super" : "special", -1);
        return;
    }
    activeSkill = -1;
    clipDriven = false;
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
    if (IsKfCharacter()) {
        // KF: la animacion completa (estela y pasos incluidos) manda; los golpes
        // se repiten mientras dura y un nuevo J/K la puede cortar.
        clipDriven = true;
        multiHitTimer = 0.0f;
        attackDuration = std::max(attackDuration, animator.ClipSeconds(ClipNameFor(id)) * 0.92f);
        stateTimer = attackDuration;
    }

    // Al lanzar: silbido del golpe (el impacto suena al conectar, ver PlayerDraw).
    switch (id) {
        case AttackId::EnergyWave: AudioSystem::Get().Play(Sfx::EnergyCharge); break;
        case AttackId::RageAttack: AudioSystem::Get().Play(Sfx::Rage); break;
        case AttackId::Finisher:   AudioSystem::Get().Play(Sfx::FuryCharge); break;
        default:                   AudioSystem::Get().Play(Sfx::Whoosh); break;
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
    if (frameInput.skillPage) skillPage = (skillPage + 1) % std::max(1, SkillPageCount());
    if (frameInput.skill >= 0 && TryStartSkill(frameInput.skill)) return;
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
    in.skillPage = input::Pressed(KEY_TAB) || input::Pressed(input::kVirtualSkillPage);
    for (int i = 0; i < kSkillCount; ++i)
        if (input::Pressed(KEY_ONE + i) || input::Pressed(input::kVirtualSkill0 + i)) in.skill = i;
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
    if (clipDriven) return attackElapsed >= std::max(def.startup + def.active, attackDuration * 0.38f);
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
    activeSkill = -1;
    clipDriven = false;
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
    if (clipDriven) {
        if (activeSkill == kSkillCount - 1) return false;   // transformacion: sin dano
        const float t = attackElapsed / std::max(0.01f, attackDuration);
        return t >= 0.12f && t <= 0.92f;
    }
    const AttackDef& def = GetAttack(currentAttack);
    return attackElapsed >= def.startup && attackElapsed <= def.startup + def.active;
}

int Player::GetAttackDamage() const {
    const AttackDef& def = GetAttack(currentAttack);
    float damage = static_cast<float>(def.damage);
    damage *= def.spawnsProjectile ? upgrades.energyDamageScale : upgrades.attackDamageScale;
    if (clipDriven && currentAttack != AttackId::Skill) damage *= 0.6f;   // KF: varios golpes por animacion
    if (IsTransformed()) damage *= 1.4f;
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
    const SpriteFrame* f = clipDriven ? animator.CurrentFrameData() : nullptr;
    if (f && f->width > 0.0f) {
        // La caja la marca el dibujo: llega hasta donde llegan el puno, la
        // patada o el fuego de la habilidad en este frame.
        const float sc = SpriteScale();
        const float front = std::max(60.0f, (f->width - f->pivotX) * sc);
        const float back = std::min(f->pivotX * sc, currentAttack == AttackId::Skill ? 400.0f : 30.0f);
        const float top = std::clamp(f->pivotY * sc, 80.0f, 260.0f);
        const float x0 = direction > 0 ? position.x - back : position.x - front;
        return {x0, position.y - top, front + back, top + 10.0f};
    }
    const float centerX = position.x + direction * def.boxForward;
    const float centerY = position.y + def.boxCenterOffsetY;
    return {centerX - def.boxWidth * 0.5f, centerY - def.boxHeight * 0.5f,
            def.boxWidth, def.boxHeight};
}

}  // namespace district_fury
