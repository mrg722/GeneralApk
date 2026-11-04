#pragma once
#include "game/Types.h"
#include "game/InputBuffer.h"
#include "game/combat/AttackData.h"
#include "rendering/Animator.h"
#include "game/combat/AttackAnimation.h"
#include "game/player/PlayerVisualState.h"
#include "game/combat/HitTypes.h"
#include "game/moves/MoveTypes.h"
#include <string>
#include <cstdint>
#include <vector>

namespace district_fury {

// Recovery va al final para no alterar los valores historicos. PlayerState::Attack
// se conserva (los stages lo consultan); el detalle vive en AttackPhase.
// Jump y Victory (F0.4): Jump = salto propio (no es Airborne, que es "derribado en
// el aire", B22); Victory = pose de victoria 1.6 s (no actua ni recibe golpes).
enum class PlayerState { Idle, Walk, Dash, Attack, Block, Hit, GuardBreak, Knockdown, Defeat, Recovery, Airborne, Jump, Victory };

// Entrada de un frame. Por defecto se lee del teclado (WASD, Shift, B, J, K, L,
// Espacio); tests, bots y repeticiones pueden inyectar una propia.
struct PlayerInput {
    float moveX = 0.0f, moveY = 0.0f;
    bool block = false, dash = false, punch = false, kick = false, energy = false, rage = false;
    // Botones tactiles de especial directo (sin tener que hacer el comando).
    bool specialWave = false, specialRise = false;
    int skill = -1;   // habilidad pulsada este frame (0..5), -1 = ninguna
    bool skillPage = false;   // pasar a la siguiente pagina de habilidades
    bool jump = false;        // salto (F0.4: tecla I / boton SALTO)
};

// Sub-estado de la FSM mientras state == Attack.
enum class AttackPhase { None, Punch1, Punch2, Punch3, Kick, Special };

// Enum historico. Se conserva porque los tres stages y el Modo VS lo consultan.
// Ahora es una proyeccion de AttackId, no la fuente de verdad.
enum class AttackType { None, Punch, Kick, Energy, Dash, Rage, Finisher };

enum class RageState { Normal, Starting, Active, Ending };

struct PlayerUpgrades {
    int bonusMaxHp = 0;
    int bonusMaxSp = 0;
    int bonusMaxShield = 0;
    float rageCapacityScale = 1.0f;
    float attackDamageScale = 1.0f;
    float energyDamageScale = 1.0f;
    float moveSpeedScale = 1.0f;
    float dashCooldownScale = 1.0f;
    float comboWindowBonus = 0.0f;
};

class Player {
public:
    Vector3D position;
    Vector3D velocity;
    Facing facing;
    PlayerState state;
    AttackType attackType;
    AttackId currentAttack;
    AttackPhase attackPhase{AttackPhase::None};

    // Buffer de entrada (15 frames) y recuperacion fisica posterior al ataque.
    InputBuffer inputBuffer;
    MotionHistory motion;       // comandos especiales estilo King Fighter
    float recoveryTimer{0.0f};
    // Si no es nullptr, reemplaza al teclado (bots/tests). No es dueno del puntero.
    const PlayerInput* scriptedInput{nullptr};
    PlayerInput frameInput;
    bool nextHitLow{false};   // alterna hit_high / hit_low en impactos seguidos

    int hp; int maxHp; int sp; int maxSp; int rage; int maxRage; bool isRageMode;
    // DF-013.2 (19-09): personaje visual seleccionable. 0 = Rayden ORIGINAL
    // (atlas rayden_clean, intacto y por defecto). 1 = Rayden clon, que dibuja
    // el set de poses de assets/bosses/rayder_clone/. Solo cambia el dibujo:
    // stats, ataques, hitboxes y hurtboxes son exactamente los mismos.
    int skin{0};
    // Aplica el personaje seleccionado (sprite + vida + multiplicadores).
    // id 0 = Rayden original: no cambia ningun valor historico.
    void ApplyCharacter(int id);
    int shield; int maxShield;

