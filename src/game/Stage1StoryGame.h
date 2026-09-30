#pragma once
#include "game/Player.h"
#include "game/StreetEnemy.h"
#include "game/combat/CombatWorld.h"
#include <string>
#include <vector>

namespace district_fury {

enum class StoryFlow { Menu, Controls, Intro, Combat, SubBossIntro, ScenarioClear, BossIntro, Boss, StageClear, GameOver, Pause, Options, Credits };
enum class StoryDifficulty { Easy, Normal, Hard };
enum class StoryBossAttack { None, ChainSwing, GroundSmash, Charge, PowerWave, Frenzy };

struct StoryProjectile { Vector3D position; float velocity; float life; float radius; int damage; bool active; bool fromBoss; };
struct StoryParticle { Vector3D position; Vector2 velocity; float life; float maxLife; float size; Color color; };
struct StoryBoss {
    Vector3D position{0, 575, 0}; int hp{1200}; int maxHp{1200}; int phase{1}; float stateTimer{0};
    float attackTimer{1.2f}; float attackElapsed{0}; float invulnerability{0}; float blockTimer{0}; float powerTimer{2.0f};
    bool blocking{false}; bool defeated{false}; StoryBossAttack attack{StoryBossAttack::None};
};

class Stage1StoryGame {
public:
    Stage1StoryGame();
    void Init(); void Update(float dt); void Draw() const;
    // DF-013.2: acceso al jugador para aplicar las mejoras de campana.
    Player& PlayerRef() { return player; }
    bool IsMenu() const { return flow == StoryFlow::Menu; }
    bool StageCleared() const { return flow == StoryFlow::StageClear; }
    bool ConsumeAdvance() { const bool a = advanceRequested; advanceRequested = false; return a; }
    void ReturnToMenu() { ResetRun(); flow = StoryFlow::Menu; }

    // DF-013: menu principal unificado (ver ui/MainMenu.h). El cursor navega
    // los 7 items del arte; VS/Salir se resuelven en main.cpp via estas
    // banderas porque Stage1StoryGame no es dueno del ciclo de vida de la
    // aplicacion ni del Modo VS.
    int MenuCursor() const { return menuCursor; }
    const char* MenuDifficultyLabel() const { return DifficultyText(); }
    bool ConsumeVsRequest() { if (vsRequested) { vsRequested = false; return true; } return false; }
    // DF-013.2: avisa que el jugador pulso NUEVA PARTIDA, para que main.cpp
    // reinicie las mejoras de campana antes de empezar.
    bool ConsumeNewGame() { if (newGameStarted) { newGameStarted = false; return true; } return false; }
    bool ExitRequested() const { return exitRequested; }

private:
    Player player; std::vector<StreetEnemy> enemies; std::vector<StoryProjectile> projectiles; std::vector<StoryParticle> particles;
    // DF-013.2: CombatWorld cableado para activar hazards/specials de enemigo
    // (ChemicalCloud, HeavySlam) que antes eran no-ops por recibir world=nullptr.
    // No reemplaza HandlePlayerHits/HandleEnemyHits/UpdateProjectiles propios
    // de este stage; es aditivo (ver docs/AUTONOMOUS_PROGRESS.md).
    CombatWorld combatWorld;
    StoryFlow flow{StoryFlow::Menu}; StoryBoss boss;
    int scenario{1}, wave{0}, combo{0}, maxCombo{0}, defeated{0}, damageTaken{0}, score{0}, xp{0}, coins{0}, gems{0}, level{1};
    int bestScore{0}, bestRank{0};
    float cameraX{640}, stageTime{0}, comboTimer{0}, hitstop{0}, shake{0}, bannerTimer{0}, transitionTimer{0}, storyTimer{0};
    bool advanceRequested{false};
    bool arenaLocked{false}, scenarioBossSpawned{false}, finalBossSpawned{false}, stageComplete{false}, saveLoaded{false};
    StoryDifficulty difficulty{StoryDifficulty::Normal}; std::string savePath{"district_fury_save.dat"}; std::string storyMessage;
    int menuCursor{0}; bool vsRequested{false}; bool newGameStarted{false}; bool exitRequested{false};

    void ResetRun(); void BuildScenario(int id); void SpawnWave(int id); void SpawnScenarioBoss(); void EnterFinalBoss(); void DefeatFinalBoss();
    void ApplyDifficulty(); void UpdateCombat(float dt); void UpdateBoss(float dt); void UpdateProjectiles(float dt); void UpdateParticles(float dt);
    void HandlePlayerHits(); void HandleEnemyHits(); void HandleBossHits(); void SpawnEnergyProjectile(); void SpawnBossPower(); void SpawnImpact(Vector3D pos, Color color, bool heavy);
    void AdvanceScenario(); bool ScenarioWaveCleared() const; bool AllCurrentEnemiesDefeated() const;
    int ScenarioStartX() const; int ScenarioEndX() const; const char* ScenarioName() const; const char* ScenarioObjective() const;
    int CalculateRank() const; int CalculateScore() const; const char* RankText() const; const char* DifficultyText() const;
    void LoadSave(); void SaveProgress();
    void DrawWorld() const; void DrawHUD() const; void DrawMenu() const; void DrawControls() const; void DrawPause() const; void DrawGameOver() const;
    void DrawScenarioClear() const; void DrawBoss() const; void DrawStageClear() const; void DrawArenaLock() const; void DrawScenarioArt() const;
    void DrawOptions() const; void DrawCredits() const;
};

}
