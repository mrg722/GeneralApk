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
#include <vector>

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
    // Todas las habilidades: cada pagina (botones 1-5) y al final TRANSFORMAR.
    struct Job { int page, skill; };
    std::vector<Job> jobs;
    p.Update(0.0f);
    for (int pg = 0; pg < p.SkillPageCount(); ++pg)
        for (int k = 0; k < Player::kSkillCount - 1; ++k) jobs.push_back({pg, k});
    jobs.push_back({0, Player::kSkillCount - 1});
    int shotIndex = 0;
    for (const Job& job : jobs) {
        const int skill = job.skill;
        p.skillPage = job.page;
        if (skill < Player::kSkillCount - 1 && p.AbilityForSlot(skill) < 0) continue;
        p.abilityCooldown.assign(p.abilityCooldown.size(), 0.0f);
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
            if (!shot && p.attackElapsed >= dur * 0.45f) { Capture("habilidad_" + std::to_string(++shotIndex)); shot = true; }
        }
        const int hpAfter = vs.Enemies().empty() ? 0 : vs.Enemies()[0].hp;
        std::printf("pag %d boton %d %-14s clip=%-10s inicio=%s duracion=%.2fs dano=%d espera=%.1fs\n", job.page + 1,
                    skill + 1, p.SkillName(skill), clip.c_str(), started ? "si" : "NO", dur, hpBefore - hpAfter,
                    p.SkillCooldownLeft(skill));
        if (!started) ++failures;
        if (skill < 5 && hpBefore - hpAfter <= 0) std::printf("  AVISO: no golpeo\n");
        // Reiniciar la vida del rival para la siguiente prueba.
        for (int f = 0; f < 40; ++f) { in = PlayerInput{}; vs.Update(1.0f / 60.0f); }
    }
    std::printf("transformado: %s, forma alternativa: %s\n", p.IsTransformed() ? "si" : "no", p.usingAlt ? "si" : "no");
    // Forma transformada: reposo y un golpe.
    for (int f = 0; f < 30; ++f) { in = PlayerInput{}; vs.Update(1.0f / 60.0f); }
    BeginDrawing(); ClearBackground(BLACK); vs.Draw(); EndDrawing();
    Capture("transformado_reposo");
    in = PlayerInput{}; in.punch = true; vs.Update(1.0f / 60.0f); in = PlayerInput{};
    for (int f = 0; f < 12; ++f) vs.Update(1.0f / 60.0f);
    BeginDrawing(); ClearBackground(BLACK); vs.Draw(); EndDrawing();
    Capture("transformado_golpe");
    CloseWindow();
    return failures;
}
