#include "game/beta/BetaMode.h"
#include "audio/AudioSystem.h"
#include "core/InputMap.h"
#include "core/Platform.h"
#include "game/CharacterVisual.h"
#include "game/beta/BetaCharacter.h"
#include "game/lab/KfReference.h"
#include "ui/GameHUD.h"
#include "ui/TouchControls.h"
#include <algorithm>
#include <cmath>

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

// Oleadas por escenario: los enemigos que el juego original pone en esas zonas
// (barco: no-muertos; montana: bestias y centauros; volcan: brutos y medusas).
// La ultima oleada es el enemigo grande de la zona.
const std::vector<std::vector<int>>& WavesFor(int stage) {
    static const std::vector<std::vector<std::vector<int>>> kWaves = {
        {{Esqueleto, Esqueleto}, {Excavador, Esqueleto, Momia}, {Momia, Excavador, Esqueleto}, {Bruto}},
        {{Esqueleto, Ave}, {Elefante, Esqueleto}, {Momia, Ave, Excavador}, {Centauro}},
        {{Esqueleto, Excavador}, {Elefante, Ave}, {Centauro, Esqueleto}, {CentauroRojo}},
        {{Excavador, Esqueleto}, {Elefante, Momia}, {Bruto, Esqueleto}, {Medusa}},
        {{Momia, Excavador}, {BrutoRojo}, {Medusa, Esqueleto}, {MedusaRoja}},
        {{Esqueleto, Ave}, {Elefante, Excavador}, {Momia, Esqueleto, Ave}, {Centauro, Elefante}, {CentauroRojo}},
    };
    return kWaves[(size_t)std::clamp(stage, 0, kStageCount - 1)];
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
    if (flow == Flow::Select) return 1;
    return result != 0 ? 2 : 0;
}

void BetaMode::Init() {
    flow = Flow::Select;
    cursor = 0;
    exitRequested = false;
    fighters.clear();
    effects.clear();
    result = 0;
    AudioSystem::Get().SetBetaAudio(true);
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
    stage.Load(kStageIds[std::clamp(selStage, 0, kStageCount - 1)]);
    const float left = stage.Loaded() ? stage.WorldLeft() : 90.0f;
    AddFighter(CharIndex(selCharacter), false, left + 220.0f, 570.0f);
    cameraX = std::max(0.0f, left);
    lockLeft = stage.Loaded() ? stage.WorldLeft() + 40.0f : 120.0f;
    lockRight = stage.Loaded() ? std::min(stage.WorldRight(), kStageEndX) - 40.0f : 1160.0f;
    if (selDuel) {
        // 1 VS 1: el rival aparece enfrente, en la misma pantalla.
        Fighter& r = AddFighter(CharIndex(selRival), true, left + 900.0f, 570.0f);
        r.p.facing = Facing::Left;
        waveCount = 1;
        waveActive = true;
        lockLeft = cameraX + 60.0f;
        lockRight = cameraX + 1220.0f;
        bannerText = "1 VS 1  //  PELEA";
    } else {
        waveCount = (int)WavesFor(selStage).size();
        bannerText = "AVANZA";
    }
    banner = 2.0f;
    PlaySound("sound24");
    PlayMusic(stage.music.empty() ? "gate1music.ogg" : stage.music);
}

void BetaMode::SpawnWave() {
    const auto& list = WavesFor(selStage)[(size_t)wave];
    lockLeft = std::max(stage.WorldLeft() + 40.0f, cameraX + 60.0f);
    lockRight = std::min(std::min(stage.WorldRight(), kStageEndX) - 40.0f, cameraX + 1220.0f);
    const float ys[] = {560.0f, 520.0f, 605.0f, 540.0f};
    for (size_t i = 0; i < list.size(); ++i) {
        // Llegan por los dos lados de la pantalla, como en el original.
        const bool fromLeft = (i % 2) == 1 && lockLeft + 200.0f < fighters[0]->p.position.x;
        const float x = fromLeft ? lockLeft + 30.0f : lockRight - 30.0f - (float)i * 40.0f;
        Fighter& e = AddFighter(CharIndex(list[i]), true, x, ys[i % 4]);
        e.p.facing = fromLeft ? Facing::Right : Facing::Left;
        if (e.p.animator.HasClip("intro")) e.p.animator.PlayNamed("intro");
    }
    waveActive = true;
    bannerText = wave + 1 == waveCount ? "ENEMIGO FINAL" : TextFormat("OLEADA %d / %d", wave + 1, waveCount);
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
            else if (cursor == 2) selDuel = !selDuel;
            else selRival = (selRival + dir + kBetaCount) % kBetaCount;
            PlaySound(cursor == 0 ? "sound14" : "sound13");
        }
        if (input::Pressed(KEY_ENTER) || input::Pressed(KEY_J)) StartFight();
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
        if (resultTimer > 0.6f && (input::Pressed(KEY_ENTER) || input::Pressed(KEY_J))) StartFight();
        return;
    }
    UpdateFight(dt);
}

