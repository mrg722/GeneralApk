// Stage 1: oleadas por linea de activacion, bloqueo de camara/arena y guardianes de escenario.
#include "game/stage1/Stage1Common.h"

namespace district_fury {

// DF-014: cada escenario se divide en dos oleadas con linea de activacion.
// Oleada 1 al entrar; oleada 2 al avanzar tras limpiar la primera. Al cruzar
// la linea la camara se bloquea; al limpiarla aparece "GO >>".
void Stage1StoryGame::BuildWaves() {
    arena.Reset();
    const int n = (int)enemies.size();
    if (n == 0) return;
    std::vector<int> order(n);
    for (int i = 0; i < n; ++i) order[i] = i;
    std::sort(order.begin(), order.end(),
              [&](int a, int b) { return enemies[a].position.x < enemies[b].position.x; });
    const int firstCount = (n + 1) / 2;
    std::vector<int> w1(order.begin(), order.begin() + firstCount),
        w2(order.begin() + firstCount, order.end());
    const float t1 = (float)ScenarioStartX() + 60.f;
    arena.AddWave(t1, w1);
    if (!w2.empty()) arena.AddWave(std::max(enemies[w2.front()].position.x - 420.f, t1 + 300.f), w2);
}

void Stage1StoryGame::SpawnWave(int id) {
    wave = id + 1;
    bannerTimer = 1.2f;
    float farX = player.position.x;
    for (int idx : arena.Waves()[(size_t)id].enemyIndices) {
        enemies[(size_t)idx].active = true;
        farX = std::max(farX, enemies[(size_t)idx].position.x);
    }
    LockArenaBetween(player.position.x, farX);
    AudioSystem::Get().Play(Sfx::Ui);
}

void Stage1StoryGame::UpdateArena(float dt) {
    const int fired = arena.CheckTrigger(player.position.x);
    if (fired >= 0) SpawnWave(fired);
    bool cleared = true;
    if (arena.ActiveWave() >= 0)
        for (int idx : arena.Waves()[(size_t)arena.ActiveWave()].enemyIndices)
            if (!enemies[(size_t)idx].IsDefeated()) cleared = false;
    // Guardian del escenario al final del tramo, con todas las oleadas limpias
    // (se evalua antes de desbloquear para que haya al menos un frame libre).
    if (!scenarioBossSpawned && !arena.Locked() && arena.AllWavesCleared() &&
        player.position.x >= (float)ScenarioEndX() - 450.f) {
        SpawnScenarioBoss();
        arena.ClearGo();
        LockArenaBetween(player.position.x, (float)ScenarioEndX() - 170.f);
    }
    const bool wasLocked = arena.Locked() && !scenarioBossSpawned;
    arena.Update(dt, cleared);
    if (wasLocked && !arena.Locked()) AudioSystem::Get().Play(Sfx::Ui);
}

// Centra el bloqueo entre el jugador y el enemigo mas lejano para que la
// oleada completa quede en pantalla (la camara se desliza hasta ahi).
void Stage1StoryGame::LockArenaBetween(float playerX, float farX) {
    float lockX = (playerX + farX) * 0.5f;
    lockX = std::max(lockX, playerX - 560.f);
    lockX = std::min(lockX, playerX + 560.f);
    arena.Lock(std::clamp(lockX, camera.minX, camera.maxX));
}

// Limita al jugador y enemigos al tramo del escenario y, con la camara
// bloqueada, a lo que se ve en pantalla.
void Stage1StoryGame::ClampToArena() {
    float left = (float)ScenarioStartX(),
          right = arenaLocked ? (float)ScenarioEndX() - 170.f : (float)ScenarioEndX();
    if (arena.Locked()) {
        left = std::max(left, arena.LockX() - 600.f);
        right = std::min(right, arena.LockX() + 580.f);
    }
    player.position.x = std::clamp(player.position.x, left, right);
    player.position.y = std::clamp(player.position.y, kLaneMin, kLaneMax);
    const float eLeft = (float)ScenarioStartX(), eRight = (float)ScenarioEndX();
    for (auto& e : enemies) {
        e.position.x = std::clamp(e.position.x, eLeft, eRight);
        e.position.y = std::clamp(e.position.y, kLaneMin, kLaneMax);
    }
}

void Stage1StoryGame::SpawnScenarioBoss() {
    scenarioBossSpawned = true;
    arenaLocked = true;
    flow = StoryFlow::SubBossIntro;
    bannerTimer = 2.f;
    enemies.clear();
    StreetEnemy e;
    StreetEnemyType t = scenario == 1   ? StreetEnemyType::Brute
                        : scenario == 2 ? StreetEnemyType::Enforcer
                                        : StreetEnemyType::ArmoredGuard;
    e.Init({(float)ScenarioEndX() - 170, 575, 0}, t);
    e.active = true;
    int bonus = scenario == 1 ? 150 : scenario == 2 ? 260 : 380;
    e.hp += bonus;
    e.maxHp = e.hp;
    e.attackDamage += scenario * 4;
    enemies.push_back(e);
    storyMessage = scenario == 1   ? "Teniente del Bloque 17: el guardian no piensa retroceder."
                   : scenario == 2 ? "Ejecutor del mercado: el cruce del canal esta cerrado."
                                   : "Guardia de la puerta: protege el suministro de Brakk.";
}

void Stage1StoryGame::AdvanceScenario() {
    if (scenario >= 4) {
        EnterFinalBoss();
        return;
    }
    ++scenario;
    wave = 0;
    scenarioBossSpawned = false;
    arenaLocked = false;
    transitionTimer = 2.2f;
    storyTimer = 2.2f;
    flow = StoryFlow::ScenarioClear;
    if (scenario == 2) storyMessage = "Los simbolos de cadena conducen al viejo mercado.";
    if (scenario == 3) storyMessage = "La ruta de carga es la ultima defensa antes del territorio de Brakk.";
    if (scenario == 4) storyMessage = "Las cadenas convergen. Brakk espera al final del astillero.";
}

bool Stage1StoryGame::ScenarioWaveCleared() const {
    return !enemies.empty() &&
           std::all_of(enemies.begin(), enemies.end(), [](const StreetEnemy& e) { return e.IsDefeated(); });
}

bool Stage1StoryGame::AllCurrentEnemiesDefeated() const {
    return ScenarioWaveCleared();
}

} // namespace district_fury
