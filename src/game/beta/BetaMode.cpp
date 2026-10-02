#include "game/beta/BetaMode.h"
#include "audio/AudioSystem.h"
#include "core/InputMap.h"
#include "core/Platform.h"
#include "game/CharacterVisual.h"
#include "game/beta/BetaCharacter.h"
#include "game/beta/BetaHud.h"
#include "game/lab/KfReference.h"
#include "rendering/AssetManager.h"
#include "ui/GameHUD.h"
#include "ui/TouchControls.h"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace district_fury {
namespace {

// Escenarios (data/beta/stages) en el orden del selector.
const char* kStageIds[] = {"barco", "nieve", "cueva_helada", "volcan", "arena_volcan", "nieve_larga"};
constexpr int kStageCount = 6;
constexpr int kBetaCount = 15;   // personajes BETA (CharacterVisual desde FirstBetaCharacter)
constexpr int kFieldCount = 4;

// Indices dentro del bloque BETA (mismo orden que CharacterVisual / KfRoster).
enum B { Espadas, Cestus, Rayo, Garras, Esqueleto, Excavador, Momia, Elefante, Ave, Centauro, CentauroRojo,
         Bruto, BrutoRojo, Medusa, MedusaRoja };
// Jefes (valores negativos en las oleadas): Spine horneado, data/beta/bosses.
enum { JefePoseidon = -1, JefeTentaculos = -2, JefeTitan = -3 };
const char* kBossIds[] = {"jefe_poseidon", "jefe_tentaculos", "jefe_titan"};
const char* kBossNames[] = {"POSEIDON", "MONSTRUO DE TENTACULOS", "TITAN"};
constexpr int kBossCount = 3;

// Oleadas por escenario: los enemigos que el juego original pone en esas zonas
// (barco: no-muertos; montana: bestias y centauros; volcan: brutos y medusas).
// Luego el enemigo grande de la zona y, al final, el jefe (Poseidon en el barco,
// el monstruo de tentaculos en la montana, el Titan en el volcan).
const std::vector<std::vector<int>>& WavesFor(int stage) {
    static const std::vector<std::vector<std::vector<int>>> kWaves = {
        {{Esqueleto, Esqueleto}, {Excavador, Esqueleto, Momia}, {Momia, Excavador, Esqueleto}, {Bruto}, {JefePoseidon}},
        {{Esqueleto, Ave}, {Elefante, Esqueleto}, {Momia, Ave, Excavador}, {Centauro}, {JefeTentaculos}},
        {{Esqueleto, Excavador}, {Elefante, Ave}, {Centauro, Esqueleto}, {CentauroRojo}, {JefeTentaculos}},
        {{Excavador, Esqueleto}, {Elefante, Momia}, {Bruto, Esqueleto}, {Medusa}, {JefeTitan}},
        {{Momia, Excavador}, {BrutoRojo}, {Medusa, Esqueleto}, {MedusaRoja}, {JefeTitan}},
        {{Esqueleto, Ave}, {Elefante, Excavador}, {Momia, Esqueleto, Ave}, {Centauro, Elefante}, {CentauroRojo},
         {JefeTentaculos}},
    };
    return kWaves[(size_t)std::clamp(stage, 0, kStageCount - 1)];
}

// HISTORIA (3 capitulos). Escenario, oleadas (negativo = jefe) y texto de cada
// capitulo. Jefes: Bruto de la bola (capitan de la costa), Medusa (la bestia del
// templo) y, en el volcan, el Titan y al final Poseidon con el remate final.
struct StoryChapterDef {
    int stage;
    std::vector<std::vector<int>> waves;
    const char* title;
    const char* lines[5];
    int showcase;     // indice BETA del personaje que se anima en la tarjeta; <0 = jefe
};
const StoryChapterDef& StoryDef(int c) {
    static const StoryChapterDef kStory[3] = {
        {0,
         {{Esqueleto, Esqueleto}, {Excavador, Esqueleto, Momia}, {Momia, Esqueleto, Excavador}, {Bruto}},
         "CAPITULO 1  //  EL ATAQUE EN LA COSTA",
         {"Todo empieza en plena accion: la flota enemiga asalta la costa.",
          "El guerrero se abre paso en la cubierta entre soldados no-muertos y bestias.",
          "Vence cada oleada para abrir el paso sellado con magia.",
          "Al final espera el capitan: el Bruto de la bola. Debilitalo y,",
          "cuando aparezca el boton rojo, pulsa GOLPE para rematarlo."},
         Bruto},
        {2,
         {{Esqueleto, Momia}, {Elefante, Esqueleto}, {Momia, Excavador, Ave}, {Medusa}},
         "CAPITULO 2  //  EL TEMPLO Y LAS CATACUMBAS",
         {"Tras limpiar la costa, el guerrero baja a las catacumbas de piedra helada.",
          "Ahora salen no-muertos, momias con escudo y bestias enormes.",
          "Cada enemigo vencido suelta un alma verde que cura un poco.",
          "En el fondo del templo aguarda Medusa, la criatura serpiente.",
          "Esquiva sus ataques, bajale la vida y rematala en la escena final."},
         Medusa},
        {3,
         {{Excavador, Esqueleto}, {CentauroRojo}, {BrutoRojo, Momia}, {JefeTitan}, {JefePoseidon}},
         "CAPITULO 3  //  EL CLIMAX: EL VOLCAN",
         {"La ultima seccion: ruinas en llamas a las puertas del Olimpo.",
          "Los enemigos aguantan mas golpes: usa habilidades, la furia (omega) y pociones.",
          "Un Titan asoma tras la lava y te ataca con sus manos gigantes.",
          "Al final te enfrentas a un dios: Poseidon. Usa la magia acumulada",
          "y vencelo con el remate final para terminar la historia."},
         -1},
    };
    return kStory[std::clamp(c, 0, 2)];
}

int CharIndex(int beta) { return FirstBetaCharacter() + std::clamp(beta, 0, kBetaCount - 1); }

const KfReference& RefFor(int character) {
    static KfReference none;
    const int roster = GetCharacterVisual(character).kfRoster;
    return roster >= 0 ? GetKfCharacter(roster) : none;
}

std::map<std::string, KfReference>& FxCache() {
    static std::map<std::string, KfReference> cache;
    return cache;
}

bool Overlap(const CombatBox& a, const CombatBox& b, Vector2& center) {
    if (!a.Intersects(b)) return false;
    const float x0 = std::max(a.x, b.x), x1 = std::min(a.x + a.width, b.x + b.width);
    const float y0 = std::max(a.y, b.y), y1 = std::min(a.y + a.height, b.y + b.height);
    center = {(x0 + x1) * 0.5f, (y0 + y1) * 0.5f};
    return true;
}

std::string FindAsset(const std::string& rel) {
    for (const std::string& p : {rel, "../" + rel, "../../" + rel})
        if (platform::AssetExists(p)) return p;
    return {};
}

}  // namespace

int BetaMode::TouchContext() const {
    if (flow == Flow::Select || flow == Flow::StoryCard) return 1;
    if (qte.active) return 3;
    return result != 0 ? 2 : 3;
}

void BetaMode::Init() {
    flow = Flow::Select;
    cursor = 0;
    exitRequested = false;
    fighters.clear();
    effects.clear();
    result = 0;
    AudioSystem::Get().SetBetaAudio(true);
    LoadProgress();
    PlayMusic("menu.ogg");
}

void BetaMode::Shutdown() {
    StopMusic();
    fighters.clear();
    effects.clear();
    AudioSystem::Get().SetBetaAudio(false);
}

int BetaMode::AliveEnemies() const {
    int n = 0;
    for (const auto& f : fighters)
        if (f->enemy && f->p.state != PlayerState::Defeat) ++n;
    if (boss.active && boss.phase != 4) ++n;
    return n;
}

BetaMode::Fighter& BetaMode::AddFighter(int character, bool enemy, float x, float y) {
    auto f = std::make_unique<Fighter>();
    f->character = character;
    f->enemy = enemy;
    f->p.Reset();
    f->p.ApplyCharacter(character);
    f->p.position = {x, y, 0};
    f->p.facing = enemy ? Facing::Left : Facing::Right;
    f->lastHp = f->p.hp;
    f->sounds = RefFor(character).sounds;
    if (!fighters.empty()) {   // todos menos el jugador los maneja RivalAI
        f->in = PlayerInput{};
        f->p.scriptedInput = &f->in;
        f->ai.Reset();
    }
    fighters.push_back(std::move(f));
    return *fighters.back();
}

