#pragma once
#include "game/Player.h"
#include "game/StreetEnemy.h"
#include "game/combat/CombatWorld.h"
#include <vector>

namespace district_fury {
class Stage3Game {
public:
    Stage3Game();
    void Init();
    void Update(float dt);
    void Draw() const;
    // DF-013.2: acceso al jugador para aplicar las mejoras de campana antes
    // de Init(). No cambia nada del comportamiento existente.
    Player& PlayerRef() { return player; }
    bool StageCleared() const { return flow == Flow::Clear; }
    bool ConsumeAdvance() { const bool a = advanceRequested; advanceRequested = false; return a; }
private:
    struct Projectile { Vector3D pos; float vx; float life; float radius; int damage; bool fromBoss; bool active; };
    struct Particle { Vector3D pos; Vector2 vel; float life; float maxLife; float size; Color color; };
    struct TitanX {
        Vector3D pos{5350,570,0}; int hp{1500}; int maxHp{1500}; int phase{1};
        float attackTimer{1.2f}; float powerTimer{2.2f}; float shieldTimer{0.8f}; float invuln{0}; float dashTimer{0}; float elapsed{0};
        bool blocking{false}; bool defeated{false};
    };
    enum class Flow { Intro, Combat, Gatekeeper, BossIntro, Boss, Clear, GameOver, Pause };
    enum class Difficulty { Easy, Normal, Hard };
    Player player;
    std::vector<StreetEnemy> enemies;
    // DF-013.2: mismo cableado aditivo de CombatWorld (ver
    // docs/AUTONOMOUS_PROGRESS.md).
    CombatWorld combatWorld;
    std::vector<Projectile> projectiles;
    std::vector<Particle> particles;
    TitanX boss;
    Flow flow{Flow::Intro};
    Difficulty difficulty{Difficulty::Normal};
    int scenario{1}; int combo{0}; int maxCombo{0}; int defeated{0}; int damageTaken{0}; int score{0};
    float stageTime{0}; float comboTimer{0}; float hitstop{0}; float shake{0}; float bannerTimer{0}; float transitionTimer{0}; float cameraX{640};
    bool arenaLocked{false};
    bool advanceRequested{false}; bool scenarioGatekeeperSpawned{false}; bool bossSpawned{false}; bool stageComplete{false};
    const char* storyMessage{"Rayden enters Astra Tower. The chain network ends in the corporate command floor."};
    void ResetRun(); void BuildScenario(int id); void SpawnScenarioGatekeeper(); void AdvanceScenario(); void EnterBoss(); void DefeatBoss(); void ApplyDifficulty();
    bool AllEnemiesDefeated() const; int ScenarioStartX() const; int ScenarioEndX() const; const char* ScenarioName() const; const char* ScenarioObjective() const; const char* DifficultyText() const;
    void UpdateCombat(float dt); void UpdateBoss(float dt); void UpdateProjectiles(float dt); void UpdateParticles(float dt);
    void HandlePlayerHits(); void HandleEnemyHits(); void HandleBossHits(); void SpawnEnergyProjectile(); void SpawnBossProjectile(int damage,float speed); void SpawnImpact(Vector3D pos,Color color,bool heavy);
    void DrawWorld() const; void DrawHUD() const; void DrawBoss() const; void DrawClear() const; void DrawOverlay() const;
};
}
