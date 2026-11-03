// Jugador: entrada (teclado/tactil), Input Buffer, comandos especiales, combos y cancelaciones.
#include "game/player/PlayerCommon.h"
#include <cstring>

namespace district_fury {

bool Player::IsKfCharacter() const {
    const CharacterVisual& cv = GetCharacterVisual(skin);
    return cv.kfRoster >= 0 || cv.kfMoves;
}

bool Player::IsRayder() const {
    // Skin 6: se mostraba como "RAYDER", ahora renombrado a "MAX" para no
    // confundirlo con Rayden Cruz (skin 0, rayden_128.png, otro personaje).
    // El nombre de la funcion se deja igual (es el mismo atlas/moveset).
    const char* name = GetCharacterVisual(skin).name;
    return name && std::string(name) == "MAX";
}

bool Player::IsRonin() const {
    const char* name = GetCharacterVisual(skin).name;
    return name && std::string(name) == "RONIN";
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
    projectileFired = false; projectilePending = false;
    hitTargets.clear(); multiHitClock = 0.0f; attackConnected = false;
    attackPrevElapsed = 0.0f;
    activeSkill = skillIndex;
    clipDriven = true;
    multiHitTimer = 0.0f;
    if (!animator.PlayNamed(clip) && !animator.PlayNamed("special")) animator.PlayNamed("idle");
    attackDuration = std::max(0.45f, animator.ClipSeconds(animator.currentClipName));
    stateTimer = attackDuration;
    // value = boton (5 = transformar); el AudioDirector elige el sonido.
    EmitPlayerEvent(*this, GameEventType::SpecialStart, true, skillIndex, AttackId::Skill);
}

bool Player::TryStartSkill(int i) {
    if (!inputEnabled || i < 0 || i >= kSkillCount || !SkillReady(i)) return false;
    switch (state) {
        case PlayerState::Idle: case PlayerState::Walk: case PlayerState::Recovery:
        case PlayerState::Block: case PlayerState::Dash: break;
        case PlayerState::Attack:
            // Como en King Fighter, una habilidad corta un golpe normal (no otra habilidad).
            if (activeSkill >= 0 || currentAttack == AttackId::Skill) return false;
            if (CurrentAttackDef().air) return false;
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
    // Nuestros personajes: MoveSet.skillSlots (F0.4; por defecto ONDA, GANCHO
    // ASCENDENTE, FURIA, EMBESTIDA, FINISHER), sin gastar energia.
    AttackId id = static_cast<AttackId>(Moves().skillSlots[i]);
    if (!MoveEnabled(id)) id = static_cast<AttackId>(DefaultSkillSlot(i));
    if (!MoveEnabled(id)) id = AttackId::Punch3;
    const int spBefore = sp;
    BeginAttack(id);
    sp = spBefore;
    attackCooldowns[static_cast<int>(id)] = 0.0f;
    activeSkill = i;
    return true;
}

// ---------------------------------------------------------------------------
// MoveSet efectivo, definicion efectiva y clips (F0.4)
// ---------------------------------------------------------------------------

namespace {
// MoveSet "legado" de los personajes por clip (KF/BETA/RONIN) mientras su bloque
// no declare uno: la cadena y las entradas de siempre.
const MoveSet& LegacyClipMoveSet() {
    static const MoveSet kLegacy = [] {
        MoveSet m;
        m.name = "legado-clip";
        m.caps = 0;
        m.feel = FeelLevel::Full;
        m.chainJ[0] = static_cast<int>(AttackId::Punch1);
        m.chainJ[1] = static_cast<int>(AttackId::Punch2);
        m.chainJ[2] = static_cast<int>(AttackId::Punch3);
        m.chainJCount = 3;
        m.chainK[0] = static_cast<int>(AttackId::Kick);
        m.chainKCount = 1;
        FillDefaultChains(m);
        return m;
    }();
    return kLegacy;
}

bool SameClip(const AnimationClip& a, const AnimationClip& b) {
    return a.frames == b.frames && a.startFrame == b.startFrame && a.endFrame == b.endFrame;
}
}  // namespace

const MoveSet& Player::Moves() const {
    if (movesSkin_ == skin && movesCustom_ == customMoveSet) return moves_;
    movesSkin_ = skin;
    movesCustom_ = customMoveSet;
    if (customMoveSet) {
        moves_ = *customMoveSet;
    } else {
        const MoveSet& reg = GetMoveSet(skin);
        moves_ = (&reg == &GenericMoveSet() && IsKfCharacter()) ? LegacyClipMoveSet() : reg;
    }
    FillDefaultChains(moves_);
    moves_.chainJCount = std::clamp(moves_.chainJCount, 1, 6);
    moves_.chainKCount = std::clamp(moves_.chainKCount, 1, 4);
    effectiveId_ = AttackId::Count;   // la definicion efectiva depende del MoveSet
    return moves_;
}

const MoveOverride* Player::OverrideFor(AttackId id) const {
    if (id == AttackId::None || id == AttackId::Skill) return nullptr;
    return FindOverride(Moves(), static_cast<int>(id));
}

AttackDef Player::EffectiveDef(AttackId id) const {
    const MoveOverride* ov = OverrideFor(id);
    AttackDef d = (ov && ov->replaceDef) ? ov->def : GetAttack(id);
    if (ov == nullptr) return d;
    if (ov->damageScale != 1.0f) d.damage = std::max(1, static_cast<int>(std::round(d.damage * ov->damageScale)));
    if (ov->timeScale != 1.0f && ov->timeScale > 0.0f) {
        const float k = ov->timeScale;
        d.startup *= k; d.active *= k; d.recovery *= k; d.multiHitInterval *= k;
        if (d.cancelFrom >= 0.0f) d.cancelFrom *= k;
        if (d.armorFrom >= 0.0f) { d.armorFrom *= k; d.armorTo *= k; }
        d.invulnTo *= k; d.landRecovery *= k;
    }
    if (ov->reachScale != 1.0f && ov->reachScale > 0.0f) {
        const float k = ov->reachScale;
        d.range *= k; d.boxWidth *= k; d.boxForward *= k;
        d.arc.L0 *= k; d.arc.L1 *= k;
    }
    if (ov->hasArc) { d.arc = ov->arc; d.hasArc = true; }
    if (ov->root.dx != 0.0f || ov->root.dz != 0.0f) d.root = ov->root;
    if (ov->hitCount > 1) {
        d.hitCount = ov->hitCount;
        if (ov->multiHitInterval > 0.0f) d.multiHitInterval = ov->multiHitInterval;
        else if (d.multiHitInterval <= 0.0f) d.multiHitInterval = d.active / static_cast<float>(ov->hitCount);
    } else if (ov->multiHitInterval > 0.0f) {
        d.multiHitInterval = ov->multiHitInterval;
    }
    if (ov->armorFrom >= 0.0f) { d.armorFrom = ov->armorFrom; d.armorTo = ov->armorTo; d.armorHits = ov->armorHits; }
    if (ov->projectile.enabled) d.spawnsProjectile = true;
    d.requiresOverride = false;   // con override completo deja de estar apagado
    if (ov->displayName) d.name = ov->displayName;
    return d;
}

const AttackDef& Player::CurrentAttackDef() const {
    const MoveSet& ms = Moves();
    if (effectiveId_ != currentAttack || effectiveSkin_ != skin || effectiveSet_ != &ms) {
        effectiveDef_ = EffectiveDef(currentAttack);
        effectiveId_ = currentAttack;
        effectiveSkin_ = skin;
        effectiveSet_ = &ms;
    }
    return effectiveDef_;
}

bool Player::CharacterHasRealClip(const char* name) const {
    if (name == nullptr || !animator.HasClip(name)) return false;
    if (std::strcmp(name, "idle") == 0) return true;
    const auto idle = animator.namedClips.find("idle");
    if (idle == animator.namedClips.end()) return true;
    return !SameClip(animator.namedClips.at(name), idle->second);
}

const char* Player::ClipOriginName(ClipOrigin o) {
    switch (o) {
        case ClipOrigin::Override:  return "override";
        case ClipOrigin::Canonical: return "canonico";
        case ClipOrigin::Fallback:  return "fallback";
        case ClipOrigin::Historic:  return "historico";
        case ClipOrigin::Pose:      return "pose";
        case ClipOrigin::Idle:      return "IDLE";
    }
    return "?";
}

const char* Player::AttackClipFor(AttackId id, ClipOrigin* origin) const {
    auto set = [&](ClipOrigin o, const char* c) { if (origin) *origin = o; return c; };
    const MoveOverride* ov = OverrideFor(id);
    // Poses sueltas (carpeta, sin atlas): las dibuja la tabla de poses o el
    // MoveVisual.segs; el animador no cuenta.
    const CharacterVisual& cv = GetCharacterVisual(skin);
    if (cv.folder != nullptr && cv.atlasId == nullptr && cv.kfRoster < 0 && !cv.kfMoves)
        return set(ClipOrigin::Pose, "pose");
    if (ov && ov->visual.segCount > 0) return set(ClipOrigin::Override, "segs");
    if (!animator.HasClips()) return set(ClipOrigin::Historic, "indices");   // atlas sin nombres (o sin ventana)
    if (ov && ov->visual.clipCount > 0 && CharacterHasRealClip(ov->visual.clips[0].clip))
        return set(ClipOrigin::Override, ov->visual.clips[0].clip);
    if (CharacterHasRealClip(ClipNameFor(id))) return set(ClipOrigin::Canonical, ClipNameFor(id));
    if (ov && CharacterHasRealClip(ov->visual.clipFallback)) return set(ClipOrigin::Fallback, ov->visual.clipFallback);
    for (const char* const* c = HistoricClipsFor(id); *c; ++c)
        if (CharacterHasRealClip(*c)) return set(ClipOrigin::Historic, *c);
    for (const char* const* c = AnyAttackClips(); *c; ++c)
        if (CharacterHasRealClip(*c)) return set(ClipOrigin::Historic, *c);
    return set(ClipOrigin::Idle, nullptr);
}

bool Player::MoveEnabled(AttackId id) const {
    if (id == AttackId::None || id == AttackId::Count) return false;
    if (id == AttackId::Skill) return true;
    const MoveSet& ms = Moves();
    const MoveOverride* ov = OverrideFor(id);
    if (ov && !ov->enabled) return false;
    const AttackDef& base = GetAttack(id);
    // Signature1-3, Chain5-6 y Fase 2: solo con un override COMPLETO (replaceDef).
    if (base.requiresOverride && !(ov && ov->enabled && ov->replaceDef)) return false;
    const unsigned caps = ms.caps;
    switch (id) {
        case AttackId::FwdPunch: case AttackId::FwdKick: case AttackId::BackElbow: case AttackId::SpinKick:
            if (!(caps & CapCommandNormals)) return false; break;
        case AttackId::Sweep: case AttackId::Stomp:
            if (!(caps & CapSweep)) return false; break;
        case AttackId::KickLauncher: case AttackId::Uppercut:
            if (!(caps & CapLauncher)) return false; break;
        case AttackId::RisingUppercut: case AttackId::Hurricane: case AttackId::DrillKick:
            if (!(caps & CapSpecials)) return false; break;
        case AttackId::AirPunch: case AttackId::AirPunch2: case AttackId::AirKick: case AttackId::DiveKick:
        case AttackId::AirSpike:
            if (!(caps & CapAir) || !(caps & CapJump)) return false; break;
        case AttackId::DashKick:
            if (!(caps & CapCommandNormals) && !(caps & CapSweep)) return false; break;
        case AttackId::Grab: case AttackId::GrabKnee: case AttackId::ThrowForward: case AttackId::ThrowBack:
            if (!(caps & CapGrab)) return false; break;
        default: break;
    }
    // B18: nunca `idle` como ataque. Sin clip/pose con que dibujarlo, no se ofrece.
    ClipOrigin o;
    AttackClipFor(id, &o);
    return o != ClipOrigin::Idle;
}

ProjectileSpec Player::CurrentProjectile() const {
    if (const MoveOverride* ov = OverrideFor(currentAttack); ov && ov->projectile.enabled) return ov->projectile;
    ProjectileSpec s;   // la onda de siempre (los stages usan sus valores)
    s.enabled = CurrentAttackDef().spawnsProjectile;
    s.damage = CurrentAttackDef().damage;
    return s;
}

void Player::PlayAttackClip(AttackId id) {
    ClipOrigin origin = ClipOrigin::Historic;
    const char* clip = AttackClipFor(id, &origin);
    attackClipOrigin = origin;
    attackClipName = clip ? clip : "";
    const bool historic = static_cast<int>(id) <= static_cast<int>(AttackId::Finisher);
    const bool named = clip && origin != ClipOrigin::Pose && origin != ClipOrigin::Idle &&
                       std::strcmp(clip, "indices") != 0 && std::strcmp(clip, "segs") != 0;
    if (named && animator.PlayNamed(clip)) {
        if (!IsKfCharacter()) {
            if (historic && origin == ClipOrigin::Canonical) {
                // Los 8 historicos: exactamente como antes (perfil de skins 2 y 6).
                if (UsesAttackAnimationProfile(skin)) {
                    const auto& profile = GetAttackAnimationProfile(skin, id);
                    if (animator.HasClip(profile.clip)) animator.ScaleCurrentClipToDuration(attackDuration);
                }
            } else {
                animator.ScaleCurrentClipToDuration(attackDuration);   // el clip dura lo que el ataque
            }
        }
        return;
    }
    const ClipRange r = ClipFor(id);
    const int start = animator.normalizedAtlas ? r.cleanStart : r.legacyStart;
    const int end = animator.normalizedAtlas ? r.cleanEnd : r.legacyEnd;
    animator.Play({start, end, r.frameTime, false});
    if (origin != ClipOrigin::Pose && origin != ClipOrigin::Override) attackClipName = "indices";
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
    const bool fromAir = InAir() || state == PlayerState::Jump;
    activeSkill = -1;
    clipDriven = false;
    currentAttack = id;
    const AttackDef& def = CurrentAttackDef();
    attackType = LegacyType(id, def);
    attackPhase = PhaseFor(id);
    recoveryTimer = 0.0f;
    state = PlayerState::Attack;
    attackElapsed=0; attackDuration=AttackTotalDuration(def); attackImpactTriggered=false; attackRootMotionApplied=0.0f;
    stateTimer = attackDuration;
    hasHit = false;
    energyReleased = false;
    projectileFired = false; projectilePending = false;
    hitTargets.clear(); multiHitClock = 0.0f; attackConnected = false;
    attackPrevElapsed = 0.0f;
    attackHitsDone = 0; armorAbsorbed = 0; projectileShots = 0; projectileClock = 0.0f; rootAppliedX = 0.0f;
    walkVelX = walkVelY = 0.0f;
    chainAttack = id;
    if (def.air && fromAir) ++airAttacks;
    if (def.cooldown > 0.0f) attackCooldowns[static_cast<int>(id)] = def.cooldown;
    if (def.spCost > 0) sp = std::max(0, sp - def.spCost);
    if (def.invulnTo > 0.0f) dashInvulnerability = std::max(dashInvulnerability, def.invulnTo);

    PlayAttackClip(id);
    if (IsKfCharacter()) {
        // KF: la animacion completa (estela y pasos incluidos) manda; los golpes
        // se repiten mientras dura y un nuevo J/K la puede cortar.
        clipDriven = true;
        multiHitTimer = 0.0f;
        attackDuration = std::max(attackDuration, animator.ClipSeconds(animator.currentClipName) * 0.92f);
        stateTimer = attackDuration;
    }

    // Al lanzar: silbido del golpe / carga del especial (peso 1/2/3 segun dano).
    // El sonido lo elige el AudioDirector segun el personaje.
    const bool special = id == AttackId::EnergyWave || id == AttackId::RageAttack || id == AttackId::Finisher;
    const int weight = def.damage >= 20 ? 3 : def.damage >= 13 ? 2 : 1;
    EmitPlayerEvent(*this, special ? GameEventType::SpecialStart : GameEventType::Swing, def.heavy, activeSkill, id,
                    weight);
}

void Player::OnAttackConnected(const AttackDef& def) {
    // F0.2 (B03): lo llama SIEMPRE el HitResolver al conectar. Cada golpe que
    // conecta suma al combo; la ventana (con la mejora comboWindowBonus) deja
    // seguir la cadena despues del ataque.
    if (def.canCombo) {
        comboWindow = def.comboWindow + upgrades.comboWindowBonus;
    } else {
        comboWindow = std::max(comboWindow, std::max(0.18f, def.comboWindow) + upgrades.comboWindowBonus);
    }
    ++comboCount;
    attackConnected = true;
    // F0.4: lanzador que conecta -> 10 f para el salto de persecucion.
    if (def.jumpCancel && (Moves().caps & CapJump)) jumpCancelTimer = kJumpCancelWindow;
    if (comboCount == 10 || comboCount == 20 || comboCount == 30)
        EmitPlayerEvent(*this, GameEventType::ComboMilestone, false, comboCount);
    AddRage(def.rageGain);
}

bool Player::StartJump(bool chase) {
    const MoveSet& ms = Moves();
    if (!(ms.caps & CapJump)) return false;
    // vz0 = MoveSet.jumpVz (720 = apex 173 px; un pesado como Titan-X pone 560 =
    // 105 px). CapHeavyJump va en CapAll, asi que NO cambia la altura por si solo.
    const float vz0 = std::max(200.0f, ms.jumpVz);
    SetState(PlayerState::Jump);
    velocity.z = vz0;
    position.z = std::max(position.z, 0.5f);
    airAttacks = 0;
    jumpCancelTimer = 0.0f;
    const float dir = facing == Facing::Right ? 1.0f : -1.0f;
    if (chase) {
        // Persecucion: llega a la X del enemigo lanzado en 0.30 s.
        velocity.x = std::clamp((lastHitTargetX - dir * 40.0f - position.x) / 0.30f, -900.0f, 900.0f);
    } else {
        velocity.x = walkVelX * 0.6f / std::max(0.01f, 1.0f);   // conserva el impulso (control aereo 60 %)
    }
    walkVelX = walkVelY = 0.0f;
    EmitPlayerEvent(*this, GameEventType::Jump, false, chase ? 1 : 0, AttackId::None,
                    BodyHeight() / kStandardBodyHeight >= 1.4f ? 3 : 1);
    return true;
}

void Player::EnterVictory() {
    if (state == PlayerState::Defeat) return;
    SetState(PlayerState::Victory);
    victoryTimer = kVictoryDuration;
    velocity = {0, 0, 0};
    position.z = 0.0f;
    walkVelX = walkVelY = 0.0f;
    inputBuffer.Clear();
    // skin >= 0 y value = -1: victoria del personaje (el modo emite la suya con value = GameMode).
    EmitPlayerEvent(*this, GameEventType::Victory, false, -1);
}

void Player::HandleInput(float dt) {
    if (!inputEnabled) return;

    const PlayerInput& in = frameInput;
    const MoveSet& ms = Moves();
    if (in.block) {
        attackType = AttackType::None;
        currentAttack = AttackId::None;
        walkVelX = walkVelY = 0.0f;
        if (state != PlayerState::Block) SetState(PlayerState::Block);
        blockTimer += dt;
        return;
    }
    if (state == PlayerState::Block) SetState(PlayerState::Idle);

    const int facingDir = facing == Facing::Right ? 1 : -1;
    const int h = in.moveX > 0.5f ? 1 : (in.moveX < -0.5f ? -1 : 0);

    // Salto (tecla I / boton SALTO).
    if (in.jump && StartJump(false)) return;

    // Dash / back-dash (< + Shift: 0.20 s, -110 px, invulnerable 0.12 s). Se
    // evalua ANTES de girar con el movimiento: "<" es respecto al facing actual.
    if (in.dash && dashCooldown <= 0.0f) {
        if (h == -facingDir && (ms.caps & CapBackDash)) {
            dashDir = -1;
            dashTimer = 0.20f;
            dashSpeed = 110.0f / 0.20f;
            dashInvulnerability = 0.12f;
        } else {
            if (h != 0) facing = h > 0 ? Facing::Right : Facing::Left;
            dashDir = 1;
            dashTimer = 0.12f;
            dashSpeed = 700.0f;
            dashInvulnerability = 0.20f;
        }
        dashCooldown = 0.38f * upgrades.dashCooldownScale;
        walkVelX = walkVelY = 0.0f;
        SetState(PlayerState::Dash);
        EmitPlayerEvent(*this, GameEventType::Dash, false, dashDir);
        return;
    }

    // J/K llegan por el Input Buffer (ver PollAttackInput): un comando pulsado
    // hasta 15 frames antes sigue siendo valido. F0.4 (B14): el ataque se resuelve
    // ANTES de mover, asi el "v" del comando no baja de carril.
    if (TryStartBufferedAttack(false)) return;

    // Carrera: doble toque adelante en 0.25 s (x1.45 mientras se mantiene).
    tapTimer = std::max(0.0f, tapTimer - dt);
    if (h != 0 && prevHoriz == 0) {
        if (h == lastTapDir && tapTimer > 0.0f && (ms.caps & CapRun)) running = true;
        lastTapDir = h;
        tapTimer = 0.25f;
    }
    if (h == 0 || h != lastTapDir) running = false;
    prevHoriz = h;

    Vector2 move = {in.moveX, in.moveY};
    const bool moving = move.x != 0 || move.y != 0;
    const float speed = GetMoveSpeed() * ms.walkSpeedScale * (running ? 1.45f : 1.0f);
    float tx = 0.0f, ty = 0.0f;
    if (moving) {
        const float length = std::sqrt(move.x * move.x + move.y * move.y);
        tx = move.x / length * speed;
        ty = move.y / length * speed * 0.70f;
        if (move.x < 0) facing = Facing::Left;
        if (move.x > 0) facing = Facing::Right;
    }
    // Aceleracion / frenado: v += (vObj - v)(1 - exp(-14 dt)).
    const float k = 1.0f - std::exp(-14.0f * dt);
    walkVelX += (tx - walkVelX) * k;
    walkVelY += (ty - walkVelY) * k;
    if (!moving && std::fabs(walkVelX) < 4.0f && std::fabs(walkVelY) < 4.0f) walkVelX = walkVelY = 0.0f;
    position.x += walkVelX * dt;
    position.y += walkVelY * dt;
    if (moving) {
        if (state != PlayerState::Walk) SetState(PlayerState::Walk);
        if (running && animator.currentClipName != "run" && CharacterHasRealClip("run")) animator.PlayNamed("run");
        else if (!running && animator.currentClipName == "run") animator.PlayNamed("walk");
    } else if (state == PlayerState::Walk) {
        SetState(PlayerState::Idle);
    }

    // Los carriles se acotan aqui, no en cada stage. Stage2 no lo hacia (DF-013 D3).
    position.y = std::clamp(position.y, kLaneMinY, kLaneMaxY);

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
            EmitPlayerEvent(*this, GameEventType::RageStart, true);
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
    if (state == PlayerState::Defeat || state == PlayerState::Knockdown || state == PlayerState::GuardBreak ||
        state == PlayerState::Victory || state == PlayerState::Airborne)
        return;
    // En Hit no se registra nada... salvo durante el congelamiento de impacto
    // (B15): lo pulsado mientras el hitstop para el juego no se pierde y sale al
    // terminar el hitstun, como si se hubiera pulsado justo despues.
    if (state == PlayerState::Hit && !inputFrozen) return;
    // Historial de direcciones para los comandos especiales (frames reales).
    const int h = frameInput.moveX > 0.5f ? 1 : (frameInput.moveX < -0.5f ? -1 : 0);
    const bool downHeld = frameInput.moveY > 0.5f;
    motion.Record(downHeld, h, motionFrame);
    const int facingDir = facing == Facing::Right ? 1 : -1;
    // El facing al empezar el "abajo puro" decide QCF/QCB (caminar en diagonal
    // atras gira al personaje a mitad del comando).
    if (downHeld && h == 0) motionFacing = facingDir;
    // B14: direccion RELATIVA al facing al pulsar, y "abajo puro".
    const int relX = h * facingDir;
    const bool down = downHeld && h == 0;
    const bool specials = (Moves().caps & CapSpecials) != 0;
    if (frameInput.specialWave) inputBuffer.Push(InputCommand::SpecialWave, h != 0 ? h : facingDir, relX, down);
    if (frameInput.specialRise) inputBuffer.Push(InputCommand::SpecialRise, h != 0 ? h : facingDir, relX, down);
    if (frameInput.skillPage) skillPage = (skillPage + 1) % std::max(1, SkillPageCount());
    if (frameInput.skill >= 0 && TryStartSkill(frameInput.skill)) return;
    // >L / vL: Signature2 / Signature3 (si el personaje las tiene; si no, L = onda).
    if (frameInput.energy && state != PlayerState::Idle && state != PlayerState::Walk) {
        // En ataque/recuperacion la L entra como comando de onda (special cancel).
        if (relX > 0 && MoveEnabled(AttackId::Signature2)) inputBuffer.Push(InputCommand::Signature2, facingDir, relX, down);
        else if (down && MoveEnabled(AttackId::Signature3)) inputBuffer.Push(InputCommand::Signature3, facingDir, relX, down);
    } else if (frameInput.energy) {
        if (relX > 0 && MoveEnabled(AttackId::Signature2)) { inputBuffer.Push(InputCommand::Signature2, facingDir, relX, down); frameInput.energy = false; }
        else if (down && MoveEnabled(AttackId::Signature3)) { inputBuffer.Push(InputCommand::Signature3, facingDir, relX, down); frameInput.energy = false; }
    }
    const int frame = motionFrame;
    if (frameInput.punch) {
        const int qcf = motion.QuarterCircleForward(frame, motionFacing);
        const int qcb = motion.QuarterCircleBack(frame, motionFacing);
        if (qcf != 0) { inputBuffer.Push(InputCommand::SpecialWave, qcf, relX, down); motion.Clear(); }
        else if (qcb != 0) {
            // QCB + J: Signature1 si existe; si no, la onda hacia ese lado (como antes).
            if (MoveEnabled(AttackId::Signature1)) inputBuffer.Push(InputCommand::Signature1, motionFacing, relX, down);
            else inputBuffer.Push(InputCommand::SpecialWave, qcb, relX, down);
            motion.Clear();
        }
        else inputBuffer.Push(InputCommand::Punch, h, relX, down);
    }
    if (frameInput.kick) {
        const int dp = motion.DragonPunch(frame);
        const int qcb = specials ? motion.QuarterCircleBack(frame, motionFacing) : 0;
        const int qcf = specials ? motion.QuarterCircleForward(frame, motionFacing) : 0;
        if (dp != 0) { inputBuffer.Push(InputCommand::SpecialRise, dp, relX, down); motion.Clear(); }
        else if (qcb != 0) { inputBuffer.Push(InputCommand::SpecialHurricane, motionFacing, relX, down); motion.Clear(); }
        else if (qcf != 0) { inputBuffer.Push(InputCommand::SpecialDrill, qcf, relX, down); motion.Clear(); }
        else inputBuffer.Push(InputCommand::Kick, h, relX, down);
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
    in.jump = input::Pressed(KEY_I) || input::Pressed(input::kVirtualJump);
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
    // Avanza frames y elimina comandos > 15 frames. B12: mientras dura el propio
    // ataque (arranque + activo + recuperacion) lo pulsado no envejece, asi un J/K
    // durante el REMATE o la PATADA no se pierde y sale al terminar. B15: el stage
    // pasa dt = 0 durante el hitstop (ver PumpPlayerInput en HitResolver.h).
    inputFrozen = dt <= 0.0f;
    if (!inputFrozen) ++motionFrame;
    inputBuffer.Tick(state == PlayerState::Attack && !clipDriven ? 0.0f : dt);
    PollAttackInput();
}

bool Player::InCancelWindow() const {
    if (state != PlayerState::Attack) return false;
    if (attackPhase == AttackPhase::None) return false;
    if (attackPhase == AttackPhase::Special && (clipDriven || !AttackIsSpecialMove(currentAttack))) return false;
    const AttackDef& def = CurrentAttackDef();
    if (clipDriven) return attackElapsed >= std::max(def.startup + def.active, attackDuration * 0.38f);
    // B13: si el golpe ya conecto, se puede cancelar desde startup + 1 frame activo
    // (o cancelFrom) hasta el final; en fallo, solo en la recuperacion.
    if (attackConnected)
        return attackElapsed >= (def.cancelFrom >= 0.0f ? def.cancelFrom : def.startup + InputBuffer::kFrameSeconds);
    return attackElapsed >= def.startup + def.active;   // hitbox ya termino: winddown
}

// Ventana de combo abierta para seguir la cadena J desde Idle/Recovery. Los
// personajes KF/BETA encadenan sus clips por cancelacion, como siempre (su cadena
// propia la definen F1.6/F1.7); el resto sigue la cadena del MoveSet (B03).
bool Player::ChainOpen() const { return comboWindow > 0.0f && !IsKfCharacter(); }

AttackId Player::ChainLink(bool j, int index) const {
    const MoveSet& ms = Moves();
    const int count = j ? ms.chainJCount : ms.chainKCount;
    if (index < 0 || index >= count) return AttackId::None;
    const int id = j ? ms.chainJ[index] : ms.chainK[index];
    if (id <= 0 || id >= kAttackIdCount) return AttackId::None;
    const AttackId a = static_cast<AttackId>(id);
    return MoveEnabled(a) ? a : AttackId::None;
}

bool Player::ChainEnded(int link, bool j) const { return ChainLink(j, link + 1) == AttackId::None; }

bool Player::CancelAllowed(const CancelRule& r, const InputBuffer::Entry& e) const {
    if (r.cmd == kCmdAnySpecial) { if (!IsSpecialCommand(e.command)) return false; }
    else if (r.cmd != static_cast<int>(e.command)) return false;
    switch (r.dir) {
        case CancelDir::Any: break;
        case CancelDir::Neutral: if (e.relX != 0 || e.down) return false; break;
        case CancelDir::Fwd: if (e.relX <= 0) return false; break;
        case CancelDir::Back: if (e.relX >= 0) return false; break;
        case CancelDir::Down: if (!e.down) return false; break;
    }
    if (attackConnected ? !r.onHit : !r.onWhiff) return false;
    const int cur = static_cast<int>(currentAttack);
    const AttackDef& def = CurrentAttackDef();
    const bool inJ = chainLinkJ >= 0 && ChainLink(true, chainLinkJ) == currentAttack;
    const bool inK = chainLinkK >= 0 && ChainLink(false, chainLinkK) == currentAttack;
    switch (r.from) {
        case kCancelAnyAttack: return true;
        case kCancelAnyNormal:
            return !def.air && !AttackIsSpecialMove(currentAttack) && !AttackIsSuper(currentAttack) &&
                   currentAttack != AttackId::Skill && !AttackIsFase2(currentAttack) && activeSkill < 0;
        case kCancelChainJ: return inJ;
        case kCancelChainK: return inK;
        case kCancelChainJ1: return inJ && chainLinkJ == 0;
        case kCancelChainJ2: return inJ && chainLinkJ == 1;
        case kCancelSpecialMove: return AttackIsSpecialMove(currentAttack);
        case kCancelAir: return def.air;
        default: return r.from == cur;
    }
}

// Resolucion de una pulsacion (V2): comando especial > modificador direccional >
// cadena. fromCancel = sobre el ataque en curso (tabla de cancelacion).
AttackId Player::ResolveCommand(const InputBuffer::Entry& e, bool fromCancel, bool* turn) const {
    if (turn) *turn = false;
    const InputCommand cmd = e.command;
    const bool air = state == PlayerState::Jump || (state == PlayerState::Attack && CurrentAttackDef().air);
    const bool finisherReady = !air && isRageMode && comboCount >= 4 &&
        sp >= GetAttack(AttackId::Finisher).spCost &&
        attackCooldowns[static_cast<int>(AttackId::Finisher)] <= 0.0f;
    auto can = [&](AttackId a) { return MoveEnabled(a); };

    // 1) Lo que pide la entrada sin contar la cadena (especial o direccion).
    AttackId req = AttackId::None;
    InputCommand base = cmd;   // movimiento base si el especial no existe
    switch (cmd) {
        case InputCommand::SpecialWave: req = AttackId::EnergyWave; break;
        case InputCommand::SpecialRise: req = can(AttackId::RisingUppercut) ? AttackId::RisingUppercut : AttackId::Punch3; break;
        case InputCommand::SpecialHurricane:
            if (can(AttackId::Hurricane) && sp >= GetAttack(AttackId::Hurricane).spCost) req = AttackId::Hurricane;
            else base = InputCommand::Kick;
            break;
        case InputCommand::SpecialDrill:
            if (can(AttackId::DrillKick) && sp >= GetAttack(AttackId::DrillKick).spCost) req = AttackId::DrillKick;
            else base = InputCommand::Kick;
            break;
        case InputCommand::Signature1: req = can(AttackId::Signature1) ? AttackId::Signature1 : AttackId::EnergyWave; break;
        case InputCommand::Signature2: req = can(AttackId::Signature2) ? AttackId::Signature2 : AttackId::EnergyWave; break;
        case InputCommand::Signature3: req = can(AttackId::Signature3) ? AttackId::Signature3 : AttackId::EnergyWave; break;
        default: break;
    }
    if (air && req != AttackId::None) return AttackId::None;   // sin especiales en el aire
    if (req == AttackId::None) {
        if (base == InputCommand::Kick && finisherReady) {
            // B03/tarea 9: en furia con combo >= 4 y SP >= 35, K = FINISHER (cola y cancel).
            if (!fromCancel || CancelAllowed({kCancelAnyNormal, static_cast<int>(InputCommand::Kick)}, e) ||
                AttackIsSpecialMove(currentAttack))
                return AttackId::Finisher;
        }
        if (air) {
            if (base == InputCommand::Punch) {
                if (e.relX > 0 && can(AttackId::AirSpike)) req = AttackId::AirSpike;
                else if (!fromCancel)
                    req = airAttacks == 0 ? AttackId::AirPunch : airAttacks == 1 ? AttackId::AirPunch2 : AttackId::AirSpike;
            } else {
                req = e.down && can(AttackId::DiveKick) ? AttackId::DiveKick : AttackId::AirKick;
            }
            if (req != AttackId::None && !can(req)) req = AttackId::None;
        } else if (base == InputCommand::Punch) {
            if (e.relX > 0 && can(AttackId::FwdPunch)) req = AttackId::FwdPunch;
            else if (e.relX < 0 && can(AttackId::BackElbow)) req = AttackId::BackElbow;
            else if (e.down) req = stompTargetAhead && can(AttackId::Stomp) ? AttackId::Stomp
                                 : can(AttackId::Uppercut) ? AttackId::Uppercut : AttackId::None;
        } else {
            if (e.relX > 0 && can(AttackId::FwdKick)) req = AttackId::FwdKick;
            else if (e.relX < 0 && can(AttackId::SpinKick)) req = AttackId::SpinKick;
            else if (e.down && can(AttackId::Sweep)) req = AttackId::Sweep;
        }
    }

    if (!fromCancel) {
        if (air) return req;
        if (req != AttackId::None) return req;
        // 2) Cadena.
        if (base == InputCommand::Punch) {
            if (ChainOpen() && chainLinkJ >= 0) {
                const AttackId n = ChainLink(true, chainLinkJ + 1);
                return n != AttackId::None ? n : ChainLink(true, 0);   // fin de cadena: empieza otra
            }
            return ChainLink(true, 0);
        }
        // K: tras el 2o golpe J (ventana abierta) -> lanzador; si no, cadena K.
        if (ChainOpen() && chainLinkJ == 1 && can(AttackId::KickLauncher)) return AttackId::KickLauncher;
        if (ChainOpen() && chainLinkK >= 0) {
            const AttackId n = ChainLink(false, chainLinkK + 1);
            if (n != AttackId::None) return n;
        }
        return ChainLink(false, 0);
    }

    // 3) Cancelacion: MoveSet.cancels y luego la tabla generica; gana la primera.
    InputBuffer::Entry eb = e;
    eb.command = base;
    const MoveSet& ms = Moves();
    int defaultCount = 0;
    const CancelRule* defaults = DefaultCancelRules(defaultCount);
    for (int pass = 0; pass < 2; ++pass) {
        const CancelRule* rules = pass == 0 ? ms.cancels : defaults;
        const int count = pass == 0 ? ms.cancelCount : (ms.replaceCancels ? 0 : defaultCount);
        if (rules == nullptr) continue;
        for (int i = 0; i < count; ++i) {
            const CancelRule& r = rules[i];
            const bool special = req != AttackId::None && IsSpecialCommand(cmd);
            if (!CancelAllowed(r, special ? e : eb)) continue;
            AttackId to = AttackId::None;
            switch (r.to) {
                case kCancelRequested: to = req; if (to == AttackId::None) continue; break;
                case kCancelChainJ: to = ChainLink(true, chainLinkJ + 1); break;   // ultimo -> nada (B12)
                case kCancelChainK: to = ChainLink(false, chainLinkK + 1); break;
                case kCancelChainJStart: to = ChainLink(true, 0); break;
                case kCancelChainKStart: to = ChainLink(false, 0); break;
                case kCancelTurnChainJ2: to = ChainLink(true, 1); if (turn) *turn = true; break;
                default: to = r.to > 0 && r.to < kAttackIdCount ? static_cast<AttackId>(r.to) : AttackId::None; break;
            }
            if (to != AttackId::None && !can(to)) continue;
            return to;
        }
    }
    return AttackId::None;
}

// Consume el comando mas antiguo del buffer y arranca el ataque que le
// corresponde. fromCancel=true aplica la tabla de cancelacion sobre el ataque en
// curso; false resuelve un inicio normal desde Idle/Walk/Recovery/Jump.
bool Player::TryStartBufferedAttack(bool fromCancel) {
    inputBuffer.PruneExpired();
    const InputBuffer::Entry* front = inputBuffer.Peek();
    if (!front) return false;
    const InputBuffer::Entry entry = *front;
    const InputCommand cmd = entry.command;
    const int dir = entry.dir;
    const bool legacy = IsKfCharacter() && std::strcmp(Moves().name, "legado-clip") == 0;

    AttackId next = AttackId::None;
    bool turn = false;
    const bool finisherReady = isRageMode && comboCount >= 4 &&
        sp >= GetAttack(AttackId::Finisher).spCost &&
        attackCooldowns[static_cast<int>(AttackId::Finisher)] <= 0.0f;
    if (legacy) {
        // KF/BETA con el MoveSet legado: exactamente la logica de antes de F0.4.
        if (fromCancel) {
            next = ResolveCancelLegacy(currentAttack, cmd);
            if (cmd == InputCommand::Kick && next != AttackId::None && finisherReady) next = AttackId::Finisher;
        } else if (cmd == InputCommand::SpecialWave || cmd == InputCommand::Signature1 ||
                   cmd == InputCommand::Signature2 || cmd == InputCommand::Signature3) {
            next = AttackId::EnergyWave;
        } else if (cmd == InputCommand::SpecialRise) {
            next = AttackId::Punch3;
        } else if (cmd == InputCommand::Kick || cmd == InputCommand::SpecialHurricane ||
                   cmd == InputCommand::SpecialDrill) {
            next = finisherReady ? AttackId::Finisher : AttackId::Kick;
        } else {
            next = ChainOpen() ? NextPunchInChain(chainAttack) : AttackId::Punch1;
        }
    } else {
        next = ResolveCommand(entry, fromCancel, &turn);
    }
    if (next == AttackId::None) return false;
    if (attackCooldowns[static_cast<int>(next)] > 0.0f) return false;   // queda en cola
    // Sin energia suficiente, el comando de onda sale como golpe normal.
    if (next == AttackId::EnergyWave && sp < GetAttack(AttackId::EnergyWave).spCost) {
        if (fromCancel) return false;
        if (legacy) next = ChainOpen() ? NextPunchInChain(chainAttack) : AttackId::Punch1;
        else next = ChainOpen() && chainLinkJ >= 0 && ChainLink(true, chainLinkJ + 1) != AttackId::None
                        ? ChainLink(true, chainLinkJ + 1) : ChainLink(true, 0);
        if (next == AttackId::None) return false;
    }
    // Especiales: su direccion de comando. J/K (F0.4, reemplaza B21): no giran;
    // <J es el CODAZO sin girar y el J siguiente gira (turn).
    if (IsSpecialCommand(cmd) && dir != 0) facing = dir > 0 ? Facing::Right : Facing::Left;
    if (turn) facing = facing == Facing::Right ? Facing::Left : Facing::Right;

    InputCommand consumed;
    inputBuffer.Pop(consumed);
    comboStep = next == AttackId::Punch1 ? 0 : next == AttackId::Punch2 ? 1
              : next == AttackId::Punch3 ? 2 : 3;
    // Eslabon de cadena del ataque nuevo.
    int linkJ = -1, linkK = -1;
    if (!legacy) {
        const MoveSet& ms = Moves();
        const int wantJ = (fromCancel || ChainOpen()) && chainLinkJ >= 0 ? chainLinkJ + 1 : 0;
        const int wantK = (fromCancel || ChainOpen()) && chainLinkK >= 0 ? chainLinkK + 1 : 0;
        if (turn) linkJ = 1;
        else if (wantJ < ms.chainJCount && ms.chainJ[wantJ] == static_cast<int>(next)) linkJ = wantJ;
        else if (ms.chainJ[0] == static_cast<int>(next)) linkJ = 0;
        if (linkJ < 0) {
            if (wantK < ms.chainKCount && ms.chainK[wantK] == static_cast<int>(next)) linkK = wantK;
            else if (ms.chainK[0] == static_cast<int>(next)) linkK = 0;
        }
    }
    BeginAttack(next);
    chainLinkJ = linkJ;
    chainLinkK = linkK;
    return true;
}

// Fin natural del ataque: si hay comando en cola se encadena; si no, el
// personaje pasa por Recovery antes de volver a Idle/Walk.
void Player::EndAttack() {
    const AttackDef& def = CurrentAttackDef();
    const bool heavy = def.heavy;
    const bool air = def.air;
    attackType = AttackType::None;
    activeSkill = -1;
    clipDriven = false;
    if (air && position.z > 0.0f) {
        // Aereo terminado en el aire: sigue cayendo (otro aereo si hay comando).
        state = PlayerState::Jump;
        attackPhase = AttackPhase::None;
        if (inputEnabled && TryStartBufferedAttack(false)) return;
        SetState(PlayerState::Jump);
        return;
    }
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
        // BETA: golpea solo en los cuadros que traen caja de ataque (datos originales).
        if (const SpriteFrame* f = animator.CurrentFrameData(); f && !f->pieces.empty()) return f->hitbox.has_value();
        const float t = attackElapsed / std::max(0.01f, attackDuration);
        return t >= 0.12f && t <= 0.92f;
    }
    const AttackDef& def = CurrentAttackDef();
    return attackElapsed >= def.startup && attackElapsed <= def.startup + def.active;
}

int Player::GetAttackDamage() const {
    const AttackDef& def = CurrentAttackDef();
    float damage = static_cast<float>(def.damage);
    damage *= def.spawnsProjectile ? upgrades.energyDamageScale : upgrades.attackDamageScale;
    if (clipDriven && currentAttack != AttackId::Skill) damage *= 0.6f;   // KF: varios golpes por animacion
    if (IsTransformed()) damage *= 1.4f;
    if (isRageMode) damage *= 1.35f;
    if (damageBuffTimer > 0.0f) damage *= 1.25f;
    return std::max(1, static_cast<int>(std::round(damage)));
}

bool Player::AttackIsImpactFrame() const {
    if(state!=PlayerState::Attack || clipDriven || !UsesAttackAnimationProfile(skin)) return false;
    const auto& def=CurrentAttackDef();
    if(attackElapsed<def.startup || attackElapsed>def.startup+def.active) return false;
    const auto& profile=GetAttackAnimationProfile(skin,currentAttack);
    return static_cast<int>(animator.CurrentClipFrameIndex())==profile.impactFrameIndex;
}

const AttackAnimationProfile& Player::CurrentAttackAnimationProfile() const {
    static const AttackAnimationProfile kNone{"idle",0.0f,0,1.0f,1.0f,1.0f};
    if(!UsesAttackAnimationProfile(skin)) return kNone;
    static AttackAnimationProfile profile;
    profile=GetAttackAnimationProfile(skin,currentAttack);
    return profile;
}

float Player::GetAttackRange() const { return CurrentAttackDef().range; }

float Player::GetAttackDepthRange() const { return CurrentAttackDef().depth; }

float Player::GetAttackKnockback() const { return CurrentAttackDef().knockback; }

CombatBox Player::GetAttackHitbox() const {
    if (!AttackIsActive()) return {};
    const AttackDef& def = CurrentAttackDef();
    const float direction = facing == Facing::Right ? 1.f : -1.f;
    const SpriteFrame* f = clipDriven ? animator.CurrentFrameData() : nullptr;
    if (f && !f->pieces.empty() && f->hitbox) {
        // BETA: la caja de ataque del cuadro, tal cual la definio el juego original.
        const float sc = SpriteScale();
        const Rectangle& b = *f->hitbox;
        const float x0 = direction > 0 ? position.x + b.x * sc : position.x - (b.x + b.width) * sc;
        return {x0, position.y - position.z + b.y * sc, b.width * sc, b.height * sc};
    }
    if (f && f->width > 0.0f) {
        // La caja la marca el dibujo: llega hasta donde llegan el puno, la
        // patada o el fuego de la habilidad en este frame.
        const float sc = SpriteScale();
        const float front = std::max(60.0f, (f->width - f->pivotX) * sc);
        const float back = std::min(f->pivotX * sc, 30.0f);   // F0.2 (B06d): tambien en Skill (antes 400 px)
        const float top = std::clamp(f->pivotY * sc, 80.0f, 260.0f);
        const float x0 = direction > 0 ? position.x - back : position.x - front;
        return {x0, position.y - top, front + back, top + 10.0f};
    }
    const float centerX = position.x + direction * def.boxForward;
    const float centerY = position.y - position.z + def.boxCenterOffsetY;
    return {centerX - def.boxWidth * 0.5f, centerY - def.boxHeight * 0.5f,
            def.boxWidth, def.boxHeight};
}

}  // namespace district_fury
