// Herramienta (no es parte de ctest): recorre el Modo VS como un jugador (teclas
// simuladas, igual que los botones tactiles): elige cada personaje en la pantalla
// de seleccion, pelea unos segundos y guarda capturas. Tambien juega cada stage
// de Historia con cada personaje jugable un rato para detectar caidas.
//   ./build/all_modes_check [carpeta_salida]
#include "core/InputMap.h"
#include "game/CharacterVisual.h"
#include "game/VSMode.h"
#include "game/Stage1StoryGame.h"
#include "game/Stage2Game.h"
#include "game/Stage3Game.h"
#include "game/Stage4Game.h"
#include "game/Stage5Game.h"
#include "game/lab/KfReference.h"
#include "rendering/AssetManager.h"
#include "raylib.h"
#include <cstdio>
#include <string>

using namespace district_fury;

namespace {
std::string gOut = ".";
void Shot(const std::string& name) {
    const std::string file = name + ".png";
    TakeScreenshot(file.c_str());
    if (gOut != ".") std::rename(file.c_str(), (gOut + "/" + file).c_str());
}
void Press(VSMode& vs, int key) {
    input::ClearNext(); input::SetVirtual(key, true); input::Commit();
    vs.Update(1.0f / 60.0f);
    input::ClearNext(); input::Commit();
    vs.Update(1.0f / 60.0f);
}
template <class G> void Frame(const G& g) { BeginDrawing(); ClearBackground(BLACK); g.Draw(); EndDrawing(); }

// Juega un stage de Historia: avanza, golpea y pulsa ENTER de vez en cuando
// (intros y carteles). Guarda capturas cada 10 s.
template <class G> void PlayStage(G& g, const std::string& tag, float seconds) {
    Player& p = g.PlayerRef();
    p.debugInvulnerable = true;
    PlayerInput in;
    p.scriptedInput = &in;
    const int frames = static_cast<int>(seconds * 60);
    for (int f = 0; f < frames; ++f) {
        in = PlayerInput{};
        in.moveX = (f / 120) % 4 == 3 ? -1.0f : 1.0f;
        in.punch = (f % 20) == 0;
        in.kick = (f % 33) == 10;
        in.energy = (f % 240) == 100;
        input::ClearNext();
        if (f % 90 == 0) input::SetVirtual(KEY_ENTER, true);
        input::Commit();
        g.Update(1.0f / 60.0f);
        if (f % 600 == 300) { Frame(g); Shot(tag + "_" + std::to_string(f / 600)); }
    }
    p.scriptedInput = nullptr;
    input::ClearNext(); input::Commit();
    std::printf("%s: x=%.0f vida=%d estado=%d\n", tag.c_str(), p.position.x, p.hp, static_cast<int>(p.state));
}
}  // namespace

