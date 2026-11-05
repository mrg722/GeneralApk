// Poses sueltas del CLON ANTIGUO (skin 1, assets/bosses/rayder_clone) y de
// GRINDER (skin 3, assets/bosses/grinder). F1.2.
//
// Cada pose se dibuja con 1-2 recortes (`source rect`) de su PNG que dejan fuera
// los restos de recorte del arte (los PNG NO se tocan) y con el ancla (pies) en
// el CENTRO DEL TORSO, medido con tools/poses/medir_clon_grinder.py:
//   kick.png   pierna suelta abajo-izquierda (isla x < 88, y >= 154): fuera;
//   hurt.png   trozo de bota pegado al borde derecho (x >= 154, y >= 186): fuera;
//   ready.png  trozo de bota en el borde derecho (x >= 162, y >= 216): fuera.
// Con el ancla en el torso el cuerpo no "salta" de lado al cambiar de pose (antes
// se anclaba en el centro del PNG, que en kick/ready/ram cae lejos del cuerpo).
//
// Los ataques con visual propio salen de los segmentos del MoveSet
// (src/game/moves/Moves_CloneGrinder.cpp); esta tabla da las poses de los
// estados (reposo, caminar, golpe, derribo, levantarse, salto, bloqueo...) y la
// reserva de los ataques sin segmentos.
#include "game/visual/PoseTables.h"
#include "game/player/PoseFxPresets.h"
#include <cmath>
#include <cstring>