    float stateTimer; float attackElapsed; float attackDuration;
    float dashTimer; float dashInvulnerability; float dashCooldown;
    float comboWindow; float spRegenAccumulator; float rageDrainAccumulator;
    float blockDamageReduction; float blockTimer;
    float shieldRegenTimer; float shieldRegenRate;

    // DF-013: Rage con estados explicitos y duracion perceptible.
    RageState rageState;
    float rageStateTimer;
    float rageAuraPhase;

    // DF-013: buffs temporales de pickups.
    float damageBuffTimer;
    float speedBuffTimer;

    // DF-013: hitstun / knockdown reales.
    float hitstunTimer;
    float knockdownTimer;
    // Impacto leve al recibir un golpe mientras una habilidad esta en curso:
    // no cancela la habilidad (ver TakeDamage), pero se nota (tinte rojo
    // breve + empujon pequeno) en vez de no reaccionar nada.
    float abilityHitFlashTimer;

    float attackCooldowns[static_cast<int>(AttackId::Count)];

    int comboCount; int comboStep; bool hasHit; bool energyReleased;

    // ---- F0.2: resolvedor unico de golpes ----
    // Ultimo ataque de la cadena (para NextPunchInChain); separado del ataque
    // en curso para que expirar la ventana de combo no borre currentAttack (B04).
    AttackId chainAttack{AttackId::None};
    // attackElapsed del frame anterior (barrido anti-tunel del arco, V5).
    float attackPrevElapsed{0.0f};
    // Proyectil de la activacion actual: se dispara UNA sola vez (B01).
    bool projectileFired{false};
    bool projectilePending{false};
    // Objetivos ya golpeados en esta activacion (multi-objetivo, B08) y reloj de
    // re-golpe (AttackDef::multiHitInterval).
    std::vector<std::uintptr_t> hitTargets;
    float multiHitClock{0.0f};
    // El ataque en curso ya conecto (abre el cancel en hit, B13).
    bool attackConnected{false};
    // Temblor de victima (V7): lo dibuja PlayerDraw (F0.3) con VictimShakeOffset.
    float hurtShakeTimer{0.0f};
    float hurtShakeAmp{0.0f};
    float hurtShakeDir{1.0f};

    // Recepcion de un golpe con toda su informacion (direccion, hitstun,
    // knockback, derribo...). TakeDamage(int) es un envoltorio con lo de siempre.
    HitResult ApplyHit(const HitInfo& info);
    // true UNA sola vez por activacion de un ataque con proyectil (B01).
    bool ConsumeProjectileEvent();
    // Alto real del cuerpo en pantalla para un personaje sin escala de carril
    // (112 = estandar) y bodyScale = (alto/112) * DepthScaleFor(y) (V3).
    float BodyHeight() const;
    float BodyScale() const;
    // true si el objetivo `id` ya recibio el golpe de esta activacion.
    bool AlreadyHit(std::uintptr_t id) const;
    void MarkHit(std::uintptr_t id);
    // El ataque en curso esta en su arranque (para el counter-hit).
    bool InAttackStartup() const;

    // Habilidades (botones 1-6, estilo King Fighter): cada una con 15 s de espera.
    static constexpr int kSkillCount = 6;
    static constexpr float kSkillCooldown = 15.0f;
    static constexpr float kTransformDuration = 12.0f;
    float skillCooldown[kSkillCount] = {};
    int activeSkill{-1};          // habilidad en curso (-1 = ataque normal)
    bool clipDriven{false};
    bool attackImpactTriggered{false};
    float attackRootMotionApplied{0.0f};
    bool animationDebug{false};
    float multiHitTimer{0.0f};    // golpes repetidos durante una animacion larga
    float transformTimer{0.0f};   // transformacion activa (dano y velocidad extra)