void BetaMode::UpdateFight(float dt) {
    banner = std::max(0.0f, banner - dt);
    shake = std::max(0.0f, shake - dt);
    Fighter& hero = *fighters[0];
    hero.p.PumpInput(dt);
    if (hitstop > 0) { hitstop -= dt; return; }

    // Avance por el escenario: la oleada siguiente aparece al llegar a su zona.
    if (!selDuel && !waveActive && wave < waveCount) {
        const float span = std::max(1.0f, std::min(stage.WorldRight(), kStageEndX) - stage.WorldLeft() - 1400.0f);
        const float trigger = stage.WorldLeft() + 500.0f + span * (waveCount > 1 ? (float)wave / (waveCount - 1) : 0.0f);
        if (hero.p.position.x >= trigger || wave == 0) SpawnWave();
    }

    for (auto& f : fighters) UpdateFighter(*f, dt);

    // Golpes: el jugador puede alcanzar a varios enemigos con el mismo golpe.
    for (size_t i = 1; i < fighters.size(); ++i) ResolveHits(hero, *fighters[i], 1.0f);
    if (hero.p.AttackIsActive() && !hero.p.hasHit) {
        for (size_t i = 1; i < fighters.size(); ++i)
            if (fighters[i]->lastHp != fighters[i]->p.hp) { hero.p.hasHit = true; break; }
    }
    for (size_t i = 1; i < fighters.size(); ++i) ResolveHits(*fighters[i], hero, selDuel ? 1.0f : 0.55f);
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
    if (waveActive && AliveEnemies() == 0) {
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
        if (f.enemy) SpawnFx("fx_alma_roja", "a0", {f.p.position.x, f.p.position.y - 90.0f}, false);
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

// ---------------------------------------------------------------- dibujo

void BetaMode::Draw() const {
    if (flow == Flow::Select) DrawSelection();
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
    const char* labels[kFieldCount] = {"PERSONAJE", "ESCENARIO", "MODO", "RIVAL (1 VS 1)"};
    for (int i = 0; i < kFieldCount; ++i) {
        const int y = 150 + i * 46;
        const bool sel = cursor == i;
        const bool dim = i == 3 && !selDuel;
        DrawRectangle(190, y, 560, 38, sel ? Color{34, 26, 18, 235} : Color{12, 14, 18, 210});
        DrawRectangleLines(190, y, 560, 38, sel ? Color{255, 200, 90, 220} : Color{80, 80, 90, 120});
        DrawText(labels[i], 208, y + 11, 16, sel ? Color{255, 215, 120, 255} : WHITE);
        const char* value = i == 0 ? GetCharacterVisual(CharIndex(selCharacter)).name
                          : i == 1 ? (shown.Loaded() ? shown.name.c_str() : kStageIds[selStage])
                          : i == 2 ? (selDuel ? "1 VS 1 (CONTRA LA MAQUINA)" : "OLEADAS DE ENEMIGOS")
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
    DrawText(selDuel ? "1 VS 1: el rival usa todos sus movimientos y habilidades."
                     : "OLEADAS: avanza por el escenario y vence a cada grupo hasta el enemigo final.",
             190, 520, 13, {170, 190, 200, 230});
    DrawText(touch::Enabled() ? "OK INICIAR   ATRAS VOLVER AL MENU"
                              : "ARRIBA/ABAJO CAMPO   IZQ/DER CAMBIAR   ENTER/J INICIAR   ESC MENU",
             190, 640, 14, {200, 210, 215, 235});
    DrawText("BETA // version de prueba aislada del juego principal", 190, 608, 12, {150, 130, 110, 220});
}

void BetaMode::DrawFight() const {
    const float sx = shake > 0 ? (float)GetRandomValue(-4, 4) : 0.0f;
    const float sy = shake > 0 ? (float)GetRandomValue(-3, 3) : 0.0f;
    if (stage.Loaded()) stage.DrawBack(cameraX + sx);
    else DrawRectangle(0, 0, 1280, 720, {20, 20, 24, 255});
    Camera2D cam{};
    cam.offset = {sx, stage.CameraOffsetY() + sy};
    cam.target = {cameraX, 0};
    cam.zoom = 1.0f;
    BeginMode2D(cam);
    // Orden por profundidad.
    std::vector<const Fighter*> order;
    for (const auto& f : fighters) order.push_back(f.get());
    std::sort(order.begin(), order.end(), [](const Fighter* a, const Fighter* b) { return a->p.position.y < b->p.position.y; });
    for (const Fighter* f : order) {
        if (f->gone > 1.8f && ((int)(f->gone * 12) % 2) == 0) continue;   // parpadeo al desaparecer
        f->p.Draw();
        if (f->enemy && f->p.state != PlayerState::Defeat) {
            const Vector2 s = f->p.position.ToScreen();
            const float w = 70.0f;
            DrawRectangle((int)(s.x - w / 2), (int)s.y + 8, (int)w, 6, {30, 10, 10, 200});
            DrawRectangle((int)(s.x - w / 2), (int)s.y + 8, (int)(w * std::max(0, f->p.hp) / std::max(1, f->p.maxHp)), 6,
                          {230, 60, 50, 255});
        }
    }
    for (const Fx& fx : effects) fx.anim.Draw(fx.pos, BetaStage::kScale, fx.flip);
    EndMode2D();
    stage.DrawWeather(cameraX, time);
    DrawHud();
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
    ui::PlayerVitals v{};
    v.player = &p;
    v.hp = p.hp; v.maxHp = p.maxHp;
    v.shield = p.shield; v.maxShield = p.maxShield;
    v.sp = p.sp; v.maxSp = p.maxSp;
    v.rage = p.rage; v.maxRage = p.maxRage;
    v.isRageMode = p.isRageMode;
    v.title = TextFormat("%s // BETA", ShortCharacterName(fighters[0]->character).c_str());
    v.x = 16; v.y = 16; v.width = 530; v.panelHeight = 122;
    ui::DrawPlayerVitals(v);
    DrawRectangle(840, 16, 424, 70, {6, 8, 12, 210});
    DrawText(stage.name.c_str(), 856, 26, 16, {255, 215, 120, 255});
    if (selDuel && fighters.size() > 1) {
        const Player& r = fighters[1]->p;
        DrawText(TextFormat("RIVAL: %s", GetCharacterVisual(fighters[1]->character).name), 856, 48, 12, WHITE);
        DrawRectangle(856, 66, 390, 10, {30, 12, 12, 255});
        DrawRectangle(856, 66, (int)(390.0f * std::max(0, r.hp) / std::max(1, r.maxHp)), 10, {225, 60, 60, 255});
    } else {
        DrawText(TextFormat("OLEADA %d / %d    ENEMIGOS %d", std::min(wave + 1, waveCount), waveCount, AliveEnemies()),
                 856, 52, 13, WHITE);
    }
    if (!touch::Enabled())
        DrawText("J GOLPE  K PATADA  L ENERGIA  B BLOQUEO  SHIFT ESQUIVA  1-6 HABILIDADES  R REINICIAR  ESC SALIR",
                 640 - MeasureText("J GOLPE  K PATADA  L ENERGIA  B BLOQUEO  SHIFT ESQUIVA  1-6 HABILIDADES  R REINICIAR  ESC SALIR", 12) / 2,
                 700, 12, {190, 200, 205, 220});
}

}  // namespace district_fury