void BetaMode::StartFight() {
    flow = Flow::Fight;
    fighters.clear();
    effects.clear();
    result = 0;
    resultTimer = 0;
    shake = hitstop = 0;
    wave = 0;
    waveActive = false;
    banner = 0;
    time = 0;
    boss = BossFight{};
    qte = Qte{};
    numbers.clear();
    comboHits = 0;
    comboTimer = 0;
    redPotions = bluePotions = 5;
    if (selMode == 3) selStage = StoryDef(storyChapter).stage;
    stage.Load(kStageIds[std::clamp(selStage, 0, kStageCount - 1)]);
    const float left = stage.Loaded() ? stage.WorldLeft() : 90.0f;
    AddFighter(CharIndex(selCharacter), false, left + 220.0f, 570.0f);
    cameraX = std::max(0.0f, left);
    lockLeft = stage.Loaded() ? stage.WorldLeft() + 40.0f : 120.0f;
    lockRight = stage.Loaded() ? std::min(stage.WorldRight(), kStageEndX) - 40.0f : 1160.0f;
    if (selMode == 2) {
        // JEFE: pelea directa contra el jefe elegido en la primera pantalla.
        lockLeft = cameraX + 60.0f;
        lockRight = cameraX + 1220.0f;
        waveCount = 1;
        waveActive = SpawnBoss(kBossIds[std::clamp(selBoss, 0, kBossCount - 1)], lockRight);
        bannerText = TextFormat("JEFE  //  %s", kBossNames[std::clamp(selBoss, 0, kBossCount - 1)]);
    } else if (selMode == 1) {
        // 1 VS 1: el rival aparece enfrente, en la misma pantalla.
        Fighter& r = AddFighter(CharIndex(selRival), true, left + 900.0f, 570.0f);
        r.p.facing = Facing::Left;
        waveCount = 1;
        waveActive = true;
        lockLeft = cameraX + 60.0f;
        lockRight = cameraX + 1220.0f;
        bannerText = "1 VS 1  //  PELEA";
    } else {
        waveCount = (int)CurrentWaves().size();
        bannerText = selMode == 3 ? StoryDef(storyChapter).title : "AVANZA";
        if (selMode == 3) {
            // Mejoras guardadas: +15 de vida maxima por historia completada (hasta 3).
            Player& h = fighters[0]->p;
            h.maxHp += 15 * std::min(3, storyClears);
            h.hp = h.maxHp;
            fighters[0]->lastHp = h.hp;
        }
    }
    banner = 2.0f;
    PlaySound("sound24");
    PlayMusic(stage.music.empty() ? "gate1music.ogg" : stage.music);
}

void BetaMode::SpawnWave() {
    const auto& list = CurrentWaves()[(size_t)wave];
    lockLeft = std::max(stage.WorldLeft() + 40.0f, cameraX + 60.0f);
    lockRight = std::min(std::min(stage.WorldRight(), kStageEndX) - 40.0f, cameraX + 1220.0f);
    const float ys[] = {560.0f, 520.0f, 605.0f, 540.0f};
    for (size_t i = 0; i < list.size(); ++i) {
        if (list[i] < 0) {   // jefe
            SpawnBoss(kBossIds[std::clamp(-list[i] - 1, 0, kBossCount - 1)], lockRight);
            continue;
        }
        // Llegan por los dos lados de la pantalla, como en el original.
        const bool fromLeft = (i % 2) == 1 && lockLeft + 200.0f < fighters[0]->p.position.x;
        const float x = fromLeft ? lockLeft + 30.0f : lockRight - 30.0f - (float)i * 40.0f;
        Fighter& e = AddFighter(CharIndex(list[i]), true, x, ys[i % 4]);
        e.p.facing = fromLeft ? Facing::Right : Facing::Left;
        if (e.p.animator.HasClip("intro")) e.p.animator.PlayNamed("intro");
    }
    waveActive = true;
    bannerText = boss.active ? TextFormat("JEFE  //  %s", boss.name.c_str())
               : wave + 2 == waveCount ? "ENEMIGO FINAL" : TextFormat("OLEADA %d / %d", wave + 1, waveCount);
    banner = 1.8f;
    PlaySound("sound44");
}

void BetaMode::Update(float dt) {
    dt = std::min(dt, 0.033f);
    time += dt;
    if (musicReady) {
        if (AudioSystem::Get().IsMuted()) PauseMusicStream(music);
        else {
            if (!IsMusicStreamPlaying(music)) ResumeMusicStream(music);
            UpdateMusicStream(music);
        }
    }
    if (flow == Flow::Select) {
        if (input::Pressed(KEY_ESCAPE)) {
            PlaySound("sound2");
            exitRequested = true;
            return;
        }
        if (input::Pressed(KEY_UP) || input::Pressed(KEY_W)) { cursor = (cursor + kFieldCount - 1) % kFieldCount; PlaySound("sound13"); }
        if (input::Pressed(KEY_DOWN) || input::Pressed(KEY_S)) { cursor = (cursor + 1) % kFieldCount; PlaySound("sound13"); }
        if (input::Pressed(KEY_LEFT) || input::Pressed(KEY_RIGHT) || input::Pressed(KEY_A) || input::Pressed(KEY_D)) {
            const int dir = (input::Pressed(KEY_RIGHT) || input::Pressed(KEY_D)) ? 1 : -1;
            if (cursor == 0) selCharacter = (selCharacter + dir + kBetaCount) % kBetaCount;
            else if (cursor == 1) selStage = (selStage + dir + kStageCount) % kStageCount;
            else if (cursor == 2) selMode = (selMode + dir + 4) % 4;
            else if (selMode == 2) selBoss = (selBoss + dir + kBossCount) % kBossCount;
            else selRival = (selRival + dir + kBetaCount) % kBetaCount;
            PlaySound(cursor == 0 ? "sound14" : "sound13");
        }
        if (input::Pressed(KEY_ENTER) || input::Pressed(KEY_J)) {
            if (selMode == 3) StartStory();
            else StartFight();
        }
        return;
    }
    if (flow == Flow::StoryCard) {
        cardTime += dt;
        cardAnim.Update(dt);
        if (cardSkel.Valid()) cardSkel.Update(dt);
        if (input::Pressed(KEY_ESCAPE)) { flow = Flow::Select; PlayMusic("menu.ogg"); return; }
        if (cardTime > 0.6f && (input::Pressed(KEY_ENTER) || input::Pressed(KEY_J))) {
            PlaySound("sound24");
            if (storyChapter >= 3) {   // final: de vuelta al menu principal
                storyChapter = 0;
                flow = Flow::Select;
                exitRequested = true;
                return;
            }
            StartFight();
        }
        return;
    }
    if (input::Pressed(KEY_ESCAPE)) {
        PlaySound("sound2");
        flow = Flow::Select;
        fighters.clear();
        effects.clear();
        PlayMusic("menu.ogg");
        return;
    }
    if (input::Pressed(KEY_R)) { StartFight(); return; }
    if (result != 0) {
        resultTimer += dt;
        for (auto& f : fighters) f->p.Update(dt);   // poses de victoria/derrota
        if (resultTimer > 0.6f && (input::Pressed(KEY_ENTER) || input::Pressed(KEY_J))) {
            if (selMode == 3 && result == 1) {   // HISTORIA: siguiente capitulo (o final)
                ++storyChapter;
                if (storyChapter >= 3) {
                    storyClears = std::min(3, storyClears + 1);
                    SaveProgress();
                }
                ShowStoryCard();
            } else {
                StartFight();
            }
        }
        return;
    }
    if (qte.active) { UpdateQte(dt); return; }
    UpdateFight(dt);
    CheckQteTriggers();
}