namespace district_fury {

// Piezas recortadas y ancladas de una pose (las usa tambien el MoveSet de
// Moves_CloneGrinder.cpp para sus segmentos y PoseCloneGrinderTests). mirrored =
// espejada (codazo hacia atras). Devuelve el numero de piezas (0 = sin pose).
int CloneGrinderPieces(const char* folder, const char* pose, bool mirrored, PosePiece out[2]);
// Recorte `index` de la tabla (para los tests); false al pasarse del final.
bool CloneGrinderCutInfo(int index, const char*& folder, const char*& pose, Rectangle& a, Rectangle& b);

namespace {

// Recorte de una pose: 1-2 rectangulos (px de textura) y el ancla (centro del
// torso en x, suela en y). b.width == 0 = una sola pieza.
struct PoseCut {
    const char* folder;
    const char* pose;
    Rectangle a, b;
    Vector2 anchor;
};

constexpr PoseCut kCuts[] = {
    // CLON ANTIGUO (alto de idle1 = 282 px de textura = 118 px de pantalla).
    {"rayder_clone", "dash", {0, 1, 207, 204}, {}, {100, 205}},
    {"rayder_clone", "death", {0, 0, 172, 130}, {}, {72, 130}},
    {"rayder_clone", "hurt", {2, 17, 185, 169}, {2, 186, 152, 62}, {83, 248}},
    {"rayder_clone", "idle1", {1, 2, 129, 277}, {}, {60, 279}},
    {"rayder_clone", "idle2", {2, 2, 132, 276}, {}, {60, 278}},
    {"rayder_clone", "idle3", {2, 2, 146, 276}, {}, {75, 278}},
    {"rayder_clone", "idle4", {0, 1, 133, 277}, {}, {76, 278}},
    {"rayder_clone", "kick", {86, 0, 126, 154}, {88, 154, 124, 116}, {150, 270}},
    {"rayder_clone", "punch", {0, 1, 136, 247}, {}, {83, 248}},
    {"rayder_clone", "ready", {2, 0, 165, 216}, {2, 216, 160, 34}, {103, 250}},
    {"rayder_clone", "release_orb", {0, 1, 289, 281}, {}, {156, 282}},
    // GRINDER (pixel art x3; sin restos: el recorte es el PNG entero).
    {"grinder", "death", {0, 0, 52, 36}, {}, {29, 36}},
    {"grinder", "hurt", {0, 0, 30, 54}, {}, {15, 54}},
    {"grinder", "idle", {0, 0, 37, 57}, {}, {18, 57}},
    {"grinder", "overdrive", {0, 0, 42, 62}, {}, {21, 62}},
    {"grinder", "ram", {0, 0, 75, 59}, {}, {55, 59}},
    {"grinder", "saw", {0, 0, 46, 66}, {}, {23, 66}},
    {"grinder", "slam", {0, 0, 52, 59}, {}, {28, 59}},
};

const PoseCut* FindCut(const char* folder, const char* pose) {
    if (folder == nullptr || pose == nullptr) return nullptr;
    for (const PoseCut& c : kCuts)
        if (std::strcmp(c.folder, folder) == 0 && std::strcmp(c.pose, pose) == 0) return &c;
    return nullptr;
}

// Pieza con el recorte `r` cuyo punto `anchor` (px de textura) cae en los pies.
// DrawBossPoseEx pone en los pies el punto (src.x + w/2 - offset.x,
// src.y + h - offset.y) del recorte, tambien espejado.
PosePiece CutPiece(const char* pose, Rectangle r, Vector2 anchor, bool facesRight) {
    PosePiece p;
    p.pose = pose;
    p.src = r;
    p.offset = {r.x + r.width * 0.5f - anchor.x, r.y + r.height - anchor.y};
    p.facesRight = facesRight;
    return p;
}

int Pieces(const char* folder, const char* pose, PoseFrame& out) {
    out.pieceCount = CloneGrinderPieces(folder, pose, false, out.pieces);
    return out.pieceCount;
}

PresetUse Rot(float a, char ph = 'A') { return posefx::CAIDA_ROT(a, ph); }

}  // namespace

int CloneGrinderPieces(const char* folder, const char* pose, bool mirrored, PosePiece out[2]) {
    const PoseCut* c = FindCut(folder, pose);
    const bool right = PoseFacesRightCloneGrinder(folder, pose) != 0;
    const bool fr = mirrored ? !right : right;
    if (c == nullptr) {   // pose sin medir: el PNG entero anclado abajo al centro
        out[0] = PosePiece{};
        out[0].pose = pose;
        out[0].facesRight = fr;
        return pose != nullptr ? 1 : 0;
    }
    out[0] = CutPiece(c->pose, c->a, c->anchor, fr);
    if (c->b.width <= 0) return 1;
    out[1] = CutPiece(c->pose, c->b, c->anchor, fr);
    return 2;
}

bool CloneGrinderCutInfo(int index, const char*& folder, const char*& pose, Rectangle& a, Rectangle& b) {
    if (index < 0 || index >= static_cast<int>(sizeof(kCuts) / sizeof(kCuts[0]))) return false;
    folder = kCuts[index].folder; pose = kCuts[index].pose; a = kCuts[index].a; b = kCuts[index].b;
    return true;
}

// Nombre de la pose (lo usan CharacterPose y moves_sheet): la misma eleccion que
// PoseFrameClone sin el reloj de ataque.
const char* PoseClone(PlayerState state, AttackType attack, bool rage, double time) {
    (void)rage;
    switch (state) {
        case PlayerState::Defeat: return "death";
        case PlayerState::Hit: case PlayerState::GuardBreak: return "hurt";
        case PlayerState::Knockdown: return "death";
        case PlayerState::Recovery: case PlayerState::Block: case PlayerState::Victory: return "ready";
        case PlayerState::Dash: return "dash";
        case PlayerState::Jump: return "dash";
        case PlayerState::Attack: {
            const float phase = std::fmod(static_cast<float>(time), 0.56f);
            if (attack == AttackType::Energy) return phase < 0.16f ? "ready" : "release_orb";
            if (attack == AttackType::Kick) return phase < 0.10f ? "ready" : "kick";
            return phase < 0.09f ? "ready" : "punch";
        }
        case PlayerState::Walk: {
            const int k = static_cast<int>(time * 6.0) % 4;
            return k == 1 ? "idle3" : k == 3 ? "idle4" : "idle1";
        }
        default: return (static_cast<int>(time * 1.6) % 2) == 0 ? "idle1" : "idle2";
    }
}

const char* PoseGrinder(PlayerState state, AttackType attack, bool rage, double time) {
    (void)time;
    switch (state) {
        case PlayerState::Defeat: case PlayerState::Knockdown: return "death";
        case PlayerState::Hit: case PlayerState::GuardBreak: return "hurt";
        case PlayerState::Recovery: return "slam";
        case PlayerState::Victory: return "overdrive";
        case PlayerState::Dash: return "ram";
        case PlayerState::Attack:
            if (attack == AttackType::Energy || rage) return "overdrive";
            return attack == AttackType::Kick ? "slam" : "saw";
        default: return "idle";
    }
}

bool PoseFrameClone(const PoseContext& ctx, PoseFrame& out) {
    const char* f = "rayder_clone";
    switch (ctx.state) {
        case PlayerState::Knockdown:
            // En el aire (Airborne llega como Knockdown): `hurt` girando hacia
            // atras alrededor de los pies; en el suelo: `death` (arrodillado).
            if (ctx.z > 0.5f) {
                Pieces(f, "hurt", out);
                out.fx.presets[out.fx.presetCount++] = Rot(80);
            } else {
                Pieces(f, "death", out);
            }
            return true;
        case PlayerState::Recovery:
            // Levantarse (y la recuperacion corta tras un golpe o aterrizaje):
            // `ready` que se estira desde agachado.
            Pieces(f, "ready", out);
            out.fx.keys[0] = FxKey{'A', 0.0f, 1.04f, 0.90f, 0, 0, 6};
            out.fx.keys[1] = FxKey{'A', 0.6f, 1.0f, 1.0f, 0, 0, 0};
            out.fx.keyCount = 2;
            return true;
        case PlayerState::Jump: {
            // Subida: `dash` inclinado hacia atras; caida: `ready`.
            const bool rising = ctx.stateTime < 0.42f;
            Pieces(f, rising ? "dash" : "ready", out);
            if (rising) {
                out.fx.keys[0] = FxKey{'A', 0.0f, 1, 1, -10, 0, 0};
                out.fx.keys[1] = FxKey{'A', 1.0f, 1, 1, -10, 0, 0};
                out.fx.keyCount = 2;
            }
            return true;
        }
        case PlayerState::Walk: {
            // Pierna adelantada (idle3) / paso cruzado (idle4) alternando con
            // idle1 al ritmo real de walkPhase (el feel pone el rebote y el vaiven).
            const float w = std::fmod(std::fabs(ctx.walkPhase), 2.0f * posefx::kPi) / (2.0f * posefx::kPi);
            const char* p = w < 0.25f ? "idle1" : w < 0.5f ? "idle3" : w < 0.75f ? "idle1" : "idle4";
            Pieces(f, p, out);
            return true;
        }
        case PlayerState::Attack:
            // Reserva (ataques sin segmentos propios): por fase real del ataque.
            if (ctx.attackType == AttackType::Energy) Pieces(f, ctx.phase == 'S' ? "ready" : "release_orb", out);
            else if (ctx.attackType == AttackType::Kick) Pieces(f, ctx.phase == 'S' ? "ready" : "kick", out);
            else Pieces(f, ctx.phase == 'S' ? "ready" : "punch", out);
            return true;
        default:
            return Pieces(f, PoseClone(ctx.state, ctx.attackType, ctx.rage, ctx.time), out) > 0;
    }
}

bool PoseFrameGrinder(const PoseContext& ctx, PoseFrame& out) {
    const char* f = "grinder";
    switch (ctx.state) {
        case PlayerState::Idle: {
            // Motor de la sierra: latido lento + zumbido continuo (0.33 px de
            // textura = 1 px de pantalla a 30 Hz, sin decaer).
            Pieces(f, "idle", out);
            const float buzz = 0.33f * std::sin(2.0f * posefx::kPi * 30.0f * static_cast<float>(ctx.time));
            for (int i = 0; i < out.pieceCount; ++i) out.pieces[i].offset.x += buzz;
            out.fx.presets[out.fx.presetCount++] = posefx::LATIDO(0.02f, 1.2f, '*');
            return true;
        }
        case PlayerState::Walk: {
            // Paso pesado: hundimiento |sin| (el feel pone el rebote) e
            // inclinacion que alterna +-2 grados con cada pie.
            Pieces(f, "idle", out);
            const float s = std::sin(ctx.walkPhase);
            for (int i = 0; i < out.pieceCount; ++i) out.pieces[i].offset.y += 0.5f * std::fabs(s);
            out.fx.keys[0] = FxKey{'A', 0.0f, 1, 1, 2.0f * s, 0, 0};
            out.fx.keys[1] = FxKey{'A', 1.0f, 1, 1, 2.0f * s, 0, 0};
            out.fx.keyCount = 2;
            return true;
        }
        case PlayerState::Knockdown:
            if (ctx.z > 0.5f) {
                Pieces(f, "hurt", out);
                out.fx.presets[out.fx.presetCount++] = Rot(75);
            } else {
                Pieces(f, "death", out);
            }
            return true;
        case PlayerState::Recovery:
            // Levantarse: death -> slam (apoya los punos) -> idle.
            Pieces(f, ctx.stateTime < 0.16f ? "slam" : "idle", out);
            return true;
        case PlayerState::Block:
            Pieces(f, "idle", out);
            out.fx.keys[0] = FxKey{'A', 0.0f, 1.03f, 0.94f, -2, 0, 0};
            out.fx.keys[1] = FxKey{'A', 1.0f, 1.03f, 0.94f, -2, 0, 0};
            out.fx.keyCount = 2;
            return true;
        default:
            return Pieces(f, PoseGrinder(ctx.state, ctx.attackType, ctx.rage, ctx.time), out) > 0;
    }
}

int PoseFacesRightCloneGrinder(const char* folder, const char* pose) {
    (void)pose;   // medido: todas las poses de las dos carpetas miran a la derecha
    if (folder == nullptr) return -1;
    if (std::strcmp(folder, "rayder_clone") == 0) return 1;
    if (std::strcmp(folder, "grinder") == 0) return 1;
    return -1;
}

}  // namespace district_fury
