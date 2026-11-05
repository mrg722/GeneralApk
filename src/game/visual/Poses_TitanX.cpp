// Poses sueltas de TITAN-X (skin 4, assets/bosses/titanx) y TITAN-X MEJORADO
// (skin 5, assets/bosses/titanx_mejorado). F1.1:
// - orientacion POR POSE (casi todo el arte mira a la derecha; solo Titan-X
//   `recoil` y Mejorado `shoot_orb` miran a la izquierda): asi camina, dashea y
//   golpea mirando hacia donde va, en los dos sentidos;
// - ancla en el CENTRO DEL TORSO (medido con tools/poses/medir_titanx.py), no en
//   el centro del PNG: al cambiar de pose el cuerpo no salta de lado;
// - recortes con `src` (los PNG no se tocan): puno suelto de punch1, trozo de
//   puno ajeno de slam, trozo ajeno de rage_aura, punta del puno de uppercut;
// - composicion "carga" = punch1 + punch2 (las dos mitades de una pose) y, para
//   los jefes (sin limite de piezas), el golpe completo con el puno en su sitio;
// - tabla de estados (caminar al ritmo de walkPhase, derribo -> tumbado ->
//   agachado, dash con estela verde...) y el dibujo compartido de los jefes.
// Los movimientos (segmentos + PoseFx de cada ataque) viven en
// src/game/moves/Moves_TitanX.cpp y usan TitanXPieces de aqui.
#include "game/visual/PoseTables.h"
#include "game/player/PoseFxEval.h"
#include "rendering/AssetManager.h"
#include "rendering/BossSprite.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>

