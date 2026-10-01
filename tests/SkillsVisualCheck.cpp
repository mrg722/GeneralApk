// Herramienta (no es parte de ctest): lanza las 6 habilidades de un personaje en
// el Modo VS // Laboratorio, guarda capturas a mitad de cada una y comprueba que
// golpean (vida del rival antes/despues).
//   ./build/skills_visual_check [carpeta_salida] [personaje=7 KF HEROE] [rival KF=2]
#include "game/CharacterVisual.h"
#include "game/VSMode.h"
#include "rendering/AssetManager.h"
#include "raylib.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>

using namespace district_fury;

int main(int argc, char** argv) {
    const std::string out = argc > 1 ? argv[1] : ".";
    const int character = argc > 2 ? std::atoi(argv[2]) : 7;
    const int rival = argc > 3 ? std::atoi(argv[3]) : 2;
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(1280, 720, "skills visual check");
    SetTargetFPS(0);
    AssetManager::Get().LoadAll();
    auto Capture = [&](const std::string& name) {
        const std::string file = name + ".png";
        TakeScreenshot(file.c_str());
        if (out != ".") std::rename(file.c_str(), (out + "/" + file).c_str());
    };
    VSMode vs;
    vs.Init();
    vs.StartLabForTest(true, StreetEnemyType::Brute, rival, character);
    Player& p = vs.PlayerRef();
    p.debugInvulnerable = true;
    PlayerInput in;
    p.scriptedInput = &in;
    std::printf("personaje: %s\n", GetCharacterVisual(character).name);
    int failures = 0;
    for (int skill = 0; skill < Player::kSkillCount; ++skill) {
        // Acercarse al rival y mirarlo.
        for (int f = 0; f < 240; ++f) {
            in = PlayerInput{};
            const auto& es = vs.Enemies();
            if (es.empty()) break;
            const float dx = es[0].position.x - p.position.x, dy = es[0].position.y - p.position.y;
            if (std::fabs(dy) > 10) in.moveY = dy > 0 ? 1.f : -1.f;
            if (std::fabs(dx) > 110) in.moveX = dx > 0 ? 1.f : -1.f;
            else if (std::fabs(dy) <= 10 && (p.state == PlayerState::Idle || p.state == PlayerState::Walk)) {
                in.moveX = dx > 0 ? 0.01f : -0.01f;
                break;
            }
            vs.Update(1.0f / 60.0f);
        }
        if (vs.Enemies().empty()) break;
        // Rival con vida completa para cada prueba.
        auto& rivalRef = const_cast<StreetEnemy&>(vs.Enemies()[0]);
        rivalRef.hp = rivalRef.maxHp;
        const int hpBefore = vs.Enemies()[0].hp;
        in = PlayerInput{};
        in.skill = skill;
        vs.Update(1.0f / 60.0f);
        in = PlayerInput{};
        const bool started = p.state == PlayerState::Attack;
        const float dur = p.attackDuration;
        int frames = 0;
        bool shot = false;
        std::string clip = p.animator.currentClipName;
        while (p.state == PlayerState::Attack && frames < 60 * 6) {
            vs.Update(1.0f / 60.0f);
            ++frames;
            BeginDrawing(); ClearBackground(BLACK); vs.Draw(); EndDrawing();
            if (!shot && p.attackElapsed >= dur * 0.45f) { Capture("habilidad_" + std::to_string(skill + 1)); shot = true; }
        }
        const int hpAfter = vs.Enemies().empty() ? 0 : vs.Enemies()[0].hp;
        std::printf("habilidad %d %-12s clip=%-10s inicio=%s duracion=%.2fs dano=%d espera=%.1fs\n", skill + 1,
                    p.SkillName(skill), clip.c_str(), started ? "si" : "NO", dur, hpBefore - hpAfter,
                    p.skillCooldown[skill]);
        if (!started) ++failures;
        if (skill < 5 && hpBefore - hpAfter <= 0) std::printf("  AVISO: no golpeo\n");
        // Reiniciar la vida del rival para la siguiente prueba.
        for (int f = 0; f < 40; ++f) { in = PlayerInput{}; vs.Update(1.0f / 60.0f); }
    }
    std::printf("transformado: %s\n", p.IsTransformed() ? "si" : "no");
    CloseWindow();
    return failures;
}