void BetaMode::UpdateFight(float dt) {
    banner = std::max(0.0f, banner - dt);
    shake = std::max(0.0f, shake - dt);
    if ((comboTimer -= dt) <= 0.0f) comboHits = 0;
    for (DamageNumber& n : numbers) n.t += dt;
    numbers.erase(std::remove_if(numbers.begin(), numbers.end(), [](const DamageNumber& n) { return n.t > 0.9f; }), numbers.end());
    Fighter& hero = *fighters[0];
    if (hero.p.state != PlayerState::Defeat) {
        // Pociones del original: roja = vida, azul = magia (energia y furia).
        if (input::Pressed(KEY_Z) && redPotions > 0 && hero.p.hp < hero.p.maxHp) {
            --redPotions;
            hero.p.hp = std::min(hero.p.maxHp, hero.p.hp + hero.p.maxHp * 35 / 100);
            hero.lastHp = hero.p.hp;
            PlaySound("sound6");
            SpawnFx("fx_curacion", "a1", {hero.p.position.x, hero.p.position.y}, false);
        }
        if (input::Pressed(KEY_X) && bluePotions > 0) {
            --bluePotions;
            hero.p.sp = hero.p.maxSp;
            hero.p.rage = std::min(hero.p.maxRage, hero.p.rage + hero.p.maxRage / 2);
            PlaySound("sound6");
            SpawnFx("fx_alma_azul", "a0", {hero.p.position.x, hero.p.position.y - 80.0f}, false);
        }
        if (input::Pressed(KEY_Q)) SwitchWeapon();
    }
    hero.p.PumpInput(dt);
    if (hitstop > 0) { hitstop -= dt; return; }

    // Avance por el escenario: la oleada siguiente aparece al llegar a su zona.
    if ((selMode == 0 || selMode == 3) && !waveActive && wave < waveCount) {
        const float span = std::max(1.0f, std::min(stage.WorldRight(), kStageEndX) - stage.WorldLeft() - 1400.0f);
        const float trigger = stage.WorldLeft() + 500.0f + span * (waveCount > 1 ? (float)wave / (waveCount - 1) : 0.0f);
        if (hero.p.position.x >= trigger || wave == 0) SpawnWave();
    }

    for (auto& f : fighters) UpdateFighter(*f, dt);
    UpdateBoss(dt);

    // Golpes: el jugador puede alcanzar a varios enemigos con el mismo golpe.
    for (size_t i = 1; i < fighters.size(); ++i) ResolveHits(hero, *fighters[i], 1.0f);
    if (hero.p.AttackIsActive() && !hero.p.hasHit) {
        for (size_t i = 1; i < fighters.size(); ++i)
            if (fighters[i]->lastHp != fighters[i]->p.hp) { hero.p.hasHit = true; break; }
    }
    for (size_t i = 1; i < fighters.size(); ++i) ResolveHits(*fighters[i], hero, selMode == 1 ? 1.0f : 0.55f);
    for (auto& f : fighters) PollSounds(*f);

    // Efectos.
    for (Fx& fx : effects) fx.anim.Update(dt);
    effects.erase(std::remove_if(effects.begin(), effects.end(), [](const Fx& fx) { return fx.anim.isFinished; }),
                  effects.end());

    // Enemigos vencidos: se quedan en el suelo un momento y desaparecen.
    for (size_t i = 1; i < fighters.size();) {
        if (fighters[i]->p.state == PlayerState::Defeat && (fighters[i]->gone += dt) > 2.6f)
            fighters.erase(fighters.begin() + (long)i);
        else
            ++i;
    }

    if (hero.p.state == PlayerState::Defeat) {
        result = -1;
        PlayMusic("music_003.ogg");
        return;
    }
    if (boss.active && boss.phase == 4 && boss.deadTime > 2.5f) boss.active = false;
    if (waveActive && AliveEnemies() == 0 && !boss.active) {
        waveActive = false;
        ++wave;
        if (wave >= waveCount) {
            result = 1;
            PlaySound("sound48");
            PlayMusic("music_002.ogg");
            return;
        }
        lockLeft = stage.WorldLeft() + 40.0f;
        lockRight = std::min(stage.WorldRight(), kStageEndX) - 40.0f;
        bannerText = "AVANZA  >>";
        banner = 2.0f;
    }
    UpdateCamera(dt);
}

void BetaMode::UpdateFighter(Fighter& f, float dt) {
    Fighter& hero = *fighters[0];
    if (&f != &hero) {
        f.in = f.p.state == PlayerState::Defeat || hero.p.state == PlayerState::Defeat ? PlayerInput{}
                                                                                    : f.ai.Think(f.p, hero.p, dt);
        if (f.p.state != PlayerState::Attack && f.p.state != PlayerState::Defeat)
            f.p.facing = hero.p.position.x > f.p.position.x ? Facing::Right : Facing::Left;
        f.p.PumpInput(dt);
    }
    f.p.Update(dt);
    f.p.position.x = std::clamp(f.p.position.x, lockLeft, lockRight);
    f.p.position.y = std::clamp(f.p.position.y, kLaneMinY, kLaneMaxY);
}

void BetaMode::ResolveHits(Fighter& a, Fighter& t, float damageScale) {
    if (a.p.state == PlayerState::Defeat || t.p.state == PlayerState::Defeat) return;
    if (!a.p.AttackIsActive() || a.p.hasHit) return;
    // Solo pelean en la misma linea de profundidad (beat'em up).
    if (std::fabs(a.p.position.y - t.p.position.y) > 42.0f) return;
    Vector2 c{};
    if (!Overlap(a.p.GetAttackHitbox(), t.p.GetHurtbox(), c)) return;
    const int before = t.p.hp;
    const int dmg = std::max(1, (int)std::lround((a.p.GetAttackDamage() + (a.p.isRageMode ? 5 : 0)) * damageScale));
    t.p.TakeDamage(dmg);
    if (&a != fighters[0].get()) a.p.hasHit = true;
    if (t.p.hp < before) {
        const bool heroHit = &a == fighters[0].get();
        if (heroHit) OnHeroHit(c, before - t.p.hp);
        else numbers.push_back({{c.x, c.y}, before - t.p.hp, 0.0f, false});
        SpawnFx(heroHit ? (a.p.isRageMode ? "fx_impacto_oro" : "fx_impacto") : "fx_impacto_morado",
                TextFormat("a%d", GetRandomValue(0, 5)), c, a.p.facing == Facing::Left);
        shake = 0.08f;
        hitstop = heroHit ? 0.035f : 0.02f;
    }
}

void BetaMode::PollSounds(Fighter& f) {
    const Animator& an = f.p.animator;
    const std::size_t idx = an.CurrentClipFrameIndex();
    if (an.currentClipName != f.lastClip || idx != f.lastFrame) {
        // Sonidos de cuadro (golpes de arma, habilidades) del juego original.
        for (const auto& s : an.currentClip.sounds)
            if ((std::size_t)s.first == idx) PlaySound(s.second);
        f.lastClip = an.currentClipName;
        f.lastFrame = idx;
    }
    if (f.p.hp < f.lastHp) {
        if (f.p.state == PlayerState::Defeat) {
            const auto it = f.sounds.find("die");
            if (it != f.sounds.end()) PlaySound(it->second);
        } else {
            const auto it = f.sounds.find("hurt");
            if (it != f.sounds.end()) PlaySound(it->second);
        }
    }
    if (f.p.state == PlayerState::Defeat && !f.deathFx) {
        f.deathFx = true;
        SpawnFx(f.enemy ? "fx_humo_enemigo" : "fx_humo_muerte", "a0", {f.p.position.x, f.p.position.y}, false);
        if (f.enemy) SpawnFx(selMode == 3 ? "fx_alma_verde" : "fx_alma_roja", "a0", {f.p.position.x, f.p.position.y - 90.0f}, false);
        // HISTORIA: el alma verde cura un poco al guerrero (orbes verdes del original).
        if (f.enemy && selMode == 3 && fighters[0]->p.state != PlayerState::Defeat) {
            Player& h = fighters[0]->p;
            h.hp = std::min(h.maxHp, h.hp + 8);
            fighters[0]->lastHp = h.hp;
        }
    }
    f.lastHp = f.p.hp;
}