namespace district_fury {

// Exportadas (declaradas en Moves_TitanX.cpp, combat/Boss.cpp, Stage3Game.cpp y
// Stage4Game.cpp: este bloque no tiene un header propio).
int TitanXPieces(const char* folder, const char* pose, PosePiece out[2], float fwd, bool mirror);
void DrawTitanXBossBody(int skin, PlayerState state, int attackId, float elapsed, float startup, float active,
                        float recovery, float stateTime, float walkPhase, Vector2 feet, float targetHeight,
                        bool facingRight, Color tint);

namespace {

constexpr float kPi = 3.14159265f;
constexpr const char* kTX = "titanx";
constexpr const char* kTXM = "titanx_mejorado";

// Medidas de tools/poses/medir_titanx.py (--cpp): ancho, alto, x del centro del
// torso (coordenadas de la textura), filas transparentes bajo el pie, orientacion.
struct Anchor { const char* folder; const char* pose; int w, h, torso, pad; bool right; };
const Anchor kAnchors[] = {
    {"titanx", "death", 244, 190, 122, 0, true},
    {"titanx", "idle1", 162, 259, 79, 2, true},
    {"titanx", "idle2", 150, 259, 74, 2, true},
    {"titanx", "idle3", 162, 264, 79, 2, true},
    {"titanx", "idle4", 158, 263, 78, 2, true},
    {"titanx", "lean", 160, 255, 75, 2, true},
    {"titanx", "punch1", 130, 217, 96, 2, true},
    {"titanx", "punch2", 77, 226, 21, 2, true},
    {"titanx", "recoil", 225, 240, 164, 6, false},
    {"titanx", "shoot_orb", 200, 254, 120, 2, true},
    {"titanx", "slam", 205, 191, 128, 1, true},
    {"titanx", "uppercut", 160, 300, 96, 2, true},
    {"titanx_mejorado", "cannon_aim", 178, 268, 74, 2, true},
    {"titanx_mejorado", "charge_orb", 165, 266, 76, 0, true},
    {"titanx_mejorado", "dash", 212, 305, 104, 2, true},
    {"titanx_mejorado", "death", 216, 125, 98, 2, true},
    {"titanx_mejorado", "idle1", 177, 284, 86, 1, true},
    {"titanx_mejorado", "idle2", 180, 277, 88, 1, true},
    {"titanx_mejorado", "idle3", 163, 277, 78, 2, true},
    {"titanx_mejorado", "idle4", 187, 269, 93, 2, true},
    {"titanx_mejorado", "rage_aura", 170, 237, 81, 5, true},
    {"titanx_mejorado", "shoot_orb", 218, 231, 116, 2, false},
    {"titanx_mejorado", "slam", 205, 219, 106, 2, true},
};

// Composicion "carga": punch1 (x 0..129) y punch2 pegado a su derecha (x 130..206
// del dibujo conjunto), pies alineados abajo. Centro del torso del conjunto (pecho,
// entre la cadera de punch1 y la cabeza de punch2).
constexpr float kCargaTorso = 128.0f;
constexpr float kPunch2X = 130.0f;

const Anchor* FindAnchor(const char* folder, const char* pose) {
    if (folder == nullptr || pose == nullptr) return nullptr;
    for (const Anchor& a : kAnchors)
        if (std::strcmp(a.folder, folder) == 0 && std::strcmp(a.pose, pose) == 0) return &a;
    return nullptr;
}

bool Is(const char* a, const char* b) { return a != nullptr && b != nullptr && std::strcmp(a, b) == 0; }

// Pieza con el centro del torso (torsoX, coordenadas de la textura) en los pies.
// fwd = px de textura hacia donde MIRA el dibujo; mirror = dibujo espejado.
PosePiece MakePiece(const Anchor& a, Rectangle src, float torsoX, float fwd, bool mirror) {
    PosePiece p;
    p.pose = a.pose;
    p.src = src;
    p.facesRight = a.right != mirror;
    const Rectangle s = src.width > 0 ? src : Rectangle{0, 0, static_cast<float>(a.w), static_cast<float>(a.h)};
    const float ox = s.x + s.width * 0.5f - torsoX + (a.right ? fwd : -fwd);
    const float oy = static_cast<float>(a.pad) - (static_cast<float>(a.h) - (s.y + s.height));
    p.offset = {ox, oy};
    return p;
}

// Golpe completo (solo jefes, 4 piezas): punch1 sin el puno suelto + su pierna
// trasera + punch2 + el puno suelto de punch1 en su sitio (a la derecha de punch2:
// el PNG viene "enrollado").
int FullPunchPieces(PosePiece out[4], float fwd) {
    const Anchor* p1 = FindAnchor(kTX, "punch1");
    const Anchor* p2 = FindAnchor(kTX, "punch2");
    if (!p1 || !p2) return 0;
    out[0] = MakePiece(*p1, {56, 0, 74, 217}, kCargaTorso, fwd, false);
    out[1] = MakePiece(*p1, {8, 81, 48, 136}, kCargaTorso, fwd, false);
    out[2] = MakePiece(*p2, {}, kCargaTorso - kPunch2X, fwd, false);
    // Puno: x 0..55, y 31..80 de punch1, a la derecha del borde de punch2 (x 207).
    out[3] = MakePiece(*p1, {0, 28, 56, 56}, kCargaTorso - (kPunch2X + 77.0f), fwd, false);
    return 4;
}

// Transformacion constante para todo el estado: dos claves iguales en A0 y A1
// (el neutro implicito queda en los extremos S0 / R1, fuera del estado).
void ConstKey(PoseFx& fx, float sx, float sy, float angle, float dx, float dy) {
    if (fx.keyCount > 6) return;
    for (int i = 0; i < 2; ++i) {
        FxKey k; k.phase = 'A'; k.v = static_cast<float>(i); k.sx = sx; k.sy = sy; k.angle = angle; k.dx = dx; k.dy = dy;
        fx.keys[fx.keyCount++] = k;
    }
}
void AddPreset(PoseFx& fx, const PresetUse& p) { if (fx.presetCount < 6) fx.presets[fx.presetCount++] = p; }
void AddOverlay(PoseFx& fx, int first, int count, float x, float y, float scale, Color tint) {
    if (fx.overlayCount >= 2) return;
    OverlayFx o; o.first = first; o.count = count; o.x = x; o.y = y; o.scale = scale; o.phase = 'A'; o.tint = tint;
    fx.overlay[fx.overlayCount++] = o;
}

const Color kGreen{120, 255, 140, 255};
const Color kGreenGhost{110, 255, 130, 255};

int Piece(PoseFrame& out, const char* folder, const char* pose) {
    out.pieceCount = TitanXPieces(folder, pose, out.pieces, 0.0f, false);
    return out.pieceCount;
}

// Tabla comun de estados (no-ataque) de los dos Titan-X. m = Mejorado.
bool StateFrame(const PoseContext& ctx, PoseFrame& out, bool m) {
    const char* f = m ? kTXM : kTX;
    const float t = ctx.stateTime;
    switch (ctx.state) {
        case PlayerState::Defeat:
            return Piece(out, f, "death") > 0;
        case PlayerState::Hit:
        case PlayerState::GuardBreak:
            Piece(out, f, m ? "idle1" : "idle4");
            AddPreset(out.fx, posefx::TEMBLOR(3, 28, 'A'));
            ConstKey(out.fx, 1, 1, -4, -4, 0);   // + los -7 grados del feel = se echa atras
            return true;
        case PlayerState::Knockdown: {
            if (ctx.z > 0.5f) {   // en el aire (Airborne): cae girando sobre los pies
                Piece(out, f, m ? "idle1" : "lean");
                AddPreset(out.fx, posefx::CAIDA_ROT(80, 'A'));
                return true;
            }
            // Tumbado; al final del derribo (0.85 s) se agacha para levantarse.
            if (t > 0.62f) {
                Piece(out, f, "slam");
                ConstKey(out.fx, 1.02f, 0.94f, 0, 0, 0);
                return true;
            }
            Piece(out, f, "death");
            AddPreset(out.fx, posefx::TEMBLOR(2, 20, 'A'));
            return true;
        }
        case PlayerState::Dash:
            Piece(out, f, m ? "dash" : "lean");
            out.fx.ghostCount = 4; out.fx.ghostInterval = 0.03f; out.fx.ghostTint = kGreenGhost;
            AddOverlay(out.fx, 37, 1, -40, -8, 0.9f, kGreen);   // polvo detras
            return true;
        case PlayerState::Block:
            Piece(out, f, "idle1");
            ConstKey(out.fx, 1.0f, 0.96f, 0, 0, 0);
            AddOverlay(out.fx, 16, 1, 6, -60, 1.25f, kGreen);
            return true;
        case PlayerState::Walk: {
            // Ciclo al ritmo de walkPhase (no del reloj): una pose por cuarto de paso.
            static const char* kTxCycle[4] = {"idle3", "idle1", "idle4", "idle2"};
            static const char* kTxmCycle[4] = {"idle3", "idle1", "idle4", "idle1"};   // sin idle2 (parpadea de orientacion)
            const float ph = std::max(0.0f, ctx.walkPhase);
            const int i = static_cast<int>(ph / (kPi * 0.5f)) % 4;
            Piece(out, f, (m ? kTxmCycle : kTxCycle)[i]);
            const float st = std::fabs(std::sin(ph));
            // Rebote pesado: el feel ya pone -3|sin| y 3.5 grados; aqui lo que falta
            // hasta -5|sin| (-6 el Mejorado) y la pisada aplastada.
            ConstKey(out.fx, 1.0f + 0.015f * (1 - st), 1.0f - 0.02f * (1 - st), -0.5f, 0, -(m ? 3.0f : 2.0f) * st);
            if (m) AddPreset(out.fx, posefx::LATIDO(0.01f, 1.2f));
            if (st < 0.35f) AddOverlay(out.fx, 37, 1, -10, -4, 0.7f, kGreen);   // polvo en la pisada
            return true;
        }
        case PlayerState::Jump:
            // Subida/caida las da MoveSet.jumpRise/jumpFall (saben la velocidad).
            Piece(out, f, "lean");
            return true;
        case PlayerState::Victory:
            Piece(out, f, m ? "rage_aura" : "uppercut");
            AddPreset(out.fx, posefx::LATIDO(m ? 0.03f : 0.04f, 3));
            AddOverlay(out.fx, 21, 2, 0, -10, 1.6f, kGreen);
            return true;
        case PlayerState::Recovery:
            Piece(out, f, "idle1");
            return true;
        default: {
            // Reposo: ciclo lento por reloj (respira el feel). Mejorado sin idle2.
            const int c = static_cast<int>(ctx.time * 3.2) % 4;
            static const char* kTxIdle[4] = {"idle1", "idle2", "idle3", "idle4"};
            static const char* kTxmIdle[4] = {"idle1", "idle3", "idle1", "idle4"};
            Piece(out, f, (m ? kTxmIdle : kTxIdle)[c]);
            return true;
        }
    }
}

// Ataque sin MoveVisual propio (no deberia pasar: todos tienen override).
bool AttackFallback(const PoseContext& ctx, PoseFrame& out, bool m) {
    const char* f = m ? kTXM : kTX;
    const char* pose;
    if (ctx.attackType == AttackType::Energy) pose = ctx.phase == 'S' ? (m ? "charge_orb" : "idle2") : (m ? "shoot_orb" : "recoil");
    else if (ctx.attackType == AttackType::Kick) pose = "slam";
    else pose = ctx.phase == 'S' ? (m ? "cannon_aim" : "carga") : (m ? "cannon_aim" : "shoot_orb");
    return Piece(out, f, pose) > 0;
}

}  // namespace

int TitanXPieces(const char* folder, const char* pose, PosePiece out[2], float fwd, bool mirror) {
    if (Is(folder, kTX) && Is(pose, "carga")) {
        // Las dos mitades de la pose de carga (punch1 sin el puno suelto + punch2).
        // El puno suelto de punch1 es el del brazo de punch2 (PNG enrollado):
        // con 2 piezas se recorta (los jefes lo dibujan en su sitio).
        const Anchor* p1 = FindAnchor(kTX, "punch1");
        const Anchor* p2 = FindAnchor(kTX, "punch2");
        out[0] = MakePiece(*p1, {56, 0, 74, 217}, kCargaTorso, fwd, mirror);
        out[1] = MakePiece(*p2, {}, kCargaTorso - kPunch2X, fwd, mirror);
        return 2;
    }
    const Anchor* a = FindAnchor(folder, pose);
    if (a == nullptr) { out[0] = PosePiece{}; out[0].pose = pose; return pose ? 1 : 0; }
    if (Is(folder, kTX) && Is(pose, "slam")) {   // sin el trozo de puno ajeno de la izquierda
        out[0] = MakePiece(*a, {35, 0, 170, 191}, static_cast<float>(a->torso), fwd, mirror);
        out[1] = MakePiece(*a, {9, 136, 26, 55}, static_cast<float>(a->torso), fwd, mirror);
        return 2;
    }
    if (Is(folder, kTX) && Is(pose, "punch1")) {   // sin el puno suelto
        out[0] = MakePiece(*a, {56, 0, 74, 217}, static_cast<float>(a->torso), fwd, mirror);
        return 1;
    }
    if (Is(folder, kTX) && Is(pose, "uppercut")) {   // 6 px menos de puno: <= 1.15x el reposo
        out[0] = MakePiece(*a, {0, 6, 160, 294}, static_cast<float>(a->torso), fwd, mirror);
        return 1;
    }
    if (Is(folder, kTXM) && Is(pose, "rage_aura")) {   // sin el trozo ajeno de abajo a la derecha
        out[0] = MakePiece(*a, {0, 0, 121, 237}, static_cast<float>(a->torso), fwd, mirror);
        out[1] = MakePiece(*a, {121, 0, 49, 185}, static_cast<float>(a->torso), fwd, mirror);
        return 2;
    }
    out[0] = MakePiece(*a, {}, static_cast<float>(a->torso), fwd, mirror);
    return 1;
}

// ---- Nombres de pose (CharacterPose / moves_sheet): la pose principal de cada estado ----
const char* PoseTitanX(PlayerState state, AttackType attack, bool rage, double time) {
    (void)rage;
    if (state == PlayerState::Defeat) return "death";
    if (state == PlayerState::Hit || state == PlayerState::GuardBreak) return "idle4";
    if (state == PlayerState::Knockdown) return "death";
    if (state == PlayerState::Dash) return "lean";
    if (state == PlayerState::Attack) {
        if (attack == AttackType::Energy) return "recoil";
        if (attack == AttackType::Kick) return "slam";
        return "shoot_orb";
    }
    static const char* kCycle[4] = {"idle1", "idle2", "idle3", "idle4"};
    return kCycle[static_cast<int>(time * 3.2) % 4];
}

const char* PoseTitanXM(PlayerState state, AttackType attack, bool rage, double time) {
    if (state == PlayerState::Defeat) return "death";
    if (state == PlayerState::Hit || state == PlayerState::GuardBreak) return "idle1";
    if (state == PlayerState::Knockdown) return "death";
    if (state == PlayerState::Dash) return "dash";
    if (state == PlayerState::Attack) {
        if (attack == AttackType::Energy) return "shoot_orb";
        if (rage) return "rage_aura";
        return attack == AttackType::Kick ? "slam" : "cannon_aim";
    }
    static const char* kCycle[4] = {"idle1", "idle3", "idle1", "idle4"};
    return kCycle[static_cast<int>(time * 3.2) % 4];
}

bool PoseFrameTitanX(const PoseContext& ctx, PoseFrame& out) {
    if (ctx.state == PlayerState::Attack) return AttackFallback(ctx, out, false);
    return StateFrame(ctx, out, false);
}

bool PoseFrameTitanXM(const PoseContext& ctx, PoseFrame& out) {
    if (ctx.state == PlayerState::Attack) return AttackFallback(ctx, out, true);
    return StateFrame(ctx, out, true);
}

int PoseFacesRightTitanX(const char* folder, const char* pose) {
    if (folder == nullptr) return -1;
    const bool tx = std::strcmp(folder, kTX) == 0, txm = std::strcmp(folder, kTXM) == 0;
    if (!tx && !txm) return -1;
    if (const Anchor* a = FindAnchor(folder, pose)) return a->right ? 1 : 0;
    return 1;   // casi todo el arte de los dos mira a la derecha
}

// ---- Dibujo compartido de los jefes Titan-X (Boss.cpp, Stage3Game, Stage4Game) ----
// Usa la MISMA resolucion que el jugador (MoveSet + tabla): orientacion por pose,
// escala uniforme (targetHeight / alto de idle1), PoseFx y superposiciones; el
// golpe de carga de Titan-X se dibuja completo (4 piezas, el jefe no tiene limite).
void DrawTitanXBossBody(int skin, PlayerState state, int attackId, float elapsed, float startup, float active,
                        float recovery, float stateTime, float walkPhase, Vector2 feet, float targetHeight,
                        bool facingRight, Color tint) {
    const char* folder = skin == 5 ? kTXM : kTX;
    const float dir = facingRight ? 1.0f : -1.0f;
    PoseContext ctx;
    ctx.state = state;
    ctx.attack = attackId;
    ctx.stateTime = stateTime;
    ctx.walkPhase = walkPhase;
    ctx.time = GetTime();
    FxClock clock;
    const MoveSet& set = VisualMoveSet(skin);
    const MoveOverride* ov = nullptr;
    if (state == PlayerState::Attack) {
        clock = AttackClock(elapsed, startup, active, recovery, dir);
        ctx.phase = clock.phase;
        ctx.v = clock.v;
        ov = FindOverride(set, attackId);
    } else {
        const MoveVisual* mv = StateVisual(set, state, 0.0f);
        clock = StateClock(stateTime, mv && mv->duration > 0 ? mv->duration : 0.35f, mv ? mv->loop : false, dir);
    }
    PoseFrame fr;
    if (!ResolvePoseFrame(skin, ctx, fr) || fr.pieceCount <= 0) return;
    PosePiece pieces[4];
    int count = std::min(fr.pieceCount, 2);
    for (int i = 0; i < count; ++i) pieces[i] = fr.pieces[i];
    if (skin == 4 && count == 2 && Is(pieces[0].pose, "punch1") && Is(pieces[1].pose, "punch2"))
        count = FullPunchPieces(pieces, 0.0f);   // el puno en su sitio
    const Texture2D idle = AssetManager::Get().GetTexture(std::string(folder) + "_idle1");
    const float scale = PoseSetScale(idle, targetHeight);
    if (scale <= 0.0f) return;
    const float bodyK = targetHeight / 112.0f;
    const FxXform xf = EvalPoseFx(fr.fx, clock);
    FxXform hop;
    if (ov) hop = EvalRootHop(ov->root, clock.u);
    const Vector2 at{feet.x + xf.dx * bodyK, feet.y + (xf.dy + hop.dy) * bodyK};
    const Color c = MultiplyColor(tint, xf.tint);
    for (int i = 0; i < count; ++i) {
        const Texture2D tex = AssetManager::Get().GetTexture(std::string(folder) + "_" + (pieces[i].pose ? pieces[i].pose : "idle1"));
        if (tex.id == 0) continue;
        bool flip = pieces[i].facesRight ? !facingRight : facingRight;
        if (xf.flip) flip = !flip;
        DrawBossPoseEx(tex, pieces[i].src, at, scale, flip, xf.angle, xf.sx, xf.sy, c, pieces[i].offset);
    }
    BodyCmd cmd;
    cmd.feet = at; cmd.scale = scale; cmd.bodyK = bodyK; cmd.dir = dir;
    cmd.fx = fr.fx; cmd.clock = clock; cmd.hasFx = HasFx(fr.fx);
    DrawBodyOverlays(cmd);
}

}  // namespace district_fury
