// Moveset de TITAN-X (skin 4) y TITAN-X MEJORADO (skin 5) — BRIEF F1.1.
//
// Todo en datos: cada movimiento declara su fila completa (replaceDef: dano base
// ANTES del multiplicador del personaje x1.20 / x1.40, S/A/R en frames, empuje,
// lanzamiento, propiedades), su arco (LimbArc, en px de cuerpo estandar de 112 px:
// el motor lo escala por bodyScale ~1.52 / ~2.10), su root motion y su visual:
// segmentos de poses sueltas (1-2 piezas con ancla en el torso, recortes y
// orientacion propia: TitanXPieces de src/game/visual/Poses_TitanX.cpp) + PoseFx
// (presets V6, claves, estelas verdes, superposiciones de efectos_azules.png).
//
// Conversion de medidas (tools/poses/medir_titanx.py): 1 px de textura =
// 0.432 px estandar en Titan-X (idle1 259 px -> 112) y 0.394 en el Mejorado
// (idle1 284 px -> 112). Los arcos se fijaron midiendo puno/hombro/pie de cada
// pose y comprobandolos con ./build/moves_sheet <dir> 4|5 (dibuja la capsula).
#include "game/moves/MoveRegistry.h"
#include "game/player/PoseFxPresets.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace district_fury {

// Definida en src/game/visual/Poses_TitanX.cpp (piezas con ancla y recorte).
int TitanXPieces(const char* folder, const char* pose, PosePiece out[2], float fwd, bool mirror);