void BetaMode::SpawnFx(const std::string& id, const std::string& clip, Vector2 pos, bool flip) {
    auto& cache = FxCache();
    auto it = cache.find(id);
    if (it == cache.end()) it = cache.emplace(id, LoadBetaCharacter(id)).first;
    if (!it->second.loaded) return;
    Fx fx;
    fx.anim = it->second.templ;
    if (!fx.anim.PlayNamed(clip) && !fx.anim.PlayNamed("a0")) return;
    fx.anim.currentClip.loop = false;
    fx.pos = pos;
    fx.flip = flip;
    effects.push_back(std::move(fx));
}

void BetaMode::UpdateCamera(float dt) {
    const Fighter& hero = *fighters[0];
    const float worldMax = std::min(stage.Loaded() ? stage.WorldWidth() : 1280.0f, kStageEndX + 90.0f);
    float target = hero.p.position.x - 560.0f;
    // Durante una oleada la camara queda fija en la zona de pelea.
    if (waveActive) target = std::clamp(target, lockLeft - 60.0f, lockRight + 60.0f - 1280.0f);
    target = std::clamp(target, 0.0f, std::max(0.0f, worldMax - 1280.0f));
    cameraX += (target - cameraX) * std::min(1.0f, dt * 6.0f);
}

void BetaMode::PlaySound(const std::string& name) {
    AudioSystem& audio = AudioSystem::Get();
    if (!audio.IsReady() || audio.IsMuted() || name.empty()) return;
    auto it = soundCache.find(name);
    if (it == soundCache.end()) {
        Sound snd{};
        const std::string path = FindAsset("assets/beta/audio/" + name + ".ogg");
        std::vector<unsigned char> bytes;
        if (!path.empty() && platform::LoadBinaryFile(path, bytes) && !bytes.empty()) {
            Wave w = LoadWaveFromMemory(".ogg", bytes.data(), (int)bytes.size());
            if (w.frameCount > 0) {
                snd = LoadSoundFromWave(w);
                UnloadWave(w);
            }
        }
        it = soundCache.emplace(name, snd).first;
    }
    if (it->second.frameCount > 0) ::PlaySound(it->second);
}

void BetaMode::PlayMusic(const std::string& file) {
    if (musicReady && musicName == file) return;
    StopMusic();
    if (!AudioSystem::Get().IsReady()) return;
    const std::string path = FindAsset("assets/beta/audio/" + file);
    if (path.empty()) return;
    music = LoadMusicStream(path.c_str());
    if (music.frameCount == 0) return;
    // Victoria/derrota suenan una vez; el resto en bucle.
    music.looping = file != "music_002.ogg" && file != "music_003.ogg";
    SetMusicVolume(music, 0.55f);
    PlayMusicStream(music);
    musicReady = true;
    musicName = file;
}

void BetaMode::StopMusic() {
    if (musicReady) {
        StopMusicStream(music);
        UnloadMusicStream(music);
    }
    musicReady = false;
    musicName.clear();
}


const std::vector<std::vector<int>>& BetaMode::CurrentWaves() const {
    return selMode == 3 ? StoryDef(storyChapter).waves : WavesFor(selStage);
}

void BetaMode::LoadProgress() {
    std::string text;
    if (platform::LoadTextFile("district_fury_beta.dat", text)) {
        std::istringstream in(text);
        std::string key;
        while (in >> key) {
            if (key == "historias") in >> storyClears;
            else if (key == "remate_explicado") { int v = 0; in >> v; qteExplained = v != 0; }
        }
    }
    storyClears = std::clamp(storyClears, 0, 3);
}

void BetaMode::SaveProgress() const {
    platform::SaveTextFile("district_fury_beta.dat",
                           TextFormat("historias %d\nremate_explicado %d\n", storyClears, qteExplained ? 1 : 0));
}

void BetaMode::StartStory() {
    // La historia se juega con el guerrero (cualquiera de sus 4 armas).
    if (selCharacter > Garras) selCharacter = Espadas;
    selMode = 3;
    storyChapter = 0;
    ShowStoryCard();
}

void BetaMode::ShowStoryCard() {
    flow = Flow::StoryCard;
    cardTime = 0.0f;
    fighters.clear();
    effects.clear();
    boss = BossFight{};
    qte = Qte{};
    result = 0;
    cardSkel = spine21::Skeleton{};
    // Animacion de la tarjeta: el enemigo principal del capitulo (o el heroe al final).
    const int who = storyChapter >= 3 ? selCharacter : StoryDef(storyChapter).showcase;
    if (who >= 0) {
        const KfReference& r = RefFor(CharIndex(who));
        if (r.loaded) {
            cardAnim = r.templ;
            cardAnim.PlayNamed(storyChapter >= 3 && r.templ.HasClip("victory") ? "victory" : "idle");
        }
    } else {
        // Poseidon (Spine) para el capitulo final.
        static std::shared_ptr<spine21::SkeletonData> pose;
        if (!pose) {
            pose = std::make_shared<spine21::SkeletonData>();
            if (!pose->Load("assets/beta/spine/Poseidon", "Poseidon")) pose.reset();
        }
        if (pose) { cardSkel.SetData(pose); cardSkel.Play("stand", true); }
    }
    PlayMusic(storyChapter >= 3 ? "music_002.ogg" : "gameCG.ogg");
}

void BetaMode::DrawStoryCard() const {
    // Fondo: el escenario del capitulo, oscurecido.
    static BetaStage bg;
    static int bgFor = -1;
    const int st = StoryDef(std::min(storyChapter, 2)).stage;
    if (bgFor != st) { bg.Load(kStageIds[st]); bgFor = st; }
    if (bg.Loaded()) bg.DrawBack(200.0f + cardTime * 12.0f);
    DrawRectangle(0, 0, 1280, 720, {0, 0, 0, 175});
    const Texture2D art = AssetManager::Get().GetTextureByPath("assets/beta/ui/guerrero.png");
    if (art.id) DrawTexturePro(art, {0, 0, (float)art.width, (float)art.height}, {40, 330, art.width * 1.4f, art.height * 1.4f}, {0, 0}, 0, WHITE);
    const float a = std::min(1.0f, cardTime * 2.0f);
    const unsigned char al = (unsigned char)(255 * a);
    if (storyChapter >= 3) {
        DrawText("FIN  //  EL GUERRERO VENCIO", 640 - MeasureText("FIN  //  EL GUERRERO VENCIO", 40) / 2, 90, 40, {255, 215, 120, al});
        DrawText("Venciste a Poseidon con el remate final.", 420, 170, 20, {235, 230, 220, al});
        DrawText(TextFormat("Mejora guardada: +%d de vida maxima en la historia (%d/3 historias completas).",
                            15 * std::min(3, storyClears), storyClears), 260, 210, 18, {150, 230, 150, al});
        DrawText("OK / ENTER: volver al menu principal (puedes jugarla otra vez con tus mejoras).", 250, 250, 16, {220, 220, 230, al});
    } else {
        const StoryChapterDef& d = StoryDef(storyChapter);
        DrawText("HISTORIA", 60, 40, 22, {200, 150, 90, al});
        DrawText(d.title, 60, 70, 34, {255, 215, 120, al});
        for (int i = 0; i < 5; ++i) {
            const float li = std::clamp(cardTime * 1.5f - i * 0.6f, 0.0f, 1.0f);   // las lineas aparecen una tras otra
            DrawText(d.lines[i], 60, 130 + i * 30, 19, {235, 230, 220, (unsigned char)(255 * li)});
        }
        DrawText("OK / ENTER: empezar capitulo      ATRAS / ESC: salir", 60, 290, 15, {200, 200, 210, al});
    }
    // Animacion del enemigo principal del capitulo.
    DrawRectangle(820, 320, 420, 360, {10, 8, 6, 160});
    DrawRectangleLines(820, 320, 420, 360, {200, 150, 70, 200});
    if (cardSkel.Valid()) cardSkel.Draw({1110, 660}, 0.75f, false, WHITE);
    else cardAnim.Draw({1030, 650}, 1.25f, true);
    DrawText(storyChapter >= 3 ? "EL GUERRERO" : "TE ESPERA:", 836, 330, 16, {255, 200, 120, al});
}

void BetaMode::OnHeroHit(Vector2 at, int damage) {
    ++comboHits;
    comboTimer = 2.0f;
    numbers.push_back({at, damage, 0.0f, true});
}