int main(int argc, char** argv) {
    if (argc > 1) gOut = argv[1];
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(1280, 720, "all modes check");
    SetTargetFPS(0);
    AssetManager::Get().LoadAll();
    int problems = 0;
    VSMode vs;
    vs.Init();
    for (int i = 0; i < 3; ++i) Press(vs, KEY_DOWN);   // campo PERSONAJE
    for (int c = 0; c < CharacterCount(); ++c) {
        if (c > 0) Press(vs, KEY_RIGHT);
        Frame(vs);
        Shot("sel_" + std::to_string(c));
        const CharacterVisual& cv = GetCharacterVisual(c);
        bool kfOk = true;
        if (cv.kfRoster >= 0) kfOk = GetKfCharacter(cv.kfRoster).loaded;
        if (cv.transformKfRoster >= 0) kfOk = kfOk && GetKfCharacter(cv.transformKfRoster).loaded;
        Press(vs, KEY_ENTER);   // pelea
        Player& p = vs.PlayerRef();
        p.debugInvulnerable = true;
        const bool skinOk = p.skin == c;
        PlayerInput in;
        p.scriptedInput = &in;
        for (int f = 0; f < 150; ++f) {
            in = PlayerInput{};
            in.punch = (f % 30) == 0;
            in.kick = (f % 45) == 20;
            in.moveX = f < 60 ? 1.0f : 0.0f;
            vs.Update(1.0f / 60.0f);
        }
        Frame(vs);
        Shot("vs_" + std::to_string(c));
        const bool drawn = p.animator.texture.id != 0 || cv.kfRoster >= 0;
        std::printf("%2d %-28s skin=%s kf=%s anim=%s\n", c, cv.name, skinOk ? "ok" : "MAL", kfOk ? "ok" : "NO CARGA",
                    drawn ? "ok" : "SIN TEXTURA");
        if (!skinOk || !kfOk || !drawn) ++problems;
        p.scriptedInput = nullptr;
        Press(vs, KEY_ESCAPE);   // vuelve a la seleccion (cursor queda en PERSONAJE)
    }
    // Rival (IA): personajes principales KF y otros, peleando contra Rayder.
    for (int rc : {7, 8, 18, 19, 6, 21, 2}) {
        VSMode duel;
        duel.Init();
        duel.StartRivalForTest(rc, 6);
        Player& me = duel.PlayerRef();   // sin invulnerabilidad: el rival debe poder golpear
        PlayerInput pin;
        me.scriptedInput = &pin;
        int skillsUsed = 0, transformed = 0, prevSkill = -1;
        bool hurtMe = false;
        const int startHp = duel.RivalRef().hp;
        for (int f = 0; f < 60 * 25; ++f) {
            pin = PlayerInput{};
            pin.punch = (f % 50) == 0;
            pin.moveX = (f / 200) % 2 ? 0.3f : -0.3f;
            duel.Update(1.0f / 60.0f);
            const Player& r = duel.RivalRef();
            if (r.activeSkill >= 0 && r.activeSkill != prevSkill) ++skillsUsed;
            prevSkill = r.activeSkill;
            if (r.IsTransformed()) transformed = 1;
            if (me.hp < me.maxHp || me.shield < me.maxShield) hurtMe = true;
            if (f % 300 == 150) { Frame(duel); Shot("rival_" + std::to_string(rc) + "_" + std::to_string(f / 300)); }
        }
        const Player& r = duel.RivalRef();
        const bool ok = skillsUsed > 0 && hurtMe && r.hp < startHp;
        std::printf("rival IA %-26s habilidades=%d transformo=%d golpes al jugador=%s golpes al rival=%s %s\n",
                    GetCharacterVisual(rc).name, skillsUsed, transformed,
                    hurtMe ? "si" : "no", r.hp < startHp ? "si" : "no",
                    ok ? "ok" : "REVISAR");
        if (!ok) ++problems;
        me.scriptedInput = nullptr;
    }
    // Rivales KF del laboratorio (campo RIVAL KF): cada uno contra Rayder.
    for (int r = 0; r < KfRosterCount(); ++r) {
        const KfReference& ref = GetKfCharacter(r);
        VSMode lab;
        lab.Init();
        lab.StartLabForTest(true, StreetEnemyType::Brute, r, 6);
        for (int f = 0; f < 90; ++f) lab.Update(1.0f / 60.0f);
        Frame(lab);
        Shot("rival_" + std::to_string(r));
        std::printf("rival %2d %-30s %s\n", r, KfRoster(r).name, ref.loaded ? "ok" : ("NO CARGA: " + ref.error).c_str());
        if (!ref.loaded) ++problems;
    }
    // Historia: los 5 stages con los dos luchadores de la campana (Rayden y Rayder).
    for (int skin : {0, 6}) {
        const std::string who = skin == 0 ? "rayden" : "rayder";
        { Stage1StoryGame g; g.Init(); g.PlayerRef().ApplyCharacter(skin); g.StartRunForTest(StoryDifficulty::Normal); PlayStage(g, "s1_" + who, 40); }
        { Stage2Game g; g.Init(); g.PlayerRef().ApplyCharacter(skin); PlayStage(g, "s2_" + who, 30); }
        { Stage3Game g; g.Init(); g.PlayerRef().ApplyCharacter(skin); PlayStage(g, "s3_" + who, 30); }
        { Stage4Game g; g.Init(); g.PlayerRef().ApplyCharacter(skin); PlayStage(g, "s4_" + who, 30); }
        { Stage5Game g; g.Init(); g.PlayerRef().ApplyCharacter(skin); PlayStage(g, "s5_" + who, 30); }
    }
    std::printf("problemas: %d\n", problems);
    CloseWindow();
    return problems ? 1 : 0;
}