    // Nombre en pantalla de la habilidad i (segun el personaje).
    const char* SkillName(int i) const;
    bool SkillReady(int i) const { return SkillCooldownLeft(i) <= 0.0f; }
    // Espera restante del boton i (segun la pagina actual en personajes KF).
    float SkillCooldownLeft(int i) const;
    // Personajes KF: TODAS sus habilidades se reparten de 5 en 5 en los botones
    // 1-5; el boton PAG (o TAB) pasa a la siguiente pagina. El 6 es TRANSFORMAR.
    int skillPage{0};
    std::vector<float> abilityCooldown;
    int AbilityCount() const;
    int SkillPageCount() const;
    int AbilityForSlot(int i) const;   // -1 si el boton no tiene habilidad
    bool IsTransformed() const { return transformTimer > 0.0f; }
    // Personaje extraido de la APK (sus clips traen su propio arte de poderes).
    bool IsKfCharacter() const;
    // Rayder (el atlas rayder_kf, no sus clones): usa su propio set de golpes/poderes.
    bool IsRayder() const;
    // Ronin (artista marcial con habilidad de espada): su propio set de golpes/espada.
    bool IsRonin() const;
    // Intenta lanzar la habilidad i ahora; false si esta en espera o no puede actuar.
    bool TryStartSkill(int i);
    // Escala de dibujo del sprite (la usa tambien la caja de golpe de las habilidades).
    float SpriteScale() const;
    // Retrato para el HUD: cabeza y torso del primer cuadro de reposo dentro de `box`.
    void DrawPortrait(Rectangle box) const;

    // Fluidez (estilo King Fighter) para nuestros personajes: respiracion,
    // balanceo, inclinacion, giro suave, aterrizaje y estelas. Solo visual,
    // salvo el avance de los golpes (root motion), que si mueve al personaje.
    static constexpr int kGhostCount = 5;   // estelas por defecto (PlayerVisualState)
    float animClock{0.0f};
    float walkPhase{0.0f};
    float turnTimer{0.0f};
    float landTimer{0.0f};
    Facing lastFacing{Facing::Right};
    PlayerState lastState{PlayerState::Idle};
    // Estado SOLO de dibujo (estelas, fase visual): lo llena F0.3. mutable porque
    // Draw() es const y lo actualiza al dibujar.
    mutable PlayerVisualState visual;

    // Efectos de poder (assets/fx/efectos_azules.png, rejilla 8x5): chispa al
    // conectar, estallido al transformarse. Solo nuestros personajes (los KF
    // traen sus efectos dentro de su animacion).
    struct Fx { int first, count; Vector2 pos; float t, dur, scale; };
    std::vector<Fx> effects;
    bool prevHasHit{false};
    void SpawnFx(int first, int count, Vector2 pos, float dur, float scale);

    PlayerUpgrades upgrades;
    Animator animator;
    // Forma transformada: se intercambia con `animator` mientras dura la
    // transformacion (Rayder de pelo blanco, heroe/heroina KF transformados).
    Animator altAnimator;
    bool usingAlt{false};
    bool altTried{false};

    // Permite que el tutorial o una cinematica corten la entrada sin tocar el loop.
    bool inputEnabled;
    bool debugInvulnerable;

    Player();

    void Update(float dt);
    // Lee la entrada del frame y avanza el Input Buffer. Los stages la llaman
    // ANTES de congelar por hitstop para no perder J/K pulsados en el impacto.
    // Si nadie la llamo, Update la invoca solo.
    void PumpInput(float dt);
    void Draw() const;
    // Si el personaje tiene clip "projectile" (Rayder), dibuja la onda con su
    // sprite animado centrado en `center` y devuelve true; si no, false.
    bool DrawEnergyProjectile(Vector2 center, bool movingLeft, float time) const;
    void TakeDamage(int damage);
    void SetState(PlayerState newState);
    void Reset();
    void AddRage(int amount);
    void ApplyUpgrades(const PlayerUpgrades& newUpgrades);

    // Llamado por el CombatWorld cuando un ataque conecta: abre ventana de combo.
    void OnAttackConnected(const AttackDef& def);