void BetaMode::SwitchWeapon() {
    // El guerrero cambia de arma en plena pelea (Espadas, Cestus, Cadena, Garras):
    // conserva vida, energia, furia, posicion y direccion.
    Fighter& hero = *fighters[0];
    const int w = hero.character - FirstBetaCharacter();
    if (w < 0 || w > Garras) return;
    const int next = (w + 1) % 4;
    const float hpRatio = (float)hero.p.hp / std::max(1, hero.p.maxHp);
    const int sp = hero.p.sp, rage = hero.p.rage;
    const Vector3D pos = hero.p.position;
    const Facing facing = hero.p.facing;
    hero.p.Reset();
    hero.p.ApplyCharacter(CharIndex(next));
    hero.p.position = pos;
    hero.p.facing = facing;
    hero.p.hp = std::max(1, (int)std::lround(hpRatio * hero.p.maxHp));
    hero.p.sp = sp;
    hero.p.rage = rage;
    hero.lastHp = hero.p.hp;
    hero.character = CharIndex(next);
    hero.sounds = RefFor(hero.character).sounds;
    selCharacter = next;
    PlaySound("sound14");
    bannerText = GetCharacterVisual(hero.character).name;
    banner = 1.2f;
}

// ---------------------------------------------------------------- jefes

bool BetaMode::SpawnBoss(const std::string& id, float rightEdge) {
    std::string text;
    const std::string rel = "data/beta/bosses/" + id + ".txt";
    for (const std::string& p : {rel, "../" + rel, "../../" + rel})
        if (platform::LoadTextFile(p, text)) break;
    if (text.empty()) return false;
    BossFight b;
    b.id = id;
    std::string dir, file;
    std::istringstream in(text);
    std::string line;
    auto& fxCache = FxCache();
    auto fit = fxCache.find("fx_rango");
    if (fit == fxCache.end()) fit = fxCache.emplace("fx_rango", LoadBetaCharacter("fx_rango")).first;
    while (std::getline(in, line)) {
        std::istringstream ls(line);
        std::string tag;
        ls >> tag;
        if (tag == "name") std::getline(ls >> std::ws, b.name);
        else if (tag == "spine") ls >> dir >> file >> b.scale;
        else if (tag == "hp") { ls >> b.maxHp; b.hp = b.lastHp = b.maxHp; }
        else if (tag == "draw") ls >> b.drawScale >> b.drawDx >> b.drawDy >> b.layer;
        else if (tag == "body") {
            float x0 = 0, y0 = 0, x1 = 0, y1 = 0;
            ls >> x0 >> y0 >> x1 >> y1;
            b.body = {x0, y0, x1 - x0, y1 - y0};
        } else if (tag == "sound") {
            std::string ev, snd;
            ls >> ev >> snd;
            b.sounds[ev] = snd;
        } else if (tag == "clip") {
            std::string c, anim;
            int loop = 0;
            ls >> c >> anim >> loop;
            b.clips[c] = {anim, loop != 0};
        } else if (tag == "attack") {
            BossAttack at;
            std::string fx;
            std::string hits;
            ls >> at.clip >> at.warn >> at.warnScale >> at.reach >> hits >> fx >> at.sound >> at.damage;
            for (char& ch : hits) if (ch == ',') ch = ' ';
            std::istringstream hs(hits);
            for (float t; hs >> t;) at.hitTimes.push_back(t);
            if (at.hitTimes.empty()) at.hitTimes.push_back(0.7f);
            const auto colon = fx.find(':');
            at.fx = fx.substr(0, colon);
            at.fxClip = colon == std::string::npos ? "a0" : fx.substr(colon + 1);
            at.area = {0, -60, at.reach, 120};
            // Area de golpe = union de las piezas del aviso (anim 99) con su escala.
            if (fit->second.loaded) {
                const auto cit = fit->second.templ.namedClips.find(at.warn);
                if (cit != fit->second.templ.namedClips.end()) {
                    float x0 = 1e9f, y0 = 1e9f, x1 = -1e9f, y1 = -1e9f;
                    for (int fi : cit->second.frames) {
                        if (fi < 0 || fi >= (int)fit->second.templ.frames.size()) continue;
                        for (const FramePiece& pc : fit->second.templ.frames[(size_t)fi].pieces) {
                            x0 = std::min(x0, pc.x); y0 = std::min(y0, pc.y);
                            x1 = std::max(x1, pc.x + pc.src.width); y1 = std::max(y1, pc.y + pc.src.height);
                        }
                    }
                    if (x1 > x0) at.area = {x0 * at.warnScale, y0 * at.warnScale, (x1 - x0) * at.warnScale, (y1 - y0) * at.warnScale};
                }
            }
            b.attacks.push_back(at);
        }
    }
    // Esqueleto compartido entre peleas (se carga una sola vez).
    static std::map<std::string, std::shared_ptr<spine21::SkeletonData>> skeletons;
    auto sit = skeletons.find(id);
    if (sit == skeletons.end()) {
        auto data = std::make_shared<spine21::SkeletonData>();
        if (!data->Load(dir, file)) {
            TraceLog(LOG_WARNING, "BETA: no cargo el Spine %s: %s", id.c_str(), data->error.c_str());
            data.reset();
        }
        sit = skeletons.emplace(id, data).first;
    }
    if (!sit->second || b.attacks.empty()) return false;
    b.skel.SetData(sit->second);
    // Medidas de la pose de reposo: cuanto baja el dibujo bajo el origen y su ancho.
    b.Play("idle");
    const Rectangle bounds = b.skel.Bounds();
    b.ground = std::max(0.0f, (bounds.y + bounds.height) * b.scale);
    if (b.layer > 0) {
        // Gigante: ocupa el lado derecho de la pantalla; el heroe pelea a su izquierda.
        b.pos = {rightEdge - 210.0f, 570.0f, 0};
        lockRight = std::min(lockRight, b.pos.x - 110.0f);   // el heroe pelea delante, no dentro
    } else {
        b.pos = {rightEdge - std::max(80.0f, (bounds.x + bounds.width) * b.scale * BetaStage::kScale) - 20.0f, 575.0f, 0};
    }
    if (!b.Play("intro")) b.Play("idle");
    b.phase = 0;
    b.facing = Facing::Left;
    b.cooldown = 1.6f;
    b.active = true;
    boss = std::move(b);
    PlaySound("sound45");
    return true;
}

CombatBox BetaMode::BossHurtbox() const {
    // Arte nativo mirando a la izquierda: la caja se refleja al mirar a la derecha.
    const float k = BetaStage::kScale;
    const Rectangle& b = boss.body;
    const float feetY = boss.layer > 0 ? boss.pos.y : boss.pos.y - boss.ground * k;
    const float x0 = boss.facing == Facing::Left ? boss.pos.x + b.x * k : boss.pos.x - (b.x + b.width) * k;
    return {x0, feetY + b.y * k, b.width * k, b.height * k};
}