namespace {

using namespace posefx;

constexpr float F(int frames) { return frames / 60.0f; }
constexpr int I(AttackId a) { return static_cast<int>(a); }
constexpr const char* kTX = "titanx";
constexpr const char* kTXM = "titanx_mejorado";
const Color kGreen{120, 255, 140, 255};
const Color kToxic{150, 255, 110, 255};
const Color kGhost{110, 255, 130, 255};

// Constructor de un MoveOverride (encadenable).
struct Mv {
    MoveOverride o;
    const char* folder;
    Mv(const char* f, AttackId id, const char* name, AttackId base = AttackId::None) : folder(f) {
        o.id = I(id);
        o.displayName = name;
        o.replaceDef = true;
        o.def = GetAttack(base == AttackId::None ? id : base);
        o.def.name = name;
        o.def.requiresOverride = false;
    }
    // Fila: dano, S/A/R (frames), empuje, aturdimiento, lanzamiento.
    Mv& row(int dmg, int s, int a, int r, float kb, float hs, float lch = 0) {
        AttackDef& d = o.def;
        d.damage = dmg; d.startup = F(s); d.active = F(a); d.recovery = F(r);
        d.knockback = kb; d.hitstun = hs; d.launch = lch;
        d.heavy = dmg >= 14; d.priority = dmg >= 16 ? 2 : 1;
        d.rageGain = std::max(4, dmg * 2 / 3);
        d.hitstop = 0.06f + 0.003f * dmg; d.shake = 0.05f + 0.005f * dmg;
        return *this;
    }
    Mv& arc(float px, float ph, float L0, float L1, float t0, float t1, float r, bool both = false, Ease e = Ease::OutCubic) {
        LimbArc a; a.px = px; a.ph = ph; a.L0 = L0; a.L1 = L1; a.theta0 = t0; a.theta1 = t1; a.r = r; a.bothSides = both; a.ease = e;
        AttackDef& d = o.def;
        d.arc = a; d.hasArc = true;
        const float reach = std::fabs(px) + std::max(L0, L1) + r;
        d.range = reach + 20.0f;
        d.boxWidth = std::max(L0, L1) + r;
        d.boxHeight = std::max(40.0f, 2.0f * r + 24.0f);
        d.boxForward = px + 0.5f * std::max(L0, L1);
        d.boxCenterOffsetY = ph;
        return *this;
    }
    // ESTOCADA(d, curva): root motion real sobre arranque + activo.
    // from = fraccion del arranque en que empieza a avanzar (cargas largas: el
    // cuerpo arranca tarde para que el arco toque antes de atravesar al rival).
    Mv& lunge(float dx, Ease e = Ease::OutCubic, float from = 0) {
        const float total = std::max(1e-4f, o.def.startup + o.def.active + o.def.recovery);
        o.root = RootCurve{dx, 0, o.def.startup * from / total, (o.def.startup + o.def.active) / total, e};
        o.def.root = o.root;
        return *this;
    }
    // Salto propio del golpe (hop) durante el activo.
    Mv& hop(float dz) {
        const float total = std::max(1e-4f, o.def.startup + o.def.active + o.def.recovery);
        o.root = RootCurve{o.root.dx, dz, o.def.startup / total, (o.def.startup + o.def.active) / total, Ease::Linear};
        o.def.root = o.root;
        return *this;
    }
    Mv& seg(char ph, float v0, float v1, const char* pose, float fwd = 0, bool mirror = false) {
        if (o.visual.segCount >= 4) return *this;
        PoseSegment& s = o.visual.segs[o.visual.segCount++];
        s.phase = ph; s.v0 = v0; s.v1 = v1;
        s.pieceCount = TitanXPieces(folder, pose, s.pieces, fwd, mirror);
        return *this;
    }
    Mv& fx(const PresetUse& p) { if (o.visual.fx.presetCount < 6) o.visual.fx.presets[o.visual.fx.presetCount++] = p; return *this; }
    Mv& key(char ph, float v, float sx, float sy, float ang, float dx, float dy) {
        if (o.visual.fx.keyCount >= 8) return *this;
        FxKey k; k.phase = ph; k.v = v; k.sx = sx; k.sy = sy; k.angle = ang; k.dx = dx; k.dy = dy;
        o.visual.fx.keys[o.visual.fx.keyCount++] = k;
        return *this;
    }
    Mv& ghost(int n, float interval = 0.03f, Color c = kGhost) {
        o.visual.fx.ghostCount = n; o.visual.fx.ghostInterval = interval; o.visual.fx.ghostTint = c; return *this;
    }
    Mv& over(int first, int count, float x, float y, float scale, char ph = 'A', Color tint = kGreen, float rot = 0) {
        if (o.visual.fx.overlayCount >= 2) return *this;
        OverlayFx f; f.first = first; f.count = count; f.x = x; f.y = y; f.scale = scale; f.rot = rot; f.phase = ph; f.tint = tint;
        o.visual.fx.overlay[o.visual.fx.overlayCount++] = f;
        return *this;
    }
    Mv& hits(int n, float interval) {
        o.hitCount = n; o.multiHitInterval = interval;
        o.def.hitCount = n; o.def.multiHitInterval = interval;
        return *this;
    }
    // Superarmadura de datos: de 0 al final del activo (o hasta `to` s), `n` golpes (0 = ilimitado).
    Mv& armor(int n, float to = -1) {
        o.armorFrom = 0; o.armorTo = to >= 0 ? to : o.def.startup + o.def.active; o.armorHits = n;
        o.def.armorFrom = o.armorFrom; o.def.armorTo = o.armorTo; o.def.armorHits = n;
        return *this;
    }
    Mv& area(float r) { o.areaRadius = r; return *this; }
    Mv& shot(int dmg, Vector2 origin, float scale, int count = 1, float interval = 0, float spread = 0, bool arcShot = false,
             float range = 260, float speed = 560) {
        ProjectileSpec& p = o.projectile;
        p.enabled = true; p.damage = dmg; p.origin = origin; p.scale = scale; p.tint = kToxic;
        p.count = count; p.interval = interval; p.spreadLane = spread; p.arc = arcShot; p.arcRange = range; p.speed = speed;
        o.def.spawnsProjectile = true;
        return *this;
    }
    Mv& sp(int cost) { o.def.spCost = cost; return *this; }
    Mv& with(void (*f)(AttackDef&)) { f(o.def); return *this; }
};

MoveOverride Off(AttackId id) { MoveOverride o; o.id = I(id); o.enabled = false; return o; }

// Visual de un movimiento de Fase 2 (agarres...): solo el dibujo; el combate lo
// activa F2.1 (sin replaceDef el movimiento sigue apagado).
MoveOverride Phase2Visual(const char* folder, AttackId id, const char* name, const char* s, const char* a, const char* r) {
    Mv m(folder, id, name);
    m.seg('S', 0, 1, s).seg('A', 0, 1, a).seg('R', 0, 1, r).fx(ANTICIPA(1.0f)).fx(IMPACTO(1.0f));
    m.o.replaceDef = false;
    m.o.def = AttackDef{};
    return m.o;
}

MoveVisual StateVis(const char* folder, const char* pose, float duration, bool loop) {
    MoveVisual v;
    v.segCount = 1;
    v.segs[0].phase = 'A'; v.segs[0].v0 = 0; v.segs[0].v1 = 1;
    v.segs[0].pieceCount = TitanXPieces(folder, pose, v.segs[0].pieces, 0, false);
    v.duration = duration; v.loop = loop;
    return v;
}
void StateKey(MoveVisual& v, float sx, float sy, float ang, float dx, float dy) {
    for (int i = 0; i < 2 && v.fx.keyCount < 8; ++i) {
        FxKey k; k.phase = 'A'; k.v = static_cast<float>(i); k.sx = sx; k.sy = sy; k.angle = ang; k.dx = dx; k.dy = dy;
        v.fx.keys[v.fx.keyCount++] = k;
    }
}

// =========================== TITAN-X (skin 4) ===========================
// "Prototipo mutante, lento y demoledor". Poses: idle1-4, lean, carga
// (punch1+punch2), shoot_orb (puno recto), recoil (disparo del orbe), slam,
// uppercut, death. Medidas clave (px estandar desde el torso / los pies):
// puno de shoot_orb +34 x -72; puno de uppercut +15 x -117; slam a ras de suelo
// hasta +33; hombro de lean +30 x -80; mano de recoil +30 x -73.
std::vector<MoveOverride> TitanXOverrides() {
    const char* f = kTX;
    std::vector<MoveOverride> v;
    // ---- Cadena J ----
    v.push_back(Mv(f, AttackId::Punch1, "MARTILLO").row(12, 9, 5, 12, 320, .28f)
                    .arc(0, -74, 16, 44, 0, 0, 18).lunge(32)
                    .seg('S', 0, 1, "carga").seg('A', 0, 1, "shoot_orb", 20).seg('R', 0, .5f, "shoot_orb", 20).seg('R', .5f, 1, "idle1")
                    .fx(ANTICIPA(1.4f)).fx(IMPACTO(1.3f)).fx(RECOBRO(1.0f))
                    .over(8, 3, 62, -76, 0.8f).o);
    v.push_back(Mv(f, AttackId::Punch2, "CRUZADO").row(13, 7, 5, 12, 340, .30f)
                    .arc(0, -72, 16, 44, -4, 2, 18).lunge(14)
                    .seg('S', 0, 1, "idle4").seg('A', 0, 1, "shoot_orb", 24).seg('R', 0, .55f, "shoot_orb", 24).seg('R', .55f, 1, "idle1")
                    .fx(ANTICIPA(0.8f)).fx(RECOBRO(1.0f)).key('A', .5f, 1.06f, .97f, 6, 10, 0).ghost(2)
                    .over(9, 3, 66, -74, 0.8f).o);
    v.push_back(Mv(f, AttackId::PunchHook, "UPPERCUT MUTANTE").row(16, 8, 6, 18, 80, .55f, 640)
                    .with([](AttackDef& d) { d.launcher = true; d.juggleCost = 2; })   // sin jumpCancel: J J J J sigue al sismo
                    .arc(8, -80, 24, 52, 10, 80, 18)
                    .seg('S', 0, 1, "slam").seg('A', 0, 1, "uppercut", 6).seg('R', 0, .6f, "uppercut", 6).seg('R', .6f, 1, "idle1")
                    .fx(ANTICIPA(1.0f)).fx(UPPER(14)).fx(RECOBRO(1.0f))
                    .over(21, 2, 20, -120, 0.9f).o);
    v.push_back(Mv(f, AttackId::Punch3, "GOLPE SISMICO").row(20, 12, 6, 20, 420, .45f, 200)
                    .with([](AttackDef& d) { d.knockdown = true; d.low = true; d.hitstopScale = 1.4f; d.breaksGuard = true; d.shakeScale = 1.4f; })
                    .arc(14, -14, 30, 62, -8, -8, 24).area(120)
                    .seg('S', 0, 1, "uppercut").seg('A', 0, 1, "slam", 6).seg('R', 0, .6f, "slam", 6).seg('R', .6f, 1, "idle1")
                    .fx(ANTICIPA(1.2f)).fx(SALTITO(8, 'S')).fx(TEMBLOR(4, 24, 'A')).fx(RECOBRO(0.8f))
                    .over(14, 2, 46, -8, 1.7f).over(22, 1, 30, -10, 1.6f, 'R').o);
    // ---- Cadena K ----
    v.push_back(Mv(f, AttackId::Kick, "PISOTON").row(15, 10, 5, 16, 260, .40f, 120)
                    .with([](AttackDef& d) { d.low = true; d.knockdown = true; })
                    .arc(18, -10, 20, 48, 0, 0, 20).area(90)
                    .seg('S', 0, 1, "lean").seg('A', 0, 1, "slam").seg('R', 0, .5f, "slam").seg('R', .5f, 1, "idle1")
                    .fx(ANTICIPA(1.0f)).fx(SALTITO(10, 'S')).key('A', .5f, 1.04f, .90f, 0, 0, 0).fx(TEMBLOR(3, 26, 'A'))
                    .over(23, 1, 24, -6, 1.3f).over(15, 1, 30, -4, 1.2f).o);
    v.push_back(Mv(f, AttackId::Kick2, "RODILLAZO BAJO").row(13, 8, 5, 14, 300, .34f)
                    .arc(6, -40, 20, 46, -12, 0, 18).lunge(12)
                    .seg('S', 0, 1, "idle3").seg('A', 0, 1, "lean", 12).seg('R', 0, .5f, "lean", 12).seg('R', .5f, 1, "idle1")
                    .fx(ANTICIPA(0.8f)).fx(IMPACTO(1.0f)).key('A', .5f, 1, 1, 10, 0, 0).ghost(2)
                    .over(8, 3, 50, -40, 0.7f).o);
    v.push_back(Mv(f, AttackId::Kick3, "EMBESTIDA DE HOMBRO").row(18, 10, 8, 18, 600, .42f, 160)
                    .with([](AttackDef& d) { d.wallBounce = true; d.knockdown = true; d.breaksGuard = true; })
                    .arc(-10, -78, 24, 62, -8, -8, 24).lunge(90, Ease::InOutSine, 1.0f).armor(1)
                    .seg('S', 0, 1, "lean", -10).seg('A', 0, 1, "lean", 22).seg('R', 0, .5f, "lean", 22).seg('R', .5f, 1, "idle1")
                    .fx(ANTICIPA(1.2f)).fx(IMPACTO(1.2f)).fx(RECOBRO(1.0f)).ghost(3)
                    .over(10, 3, 56, -80, 1.0f).o);
    v.push_back(Mv(f, AttackId::KickLauncher, "LANZADOR").row(14, 8, 5, 16, 80, .55f, 720)
                    .with([](AttackDef& d) { d.launcher = true; d.jumpCancel = true; d.juggleCost = 2; })
                    .arc(8, -78, 24, 50, 0, 85, 18)
                    .seg('S', 0, 1, "idle4").seg('A', 0, 1, "uppercut", 8).seg('R', 0, .6f, "uppercut", 8).seg('R', .6f, 1, "idle1")
                    .fx(ANTICIPA(0.8f)).fx(UPPER(10)).fx(RECOBRO(0.8f)).over(21, 2, 18, -110, 0.8f).o);
    // ---- Normales de comando ----
    v.push_back(Mv(f, AttackId::FwdPunch, "ARIETE").row(16, 12, 8, 16, 560, .38f)
                    .with([](AttackDef& d) { d.overhead = false; d.breaksGuard = true; })
                    .arc(10, -82, 10, 46, -6, -6, 24).lunge(120, Ease::InOutSine, 0.8f).armor(1)
                    .seg('S', 0, 1, "carga", -12).seg('A', 0, 1, "lean", 24).seg('R', 0, .5f, "lean", 24).seg('R', .5f, 1, "idle1")
                    .fx(ANTICIPA(1.3f)).fx(IMPACTO(1.1f)).ghost(4).over(42, 4, -30, -60, 1.0f).o);
    v.push_back(Mv(f, AttackId::BackElbow, "CODAZO TRASERO").row(12, 6, 4, 12, 380, .34f)
                    .with([](AttackDef& d) { d.hitsBehind = true; d.boxForward = -40.0f; })
                    .arc(-6, -74, 14, 42, 180, 180, 18)
                    .seg('S', 0, 1, "idle2").seg('A', 0, 1, "shoot_orb", 18, true).seg('R', 0, .5f, "shoot_orb", 18, true).seg('R', .5f, 1, "idle1")
                    .fx(ANTICIPA(0.6f)).key('A', .5f, 1.04f, .98f, -4, -6, 0).over(8, 3, -58, -76, 0.8f).o);
    v.push_back(Mv(f, AttackId::Uppercut, "GANCHO AL CIELO", AttackId::Uppercut).row(14, 6, 6, 18, 60, .60f, 680)
                    .arc(6, -74, 20, 52, -10, 85, 18).hop(12)
                    .seg('S', 0, 1, "slam").seg('A', 0, 1, "uppercut", 4).seg('R', 0, .6f, "uppercut", 4).seg('R', .6f, 1, "idle1")
                    .fx(ANTICIPA(0.8f)).fx(UPPER(10)).over(21, 2, 16, -118, 0.8f).o);
    v.push_back(Mv(f, AttackId::Sweep, "BARRIDO DE BRAZO").row(12, 8, 6, 16, 140, .50f)
                    .arc(10, -10, 30, 70, 0, 0, 18)
                    .seg('S', 0, 1, "idle4").seg('A', 0, 1, "slam", 14).seg('R', 0, .5f, "slam", 14).seg('R', .5f, 1, "idle1")
                    .fx(ANTICIPA(0.8f)).fx(BARRIDA('A')).over(37, 1, 50, -10, 1.0f).o);
    v.push_back(Mv(f, AttackId::SpinKick, "GIRO DE BRAZOS").row(14, 9, 8, 16, 420, .34f)
                    .with([](AttackDef& d) { d.hitsBehind = true; })
                    .arc(0, -72, 30, 42, 0, 0, 20, true)
                    .seg('S', 0, 1, "idle1").seg('A', 0, 1, "shoot_orb", 8).seg('R', 0, .5f, "shoot_orb", 8).seg('R', .5f, 1, "idle1")
                    .fx(ANTICIPA(0.6f)).fx(GIRO(1)).ghost(3).over(36, 1, 0, -74, 1.3f).o);
    v.push_back(Mv(f, AttackId::FwdKick, "PATADA PESADA").row(15, 10, 6, 16, 430, .34f)
                    .arc(4, -46, 30, 58, -16, -4, 18).lunge(40)
                    .seg('S', 0, 1, "idle3").seg('A', 0, 1, "lean", 14).seg('R', 0, .5f, "lean", 14).seg('R', .5f, 1, "idle1")
                    .fx(ANTICIPA(1.0f)).key('A', .5f, 1.05f, .97f, -12, 6, 0).fx(RECOBRO(1.0f)).over(9, 3, 56, -46, 0.8f).o);
    v.push_back(Mv(f, AttackId::Stomp, "PISOTON EN EL SUELO", AttackId::Stomp).row(12, 10, 4, 14, 0, .30f)
                    .with([](AttackDef& d) { d.otg = true; d.hasArc = false; })
                    .seg('S', 0, 1, "lean").seg('A', 0, 1, "slam", 6).seg('R', 0, .5f, "slam", 6).seg('R', .5f, 1, "idle1")
                    .fx(SALTITO(12, 'S')).key('A', .5f, 1.04f, .88f, 0, 0, 0).fx(TEMBLOR(3, 26, 'A')).over(15, 1, 30, -4, 1.1f).o);
    // ---- Dash ----
    v.push_back(Mv(f, AttackId::DashAttack, "CARGA VERDE").row(15, 6, 10, 16, 540, .32f)
                    .with([](AttackDef& d) { d.breaksGuard = true; d.cooldown = 0.35f; })
                    .arc(10, -80, 10, 44, -6, -6, 24).lunge(140, Ease::InOutSine, 0.8f)
                    .seg('S', 0, 1, "lean", 10).seg('A', 0, 1, "lean", 26).seg('R', 0, .5f, "lean", 26).seg('R', .5f, 1, "idle1")
                    .fx(IMPACTO(1.0f)).ghost(5, 0.03f).over(42, 4, -36, -60, 1.1f).o);
    v.push_back(Mv(f, AttackId::DashKick, "DESLIZAMIENTO").row(13, 4, 10, 14, 300, .40f)
                    .with([](AttackDef& d) { d.low = true; d.knockdown = true; d.cooldown = 0.35f; })
                    .arc(10, -16, 30, 66, -4, -4, 18).lunge(100, Ease::OutQuad)
                    .seg('S', 0, 1, "slam").seg('A', 0, 1, "slam", 16).seg('R', 0, .6f, "slam", 16).seg('R', .6f, 1, "idle1")
                    .key('A', .5f, 1.06f, .80f, -6, 0, 6).ghost(3).over(37, 1, -20, -8, 1.2f).o);
    // ---- Aereos (salto pesado) ----
    v.push_back(Mv(f, AttackId::AirPunch, "CAIDA SISMICA", AttackId::AirPunch).row(14, 5, 8, 8, 260, .34f, -300)
                    .with([](AttackDef& d) { d.groundBounce = true; })
                    .arc(10, -40, 30, 56, -40, -60, 20)
                    .seg('S', 0, 1, "uppercut").seg('A', 0, 1, "slam", 8).seg('R', 0, 1, "slam", 8)
                    .fx(ANTICIPA(0.8f)).key('A', .5f, 1, 1, -12, 0, 0).over(14, 1, 30, -8, 1.3f, 'R').o);
    v.push_back(Mv(f, AttackId::AirPunch2, "MARTILLO AEREO", AttackId::AirPunch2).row(12, 4, 6, 8, 200, .30f, 200)
                    .arc(4, -60, 24, 60, -30, -45, 20)
                    .seg('S', 0, 1, "carga").seg('A', 0, 1, "shoot_orb", 16).seg('R', 0, 1, "lean")
                    .fx(IMPACTO(1.0f)).key('A', .5f, 1, 1, -15, 0, 0).over(8, 3, 54, -70, 0.8f).o);
    v.push_back(Mv(f, AttackId::AirKick, "PATADA AEREA", AttackId::AirKick).row(14, 5, 12, 6, 380, .34f)
                    .with([](AttackDef& d) { d.knockdown = true; })
                    .arc(4, -46, 36, 60, -25, -25, 20)
                    .seg('S', 0, 1, "idle4").seg('A', 0, 1, "lean", 14).seg('R', 0, 1, "lean")
                    .fx(IMPACTO(0.8f)).key('A', .3f, 1, 1, 25, 0, 0).key('A', .8f, 1, 1, 25, 0, 0).ghost(2).o);
    v.push_back(Mv(f, AttackId::DiveKick, "PICADA DE TITAN", AttackId::DiveKick).row(14, 6, 60, 10, 300, .34f, -500)
                    .arc(0, -30, 40, 60, -45, -45, 20)
                    .seg('S', 0, 1, "idle4").seg('A', 0, 1, "lean", 10).seg('R', 0, 1, "slam")
                    .key('A', .1f, 1, 1, -40, 0, 0).key('A', 1, 1, 1, -40, 0, 0).ghost(4).over(14, 1, 20, -6, 1.2f, 'R').o);
    v.push_back(Mv(f, AttackId::AirSpike, "MAZO DE CAIDA", AttackId::AirSpike).row(18, 8, 5, 14, 200, .40f, -700)
                    .arc(8, -70, 40, 70, 60, -60, 20)
                    .seg('S', 0, 1, "uppercut").seg('A', 0, 1, "slam", 8).seg('R', 0, 1, "slam", 8)
                    .fx(ANTICIPA(1.0f)).key('A', .5f, 1, 1, -20, 0, 0).over(14, 2, 34, -8, 1.4f, 'R').o);
    // ---- Especiales y habilidades ----
    v.push_back(Mv(f, AttackId::EnergyWave, "ORBE TOXICO").row(20, 12, 14, 14, 500, .32f)
                    .with([](AttackDef& d) { d.breaksGuard = true; d.spCost = 20; d.spawnsProjectile = true; })
                    .arc(0, -74, 20, 50, 0, 0, 22).shot(20, {34, -74}, 1.2f)
                    .seg('S', 0, 1, "idle2").seg('A', 0, 1, "recoil", 6).seg('R', 0, .6f, "recoil", 6).seg('R', .6f, 1, "idle1")
                    .fx(LATIDO(0.05f, 4, 'S')).fx(TINTE(kToxic, 0, 'S')).fx(IMPACTO(0.8f))
                    .over(0, 4, 30, -76, 0.9f, 'S').over(10, 2, 46, -74, 0.9f).o);
    v.push_back(Mv(f, AttackId::RisingUppercut, "UPPERCUT VOLCANICO").row(13, 3, 12, 22, 200, .50f, 800)
                    .with([](AttackDef& d) { d.launcher = true; d.jumpCancel = true; d.juggleCost = 2; d.invulnTo = F(5); d.heavy = true; d.priority = 3; })
                    .hits(2, F(6)).arc(6, -80, 30, 70, -20, 95, 22).hop(70)
                    .seg('S', 0, 1, "slam").seg('A', 0, 1, "uppercut", 4).seg('R', 0, .6f, "uppercut", 4).seg('R', .6f, 1, "idle1")
                    .fx(UPPER(24)).fx(TINTE(kToxic, 0.2f, 'A')).over(21, 2, 10, -20, 1.0f).over(22, 1, 10, -10, 1.0f, 'R').o);
    v.push_back(Off(AttackId::Hurricane));   // TX no gira sobre si mismo en el aire: QCB+K cae al base
    v.push_back(Mv(f, AttackId::DrillKick, "BOLA DE DEMOLICION").row(7, 10, 14, 18, 320, .32f)
                    .with([](AttackDef& d) { d.knockdown = true; d.spCost = 15; d.priority = 3; })
                    .hits(3, F(14) / 3.0f).arc(0, -70, 30, 46, 0, 0, 22, true).lunge(160, Ease::InOutSine, 0.8f)
                    .seg('S', 0, 1, "carga", -8).seg('A', 0, 1, "lean", 18).seg('R', 0, .5f, "lean", 18).seg('R', .5f, 1, "idle1")
                    .fx(ANTICIPA(1.0f)).fx(GIRO(2)).ghost(4).over(36, 1, 0, -70, 1.4f).o);
    v.push_back(Mv(f, AttackId::Signature1, "APLASTAMIENTO DOBLE", AttackId::DrillKick).row(16, 10, 20, 20, 460, .45f, 220)
                    .with([](AttackDef& d) { d.knockdown = true; d.low = true; d.spCost = 20; d.priority = 3; d.hitstopScale = 1.3f; d.root = RootCurve{0, 0, 0, 1}; })
                    .hits(2, F(10)).arc(14, -14, 30, 66, -8, -8, 24).area(150).armor(0)
                    .seg('S', 0, 1, "uppercut").seg('A', 0, .5f, "slam", 6).seg('A', .5f, .62f, "uppercut").seg('*', 0, 1, "slam", 6)
                    .fx(ANTICIPA(1.2f)).fx(TEMBLOR(5, 22, 'A')).over(14, 2, 40, -8, 1.8f).over(22, 1, 30, -12, 2.0f, 'R').o);
    v.push_back(Mv(f, AttackId::Signature2, "LLUVIA ACIDA", AttackId::EnergyWave).row(10, 12, 18, 16, 360, .30f)
                    .with([](AttackDef& d) { d.breaksGuard = true; })
                    .sp(25).arc(0, -74, 20, 50, 0, 0, 22).shot(10, {34, -74}, 0.9f, 3, 0.06f, 18)
                    .seg('S', 0, 1, "idle2").seg('A', 0, 1, "recoil", 6).seg('R', 0, .6f, "recoil", 6).seg('R', .6f, 1, "idle1")
                    .fx(LATIDO(0.06f, 6, 'S')).fx(TINTE(kToxic, 0, 'S')).fx(IMPACTO(0.6f))
                    .over(0, 4, 30, -76, 1.0f, 'S').over(24, 2, 46, -74, 0.9f).o);
    v.push_back(Mv(f, AttackId::Signature3, "TERREMOTO", AttackId::DrillKick).row(8, 10, 36, 20, 380, .40f, 180)
                    .with([](AttackDef& d) { d.knockdown = true; d.low = true; d.spCost = 30; d.priority = 4; d.shakeScale = 1.8f; d.root = RootCurve{0, 0, 0, 1}; })
                    .hits(3, F(12)).arc(0, -14, 40, 80, -8, -8, 26).area(160).armor(0)
                    .seg('S', 0, 1, "uppercut").seg('A', .3f, .42f, "uppercut").seg('A', .64f, .76f, "uppercut").seg('*', 0, 1, "slam", 4)
                    .fx(ANTICIPA(1.3f)).fx(TEMBLOR(7, 18, 'A')).over(14, 2, 0, -8, 2.2f).over(22, 1, 0, -12, 2.4f).o);
    v.push_back(Mv(f, AttackId::RageAttack, "TRIPLE SISMO", AttackId::RageAttack).row(30, 13, 22, 20, 620, .42f, 220)
                    .with([](AttackDef& d) { d.knockdown = true; })
                    .hits(3, F(7)).area(150)
                    .seg('S', 0, 1, "uppercut").seg('A', .33f, .45f, "uppercut").seg('A', .66f, .78f, "uppercut").seg('*', 0, 1, "slam", 4)
                    .fx(ANTICIPA(1.4f)).fx(TEMBLOR(6, 20, 'A')).fx(TINTE(kToxic, 0, 'A')).over(14, 2, 0, -8, 2.0f).over(22, 1, 0, -12, 2.6f, 'R').o);
    v.push_back(Mv(f, AttackId::Finisher, "EJECUCION", AttackId::Finisher).row(44, 16, 24, 26, 760, .55f, 300)
                    .with([](AttackDef& d) { d.knockdown = true; })
                    .arc(6, -78, 30, 70, -20, 40, 30)
                    .seg('S', 0, 1, "uppercut").seg('A', 0, .5f, "recoil", 10).seg('A', .5f, 1, "recoil", 16).seg('R', 0, 1, "recoil", 16)
                    .fx(ANTICIPA(1.4f)).fx(UPPER(10)).fx(TINTE(kToxic, 0.25f, 'A')).over(11, 3, 50, -74, 1.4f).over(26, 3, 50, -74, 1.6f, 'R').o);
    // ---- Fase 2 (F2.1): solo el visual ----
    v.push_back(Phase2Visual(f, AttackId::Grab, "AGARRE", "idle4", "carga", "carga"));
    v.push_back(Phase2Visual(f, AttackId::GrabKnee, "RODILLAZO", "carga", "lean", "carga"));
    v.push_back(Phase2Visual(f, AttackId::ThrowForward, "LANZAMIENTO", "carga", "uppercut", "slam"));
    v.push_back(Phase2Visual(f, AttackId::ThrowBack, "SUPLEX MUTANTE", "carga", "uppercut", "slam"));
    v.push_back(Phase2Visual(f, AttackId::GuardCounter, "CONTRA DE TITAN", "idle1", "shoot_orb", "idle1"));
    v.push_back(Phase2Visual(f, AttackId::WakeupAttack, "BARRIDO AL LEVANTARSE", "death", "slam", "idle1"));
    return v;
}

// ======================= TITAN-X MEJORADO (skin 5) =======================
// "Forma final con canon". Poses: idle1-4 (idle2 casi de frente), cannon_aim,
// charge_orb, shoot_orb (mira a la IZQUIERDA), rage_aura, dash, slam, death.
// Medidas (px estandar): canon de cannon_aim +40 x -71; boca de charge_orb +35 x
// -59; orbe de shoot_orb +46 x -58; punos de rage_aura +30 x -64; dash +42.
std::vector<MoveOverride> TitanXMOverrides() {
    const char* f = kTXM;
    std::vector<MoveOverride> v;
    v.push_back(Mv(f, AttackId::Punch1, "CULATAZO").row(13, 8, 5, 12, 320, .28f)
                    .arc(0, -50, 16, 44, 0, 0, 24).lunge(6)
                    .seg('S', 0, 1, "idle4").seg('A', 0, 1, "cannon_aim", 16).seg('R', 0, .5f, "cannon_aim", 16).seg('R', .5f, 1, "idle1")
                    .fx(ANTICIPA(1.0f)).fx(IMPACTO(1.2f)).fx(RECOBRO(1.0f)).over(8, 3, 56, -72, 0.8f).o);
    v.push_back(Mv(f, AttackId::Punch2, "RAFAGA CORTA").row(5, 8, 12, 12, 160, .26f)
                    .hits(3, F(4)).arc(0, -46, 40, 80, 0, 0, 22)
                    .seg('S', 0, 1, "cannon_aim").seg('A', 0, 1, "charge_orb", 8).seg('R', 0, .5f, "charge_orb", 8).seg('R', .5f, 1, "idle1")
                    .fx(ANTICIPA(0.6f)).fx(TEMBLOR(2, 30, 'A')).over(8, 3, 50, -60, 0.9f).over(5, 3, 80, -60, 0.8f).o);
    v.push_back(Mv(f, AttackId::PunchHook, "EMBESTIDA").row(16, 10, 6, 16, 520, .38f)
                    .with([](AttackDef& d) { d.wallBounce = true; })
                    .arc(6, -50, 10, 46, -5, -5, 28).lunge(70, Ease::InOutSine, 0.8f)
                    .seg('S', 0, 1, "idle3").seg('A', 0, 1, "rage_aura", 16).seg('R', 0, .5f, "rage_aura", 16).seg('R', .5f, 1, "idle1")
                    .fx(ANTICIPA(1.2f)).fx(IMPACTO(1.0f)).ghost(3).over(10, 3, 50, -64, 1.0f).o);
    v.push_back(Mv(f, AttackId::Punch3, "CANONAZO A QUEMARROPA").row(22, 12, 6, 20, 560, .45f, 300)
                    .with([](AttackDef& d) { d.knockdown = true; d.breaksGuard = true; d.hitstopScale = 1.3f; })
                    .arc(0, -46, 24, 60, 0, 0, 30)
                    .seg('S', 0, 1, "charge_orb").seg('A', 0, 1, "shoot_orb", 6).seg('R', 0, .6f, "shoot_orb", 6).seg('R', .6f, 1, "idle1")
                    .fx(LATIDO(0.04f, 6, 'S')).fx(IMPACTO(1.0f)).fx(RECOBRO(1.2f))
                    .over(0, 4, 46, -60, 1.0f, 'S').over(11, 3, 60, -60, 1.5f).o);
    v.push_back(Mv(f, AttackId::Kick, "ONDA DE CHOQUE").row(15, 10, 5, 16, 300, .40f, 120)
                    .with([](AttackDef& d) { d.low = true; d.knockdown = true; })
                    .arc(16, -10, 24, 56, 0, 0, 20).area(110)
                    .seg('S', 0, 1, "idle4").seg('A', 0, 1, "slam").seg('R', 0, .5f, "slam").seg('R', .5f, 1, "idle1")
                    .fx(ANTICIPA(1.0f)).fx(SALTITO(8, 'S')).fx(TEMBLOR(3, 24, 'A')).over(23, 1, 10, -6, 1.8f).over(14, 1, 30, -6, 1.6f).o);
    v.push_back(Mv(f, AttackId::Kick2, "PISOTON DOBLE").row(11, 10, 14, 18, 340, .42f, 160)
                    .with([](AttackDef& d) { d.low = true; d.knockdown = true; })
                    .hits(2, F(9)).arc(16, -10, 24, 56, 0, 0, 20).area(110)
                    .seg('S', 0, 1, "idle3").seg('A', 0, .45f, "slam").seg('A', .45f, .6f, "idle4").seg('*', 0, 1, "slam")
                    .fx(ANTICIPA(0.8f)).fx(TEMBLOR(4, 22, 'A')).over(14, 2, 20, -6, 1.6f).over(23, 1, 10, -6, 2.0f, 'R').o);
    v.push_back(Off(AttackId::Kick3));   // cadena K de 2 eslabones (K, K K): el 3er K no existe
    v.push_back(Mv(f, AttackId::KickLauncher, "LANZADOR").row(14, 8, 5, 16, 80, .55f, 700)
                    .with([](AttackDef& d) { d.launcher = true; d.jumpCancel = true; d.juggleCost = 2; })
                    .arc(6, -50, 20, 48, -10, 80, 26).hop(10)
                    .seg('S', 0, 1, "idle4").seg('A', 0, 1, "rage_aura", 8).seg('R', 0, .6f, "rage_aura", 8).seg('R', .6f, 1, "idle1")
                    .fx(ANTICIPA(0.8f)).key('A', .5f, 1, 1, -15, 0, 0).over(21, 2, 20, -100, 0.9f).o);
    v.push_back(Mv(f, AttackId::FwdPunch, "CARGA DE TANQUE").row(18, 12, 10, 18, 600, .40f)
                    .with([](AttackDef& d) { d.overhead = false; d.breaksGuard = true; })
                    .arc(10, -54, 10, 48, -6, -6, 32).lunge(160, Ease::InOutSine, 0.8f).armor(2)
                    .seg('S', 0, 1, "idle3", -8).seg('A', 0, 1, "dash", 16).seg('R', 0, .5f, "dash", 16).seg('R', .5f, 1, "idle1")
                    .fx(ANTICIPA(1.3f)).fx(IMPACTO(0.8f)).ghost(4).over(42, 4, -40, -60, 1.3f).o);
    v.push_back(Mv(f, AttackId::BackElbow, "CODAZO").row(12, 6, 4, 12, 380, .34f)
                    .with([](AttackDef& d) { d.hitsBehind = true; d.boxForward = -40.0f; })
                    .arc(-6, -50, 14, 42, 180, 180, 24)
                    .seg('S', 0, 1, "idle1").seg('A', 0, 1, "cannon_aim", 14, true).seg('R', 0, .5f, "cannon_aim", 14, true).seg('R', .5f, 1, "idle1")
                    .fx(ANTICIPA(0.6f)).key('A', .5f, 1.04f, .98f, -4, -6, 0).over(8, 3, -56, -72, 0.8f).o);
    v.push_back(Mv(f, AttackId::Uppercut, "GANCHO", AttackId::Uppercut).row(14, 6, 6, 18, 60, .60f, 680)
                    .arc(6, -50, 20, 50, -10, 85, 26).hop(16)
                    .seg('S', 0, 1, "slam").seg('A', 0, 1, "rage_aura", 6).seg('R', 0, .6f, "rage_aura", 6).seg('R', .6f, 1, "idle1")
                    .fx(ANTICIPA(0.8f)).key('A', .5f, 1, 1, -20, 0, 0).over(21, 2, 18, -104, 0.9f).o);
    v.push_back(Mv(f, AttackId::Sweep, "BARRIDO").row(12, 8, 6, 16, 140, .50f)
                    .arc(10, -10, 30, 70, 0, 0, 18)
                    .seg('S', 0, 1, "idle4").seg('A', 0, 1, "slam", 12).seg('R', 0, .5f, "slam", 12).seg('R', .5f, 1, "idle1")
                    .fx(ANTICIPA(0.8f)).fx(BARRIDA('A')).over(37, 1, 50, -10, 1.1f).o);
    v.push_back(Mv(f, AttackId::SpinKick, "GIRO").row(14, 9, 8, 16, 420, .34f)
                    .with([](AttackDef& d) { d.hitsBehind = true; })
                    .arc(0, -46, 30, 42, 0, 0, 26, true)
                    .seg('S', 0, 1, "idle1").seg('A', 0, 1, "charge_orb", 6).seg('R', 0, .5f, "charge_orb", 6).seg('R', .5f, 1, "idle1")
                    .fx(ANTICIPA(0.6f)).fx(GIRO(1)).ghost(3).over(36, 1, 0, -64, 1.4f).o);
    v.push_back(Mv(f, AttackId::FwdKick, "PATADA DE TANQUE").row(15, 10, 6, 16, 430, .34f)
                    .arc(4, -44, 30, 58, -16, -4, 20).lunge(50)
                    .seg('S', 0, 1, "idle3").seg('A', 0, 1, "dash", 10).seg('R', 0, .5f, "dash", 10).seg('R', .5f, 1, "idle1")
                    .fx(ANTICIPA(1.0f)).key('A', .5f, 1.04f, .97f, -10, 6, 0).fx(RECOBRO(1.0f)).over(9, 3, 56, -44, 0.9f).o);
    v.push_back(Mv(f, AttackId::Stomp, "PISOTON", AttackId::Stomp).row(12, 10, 4, 14, 0, .30f)
                    .with([](AttackDef& d) { d.otg = true; d.hasArc = false; })
                    .seg('S', 0, 1, "idle4").seg('A', 0, 1, "slam", 6).seg('R', 0, .5f, "slam", 6).seg('R', .5f, 1, "idle1")
                    .fx(SALTITO(10, 'S')).key('A', .5f, 1.04f, .9f, 0, 0, 0).fx(TEMBLOR(3, 26, 'A')).over(15, 1, 30, -4, 1.2f).o);
    v.push_back(Mv(f, AttackId::DashAttack, "ARROLLAR").row(16, 6, 10, 16, 560, .34f)
                    .with([](AttackDef& d) { d.breaksGuard = true; d.cooldown = 0.35f; })
                    .arc(10, -52, 10, 46, -6, -6, 32).lunge(140, Ease::InOutSine, 0.8f)
                    .seg('S', 0, 1, "dash", 6).seg('A', 0, 1, "dash", 20).seg('R', 0, .5f, "dash", 20).seg('R', .5f, 1, "idle1")
                    .fx(IMPACTO(1.0f)).ghost(5).over(42, 4, -40, -60, 1.3f).o);
    v.push_back(Mv(f, AttackId::DashKick, "DERRAPE").row(13, 4, 10, 14, 300, .40f)
                    .with([](AttackDef& d) { d.low = true; d.knockdown = true; d.cooldown = 0.35f; })
                    .arc(10, -16, 30, 66, -4, -4, 18).lunge(100, Ease::OutQuad)
                    .seg('S', 0, 1, "slam").seg('A', 0, 1, "slam", 14).seg('R', 0, .6f, "slam", 14).seg('R', .6f, 1, "idle1")
                    .key('A', .5f, 1.06f, .80f, -6, 0, 6).ghost(3).over(37, 1, -20, -8, 1.3f).o);
    // ---- Especiales ----
    // Canonazo cargado: el motor no tiene "mantener L" (PEDIDO AL MOTOR); se hace
    // con la carga fija mas larga (orbe que crece en la boca) y el dano medio.
    v.push_back(Mv(f, AttackId::EnergyWave, "CANONAZO CARGADO").row(26, 18, 10, 16, 560, .36f)
                    .with([](AttackDef& d) { d.breaksGuard = true; d.spCost = 20; d.spawnsProjectile = true; d.knockdown = true; })
                    .arc(0, -46, 24, 60, 0, 0, 30).shot(26, {46, -58}, 1.8f)
                    .seg('S', 0, 1, "charge_orb").seg('A', 0, 1, "shoot_orb", 6).seg('R', 0, .6f, "shoot_orb", 6).seg('R', .6f, 1, "idle1")
                    .fx(LATIDO(0.03f, 5, 'S')).fx(TINTE(kToxic, 0, 'S')).fx(IMPACTO(0.8f))
                    .over(0, 4, 42, -60, 1.4f, 'S').over(11, 3, 56, -58, 1.4f).o);
    v.push_back(Mv(f, AttackId::Signature2, "RAFAGA CONTINUA", AttackId::EnergyWave).row(5, 10, 30, 18, 200, .26f)
                    .with([](AttackDef& d) { d.breaksGuard = false; })
                    .sp(25).hits(6, F(5)).arc(0, -46, 40, 90, 0, 0, 22).shot(5, {42, -60}, 0.7f, 6, 0.08f, 0, false, 260, 760)
                    .seg('S', 0, 1, "cannon_aim").seg('A', 0, 1, "charge_orb", 4).seg('R', 0, .5f, "charge_orb", 4).seg('R', .5f, 1, "idle1")
                    .fx(TEMBLOR(2, 30, 'A')).fx(LATIDO(0.02f, 12, 'A')).over(8, 2, 48, -60, 0.9f).over(5, 3, 80, -60, 0.8f).o);
    v.push_back(Mv(f, AttackId::Signature1, "MORTERO", AttackId::EnergyWave).row(20, 14, 8, 18, 400, .40f, 200)
                    .with([](AttackDef& d) { d.knockdown = true; d.spCost = 20; })
                    .arc(0, -46, 24, 56, 20, 40, 28).shot(20, {40, -80}, 1.3f, 1, 0, 0, true, 260, 420)
                    .seg('S', 0, 1, "charge_orb").seg('A', 0, 1, "shoot_orb", 4).seg('R', 0, .6f, "shoot_orb", 4).seg('R', .6f, 1, "idle1")
                    .fx(ANTICIPA(0.8f)).key('A', .5f, 1, 1, -30, 0, -4).over(0, 4, 40, -60, 1.0f, 'S').over(12, 2, 46, -66, 1.0f).o);
    v.push_back(Mv(f, AttackId::Signature3, "AURA MUTANTE", AttackId::RageAttack).row(6, 8, 40, 16, 300, .32f, 120)
                    .with([](AttackDef& d) { d.spCost = 30; d.priority = 4; d.knockdown = false; })
                    .hits(4, F(10)).area(130).armor(0)
                    .seg('S', 0, 1, "idle1").seg('A', 0, 1, "rage_aura").seg('R', 0, .5f, "rage_aura").seg('R', .5f, 1, "idle1")
                    .fx(LATIDO(0.05f, 5, '*')).fx(TINTE(kToxic, 0, 'A')).over(21, 2, 0, -20, 2.4f).over(23, 1, 0, -6, 2.6f).o);
    v.push_back(Mv(f, AttackId::RisingUppercut, "UPPERCUT DE CANON").row(13, 3, 12, 22, 200, .50f, 780)
                    .with([](AttackDef& d) { d.launcher = true; d.jumpCancel = true; d.juggleCost = 2; d.invulnTo = F(5); d.heavy = true; d.priority = 3; })
                    .hits(2, F(6)).arc(6, -50, 30, 66, -20, 95, 30).hop(60)
                    .seg('S', 0, 1, "slam").seg('A', 0, 1, "rage_aura", 4).seg('R', 0, .6f, "rage_aura", 4).seg('R', .6f, 1, "idle1")
                    .fx(UPPER(20)).fx(TINTE(kToxic, 0.2f, 'A')).over(21, 2, 10, -20, 1.0f).over(22, 1, 10, -10, 1.0f, 'R').o);
    v.push_back(Off(AttackId::Hurricane));   // forma final: ni giro aereo...
    v.push_back(Off(AttackId::DrillKick));   // ...ni taladro: QCB+K / QCF+K caen al base
    v.push_back(Mv(f, AttackId::RageAttack, "BOMBARDEO", AttackId::RageAttack).row(30, 13, 22, 20, 620, .42f, 220)
                    .with([](AttackDef& d) { d.knockdown = true; })
                    .hits(2, F(10)).area(150)
                    .seg('S', 0, 1, "charge_orb").seg('A', 0, .3f, "rage_aura").seg('A', .3f, 1, "slam").seg('*', 0, 1, "slam")
                    .fx(ANTICIPA(1.3f)).fx(TEMBLOR(6, 20, 'A')).over(14, 2, 0, -8, 2.2f).over(24, 2, 0, -180, 1.6f).o);
    v.push_back(Mv(f, AttackId::Finisher, "JUICIO FINAL", AttackId::Finisher).row(44, 16, 24, 26, 760, .55f, 300)
                    .with([](AttackDef& d) { d.knockdown = true; })
                    .arc(6, -46, 30, 70, -10, 20, 36)
                    .seg('S', 0, 1, "slam").seg('A', 0, .45f, "slam", 6).seg('A', .45f, 1, "shoot_orb", 8).seg('R', 0, 1, "shoot_orb", 8)
                    .fx(ANTICIPA(1.4f)).fx(TEMBLOR(4, 22, 'A')).over(11, 3, 56, -60, 1.6f).over(26, 3, 60, -60, 1.8f, 'R').o);
    v.push_back(Phase2Visual(f, AttackId::Grab, "AGARRE", "idle4", "cannon_aim", "cannon_aim"));
    v.push_back(Phase2Visual(f, AttackId::GrabKnee, "RODILLAZO", "cannon_aim", "rage_aura", "cannon_aim"));
    v.push_back(Phase2Visual(f, AttackId::ThrowForward, "LANZAMIENTO", "cannon_aim", "rage_aura", "slam"));
    v.push_back(Phase2Visual(f, AttackId::ThrowBack, "SUPLEX DE CANON", "cannon_aim", "rage_aura", "slam"));
    v.push_back(Phase2Visual(f, AttackId::GuardCounter, "CONTRA DE CANON", "idle1", "cannon_aim", "idle1"));
    v.push_back(Phase2Visual(f, AttackId::WakeupAttack, "BARRIDO AL LEVANTARSE", "death", "slam", "idle1"));
    return v;
}

MoveSet BuildTitanX() {
    static const std::vector<MoveOverride> kOv = TitanXOverrides();
    MoveSet m;
    m.name = "titan-x";
    m.caps = CapAll;   // pesado: CapHeavyJump (salto de 105 px)
    m.jumpVz = 560;
    m.walkSpeedScale = 1.0f;
    m.chainJ[0] = I(AttackId::Punch1); m.chainJ[1] = I(AttackId::Punch2);
    m.chainJ[2] = I(AttackId::PunchHook); m.chainJ[3] = I(AttackId::Punch3);
    m.chainJCount = 4;
    m.chainK[0] = I(AttackId::Kick); m.chainK[1] = I(AttackId::Kick2); m.chainK[2] = I(AttackId::Kick3);
    m.chainKCount = 3;
    m.skillSlots[0] = I(AttackId::EnergyWave);
    m.skillSlots[1] = I(AttackId::Signature1);
    m.skillSlots[2] = I(AttackId::RisingUppercut);
    m.skillSlots[3] = I(AttackId::DrillKick);
    m.skillSlots[4] = I(AttackId::Signature3);
    m.overrides = kOv.data(); m.overrideCount = static_cast<int>(kOv.size());
    // Estados con MoveVisual propio (el resto -- caminar al ritmo de walkPhase,
    // golpe, derribo, bloqueo, dash, victoria -- los arma la tabla de Poses_TitanX.cpp).
    m.jumpRise = StateVis(kTX, "lean", 0.30f, false);
    StateKey(m.jumpRise, 1.0f, 1.03f, -10, 0, 0);
    m.jumpFall = StateVis(kTX, "uppercut", 0.30f, false);
    StateKey(m.jumpFall, 1.0f, 0.98f, 4, 0, 0);
    return m;
}

MoveSet BuildTitanXM() {
    static const std::vector<MoveOverride> kOv = TitanXMOverrides();
    MoveSet m;
    m.name = "titan-x mejorado";
    m.caps = CapAll & ~(CapJump | CapAir);   // forma final: no salta
    m.jumpVz = 560;
    m.chainJ[0] = I(AttackId::Punch1); m.chainJ[1] = I(AttackId::Punch2);
    m.chainJ[2] = I(AttackId::PunchHook); m.chainJ[3] = I(AttackId::Punch3);
    m.chainJCount = 4;
    m.chainK[0] = I(AttackId::Kick); m.chainK[1] = I(AttackId::Kick2);
    m.chainKCount = 2;
    m.skillSlots[0] = I(AttackId::EnergyWave);
    m.skillSlots[1] = I(AttackId::Signature1);
    m.skillSlots[2] = I(AttackId::Signature2);
    m.skillSlots[3] = I(AttackId::RisingUppercut);
    m.skillSlots[4] = I(AttackId::Signature3);
    m.overrides = kOv.data(); m.overrideCount = static_cast<int>(kOv.size());
    return m;
}

}  // namespace

const MoveSet* MovesetFor_TitanX(int skin) {
    if (skin == 4) { static const MoveSet kSet = BuildTitanX(); return &kSet; }
    if (skin == 5) { static const MoveSet kSet = BuildTitanXM(); return &kSet; }
    return nullptr;
}

}  // namespace district_fury
