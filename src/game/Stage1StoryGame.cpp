// Stage 1 (La ruta de las cadenas): ciclo de vida, flujo de pantallas, dificultad, guardado y puntuacion.
#include "game/stage1/Stage1Common.h"

namespace district_fury {

Stage1StoryGame::Stage1StoryGame() = default;

void Stage1StoryGame::Init() {
    LoadSave();
    ResetRun();
    flow = StoryFlow::Menu;
}

void Stage1StoryGame::ResetRun() {
    player.Reset();
    player.position = {180, 585, 0};
    enemies.clear();
    projectiles.clear();
    particles.clear();
    combatWorld.Reset();
    scenario = 1;
    wave = 0;
    combo = 0;
    maxCombo = 0;
    defeated = 0;
    damageTaken = 0;
    score = 0;
    stageTime = 0;
    comboTimer = 0;
    hitstop.Reset();
    shake = 0;
    bannerTimer = 0;
    transitionTimer = 0;
    storyTimer = 0;
    arenaLocked = false;
    scenarioBossSpawned = false;
    finalBossSpawned = false;
    stageComplete = false;
    cameraX = 640;
    camera = StageCamera{};
    camera.maxX = kStageEnd - 640.f;
    boss = StoryBoss{};
    boss.position = {5500, 575, 0};
    storyMessage = "Rayden entra al Barrio Bajo. La banda de Brakk controla la ruta hacia el astillero.";
    BuildScenario(1);
}

int Stage1StoryGame::ScenarioStartX() const {
    return scenario == 1 ? 180 : scenario == 2 ? 1500 : scenario == 3 ? 2900 : 4300;
}

int Stage1StoryGame::ScenarioEndX() const {
    return scenario == 1 ? 1450 : scenario == 2 ? 2850 : scenario == 3 ? 4250 : 5900;
}

const char* Stage1StoryGame::ScenarioName() const {
    switch (scenario) {
    case 1:
        return "BARRIO BAJO // BLOQUE 17";
    case 2:
        return "MERCADO ANTIGUO // LINEA DEL CANAL";
    case 3:
        return "PUERTA DE ACERO // RUTA DE CARGA";
    default:
        return "ASTILLERO DE CADENAS // TERRITORIO DE BRAKK";
    }
}

const char* Stage1StoryGame::ScenarioObjective() const {
    switch (scenario) {
    case 1:
        return "Rompe el bloqueo de la calle.";
    case 2:
        return "Atraviesa la ruta del mercado.";
    case 3:
        return "Toma la puerta de carga.";
    default:
        return "Llega al astillero de Brakk.";
    }
}

const char* Stage1StoryGame::DifficultyText() const {
    return difficulty == StoryDifficulty::Easy   ? "FACIL"
           : difficulty == StoryDifficulty::Hard ? "DIFICIL"
                                                 : "NORMAL";
}

void Stage1StoryGame::ApplyDifficulty() {
    float hm = difficulty == StoryDifficulty::Easy   ? .82f
               : difficulty == StoryDifficulty::Hard ? 1.20f
                                                     : 1.f,
          dm = difficulty == StoryDifficulty::Easy   ? .82f
               : difficulty == StoryDifficulty::Hard ? 1.18f
                                                     : 1.f;
    for (auto& e : enemies) {
        e.hp = std::max(1, (int)std::round(e.hp * hm));
        e.maxHp = e.hp;
        e.attackDamage = std::max(1, (int)std::round(e.attackDamage * dm));
    }
    boss.maxHp = difficulty == StoryDifficulty::Easy ? 650 : difficulty == StoryDifficulty::Hard ? 1050 : 820;
    boss.hp = boss.maxHp;
}

void Stage1StoryGame::BuildScenario(int id) {
    enemies.clear();
    wave = 0;
    scenarioBossSpawned = false;
    arenaLocked = false;
    for (const auto& s : kScenarioWaves[id - 1])
        if (s.x > 0) {
            StreetEnemy e;
            e.Init({s.x, s.y, 0}, s.type);
            e.active = false;
            enemies.push_back(e);
        }
    ApplyDifficulty();
    BuildWaves();
}

int Stage1StoryGame::CalculateRank() const {
    float v = std::max(0.f, 360.f - stageTime) * .32f + maxCombo * 11.f + player.hp * 1.7f -
              damageTaken * 1.7f + (scenario == 4 && stageComplete ? 100.f : 0.f);
    if (v >= 520) return 7;
    if (v >= 430) return 6;
    if (v >= 350) return 5;
    if (v >= 275) return 4;
    if (v >= 210) return 3;
    if (v >= 145) return 2;
    if (v >= 80) return 1;
    return 0;
}

int Stage1StoryGame::CalculateScore() const {
    return score + player.hp * 5 + maxCombo * 110;
}

const char* Stage1StoryGame::RankText() const {
    return RankName(CalculateRank());
}

void Stage1StoryGame::LoadSave() {
    std::string text;
    if (!platform::LoadTextFile(savePath, text)) return;
    std::istringstream in(text);
    in >> xp >> coins >> gems >> level >> bestScore >> bestRank;
    int d = 1;
    in >> d;
    difficulty = d == 0 ? StoryDifficulty::Easy : d == 2 ? StoryDifficulty::Hard : StoryDifficulty::Normal;
    saveLoaded = true;
}

void Stage1StoryGame::SaveProgress() {
    std::ostringstream out;
    out << xp << ' ' << coins << ' ' << gems << ' ' << level << ' ' << bestScore << ' ' << bestRank << ' '
        << (difficulty == StoryDifficulty::Easy   ? 0
            : difficulty == StoryDifficulty::Hard ? 2
                                                  : 1)
        << '\n';
    platform::SaveTextFile(savePath, out.str());
}

void Stage1StoryGame::Update(float dt) {
    dt = std::min(dt, .033f);
    if (flow == StoryFlow::Menu) {
        // DF-013: navegacion real de 7 items (ui/MainMenu.h dibuja el
        // selector). Se conservan los atajos directos (V, C) para no romper
        // habitos de quien ya jugaba la version anterior.
        constexpr int kMenuItemCount = 7;
        auto CycleDifficulty = [&](int dir) {
            const int order[3] = {0, 1, 2};
            (void)order;
            if (dir > 0)
                difficulty = difficulty == StoryDifficulty::Easy     ? StoryDifficulty::Normal
                             : difficulty == StoryDifficulty::Normal ? StoryDifficulty::Hard
                                                                     : StoryDifficulty::Easy;
            else
                difficulty = difficulty == StoryDifficulty::Hard     ? StoryDifficulty::Normal
                             : difficulty == StoryDifficulty::Normal ? StoryDifficulty::Easy
                                                                     : StoryDifficulty::Hard;
            SaveProgress();
        };
        if (input::Pressed(KEY_UP) || input::Pressed(KEY_W))
            menuCursor = (menuCursor + kMenuItemCount - 1) % kMenuItemCount;
        if (input::Pressed(KEY_DOWN) || input::Pressed(KEY_S)) menuCursor = (menuCursor + 1) % kMenuItemCount;
        if (menuCursor == 2 && (input::Pressed(KEY_LEFT) || input::Pressed(KEY_RIGHT)))
            CycleDifficulty(input::Pressed(KEY_RIGHT) ? 1 : -1);
        if (input::Pressed(KEY_V)) {
            vsRequested = true;
            return;
        }
        if (input::Pressed(KEY_C)) {
            flow = StoryFlow::Controls;
            return;
        }
        if (input::Pressed(KEY_ENTER) || input::Pressed(KEY_J)) {
            switch (menuCursor) {
            case 0:
                ResetRun();
                newGameStarted = true;
                flow = StoryFlow::CharacterSelect;
                break;
            case 1:
                vsRequested = true;
                break;
            case 2:
                CycleDifficulty(1);
                break;
            case 3:
                flow = StoryFlow::Controls;
                break;
            case 4:
                flow = StoryFlow::Options;
                break;
            case 5:
                flow = StoryFlow::Credits;
                break;
            case 6:
                exitRequested = true;
                break;
            default:
                break;
            }
        }
        return;
    }
    // DF-014: eleccion de luchador al empezar la historia (Rayden o Rayder).
    if (flow == StoryFlow::CharacterSelect) {
        constexpr int n = (int)(sizeof(kStoryCharacters) / sizeof(kStoryCharacters[0]));
        if (input::Pressed(KEY_LEFT) || input::Pressed(KEY_A)) {
            characterCursor = (characterCursor + n - 1) % n;
            AudioSystem::Get().Play(Sfx::Ui);
        }
        if (input::Pressed(KEY_RIGHT) || input::Pressed(KEY_D)) {
            characterCursor = (characterCursor + 1) % n;
            AudioSystem::Get().Play(Sfx::Ui);
        }
        if (input::Pressed(KEY_ESCAPE)) {
            flow = StoryFlow::Menu;
            return;
        }
        if (input::Pressed(KEY_ENTER) || input::Pressed(KEY_J)) {
            player.ApplyCharacter(kStoryCharacters[characterCursor]);
            ResetRun();
            flow = StoryFlow::Intro;
            bannerTimer = 2.4f;
        }
        return;
    }
    if (flow == StoryFlow::Options) {
        if (input::Pressed(KEY_ENTER) || input::Pressed(KEY_J))
            AudioSystem::Get().SetMuted(!AudioSystem::Get().IsMuted());
        if (input::Pressed(KEY_ESCAPE)) flow = StoryFlow::Menu;
        return;
    }
    if (flow == StoryFlow::Credits) {
        if (input::Pressed(KEY_ESCAPE) || input::Pressed(KEY_ENTER) || input::Pressed(KEY_J))
            flow = StoryFlow::Menu;
        return;
    }
    if (flow == StoryFlow::Controls) {
        if (input::Pressed(KEY_ESCAPE) || input::Pressed(KEY_C)) flow = StoryFlow::Menu;
        return;
    }
    if (input::Pressed(KEY_ESCAPE)) {
        if (flow == StoryFlow::Combat || flow == StoryFlow::Boss)
            flow = StoryFlow::Pause;
        else if (flow == StoryFlow::Pause)
            flow = finalBossSpawned ? StoryFlow::Boss : StoryFlow::Combat;
    }
    if (flow == StoryFlow::Pause) return;
    if (flow == StoryFlow::GameOver) {
        if (input::Pressed(KEY_R)) {
            ResetRun();
            flow = StoryFlow::Intro;
        }
        if (input::Pressed(KEY_Q)) flow = StoryFlow::Menu;
        return;
    }
    if (flow == StoryFlow::StageClear) {
        if (input::Pressed(KEY_ENTER) || input::Pressed(KEY_J))
            advanceRequested = true;
        else if (input::Pressed(KEY_R))
            flow = StoryFlow::Menu;
        return;
    }
    if (flow == StoryFlow::ScenarioClear) {
        transitionTimer -= dt;
        if (transitionTimer <= 0) {
            BuildScenario(scenario);
            player.position.x = ScenarioStartX() + 90;
            camera.x = std::clamp(player.position.x, camera.minX, camera.maxX);
            cameraX = camera.x;
            flow = StoryFlow::Combat;
            bannerTimer = 2.f;
        }
        return;
    }
    player.PumpInput(dt);
    if (hitstop.Consume(dt)) return;
    shake = std::max(0.f, shake - dt);
    bannerTimer = std::max(0.f, bannerTimer - dt);
    UpdateParticles(dt);
    if (flow == StoryFlow::Intro) {
        bannerTimer -= dt;
        if (bannerTimer <= 0) {
            flow = StoryFlow::Combat;
        }
        return;
    }
    if (flow == StoryFlow::SubBossIntro) {
        bannerTimer -= dt;
        if (bannerTimer <= 0) flow = StoryFlow::Combat;
        UpdateCombat(dt);
        return;
    }
    if (flow == StoryFlow::Combat) {
        stageTime += dt;
        UpdateCombat(dt);
    }
    if (flow == StoryFlow::BossIntro || flow == StoryFlow::Boss) {
        stageTime += dt;
        UpdateBossFight(dt);
        if (flow == StoryFlow::Boss && player.state == PlayerState::Defeat) flow = StoryFlow::GameOver;
    }
    camera.Follow(player.position.x, dt, arena.Locked(), arena.LockX());
    cameraX = camera.x;
}

} // namespace district_fury