void BetaMode::UpdateBoss(float dt) {
    if (!boss.active) return;
    Fighter& hero = *fighters[0];
    boss.flash = std::max(0.0f, boss.flash - dt);
    boss.skel.Update(dt);
    if (boss.warnOn) boss.warnAnim.Update(dt);
    const float k = BetaStage::kScale;
    if (boss.phase == 4) {
        boss.deadTime += dt;
        if (boss.skel.Finished() && boss.clip != "down") boss.Play("down");
        return;
    }
    // Golpes del heroe al jefe (misma linea de profundidad que en el resto del modo).
    if (hero.p.AttackIsActive() && !hero.p.hasHit && std::fabs(hero.p.position.y - boss.pos.y) < 70.0f) {
        Vector2 c{};
        if (Overlap(hero.p.GetAttackHitbox(), BossHurtbox(), c)) {
            hero.p.hasHit = true;
            const int dmg = std::max(1, hero.p.GetAttackDamage() + (hero.p.isRageMode ? 5 : 0));
            boss.hp -= dmg;
            OnHeroHit(c, dmg);
            boss.flash = 0.12f;
            hitstop = 0.03f;
            shake = 0.06f;
            SpawnFx(hero.p.isRageMode ? "fx_impacto_oro" : "fx_impacto", TextFormat("a%d", GetRandomValue(0, 5)), c,
                    hero.p.facing == Facing::Left);
            const auto it = boss.sounds.find(boss.hp > 0 ? "hurt" : "die");
            if (it != boss.sounds.end()) PlaySound(it->second);
            if (boss.hp <= 0) {
                boss.hp = 0;
                boss.phase = 4;
                boss.warnOn = false;
                boss.Play("defeat");
                SpawnFx("fx_humo_muerte", "a0", {boss.pos.x, boss.pos.y}, false);
                SpawnFx("fx_alma_roja", "a0", {boss.pos.x, boss.pos.y - 120.0f}, false);
                return;
            }
            // Solo se interrumpe si no esta atacando (los jefes tienen armadura al atacar).
            if (boss.phase == 1 && boss.Play("hit")) boss.phase = 3;
        }
    }
    if (boss.phase == 0 || boss.phase == 3) {
        if (boss.skel.Finished() || boss.clip == "idle") { boss.phase = 1; boss.Play("idle"); }
        return;
    }
    if (boss.phase == 1) {
        if (boss.layer == 0) boss.facing = hero.p.position.x < boss.pos.x ? Facing::Left : Facing::Right;
        boss.cooldown -= dt;
        if (boss.cooldown > 0.0f || hero.p.state == PlayerState::Defeat) return;
        // Elige el ataque que alcanza al heroe; si ninguno, el de mas alcance.
        const float dist = std::fabs(hero.p.position.x - boss.pos.x);
        int pick = GetRandomValue(0, (int)boss.attacks.size() - 1);
        if (boss.attacks[(size_t)pick].area.x * k + boss.attacks[(size_t)pick].area.width * k < dist) {
            for (int i = 0; i < (int)boss.attacks.size(); ++i)
                if ((boss.attacks[(size_t)i].area.x + boss.attacks[(size_t)i].area.width) >
                    (boss.attacks[(size_t)pick].area.x + boss.attacks[(size_t)pick].area.width))
                    pick = i;
        }
        boss.attack = pick;
        const BossAttack& a = boss.attacks[(size_t)pick];
        boss.Play(a.clip);
        boss.strikes = 0;
        auto& fxCache = FxCache();
        const auto fit = fxCache.find("fx_rango");
        boss.warnOn = fit != fxCache.end() && fit->second.loaded;
        if (boss.warnOn) {
            boss.warnAnim = fit->second.templ;
            boss.warnOn = boss.warnAnim.PlayNamed(a.warn);
            if (boss.warnOn) boss.warnAnim.currentClip.loop = true;
        }
        boss.phase = 2;
        PlaySound("sound44");
        return;
    }
    // Ataque: el golpe cae en el cuadro del evento Spine (attackEffect/skillEffect).
    const BossAttack& a = boss.attacks[(size_t)std::clamp(boss.attack, 0, (int)boss.attacks.size() - 1)];
    if (boss.strikes < (int)a.hitTimes.size() && boss.skel.Time() >= a.hitTimes[(size_t)boss.strikes]) {
        ++boss.strikes;
        if (boss.strikes >= (int)a.hitTimes.size()) boss.warnOn = false;
        const float dir = boss.facing == Facing::Left ? -1.0f : 1.0f;
        const float rel = (hero.p.position.x - boss.pos.x) * dir;
        const float depth = hero.p.position.y - boss.pos.y;
        const bool inX = rel >= a.area.x * k && rel <= (a.area.x + a.area.width) * k;
        const bool inY = depth >= a.area.y * k * 0.5f && depth <= (a.area.y + a.area.height) * k * 0.5f + 20.0f;
        SpawnFx(a.fx, a.fxClip, {boss.pos.x + dir * (a.area.x + a.area.width * 0.5f) * k, boss.pos.y}, dir < 0);
        PlaySound(a.sound);
        shake = 0.18f;
        if (inX && inY && hero.p.state != PlayerState::Defeat && hero.p.dashInvulnerability <= 0.0f) {
            hero.p.TakeDamage(a.damage);
            SpawnFx("fx_impacto_morado", "a0", {hero.p.position.x, hero.p.position.y - 90.0f}, dir < 0);
        }
    }
    if (boss.skel.Finished()) {
        boss.phase = 1;
        boss.warnOn = false;
        boss.Play("idle");
        boss.cooldown = 0.6f + GetRandomValue(0, 100) / 100.0f;
    }
}

void BetaMode::DrawBossWarning() const {
    if (!boss.active || !boss.warnOn) return;
    const float ws = boss.attacks[(size_t)std::clamp(boss.attack, 0, (int)boss.attacks.size() - 1)].warnScale;
    // El aviso del original apunta a +x: se refleja cuando el jefe mira a la izquierda.
    boss.warnAnim.Draw({boss.pos.x, boss.pos.y}, BetaStage::kScale * ws, boss.facing == Facing::Left);
}

void BetaMode::DrawBoss() const {
    if (!boss.active) return;
    const bool flip = boss.facing == Facing::Right;   // arte nativo: mirando a la izquierda
    const unsigned char a = boss.phase == 4 && boss.deadTime > 1.6f ? (unsigned char)(255 * std::max(0.0f, 1.0f - (boss.deadTime - 1.6f))) : 255;
    const Color tint = boss.flash > 0 ? Color{255, 170, 170, a} : Color{255, 255, 255, a};
    if (boss.layer > 0)
        boss.skel.Draw({boss.pos.x + boss.drawDx, boss.pos.y + boss.drawDy}, boss.drawScale, flip, tint);
    else
        boss.skel.Draw({boss.pos.x, boss.pos.y - boss.ground * boss.drawScale}, boss.drawScale * boss.scale, flip, tint);
}

// ---------------------------------------------------------------- remate (QTE)

namespace {
const char* QteKeyFor(int betaIndex) {
    switch (betaIndex) {
    case Centauro: case CentauroRojo: return "centauro";
    case Bruto: case BrutoRojo: return "bruto";
    case Medusa: case MedusaRoja: return "medusa";
    default: return nullptr;
    }
}
}  // namespace

void BetaMode::CheckQteTriggers() {
    if (qte.active || fighters.empty()) return;
    Fighter& hero = *fighters[0];
    // Las vinetas muestran al guerrero: solo si el jugador usa una de sus 4 armas.
    if (selCharacter > Garras || hero.p.state == PlayerState::Defeat) return;
    for (size_t i = 1; i < fighters.size(); ++i) {
        Fighter& f = *fighters[i];
        const char* key = QteKeyFor(f.character - FirstBetaCharacter());
        if (!key || f.qteDone || f.p.state == PlayerState::Defeat || f.p.hp <= 0) continue;
        if (f.p.hp * 5 <= f.p.maxHp) {   // <= 20% (QteControl.lua Section1)
            f.qteDone = true;
            if (StartQte(key, (int)i)) return;
        }
    }
    if (boss.active && !boss.qteDone && boss.phase != 4 && boss.hp * 5 <= boss.maxHp) {
        boss.qteDone = true;
        const char* key = boss.id == "jefe_poseidon" ? "poseidon" : boss.id == "jefe_tentaculos" ? "tentaculos" : nullptr;
        if (key) StartQte(key, -1);
    }
}

bool BetaMode::StartQte(const std::string& key, int fighter) {
    std::string text;
    const std::string rel = "data/beta/qte/" + key + ".txt";
    for (const std::string& p : {rel, "../" + rel, "../../" + rel})
        if (platform::LoadTextFile(p, text)) break;
    if (text.empty()) return false;
    Qte q;
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream ls(line);
        std::string tag;
        ls >> tag;
        if (tag == "dir") ls >> q.dir;
        else if (tag == "seg") {
            QteSeg sg;
            ls >> sg.file >> sg.anim >> sg.prompt >> sg.fail;
            if (sg.fail == "-") sg.fail.clear();
            q.segs.push_back(sg);
        }
    }
    if (q.segs.empty()) return false;
    q.fighter = fighter;
    q.active = true;
    qte = std::move(q);
    QtePlaySegment();
    if (!qte.skel.Valid()) { qte = Qte{}; return false; }
    PlaySound("sound25");
    return true;
}

