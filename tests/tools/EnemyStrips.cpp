// Tiras de estados de los enemigos de calle (F1.9): una imagen por tipo con sus
// ataques de datos (mitad del activo, con la caja real en verde) y los estados
// nuevos Launched (subiendo / bajando), WallBounce, Down, WakeUp, Grabbed y
// Thrown (giro de 540 grados).
// Uso: enemy_strips <dir_salida> [--no-box]   (correr desde la raiz del repo).
// Escribe <dir_salida>/enemy_<tipo>.png y nada en la raiz del repo.
#include "game/StreetEnemy.h"
#include "game/combat/CombatMath.h"
#include "rendering/AssetManager.h"
#include "raylib.h"
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>
using namespace district_fury;

namespace {
constexpr float kDt = 1.0f / 60.0f;
constexpr float kLane = 565.0f;
constexpr int kCellW = 190, kCellH = 300, kFeetY = 250;

struct Cell {
    std::string label;
    std::function<void(StreetEnemy&)> setup;
};

void Run(StreetEnemy& e, int frames) {
    Player far;
    far.position = {9000, kLane, 0};
    for (int i = 0; i < frames; ++i) e.Update(kDt, far);
}

HitInfo Hit(float launch, float kb = 160.0f) {
    HitInfo h;
    h.damage = 1;
    h.dirX = -1.0f;   // el enemigo mira a la derecha: lo empujan hacia atras
    h.knockback = kb;
    h.launch = launch;
    h.hitstun = 0.4f;
    h.unblockable = true;
    return h;
}

const char* Slug(StreetEnemyType t) {
    switch (t) {
        case StreetEnemyType::Brute: return "brute";
        case StreetEnemyType::Charger: return "charger";
        case StreetEnemyType::Enforcer: return "enforcer";
        case StreetEnemyType::ChemicalSoldier: return "chemical";
        case StreetEnemyType::UrbanNinja: return "ninja";
        case StreetEnemyType::Mutant: return "mutant";
        case StreetEnemyType::ArmoredGuard: return "armored";
        default: return "punk";
    }
}

void DrawShape(const StreetEnemy& e) {
    if (!e.AttackIsActive()) return;
    const CombatBox b = e.GetAttackHitbox();
    if (!b.IsValid()) return;
    DrawRectangleLinesEx({b.x, b.y, b.width, b.height}, 2.0f, {60, 255, 120, 220});
}
}  // namespace

