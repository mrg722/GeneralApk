// Hoja de movimientos por personaje (F0.4, tarea 10).
// Uso (desde la raiz del repo): moves_sheet <dir_salida> [id] [--partes]
// Para cada personaje (o solo `id`) y cada movimiento HABILITADO de su MoveSet,
// ejecuta la entrada real (scriptedInput: J, >J, vK, QCB+K, salto + J...) contra
// un enemigo de prueba y captura 6 cuadros (S, A inicio, A medio, A fin, R medio,
// fin) con la forma de golpe (arco o caja) y las hurtboxes dibujadas. Escribe
// <dir>/moves_<id>.png y <dir>/moves.tsv
//   (id, nombre, movimiento, clip usado, origen = override|canonico|fallback|
//    historico|pose|IDLE, golpeo s/n)
// e imprime el conteo por personaje y "idle-como-ataque: N" (debe ser 0).
#include "game/CharacterVisual.h"
#include "game/Player.h"
#include "game/StreetEnemy.h"
#include "game/combat/HitResolver.h"
#include "game/player/PlayerCommon.h"
#include "rendering/AssetManager.h"
#include "raylib.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

using namespace district_fury;

namespace {
constexpr float kDt = 1.0f / 60.0f;
constexpr int CW = 200, CH = 230;

PlayerInput Dir(float x, float y) { PlayerInput i; i.moveX = x; i.moveY = y; return i; }

// Entrada que pide `id` (frames de una sola pulsacion; los eslabones se
// encadenan con NextPress). f = +1 mirando a la derecha.
struct Script {
    std::vector<PlayerInput> pre;     // frames previos (comando de direccion, salto...)
    std::vector<PlayerInput> presses; // pulsaciones de la cadena (cada una al arrancar el golpe anterior)
    bool rage = false, finisher = false, stomp = false, enemyBehind = false, air = false;
};

Script ScriptFor(AttackId id) {
    Script s;
    PlayerInput J; J.punch = true;
    PlayerInput K; K.kick = true;
    auto dirJ = [&](float x, float y) { PlayerInput i = Dir(x, y); i.punch = true; return i; };
    auto dirK = [&](float x, float y) { PlayerInput i = Dir(x, y); i.kick = true; return i; };
    const PlayerInput down = Dir(0, 1), df = Dir(1, 1), db = Dir(-1, 1), fwd = Dir(1, 0);
    PlayerInput jump; jump.jump = true;
    PlayerInput dash; dash.dash = true;
    switch (id) {
        case AttackId::Punch1: s.presses = {J}; break;
        case AttackId::Punch2: s.presses = {J, J}; break;
        case AttackId::PunchHook: s.presses = {J, J, J}; break;
        case AttackId::Punch3: s.presses = {J, J, J, J}; break;
        case AttackId::Chain5: s.presses = {J, J, J, J, J}; break;
        case AttackId::Chain6: s.presses = {J, J, J, J, J, J}; break;
        case AttackId::Kick: s.presses = {K}; break;
        case AttackId::Kick2: s.presses = {K, K}; break;
        case AttackId::Kick3: s.presses = {K, K, K}; break;
        case AttackId::KickLauncher: s.presses = {J, J, K}; break;
        case AttackId::FwdPunch: s.presses = {dirJ(1, 0)}; break;
        case AttackId::FwdKick: s.presses = {dirK(1, 0)}; break;
        case AttackId::BackElbow: s.presses = {dirJ(-1, 0)}; s.enemyBehind = true; break;
        case AttackId::SpinKick: s.presses = {dirK(-1, 0)}; s.enemyBehind = true; break;
        case AttackId::Sweep: s.presses = {dirK(0, 1)}; break;
        case AttackId::Uppercut: s.presses = {dirJ(0, 1)}; break;
        case AttackId::Stomp: s.presses = {dirJ(0, 1)}; s.stomp = true; break;
        case AttackId::DashAttack: s.pre = {dash}; s.presses = {J}; break;
        case AttackId::DashKick: s.pre = {dash}; s.presses = {K}; break;
        case AttackId::AirPunch: s.pre = {jump, {}, {}, {}, {}, {}, {}, {}}; s.presses = {J}; s.air = true; break;
        case AttackId::AirPunch2: s.pre = {jump, {}, {}, {}, {}, {}}; s.presses = {J, J}; s.air = true; break;
        case AttackId::AirKick: s.pre = {jump, {}, {}, {}, {}, {}, {}, {}}; s.presses = {K}; s.air = true; break;
        case AttackId::DiveKick: s.pre = {jump, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}}; s.presses = {dirK(0, 1)}; s.air = true; break;
        case AttackId::AirSpike: s.pre = {jump, {}, {}, {}, {}, {}, {}, {}}; s.presses = {dirJ(1, 0)}; s.air = true; break;
        case AttackId::EnergyWave: s.pre = {down, down, df, df}; s.presses = {dirJ(1, 0)}; break;
        case AttackId::RisingUppercut: s.pre = {fwd, fwd, down, down}; s.presses = {dirK(1, 1)}; break;
        case AttackId::Hurricane: s.pre = {down, down, db, db}; s.presses = {dirK(-1, 0)}; break;
        case AttackId::DrillKick: s.pre = {down, down, df, df}; s.presses = {dirK(1, 0)}; break;
        case AttackId::Signature1: s.pre = {down, down, db, db}; s.presses = {dirJ(-1, 0)}; break;
        case AttackId::Signature2: { PlayerInput l = Dir(1, 0); l.energy = true; s.presses = {l}; break; }
        case AttackId::Signature3: { PlayerInput l = Dir(0, 1); l.energy = true; s.presses = {l}; break; }
        case AttackId::RageAttack: { PlayerInput r; r.rage = true; s.presses = {r}; s.rage = true; break; }
        case AttackId::Finisher: s.presses = {K}; s.rage = true; s.finisher = true; break;
        default: break;   // Grab..WakeupAttack: Fase 2 (sin entrada todavia)
    }
    return s;
}