void BetaMode::QtePlaySegment() {
    static std::map<std::string, std::shared_ptr<spine21::SkeletonData>> cache;
    const QteSeg& sg = qte.segs[(size_t)qte.seg];
    const std::string key = qte.dir + "/" + sg.file;
    auto it = cache.find(key);
    if (it == cache.end()) {
        auto data = std::make_shared<spine21::SkeletonData>();
        if (!data->Load(qte.dir, sg.file)) {
            TraceLog(LOG_WARNING, "BETA: no cargo la cinematica %s: %s", key.c_str(), data->error.c_str());
            data.reset();
        }
        it = cache.emplace(key, data).first;
    }
    qte.skel = spine21::Skeleton{};
    if (!it->second) return;
    qte.skel.SetData(it->second);
    qte.skel.Play(qte.failing ? sg.fail : sg.anim, false);
    qte.waiting = false;
    qte.resolved = false;
}

void BetaMode::UpdateQte(float dt) {
    if (qte.waiting) {
        // Boton a tiempo: golpe (J / boton GOLPE), patada o ENTER.
        if (input::Pressed(KEY_J) || input::Pressed(KEY_K) || input::Pressed(KEY_ENTER)) {
            qte.waiting = false;
            qte.resolved = true;
            PlaySound("sound61");
            shake = 0.15f;
        } else if ((qte.timer -= dt) <= 0.0f) {
            qte.waiting = false;
            const QteSeg& sg = qte.segs[(size_t)qte.seg];
            if (sg.fail.empty()) { EndQte(false); return; }
            qte.failing = true;
            QtePlaySegment();
        }
        return;
    }
    qte.skel.Update(dt);
    const QteSeg& sg = qte.segs[(size_t)qte.seg];
    if (!qte.failing && !qte.resolved && sg.prompt >= 0.0f && qte.skel.Time() >= sg.prompt) {
        qte.waiting = true;
        qte.timer = 2.0f;
        PlaySound("sound24");
        return;
    }
    if (qte.skel.Finished()) {
        if (qte.failing) { EndQte(false); return; }
        if (++qte.seg >= (int)qte.segs.size()) { EndQte(true); return; }
        QtePlaySegment();
    }
}

void BetaMode::EndQte(bool success) {
    const int target = qte.fighter;
    qte = Qte{};
    if (target >= 0 && target < (int)fighters.size()) {
        Fighter& f = *fighters[(size_t)target];
        if (success) {   // remate
            f.p.hp = 0;
            f.p.SetState(PlayerState::Defeat);
            PlaySound("sound48");
        } else {         // QteControl.lua AddHP: +10%
            f.p.hp = std::min(f.p.maxHp, f.p.hp + f.p.maxHp / 10);
        }
        f.lastHp = f.p.hp + (success ? 1 : 0);
    } else if (target < 0 && boss.active) {
        if (success) {
            boss.hp = 0;
            boss.phase = 4;
            boss.warnOn = false;
            boss.Play("defeat");
            const auto it = boss.sounds.find("die");
            if (it != boss.sounds.end()) PlaySound(it->second);
            SpawnFx("fx_alma_roja", "a0", {boss.pos.x, boss.pos.y - 120.0f}, false);
        } else {
            boss.hp = std::min(boss.maxHp, boss.hp + boss.maxHp / 10);
        }
    }
    bannerText = success ? "REMATE" : "FALLASTE EL REMATE";
    banner = 1.6f;
}

void BetaMode::DrawQte() const {
    if (!qte.active) return;
    DrawRectangle(0, 0, 1280, 720, {0, 0, 0, 190});
    // Disenadas para 640 px de alto con el origen al centro: 720/640.
    qte.skel.Draw({640.0f, 360.0f}, 1.125f, false, WHITE);
    DrawText("REMATE", 24, 20, 30, {255, 215, 120, 255});
    // Leyenda: estas vinetas tipo comic son la escena de remate del juego original
    // (las letras como "SOUGH" son sonidos dibujados, como en un comic).
    DrawRectangle(0, 676, 1280, 44, {0, 0, 0, 200});
    DrawText(TextFormat("ESCENA DE REMATE  %d/%d  -  el guerrero termina con el enemigo. Cuando aparezca el boton rojo, pulsa GOLPE.",
                        qte.seg + 1, (int)qte.segs.size()),
             24, 690, 16, {235, 225, 200, 255});
    if (qte.waiting) {
        const float k = std::max(0.0f, qte.timer / 2.0f);
        const float pulse = 1.0f + 0.08f * std::sin(time * 14.0f);
        DrawCircle(640, 600, 58 * pulse, {200, 30, 30, 230});
        DrawCircleLines(640, 600, 66, {255, 220, 120, 255});
        DrawRing({640, 600}, 66, 72, -90, -90 + 360 * k, 48, {255, 220, 120, 255});
        const char* t = touch::Enabled() ? "GOLPE" : "J";
        DrawText(t, 640 - MeasureText(t, 34) / 2, 583, 34, WHITE);
        const char* h = "PULSA AHORA";
        DrawText(h, 640 - MeasureText(h, 22) / 2, 520, 22, {255, 235, 180, 255});
    }
}

// ---------------------------------------------------------------- dibujo

void BetaMode::Draw() const {
    if (flow == Flow::Select) DrawSelection();
    else if (flow == Flow::StoryCard) DrawStoryCard();
    else DrawFight();
}

void BetaMode::DrawSelection() const {
    // Fondo: el escenario elegido (se carga de la cache de texturas).
    static BetaStage shown;
    static int shownIndex = -1;
    if (shownIndex != selStage) { shown.Load(kStageIds[selStage]); shownIndex = selStage; }
    if (shown.Loaded()) shown.DrawBack(300.0f);
    else DrawRectangle(0, 0, 1280, 720, {8, 10, 14, 255});
    DrawRectangle(0, 0, 1280, 720, {0, 0, 0, 120});
    DrawRectangle(150, 40, 980, 640, {6, 8, 12, 225});
    DrawRectangleLines(150, 40, 980, 640, {200, 150, 70, 200});
    DrawRectangle(150, 40, 6, 640, {220, 60, 40, 230});
    DrawText("MODO BETA", 640 - MeasureText("MODO BETA", 44) / 2, 62, 44, {255, 215, 120, 255});
    DrawText("CONTENIDO DE NUEVOSSPRITES: PERSONAJES, ESCENARIOS, EFECTOS Y SONIDOS ORIGINALES", 640 -
             MeasureText("CONTENIDO DE NUEVOSSPRITES: PERSONAJES, ESCENARIOS, EFECTOS Y SONIDOS ORIGINALES", 13) / 2,
             114, 13, {170, 190, 200, 230});
    const char* labels[kFieldCount] = {"PERSONAJE", "ESCENARIO", "MODO", selMode == 2 ? "JEFE" : "RIVAL (1 VS 1)"};
    for (int i = 0; i < kFieldCount; ++i) {
        const int y = 150 + i * 46;
        const bool sel = cursor == i;
        const bool dim = (i == 3 && (selMode == 0 || selMode == 3)) || (i == 1 && selMode == 3);
        DrawRectangle(190, y, 560, 38, sel ? Color{34, 26, 18, 235} : Color{12, 14, 18, 210});
        DrawRectangleLines(190, y, 560, 38, sel ? Color{255, 200, 90, 220} : Color{80, 80, 90, 120});
        DrawText(labels[i], 208, y + 11, 16, sel ? Color{255, 215, 120, 255} : WHITE);
        const char* value = i == 0 ? GetCharacterVisual(CharIndex(selCharacter)).name
                          : i == 1 ? (shown.Loaded() ? shown.name.c_str() : kStageIds[selStage])
                          : i == 2 ? (selMode == 1 ? "1 VS 1 (CONTRA LA MAQUINA)" : selMode == 2 ? "JEFE" : selMode == 3 ? "HISTORIA (3 CAPITULOS)" : "OLEADAS + JEFE FINAL")
                          : selMode == 2 ? kBossNames[selBoss]
                                   : GetCharacterVisual(CharIndex(selRival)).name;
        DrawText(TextFormat("<  %s  >", value), 380, y + 11, 15, dim ? Color{90, 95, 100, 150} : Color{220, 225, 230, 255});
    }
    // Ficha del personaje: retrato animado y habilidades (con sus nombres en espanol).
    const KfReference& ref = RefFor(CharIndex(selCharacter));
    DrawRectangle(790, 150, 310, 360, {10, 12, 16, 220});
    DrawRectangleLines(790, 150, 310, 360, {90, 80, 70, 160});
    if (ref.loaded) {
        Animator a = ref.templ;
        const char* clip = a.HasClip("victory") && std::fmod(time, 6.0f) > 4.0f ? "victory" : "idle";
        a.PlayNamed(clip);
        a.Update(std::fmod(time, std::max(0.1f, a.ClipSeconds(clip))));
        a.Draw({945, 470}, 1.0f, false);
        DrawText(ref.betaHero ? "HEROE" : "ENEMIGO", 806, 160, 12, {255, 190, 110, 230});
        DrawText(TextFormat("VIDA %d", ref.maxHp), 1020, 160, 12, {200, 210, 215, 230});
    } else {
        DrawText("NO SE PUDO CARGAR", 820, 320, 16, {255, 110, 100, 255});
        DrawText(ref.error.c_str(), 820, 345, 10, {200, 150, 140, 255});
    }
    DrawText("HABILIDADES", 190, 345, 15, {255, 200, 90, 255});
    for (size_t i = 0; i < ref.abilityNames.size() && i < 6; ++i)
        DrawText(TextFormat("%d  %s", (int)i + 1, ref.abilityNames[i].c_str()), 200, 372 + (int)i * 22, 14,
                 {210, 220, 225, 240});
    DrawText(selMode == 3   ? "HISTORIA: costa, templo y volcan; jefes con remate. Se juega con el guerrero."
             : selMode == 1 ? "1 VS 1: el rival usa todos sus movimientos y habilidades."
             : selMode == 2 ? "JEFE: esquiva la zona roja del aviso y golpea al jefe entre sus ataques."
                            : "OLEADAS: avanza por el escenario, vence a cada grupo y al jefe final.",
             190, 520, 13, {170, 190, 200, 230});
    DrawText(touch::Enabled() ? "OK INICIAR   ATRAS VOLVER AL MENU"
                              : "ARRIBA/ABAJO CAMPO   IZQ/DER CAMBIAR   ENTER/J INICIAR   ESC MENU",
             190, 640, 14, {200, 210, 215, 235});
    DrawText("BETA // version de prueba aislada del juego principal", 190, 608, 12, {150, 130, 110, 220});
}

