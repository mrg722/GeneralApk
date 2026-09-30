#pragma once
#include "game/Types.h"
#include "game/InputBuffer.h"
#include "game/combat/AttackData.h"
#include "rendering/Animator.h"

namespace district_fury {

// Recovery va al final para no alterar los valores historicos. PlayerState::Attack
// se conserva (los stages lo consultan); el detalle vive en AttackPhase.
enum class PlayerState { Idle, Walk, Dash, Attack, Block, Hit, GuardBreak, Knockdown, Defeat, Recovery };

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
    float recoveryTimer{0.0f};

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

    float attackCooldowns[static_cast<int>(AttackId::Count)];

    int comboCount; int comboStep; bool hasHit; bool energyReleased;

    PlayerUpgrades upgrades;
    Animator animator;

    // Permite que el tutorial o una cinematica corten la entrada sin tocar el loop.
    bool inputEnabled;
    bool debugInvulnerable;

    Player();

    void Update(float dt);
    void Draw() const;
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

private:
    void BeginAttack(AttackId id);
    void PollAttackInput();
    bool TryStartBufferedAttack(bool fromCancel);
    void EndAttack();
    void UpdateRage(float dt);
    void HandleInput(float dt);
    void DrawRageAura(Vector2 screen, float scale) const;
};

}  // namespace district_fury