void DrawShape(const Player& p) {
    const AttackShape s = PlayerAttackShape(p);
    if (!s.valid) return;
    const float lane = p.position.y;
    if (s.arc) {
        const int sides = s.limb.bothSides ? 2 : 1;
        for (int k = 0; k < sides; ++k) {
            const LimbPose a = EvalLimbArc(s.limb, s.x, s.z, s.facing, s.scale, s.u, k == 0 ? 1.0f : -1.0f);
            const LimbPose b = EvalLimbArc(s.limb, s.x, s.z, s.facing, s.scale, s.uPrev, k == 0 ? 1.0f : -1.0f);
            const Vector2 p0{a.base.x, lane - a.base.y}, p1{a.tip.x, lane - a.tip.y}, pp{b.tip.x, lane - b.tip.y};
            DrawLineEx(p0, p1, a.radius * 2.0f, {255, 60, 60, 110});
            DrawCircleV(p0, a.radius, {255, 60, 60, 110});
            DrawCircleV(p1, a.radius, {255, 60, 60, 150});
            DrawLineEx(pp, p1, 2.0f, {255, 230, 80, 220});
            DrawCircleLinesV({a.pivot.x, lane - a.pivot.y}, 3, YELLOW);
        }
    } else {
        DrawRectangleLinesEx({s.rect.x0, lane - s.rect.y1, s.rect.x1 - s.rect.x0, s.rect.y1 - s.rect.y0}, 2, RED);
    }
}