void BetaMode::DrawFight() const {
    const float sx = shake > 0 ? (float)GetRandomValue(-4, 4) : 0.0f;
    const float sy = shake > 0 ? (float)GetRandomValue(-3, 3) : 0.0f;
    Camera2D cam{};
    cam.offset = {sx, stage.CameraOffsetY() + sy};
    cam.target = {cameraX, 0};
    cam.zoom = 1.0f;
    if (stage.Loaded()) stage.DrawSky(cameraX + sx);
    else DrawRectangle(0, 0, 1280, 720, {20, 20, 24, 255});
    if (boss.active && boss.layer == 2) {   // el Titan asoma por detras del escenario
        BeginMode2D(cam);
        DrawBoss();
        EndMode2D();
    }
    if (stage.Loaded()) stage.DrawMap(cameraX + sx);
    BeginMode2D(cam);
    DrawBossWarning();
    if (boss.active && boss.layer == 1) DrawBoss();
    // Orden por profundidad.
    std::vector<const Fighter*> order;
    for (const auto& f : fighters) order.push_back(f.get());
    std::sort(order.begin(), order.end(), [](const Fighter* a, const Fighter* b) { return a->p.position.y < b->p.position.y; });
    bool bossDrawn = false;
    for (const Fighter* f : order) {
        if (!bossDrawn && boss.layer == 0 && f->p.position.y > boss.pos.y) { DrawBoss(); bossDrawn = true; }
        if (f->gone > 1.8f && ((int)(f->gone * 12) % 2) == 0) continue;   // parpadeo al desaparecer
        f->p.Draw();
        if (f->enemy && f->p.state != PlayerState::Defeat) {
            const CombatBox hb = f->p.GetHurtbox();
            beta_hud::DrawEnemyBar({f->p.position.x, hb.y - 18.0f}, (float)f->p.hp / std::max(1, f->p.maxHp));
        }
    }
    if (!bossDrawn && boss.layer == 0) DrawBoss();
    for (const Fx& fx : effects) fx.anim.Draw(fx.pos, BetaStage::kScale, fx.flip);
    for (const DamageNumber& n : numbers) {
        const unsigned char a = (unsigned char)(255 * std::max(0.0f, 1.0f - n.t / 0.9f));
        // attnum: "0123456789-+" (12 celdas); golpes al heroe en rojo
        beta_hud::DrawDigits("numeros_dano", 12, 0, n.value, {n.pos.x, n.pos.y - 40.0f - n.t * 60.0f}, 30,
                             n.hero ? Color{255, 255, 255, a} : Color{255, 120, 120, a});
    }
    EndMode2D();
    stage.DrawWeather(cameraX, time);
    DrawHud();
    DrawQte();
    if (banner > 0 && result == 0) {
        const unsigned char a = (unsigned char)(255 * std::min(1.0f, banner));
        DrawText(bannerText.c_str(), 640 - MeasureText(bannerText.c_str(), 46) / 2, 300, 46, {255, 215, 120, a});
    }
    if (result != 0) {
        DrawRectangle(0, 0, 1280, 720, {0, 0, 0, 150});
        const char* t = result > 0 ? "VICTORIA" : "DERROTA";
        DrawText(t, 640 - MeasureText(t, 60) / 2, 270, 60, result > 0 ? Color{255, 215, 120, 255} : Color{240, 80, 70, 255});
        const char* h = touch::Enabled() ? "OK REINTENTAR   ATRAS ELEGIR OTRA VEZ"
                                         : "ENTER/J REINTENTAR   ESC ELEGIR OTRA VEZ";
        DrawText(h, 640 - MeasureText(h, 18) / 2, 350, 18, WHITE);
    }
}

void BetaMode::DrawHud() const {
    const Player& p = fighters[0]->p;
    const int w = fighters[0]->character - FirstBetaCharacter();
    const bool hero = w >= 0 && w <= Garras;
    beta_hud::DrawPlayer(p, hero, ShortCharacterName(fighters[0]->character).c_str());
    // Objetivo arriba a la derecha (como el cuadro del original).
    DrawRectangle(1010, 16, 254, 62, {30, 10, 8, 200});
    DrawRectangleLines(1010, 16, 254, 62, {200, 150, 70, 220});
    DrawText(stage.name.c_str(), 1022, 24, 13, {255, 215, 120, 255});
    if (selMode == 1 && fighters.size() > 1)
        DrawText(TextFormat("RIVAL: %s", GetCharacterVisual(fighters[1]->character).name), 1022, 48, 11, WHITE);
    else if (boss.active)
        DrawText("VENCE AL JEFE  0/1", 1022, 48, 13, WHITE);
    else
        DrawText(TextFormat("OLEADA %d/%d   ENEMIGOS %d", std::min(wave + 1, waveCount), waveCount, AliveEnemies()), 1022, 48, 12, WHITE);
    if (selMode == 1 && fighters.size() > 1) {
        const Player& r = fighters[1]->p;
        beta_hud::DrawBossBar(GetCharacterVisual(fighters[1]->character).name, r.hp, r.maxHp);
    }
    if (boss.active) beta_hud::DrawBossBar(boss.name.c_str(), boss.hp, boss.maxHp);
    beta_hud::DrawCombo(comboHits, std::min(1.0f, comboTimer));
    beta_hud::DrawPotions(redPotions, bluePotions, touch::Enabled());
    if (!touch::Enabled()) {
        const char* keys = hero ? "J GOLPE  K PATADA  1-5 HABILIDADES  ESPACIO FURIA  B BLOQUEO  SHIFT ESQUIVA  Q CAMBIAR ARMA  ESC SALIR"
                                : "J GOLPE  K PATADA  1-5 HABILIDADES  ESPACIO FURIA  B BLOQUEO  SHIFT ESQUIVA  ESC SALIR";
        DrawText(keys, 640 - MeasureText(keys, 11) / 2, 706, 11, {190, 200, 205, 220});
    }
}

}  // namespace district_fury