int main(int argc, char** argv) {
    std::string out;
    bool box = true;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--no-box") == 0) box = false;
        else out = argv[i];
    }
    if (out.empty()) { std::fprintf(stderr, "uso: enemy_strips <dir_salida> [--no-box]\n"); return 2; }
    std::error_code ec;
    std::filesystem::create_directories(out, ec);
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(640, 360, "enemy_strips");
    AssetManager::Get().LoadAll();

    const StreetEnemyType types[] = {StreetEnemyType::Punk, StreetEnemyType::Brute, StreetEnemyType::Charger,
                                     StreetEnemyType::Enforcer, StreetEnemyType::ChemicalSoldier,
                                     StreetEnemyType::UrbanNinja, StreetEnemyType::Mutant,
                                     StreetEnemyType::ArmoredGuard};
    for (StreetEnemyType t : types) {
        std::vector<Cell> cells;
        cells.push_back({"IDLE", [](StreetEnemy& e) { Run(e, 4); }});
        const EnemyMoveSet& m = GetEnemyMoveSet(t);
        for (int i = 0; i < static_cast<int>(m.attacks.size()); ++i) {
            const EnemyAttackDef& a = m.attacks[static_cast<std::size_t>(i)];
            cells.push_back({a.name, [i, a](StreetEnemy& e) {
                                 e.StartAttack(i);
                                 Run(e, static_cast<int>((a.startup + a.active * 0.5f) / kDt));
                             }});
        }
        cells.push_back({"LAUNCHED sube", [](StreetEnemy& e) { e.ApplyHit(Hit(720.0f)); Run(e, 8); }});
        cells.push_back({"LAUNCHED baja", [](StreetEnemy& e) { e.ApplyHit(Hit(720.0f)); Run(e, 40); }});
        cells.push_back({"WALLBOUNCE", [](StreetEnemy& e) {
                             e.arenaMinX = e.position.x - 40.0f;
                             HitInfo h = Hit(300.0f, 600.0f);
                             h.wallBounce = true;
                             e.ApplyHit(h);
                             for (int f = 0; f < 60 && e.state != StreetEnemyState::WallBounce; ++f) Run(e, 1);
                             Run(e, 3);
                         }});
        cells.push_back({"DOWN", [](StreetEnemy& e) {
                             HitInfo h = Hit(0.0f, 60.0f);
                             h.low = true; h.knockdown = true;
                             e.ApplyHit(h);
                             Run(e, 14);
                         }});
        for (int k = 0; k < 3; ++k) {
            cells.push_back({k == 0 ? "WAKEUP 1" : k == 1 ? "WAKEUP 2" : "WAKEUP 3", [k](StreetEnemy& e) {
                                 HitInfo h = Hit(0.0f, 60.0f);
                                 h.low = true; h.knockdown = true;
                                 e.ApplyHit(h);
                                 for (int f = 0; f < 60 && e.state != StreetEnemyState::WakeUp; ++f) Run(e, 1);
                                 Run(e, 3 + k * 7);
                             }});
        }
        cells.push_back({"GRABBED", [](StreetEnemy& e) {
                             e.SetGrabbed(true, {e.position.x, e.position.y, 30.0f});
                             Run(e, 1);
                         }});
        for (int k = 0; k < 2; ++k) {
            cells.push_back({k == 0 ? "THROWN 1/3" : "THROWN 2/3", [k](StreetEnemy& e) {
                                 HitInfo h;
                                 e.Throw(h, {-80.0f, 0.0f, 520.0f});
                                 Run(e, static_cast<int>(e.throwDuration / kDt * (k == 0 ? 0.33f : 0.66f)));
                             }});
        }

        const int W = kCellW * static_cast<int>(cells.size());
        RenderTexture2D rt = LoadRenderTexture(W, kCellH);
        BeginTextureMode(rt);
        ClearBackground({34, 36, 44, 255});
        for (std::size_t i = 0; i < cells.size(); ++i) {
            const float cx = kCellW * (static_cast<float>(i) + 0.5f);
            StreetEnemy e;
            e.Init({cx, kLane, 0.0f}, t);
            e.Activate();
            e.facing = Facing::Right;
            e.arenaMinX = 0.0f;
            e.arenaMaxX = 1e6f;
            cells[i].setup(e);
            e.position.x = cx;   // el lunge / empuje no sale de la celda
            Camera2D cam{};
            cam.zoom = 1.0f;
            cam.target = {0.0f, kLane - kFeetY};
            BeginMode2D(cam);
            DrawLine(static_cast<int>(cx - kCellW * 0.45f), static_cast<int>(kLane), static_cast<int>(cx + kCellW * 0.45f),
                     static_cast<int>(kLane), {80, 84, 96, 255});
            e.Draw();
            if (box) DrawShape(e);
            EndMode2D();
            DrawText(cells[i].label.c_str(), static_cast<int>(cx - kCellW * 0.5f + 6), kCellH - 30, 12,
                     {235, 235, 235, 255});
            DrawText(e.GetStateName(), static_cast<int>(cx - kCellW * 0.5f + 6), kCellH - 15, 10, {255, 200, 90, 255});
            DrawLine(static_cast<int>(kCellW * (i + 1)), 0, static_cast<int>(kCellW * (i + 1)), kCellH, {20, 20, 24, 255});
        }
        EndTextureMode();
        Image img = LoadImageFromTexture(rt.texture);
        ImageFlipVertical(&img);
        const std::string path = out + "/enemy_" + Slug(t) + ".png";
        ExportImage(img, path.c_str());
        UnloadImage(img);
        UnloadRenderTexture(rt);
        std::printf("%s (%d celdas)\n", path.c_str(), static_cast<int>(cells.size()));
    }
    CloseWindow();
    return 0;
}