    // Registra un comando como si se hubiera pulsado J/K (tests, IA, replays).
    void QueueCommand(InputCommand command) { inputBuffer.Push(command); }
    // Nombre de la fase de ataque para depuracion ("ATTACK_PUNCH_1"...).
    const char* AttackPhaseName() const;
    // True si el ataque actual esta en su ventana de cancelacion.
    bool InCancelWindow() const;

    bool AttackIsActive() const;
    bool AttackIsImpactFrame() const;
    const AttackAnimationProfile& CurrentAttackAnimationProfile() const;
    bool IsBlocking() const;
    bool IsGuardBroken() const;
    bool IsKnockedDown() const;
    bool IsInvulnerable() const;
    bool CanAct() const;

    int GetAttackDamage() const;
    float GetAttackRange() const;
    float GetAttackDepthRange() const;
    float GetAttackKnockback() const;
    float GetMoveSpeed() const;
    const AttackDef& CurrentAttackDef() const;
    const char* RageStateName() const;

    CombatBox GetHurtbox() const;
    CombatBox GetAttackHitbox() const;

    // ================= F0.4: framework de movimientos =================
    // MoveSet efectivo del personaje: GetMoveSet(skin) con los huecos rellenados
    // (FillDefaultChains). Los KF/BETA (clip) con el MoveSet generico usan el
    // "legado": cadena J P1 P2 P3, K = Kick, sin salto ni normales de comando
    // (como antes de F0.4) hasta que F1.6/F1.7/F1.8 declaren el suyo.
    const MoveSet& Moves() const;
    // Tests y herramientas: reemplaza GetMoveSet(skin) (nullptr = el del registro).
    const MoveSet* customMoveSet{nullptr};
    // Override del personaje para el ataque `id` (nullptr = fila generica).
    const MoveOverride* OverrideFor(AttackId id) const;
    // Definicion EFECTIVA del ataque `id` para este personaje: fila generica o
    // replaceDef, con damageScale/timeScale/reachScale, arco, root, hitCount,
    // armadura y proyectil del override.
    AttackDef EffectiveDef(AttackId id) const;
    // El personaje puede usar `id` (override.enabled / requiresOverride / caps y
    // que tenga con que dibujarlo: nunca `idle` como ataque, B18).
    bool MoveEnabled(AttackId id) const;
    // El clip existe en el animador y no es un relleno de `idle` (KfReference
    // rellena con idle los clips que faltan: esos cuentan como inexistentes).
    bool CharacterHasRealClip(const char* name) const;
    // Clip que usaria el ataque `id` y de donde sale (para moves_sheet / tests).
    enum class ClipOrigin { Override, Canonical, Fallback, Historic, Pose, Idle };
    const char* AttackClipFor(AttackId id, ClipOrigin* origin) const;
    std::string attackClipName;               // clip reproducido por el ataque en curso
    ClipOrigin attackClipOrigin{ClipOrigin::Historic};
    static const char* ClipOriginName(ClipOrigin o);
    // Proyectil del ataque en curso (override.projectile o el de siempre) y
    // numero de disparo (0..count-1) del ultimo ConsumeProjectileEvent().
    ProjectileSpec CurrentProjectile() const;
    int ProjectileShotIndex() const { return projectileShots - 1; }
    // Victoria: pose 1.6 s (no actua, no recibe golpes) y luego Idle.
    void EnterVictory();
    static constexpr float kVictoryDuration = 1.6f;
    float victoryTimer{0.0f};
    // Salto (V9): vz0 = MoveSet.jumpVz (720; pesado 560), subida g 1500, caida
    // 1875, control aereo 60 %, sin cambio de carril, aterrizaje 4 f.
    bool StartJump(bool chase);
    static constexpr float kJumpRiseGravity = 1500.0f;
    static constexpr float kJumpFallGravity = 1875.0f;
    static constexpr float kJumpCancelWindow = 10.0f / 60.0f;
    float jumpCancelTimer{0.0f};    // ventana de persecucion tras un lanzador que conecta
    float lastHitTargetX{0.0f};     // X del ultimo objetivo golpeado (persecucion)
    int airAttacks{0};              // golpes aereos de este salto
    bool InAir() const { return state == PlayerState::Jump || (state == PlayerState::Attack && CurrentAttackDef().air); }
    // Cadena: eslabon (indice en chainJ / chainK) del ultimo ataque de cadena, -1 = ninguno.
    int chainLinkJ{-1}, chainLinkK{-1};
    // Movimiento: velocidad suavizada al caminar, carrera (doble toque), back-dash.
    float walkVelX{0.0f}, walkVelY{0.0f};
    bool running{false};
    float tapTimer{0.0f};
    int lastTapDir{0}, prevHoriz{0};
    int dashDir{1};                 // +1 = hacia facing, -1 = back-dash
    float turnClipTimer{0.0f};      // clip "turn" en curso al girar caminando
    float dashSpeed{700.0f};
    int motionFrame{0};             // frames reales (MotionHistory; no se congela en el ataque)
    int motionFacing{1};            // facing (+1/-1) al empezar el ultimo "abajo puro" (QCF/QCB)
    int stepCount{0};               // medio pasos (Footstep en cada cruce de walkPhase por 0 y pi)
    // Ataque en curso: golpes hechos (hitCount), armadura gastada, root motion.
    int attackHitsDone{0};
    int armorAbsorbed{0};
    int projectileShots{0};
    float projectileClock{0.0f};
    float rootAppliedX{0.0f};
    // Resolucion pura de una pulsacion (V2: comando > direccion > cadena); la usa
    // TryStartBufferedAttack. turn = girar antes de golpear (<J J).
    AttackId ResolveCommand(const InputBuffer::Entry& e, bool fromCancel, bool* turn) const;
    // Stomp: hay un enemigo derribado delante (lo fija el stage/HitResolver; F1.9).
    bool stompTargetAhead{false};

private:
    mutable AttackDef effectiveDef_{};
    mutable AttackId effectiveId_{AttackId::Count};
    mutable int effectiveSkin_{-1};
    mutable const MoveSet* effectiveSet_{nullptr};
    mutable MoveSet moves_{};
    mutable int movesSkin_{-999};
    mutable const MoveSet* movesCustom_{nullptr};
    bool ChainEnded(int link, bool j) const;
    AttackId ChainLink(bool j, int index) const;
    void PlayAttackClip(AttackId id);
    void UpdateJump(float dt);
    void UpdateAttackMotion(float dt, const AttackDef& def);
    void FinishLanding(float recovery);
    bool CancelAllowed(const CancelRule& r, const InputBuffer::Entry& e) const;

    void BeginAttack(AttackId id);
    void BeginClipAttack(const char* clip, int skillIndex);
    float ClipSeconds(const char* clip) const;
    void PollAttackInput();
    PlayerInput ReadInput() const;
    bool inputPumped{false};
    bool inputFrozen{false};   // PumpInput(0): el stage esta en hitstop (B15)
    bool TryStartBufferedAttack(bool fromCancel);
    bool ChainOpen() const;
    void EndAttack();
    void UpdateRage(float dt);
    void HandleInput(float dt);
    void DrawRageAura(Vector2 screen, float scale) const;
    // Cuerpo del personaje, partido de Draw (F0.1). DrawPoseBody (PlayerPoseDraw.cpp):
    // poses sueltas de assets/bosses/; devuelve false si este personaje no las usa o
    // falta la textura. DrawAtlasBody (PlayerAtlasDraw.cpp): atlas del Animator.
    bool DrawPoseBody(Vector2 p, float scale, Color spriteTint) const;
    void DrawAtlasBody(Vector2 p, float scale, float spriteScale, Color spriteTint) const;
    void UpdateMotionFeel(float dt);
    void UpdateTransformForm();
    void DrawAnimationDebug(Vector2 screen) const;
};

}  // namespace district_fury
