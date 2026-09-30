#pragma once
#include "game/combat/BossDefinition.h"
#include "game/Player.h"
#include "game/combat/CombatWorld.h"
#include <vector>

// DF-013.2 — clase Boss unica que interpreta un BossDefinition (fase 2 de
// la unificacion Boss/BossDefinition descrita en ARCHITECTURE.md). Primer
// consumidor real: la pelea de bosses en Modo VS. Los 5 stages de Historia
// SIGUEN con su propia implementacion probada (StoryBoss/Grinder/TitanX/
// TitanXImproved/RayderClone) — no se tocan en este bloque para no arriesgar
// contenido ya verificado sin poder probarlo visualmente en este sandbox.
// Migrar cada stage a esta clase es el siguiente paso incremental, uno a la
// vez, tal como quedo planificado.
//
// Simplificaciones honestas frente a la logica bespoke de cada stage (para
// que quede documentado, no oculto):
// - El acercamiento es generico (persigue al jugador cuando esta lejos);
//   no reproduce movimientos especiales como el dash-embestida real de
//   cada ataque con "Dash" en el nombre.
// - Brakk y Grinder no tienen sprite real todavia (assets/bosses/ solo
//   tiene titanx/titanx_mejorado/rayder_clone) — se dibujan con una
//   silueta primitiva generica usando su color/tamaño de BossDefinition,
//   MAS simple que el dibujo a mano de Stage1Game/Stage2Game (que sigue
//   intacto y es el que de verdad se usa en Historia).
namespace district_fury {

struct BossProjectile { Vector3D pos; float vx; float life; int damage; bool active; bool fromBoss; };

class Boss {
public:
    void Reset(BossId bossId, Vector3D startPos);
    void Update(float dt, Player& player, CombatWorld* world, std::vector<BossProjectile>& projectiles, float* hitstop, float* shake);
    void ApplyDamage(int dmg);
    bool IsDefeated() const { return defeated; }
    bool CanBeHit() const { return !defeated && invuln <= 0.0f; }
    CombatBox GetHurtbox() const;
    Vector3D GetPos() const { return pos; }
    int GetHp() const { return hp; }
    int GetMaxHp() const { return maxHp; }
    int GetPhase() const { return phase; }
    const BossDefinition& Def() const { return *def; }
    void Draw(float playerX) const;

private:
    BossId id{BossId::Brakk};
    const BossDefinition* def{nullptr};
    Vector3D pos{};
    int hp{0};
    int maxHp{0};
    int phase{1};
    float attackTimer{1.0f};
    float elapsed{0.0f};
    float invuln{0.0f};
    int currentAttack{-1};
    bool attackResolved{false};
    bool defeated{false};

    const BossPhaseDef& ActivePhase() const;
    void PickAttack();
};

}  // namespace district_fury