void DrawHurt(const Rect2& r, float lane, Color c) {
    DrawRectangleLinesEx({r.x0, lane - r.y1, r.x1 - r.x0, r.y1 - r.y0}, 1.5f, c);
}
}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) { std::fprintf(stderr, "uso: moves_sheet <dir_salida> [id]\n"); return 2; }
    const std::string out = argv[1];
    int only = -1;
    bool parts = false;
    for (int i = 2; i < argc; ++i) {
        if (std::strcmp(argv[i], "--partes") == 0) parts = true;
        else only = std::atoi(argv[i]);
    }
    std::error_code ec;
    std::filesystem::create_directories(out, ec);
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(640, 360, "moves");
    AssetManager::Get().LoadAll();
    FILE* tsv = std::fopen((out + "/moves.tsv").c_str(), "w");
    if (tsv) std::fprintf(tsv, "id\tnombre\tmovimiento\tclip\torigen\tgolpeo\n");
    int idleTotal = 0, minDefault = 1000;
    for (int id = 0; id < CharacterCount(); ++id) {
        if (only >= 0 && id != only) continue;
        const CharacterVisual& cv = GetCharacterVisual(id);
        std::vector<std::vector<Image>> rows;
        std::vector<std::string> labels;
        int enabled = 0, idleHere = 0, hits = 0;
        for (int m = 1; m < kAttackIdCount; ++m) {
            const AttackId mid = static_cast<AttackId>(m);
            if (mid == AttackId::Skill) continue;
            Player p;
            p.ApplyCharacter(id);
            p.position = {1200, 565, 0};
            p.facing = Facing::Right;
            p.Update(kDt);   // carga el animador
            if (!p.MoveEnabled(mid)) continue;
            Script sc = ScriptFor(mid);
            if (sc.presses.empty()) continue;   // Fase 2: sin entrada
            ++enabled;
            std::vector<StreetEnemy> enemies(1);
            const float ex = sc.enemyBehind ? 1200 - 80 : 1200 + 82;
            enemies[0].Init({ex, 565, 0}, StreetEnemyType::Brute);
            enemies[0].active = true;
            enemies[0].hp = enemies[0].maxHp = 100000;
            enemies[0].facing = sc.enemyBehind ? Facing::Right : Facing::Left;
            PlayerInput in;
            p.scriptedInput = &in;
            p.stompTargetAhead = sc.stomp;
            if (sc.rage) { p.rage = p.maxRage; p.rageState = RageState::Active; p.isRageMode = true; p.rageStateTimer = 9.0f; }
            if (sc.finisher) p.comboCount = 5;
            StageHitStyle st;
            CombatFeedback fb;
            bool hit = false;
            auto step = [&]() {
                if (sc.finisher) p.comboCount = std::max(p.comboCount, 5);
                p.Update(kDt);
                in = PlayerInput{};
                const HitReport r = ResolvePlayerVsEnemies(p, enemies, st, fb);
                if (r.hits > 0 && p.currentAttack == mid) hit = true;
                enemies[0].Update(kDt, p, nullptr);
                enemies[0].position.x = ex;   // que no se aleje
                enemies[0].hp = 100000;
            };
            for (const PlayerInput& f : sc.pre) { in = f; step(); }
            AttackId prev = AttackId::None;
            for (const PlayerInput& f : sc.presses) {
                in = f;
                step();
                for (int i = 0; i < 70 && !(p.state == PlayerState::Attack && p.currentAttack != prev); ++i) step();
                prev = p.currentAttack;
            }
            std::vector<Image> shots;
            std::string clip = "-", origin = "-";
            bool sawIdle = false;
            if (p.state == PlayerState::Attack && p.currentAttack == mid) {
                const AttackDef d = p.CurrentAttackDef();
                const float tot = AttackTotalDuration(d);
                const float marks[6] = {d.startup * 0.5f, d.startup + kDt * 0.5f, d.startup + d.active * 0.5f,
                                        d.startup + d.active - kDt * 0.5f, d.startup + d.active + d.recovery * 0.5f,
                                        tot - kDt * 1.5f};
                const char* names[6] = {"S", "A0", "A/2", "A1", "R/2", "fin"};
                clip = p.attackClipName;
                origin = Player::ClipOriginName(p.attackClipOrigin);
                int next = 0;
                for (int guard = 0; guard < 240 && next < 6; ++guard) {
                    const bool inMove = p.state == PlayerState::Attack && p.currentAttack == mid;
                    if (inMove && p.animator.currentClipName == "idle") sawIdle = true;
                    if (inMove && cv.folder && !cv.atlasId && cv.kfRoster < 0) {
                        // Poses sueltas: la tabla (F1.1/F1.2) cicla por reloj e incluye
                        // cuadros de reposo en la cola; es "idle como ataque" solo si
                        // ningun instante del ciclo da una pose de ataque.
                        bool any = false;
                        for (double t = 0.0; t < 0.6 && !any; t += 0.02)
                            any = std::strncmp(CharacterPose(id, p.state, p.attackType, p.isRageMode, t), "idle", 4) != 0;
                        if (!any) sawIdle = true;
                    }
                    while (next < 6 && (!inMove || p.attackElapsed >= marks[next])) {
                        RenderTexture2D rt = LoadRenderTexture(CW, CH);
                        Camera2D cam{};
                        cam.target = {p.position.x, p.position.y};
                        cam.offset = {CW / 2.0f, CH - 24.0f};
                        cam.zoom = 0.85f;
                        BeginTextureMode(rt);
                        ClearBackground({62, 70, 84, 255});
                        BeginMode2D(cam);
                        DrawLine((int)p.position.x - 200, (int)p.position.y, (int)p.position.x + 200, (int)p.position.y,
                                 {255, 255, 0, 120});
                        enemies[0].Draw();
                        p.Draw();
                        DrawHurt(EnemyHurtRect(enemies[0]), enemies[0].position.y, {80, 200, 255, 220});
                        DrawHurt(PlayerHurtRect(p), p.position.y, {120, 255, 120, 220});
                        DrawShape(p);
                        EndMode2D();
                        DrawRectangle(0, 0, CW, 16, {0, 0, 0, 170});
                        DrawText(TextFormat("%s %s", names[next], inMove ? "" : "(fuera)"), 4, 2, 12, WHITE);
                        EndTextureMode();
                        Image img = LoadImageFromTexture(rt.texture);
                        ImageFlipVertical(&img);
                        UnloadRenderTexture(rt);
                        shots.push_back(img);
                        ++next;
                    }
                    if (next >= 6) break;
                    step();
                }
            }
            if (sawIdle) { origin = "IDLE"; ++idleHere; }
            if (hit) ++hits;
            if (tsv)
                std::fprintf(tsv, "%d\t%s\t%s\t%s\t%s\t%s\n", id, cv.name, AttackIdName(mid), clip.c_str(),
                             origin.c_str(), hit ? "s" : "n");
            labels.push_back(std::string(AttackIdName(mid)) + "  " + clip + " [" + origin + "]" + (hit ? "  GOLPEA" : "  (no golpea)"));
            rows.push_back(shots);
        }
        idleTotal += idleHere;
        if (id <= 6) minDefault = std::min(minDefault, enabled);
        // Hoja: una fila por movimiento, 6 columnas.
        const int rh = CH + 16;
        Image sheet = GenImageColor(6 * CW, 26 + static_cast<int>(rows.size()) * rh, {20, 20, 26, 255});
        ImageDrawText(&sheet, TextFormat("%d %s  movimientos=%d golpean=%d idle=%d", id, cv.name, enabled, hits, idleHere),
                      6, 4, 18, YELLOW);
        for (size_t r = 0; r < rows.size(); ++r) {
            const int y = 26 + static_cast<int>(r) * rh;
            ImageDrawText(&sheet, labels[r].c_str(), 6, y + 1, 12, {255, 230, 160, 255});
            for (size_t c = 0; c < rows[r].size() && c < 6; ++c) {
                ImageDraw(&sheet, rows[r][c], {0, 0, (float)CW, (float)CH}, {(float)(c * CW), (float)(y + 16), (float)CW, (float)CH},
                          WHITE);
                UnloadImage(rows[r][c]);
            }
        }
        ExportImage(sheet, (out + "/moves_" + std::to_string(id) + ".png").c_str());
        if (parts) {   // --partes: tambien trozos de 5 filas (para revisarlos a ojo)
            for (size_t r0 = 0, k = 0; r0 < rows.size(); r0 += 5, ++k) {
                const int y0 = 26 + static_cast<int>(r0) * rh;
                const int h = static_cast<int>(std::min<size_t>(5, rows.size() - r0)) * rh;
                Image part = ImageFromImage(sheet, {0, (float)y0, (float)sheet.width, (float)h});
                ExportImage(part, (out + "/moves_" + std::to_string(id) + "_p" + std::to_string(k) + ".png").c_str());
                UnloadImage(part);
            }
        }
        UnloadImage(sheet);
        std::printf("%2d %-34s movimientos=%2d golpean=%2d idle=%d\n", id, cv.name, enabled, hits, idleHere);
        std::fflush(stdout);
    }
    if (tsv) std::fclose(tsv);
    if (only < 0 || only <= 6) std::printf("minimo ids 0-6: %d\n", minDefault);
    std::printf("idle-como-ataque: %d\n", idleTotal);
    CloseWindow();
    return idleTotal == 0 ? 0 : 1;
}
