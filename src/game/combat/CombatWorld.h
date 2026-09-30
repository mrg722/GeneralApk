#pragma once
#include "game/Types.h"
#include "game/Player.h"
#include "game/StreetEnemy.h"
#include "game/combat/AttackData.h"
#include <vector>

// DF-013 FASE 1 — Combat Core.
// Antes de esta fase existian CUATRO copias divergentes de UpdateProjectiles,
// HandlePlayerHits, HandleEnemyHits, HandleBossHits, SpawnImpact y
// UpdateParticles (Stage1, Stage2, Stage3 y ProductionGame). Todas viven aqui.
//
// Regla: el Stage describe contenido, el CombatWorld resuelve combate.

namespace district_fury {

enum class PickupType { Health, Energy, Rage, Coins, DamageBuff, SpeedBuff };

struct Projectile {
    Vector3D position{};
    float velocityX = 0.0f;
    float life = 0.0f;
    float radius = 20.0f;
    int damage = 0;
    bool fromEnemy = false;
    bool active = false;
    bool piercing = false;
    Color color{60, 220, 255, 255};
};

struct Particle {
    Vector3D position{};
    Vector2 velocity{};
    float life = 0.0f;
    float maxLife = 0.0f;
    float size = 0.0f;
    float gravity = 0.0f;
    Color color{255, 255, 255, 255};
};

struct Pickup {
    Vector3D position{};
    PickupType type = PickupType::Health;
    int amount = 0;
    float life = 0.0f;
    float bob = 0.0f;
    bool active = false;
};

struct Destructible {
    Vector3D position{};
    int hp = 0;
    int maxHp = 0;
    float width = 46.0f;
    float height = 58.0f;
    float shake = 0.0f;
    bool active = false;
    bool dropsReward = false;
    Color color{120, 92, 54, 255};
};

enum class HazardKind { Fire, Steam, Acid, Electric, Laser, Explosion };

struct Hazard {
    Vector3D position{};
    HazardKind kind = HazardKind::Fire;
    float width = 90.0f;
    float height = 70.0f;
    int damage = 6;
    float cycle = 2.0f;     // periodo total
    float activeWindow = 0.8f;  // cuanto tiempo del ciclo es peligroso
    float timer = 0.0f;
    float tickTimer = 0.0f;
    bool active = false;
};

// Adaptador ligero para que los tres bosses (StoryBoss, Grinder, TitanX), que
// son structs incompatibles entre si, puedan compartir la resolucion de golpes
// sin reescribirlos todavia.
struct BossTarget {
    Vector3D position{};
    int* hp = nullptr;
    float* invulnerability = nullptr;
    bool blocking = false;
    float width = 156.0f;
    float height = 145.0f;
    bool valid = false;

    CombatBox Box() const {
        if (!valid) return {};
        return {position.x - width * 0.5f, position.y - height, width, height};
    }
};

struct HitReport {
    int hits = 0;
    int kills = 0;
    int damageDealt = 0;
    bool hitBoss = false;
};

class CombatWorld {
public:
    std::vector<Projectile> projectiles;
    std::vector<Particle> particles;
    std::vector<Pickup> pickups;
    std::vector<Destructible> destructibles;
    std::vector<Hazard> hazards;

    float hitstop = 0.0f;
    float shake = 0.0f;

    int combo = 0;
    int maxCombo = 0;
    int score = 0;
    int defeated = 0;
    int damageTaken = 0;
    int coinsCollected = 0;
    float comboTimer = 0.0f;
    float comboWindow = 1.0f;

    void Reset();

    // --- feedback ---
    void DoHitstop(float duration);
    void DoShake(float amount);
    void SpawnImpact(Vector3D position, Color color, bool heavy);
    void SpawnDebris(Vector3D position, Color color, int count);

    // --- spawners ---
    void SpawnProjectile(Vector3D position, float velocityX, int damage,
                         bool fromEnemy, Color color, float radius = 20.0f,
                         float life = 0.9f);
    void SpawnPlayerEnergyWave(const Player& player);
    void SpawnPickup(Vector3D position, PickupType type, int amount);
    void AddDestructible(Vector3D position, int hp, bool dropsReward, Color color);
    void AddHazard(Vector3D position, HazardKind kind, float width, float height,
                   int damage, float cycle, float activeWindow, float phase = 0.0f);

    // --- simulacion ---
    void Update(float dt);  // particulas, proyectiles, pickups, hazards, timers
    bool ConsumeHitstop(float dt);  // true si el frame esta congelado

    // --- resolucion de combate ---
    HitReport ResolvePlayerMelee(Player& player, std::vector<StreetEnemy>& enemies);
    bool ResolvePlayerMeleeOnBoss(Player& player, BossTarget& boss);
    void ResolveEnemyMelee(std::vector<StreetEnemy>& enemies, Player& player,
                           int maxSimultaneousAttackers = 2);
    HitReport ResolveProjectiles(std::vector<StreetEnemy>& enemies, Player& player,
                                 BossTarget* boss);
    void ResolveDestructibles(Player& player);
    void ResolveHazards(Player& player, std::vector<StreetEnemy>& enemies);
    void ResolvePickups(Player& player);

    void RegisterCombo(int scoreGain);
    void BreakCombo();

    // --- dibujo (mundo, dentro de BeginMode2D) ---
    void DrawGround() const;
    void DrawEffects() const;

private:
    void UpdateParticles(float dt);
    void UpdateProjectiles(float dt);
};

}  // namespace district_fury
