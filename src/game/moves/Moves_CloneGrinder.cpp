// Moveset de CLON ANTIGUO (skin 1) y GRINDER (skin 3). F1.2.
//
// Los dos se dibujan con poses sueltas (assets/bosses/rayder_clone: 11 PNG;
// assets/bosses/grinder: 7 PNG). Cada movimiento tiene su visual propio: 1-4
// segmentos de pose por fase (S arranque, A activo, R recuperacion) mas PoseFx
// (presets V6, claves, estelas y superposiciones de efectos_azules.png). Las
// piezas salen ya recortadas (sin los restos de recorte) y ancladas en el torso
// por CloneGrinderPieces (src/game/visual/Poses_CloneGrinder.cpp).
//
// Arcos (LimbArc, px de cuerpo estandar de 112 px; el motor escala): el arte del
// clon tiene los miembros cortos y pegados al cuerpo (punch.png: el puno llega a
// 21 px del torso; kick.png: el pie a 24 px), asi que los arcos cubren el
// miembro DIBUJADO con su PoseFx (IMPACTO/claves lo adelantan 10-16 px) y el
// alcance se gana con root motion (ESTOCADA) en vez de con cajas fantasma.
// Medidas: tools/poses/medir_clon_grinder.py.
#include "game/moves/MoveRegistry.h"
#include "game/player/PoseFxPresets.h"
#include <vector>

namespace district_fury {

int CloneGrinderPieces(const char* folder, const char* pose, bool mirrored, PosePiece out[2]);

namespace {

using namespace posefx;

constexpr Color kMorado{190, 110, 255, 255};     // estelas / aura del clon
constexpr Color kMoradoFx{170, 90, 255, 255};    // superposiciones aditivas
constexpr Color kMoradoProj{185, 95, 255, 255};  // proyectil
constexpr Color kChispa{255, 170, 70, 255};      // chispas de la sierra
constexpr Color kRojo{255, 80, 50, 255};         // estela de la embestida turbo
constexpr Color kAscua{255, 120, 40, 255};       // aura de sobrecarga

int Id(AttackId a) { return static_cast<int>(a); }

// ---- constructor de visuales ----
struct Vis {
    const char* folder;
    MoveVisual mv;
    explicit Vis(const char* f) : folder(f) {}
    Vis& seg(char ph, float v0, float v1, const char* pose, bool mirror = false) {
        if (mv.segCount >= 4) return *this;
        PoseSegment& s = mv.segs[mv.segCount++];
        s.phase = ph; s.v0 = v0; s.v1 = v1;
        s.pieceCount = CloneGrinderPieces(folder, pose, mirror, s.pieces);
        return *this;
    }
    // Atajo: S = `s`, A = `a`, R = `a` hasta rv y luego `r`.
    Vis& sar(const char* s, const char* a, const char* r, float rv = 0.5f, bool mirror = false) {
        seg('S', 0, 1, s, mirror).seg('A', 0, 1, a, mirror);
        if (r && rv < 1.0f) seg('R', 0, rv, a, mirror).seg('R', rv, 1, r, mirror);
        else seg('R', 0, 1, a, mirror);
        return *this;
    }
    Vis& fx(PresetUse p) { if (mv.fx.presetCount < 6) mv.fx.presets[mv.fx.presetCount++] = p; return *this; }
    Vis& key(char ph, float v, float ang, float dx = 0, float dy = 0, float sx = 1, float sy = 1) {
        if (mv.fx.keyCount < 8) mv.fx.keys[mv.fx.keyCount++] = FxKey{ph, v, sx, sy, ang, dx, dy};
        return *this;
    }
    Vis& ghosts(int n, Color c, float interval = 0.03f) {
        mv.fx.ghostCount = n; mv.fx.ghostTint = c; mv.fx.ghostInterval = interval; return *this;
    }
    Vis& over(int first, int count, float x, float y, float scale, char ph, Color tint, float rot = 0) {
        if (mv.fx.overlayCount >= 2) return *this;
        OverlayFx& o = mv.fx.overlay[mv.fx.overlayCount++];
        o.first = first; o.count = count; o.x = x; o.y = y; o.scale = scale; o.phase = ph; o.tint = tint; o.rot = rot;
        return *this;
    }
    Vis& state(float duration, bool loop) { mv.duration = duration; mv.loop = loop; return *this; }
};

// Golpe normal: anticipa, impacto y recobro con la fuerza k.
Vis& Golpe(Vis& v, float k) { return v.fx(ANTICIPA(0.6f * k)).fx(IMPACTO(k)).fx(RECOBRO(0.8f * k)); }

MoveOverride Ov(AttackId id, const char* name, const Vis& v) {
    MoveOverride o;
    o.id = Id(id);
    o.displayName = name;
    o.visual = v.mv;
    return o;
}
MoveOverride& Arc(MoveOverride& o, LimbArc a) { o.hasArc = true; o.arc = a; return o; }
LimbArc LA(float px, float ph, float l0, float l1, float t0, float t1, float r, bool bothSides = false,
           bool behind = false) {
    LimbArc a; a.px = px; a.ph = ph; a.L0 = l0; a.L1 = l1; a.theta0 = t0; a.theta1 = t1; a.r = r;
    a.bothSides = bothSides; a.behind = behind;
    return a;
}
// ESTOCADA(d): root motion real sobre arranque + activo de `d` (definicion efectiva).
RootCurve Estocada(float dx, const AttackDef& d, Ease e = Ease::OutCubic) {
    RootCurve r;
    r.dx = dx;
    const float total = std::max(1e-4f, d.startup + d.active + d.recovery);
    r.t0 = 0; r.t1 = (d.startup + d.active) / total; r.ease = e;
    return r;
}
// Salto propio del golpe (hop real, como el gancho ascendente generico).
RootCurve Hop(float dz, const AttackDef& d) {
    RootCurve r;
    const float total = std::max(1e-4f, d.startup + d.active + d.recovery);
    r.dz = dz; r.t0 = d.startup / total; r.t1 = (d.startup + d.active) / total; r.ease = Ease::Linear;
    return r;
}
// Fila completa (replaceDef) a partir de la generica `base`, con S/A/R en frames.
AttackDef Row(AttackId base, const char* name, int dmg, int s, int a, int r) {
    AttackDef d = GetAttack(base);
    d.name = name; d.damage = dmg;
    d.startup = s / 60.0f; d.active = a / 60.0f; d.recovery = r / 60.0f;
    d.requiresOverride = false;
    return d;
}
MoveOverride& Replace(MoveOverride& o, const AttackDef& d) { o.replaceDef = true; o.def = d; return o; }
// Golpes multiples: n golpes repartidos en el activo.
MoveOverride& Multi(MoveOverride& o, int n, float interval) { o.hitCount = n; o.multiHitInterval = interval; return o; }

// =====================================================================
// CLON ANTIGUO: rival espejo de Rayden, energia morada. Sin sierra ni peso:
// cadenas largas, aereos y proyectiles de sombra.
// =====================================================================
std::vector<MoveOverride> CloneOverrides() {
    const char* F = "rayder_clone";
    std::vector<MoveOverride> v;
    auto add = [&](MoveOverride o) -> MoveOverride& { v.push_back(o); return v.back(); };
    const AttackDef& g = GetAttack(AttackId::Punch1);
    (void)g;

    // ---- cadena J: Jab / Cruzado / Gancho / Puno volador ----
    { Vis x(F); x.sar("ready", "punch", "ready", 0.55f); Golpe(x, 1.4f);
      MoveOverride& o = add(Ov(AttackId::Punch1, "JAB", x));
      Arc(o, LA(4, -76, 18, 40, -4, 0, 14)); o.root = Estocada(14, GetAttack(AttackId::Punch1)); }
    { Vis x(F); x.sar("ready", "punch", "ready", 0.5f).fx(ANTICIPA(0.8f)).fx(RECOBRO(1))
          .key('A', 0.5f, 4, 10, 0, 1.10f, 0.96f).ghosts(2, kMorado);
      MoveOverride& o = add(Ov(AttackId::Punch2, "CRUZADO", x));
      Arc(o, LA(4, -78, 20, 44, 0, 2, 15)); o.root = Estocada(20, GetAttack(AttackId::Punch2)); }
    { Vis x(F); x.sar("ready", "punch", "ready", 0.5f).fx(ANTICIPA(0.8f)).fx(SALTITO(6)).fx(RECOBRO(1))
          .key('A', 0.5f, -12, 10, 0, 1.08f, 1.0f).over(36, 1, 30, -84, 0.45f, 'A', kMoradoFx, -20);
      MoveOverride& o = add(Ov(AttackId::PunchHook, "GANCHO", x));
      Arc(o, LA(2, -84, 24, 40, -40, 25, 16)); o.root = Estocada(16, GetAttack(AttackId::PunchHook)); }
    { Vis x(F); x.seg('S', 0, 1, "ready").seg('A', 0, 1, "dash").seg('R', 0, 0.45f, "dash").seg('R', 0.45f, 1, "ready")
          .fx(ANTICIPA(1.2f)).fx(IMPACTO(1.0f)).fx(RECOBRO(1)).ghosts(4, kMorado, 0.025f)
          .over(10, 3, 48, -66, 0.55f, 'A', kMoradoFx);
      MoveOverride& o = add(Ov(AttackId::Punch3, "PUNO VOLADOR", x));
      Arc(o, LA(6, -66, 28, 50, 0, 0, 20)); o.root = Estocada(90, GetAttack(AttackId::Punch3), Ease::InQuad); }

    // ---- cadena K: Patada / Patada giratoria / Patada alta ----
    { Vis x(F); x.sar("ready", "kick", "ready", 0.5f); Golpe(x, 1.2f);
      MoveOverride& o = add(Ov(AttackId::Kick, "PATADA", x));
      Arc(o, LA(0, -66, 20, 40, -8, 0, 16)); o.root = Estocada(20, GetAttack(AttackId::Kick)); }
    { Vis x(F); x.sar("ready", "kick", "ready", 0.5f).fx(ANTICIPA(0.8f)).fx(GIRO(1)).fx(RECOBRO(1))
          .key('A', 1.0f, 0, 12).ghosts(3, kMorado);
      MoveOverride& o = add(Ov(AttackId::Kick2, "PATADA GIRATORIA", x));
      Arc(o, LA(0, -64, 24, 44, -30, 10, 18)); o.root = Estocada(24, GetAttack(AttackId::Kick2)); }
    { Vis x(F); x.sar("ready", "kick", "ready", 0.55f).fx(ANTICIPA(1)).fx(SALTITO(20)).fx(RECOBRO(1))
          .key('A', 0.4f, -25, 16, 0, 1.06f, 1.0f).ghosts(3, kMorado).over(9, 3, 36, -96, 0.5f, 'A', kMoradoFx);
      MoveOverride& o = add(Ov(AttackId::Kick3, "PATADA ALTA", x));
      Arc(o, LA(0, -78, 22, 40, 10, 45, 20)); o.root = Estocada(18, GetAttack(AttackId::Kick3)); }

    // ---- lanzadores ----
    { Vis x(F); x.sar("ready", "kick", "ready", 0.6f).fx(ANTICIPA(1)).fx(RECOBRO(1))
          .key('A', 0.5f, -35, 10, 0, 1.04f, 1.04f).ghosts(3, kMorado).over(28, 3, 24, -40, 0.5f, 'A', kMoradoFx);
      MoveOverride& o = add(Ov(AttackId::KickLauncher, "LANZADOR", x));
      Arc(o, LA(0, -64, 20, 40, -20, 70, 18)); o.root = Hop(12, GetAttack(AttackId::KickLauncher)); }
    { Vis x(F); x.sar("ready", "punch", "ready", 0.6f).fx(ANTICIPA(1)).fx(UPPER(10)).fx(RECOBRO(1))
          .key('A', 0.5f, -8, 12, 0, 1.06f, 1.04f).ghosts(2, kMorado);
      MoveOverride& o = add(Ov(AttackId::Uppercut, "GANCHO ASCENDENTE", x));
      Arc(o, LA(4, -70, 20, 42, -20, 80, 18)); }

    // ---- normales de comando ----
    { Vis x(F); x.seg('S', 0, 1, "ready").seg('A', 0, 1, "dash").seg('R', 0, 0.4f, "dash").seg('R', 0.4f, 1, "ready")
          .fx(ANTICIPA(1.2f)).fx(IMPACTO(1.2f)).fx(RECOBRO(1)).ghosts(3, kMorado);
      MoveOverride& o = add(Ov(AttackId::FwdPunch, "EMBESTIDA", x));
      Arc(o, LA(6, -66, 28, 50, 0, 0, 20)); }
    { Vis x(F); x.sar("ready", "kick", "ready", 0.5f); Golpe(x, 1.3f); x.ghosts(2, kMorado);
      MoveOverride& o = add(Ov(AttackId::FwdKick, "PATADA DE AVANCE", x));
      Arc(o, LA(0, -66, 22, 42, -8, 0, 17)); }
    { Vis x(F); x.sar("ready", "punch", "ready", 0.5f, true).fx(ANTICIPA(0.8f)).fx(RECOBRO(0.8f))
          .key('A', 0.5f, 6, 8, 0, 1.08f, 0.97f);   // espejado: dx/angulo hacia la espalda
      MoveOverride& o = add(Ov(AttackId::BackElbow, "CODAZO", x));
      Arc(o, LA(-4, -76, 18, 38, 180, 180, 15)); }
    { Vis x(F); x.sar("ready", "kick", "ready", 0.5f).fx(ANTICIPA(0.8f)).fx(GIRO(1)).fx(RECOBRO(1)).ghosts(3, kMorado);
      MoveOverride& o = add(Ov(AttackId::SpinKick, "GIRATORIA", x));
      Arc(o, LA(0, -64, 22, 42, -10, 10, 18, true)); }
    { Vis x(F); x.sar("ready", "kick", "ready", 0.6f).fx(ANTICIPA(0.6f)).fx(BARRIDA('A')).fx(BARRIDA('R'))
          .over(37, 1, 30, -10, 0.5f, 'A', WHITE);
      MoveOverride& o = add(Ov(AttackId::Sweep, "BARRIDA", x));
      Arc(o, LA(0, -24, 18, 42, -4, -4, 16)); o.root = Estocada(16, GetAttack(AttackId::Sweep)); }
    { Vis x(F); x.sar("ready", "kick", "ready", 0.5f).fx(ANTICIPA(1)).fx(RECOBRO(1))
          .key('A', 0.3f, 20, 6, 8, 1.0f, 0.94f).over(14, 1, 26, -6, 0.45f, 'A', kMoradoFx);
      add(Ov(AttackId::Stomp, "PISOTON", x)); }   // caja generica a ras de suelo (sin arco)

    // ---- dash ----
    { Vis x(F); x.seg('S', 0, 1, "dash").seg('A', 0, 1, "dash").seg('R', 0, 0.5f, "dash").seg('R', 0.5f, 1, "ready")
          .fx(IMPACTO(1.6f)).fx(RECOBRO(1)).ghosts(4, kMorado, 0.025f);
      MoveOverride& o = add(Ov(AttackId::DashAttack, "HOMBRO OSCURO", x));
      Arc(o, LA(6, -66, 28, 50, -4, 0, 22)); }
    { Vis x(F); x.sar("dash", "kick", "ready", 0.5f).fx(BARRIDA('A')).fx(RECOBRO(1)).ghosts(4, kMorado, 0.025f)
          .over(37, 1, 20, -10, 0.6f, 'A', WHITE);
      MoveOverride& o = add(Ov(AttackId::DashKick, "DESLIZAMIENTO", x));
      Arc(o, LA(0, -26, 20, 44, -4, -4, 16)); }

    // ---- aereos (angulo + = inclinado hacia delante/abajo, rotando en los
    // pies). El cuerpo girado adelanta el miembro: el arco sigue al miembro
    // girado (pivote en el hombro girado) y cada aereo se lanza hacia delante
    // (root motion en el aire) para que el golpe llegue con los miembros cortos.
    { Vis x(F); x.sar("ready", "punch", "punch", 1.0f).key('S', 1, 30).key('A', 0.5f, 30, 6).key('R', 0.6f, 25);
      MoveOverride& o = add(Ov(AttackId::AirPunch, "PUNO AEREO", x));
      Arc(o, LA(30, -66, 14, 34, -20, -30, 15)); o.root = Estocada(36, GetAttack(AttackId::AirPunch)); }
    { Vis x(F); x.sar("ready", "punch", "punch", 1.0f).key('S', 1, 36).key('A', 0.5f, 36, 8).key('R', 0.6f, 30)
          .ghosts(2, kMorado);
      MoveOverride& o = add(Ov(AttackId::AirPunch2, "PUNO AEREO 2", x));
      Arc(o, LA(34, -62, 14, 36, -30, -40, 16)); o.root = Estocada(40, GetAttack(AttackId::AirPunch2)); }
    { Vis x(F); x.sar("ready", "kick", "kick", 1.0f).key('S', 1, 40).key('A', 0.5f, 40, 6).key('R', 0.7f, 35)
          .ghosts(2, kMorado);
      MoveOverride& o = add(Ov(AttackId::AirKick, "PATADA VOLADORA", x));
      Arc(o, LA(30, -52, 14, 36, -35, -45, 18)); o.root = Estocada(50, GetAttack(AttackId::AirKick)); }
    { Vis x(F); x.sar("ready", "kick", "kick", 1.0f).key('S', 1, 45).key('A', 0.1f, 45).key('R', 0.5f, 40)
          .ghosts(4, kMorado, 0.025f);
      MoveOverride& o = add(Ov(AttackId::DiveKick, "PATADA PICADA", x));
      Arc(o, LA(4, -56, 30, 46, -45, -45, 18)); }
    { Vis x(F); x.sar("ready", "punch", "punch", 1.0f).key('S', 1, -40).key('A', 1, 60, 8).key('R', 0.6f, 30)
          .ghosts(3, kMorado);
      MoveOverride& o = add(Ov(AttackId::AirSpike, "MARTILLO OSCURO", x));
      Arc(o, LA(20, -70, 26, 44, 60, -70, 20)); o.root = Estocada(40, GetAttack(AttackId::AirSpike)); }

    // ---- especiales ----
    { Vis x(F); x.seg('S', 0, 1, "ready").seg('A', 0, 1, "release_orb").seg('R', 0, 0.6f, "release_orb")
          .seg('R', 0.6f, 1, "ready").fx(ANTICIPA(1)).fx(IMPACTO(0.6f)).fx(RECOBRO(1))
          .over(1, 3, 34, -68, 0.6f, 'S', kMoradoFx).over(9, 3, 58, -68, 0.6f, 'A', kMoradoFx);
      MoveOverride& o = add(Ov(AttackId::EnergyWave, "ORBE OSCURO", x));
      o.projectile.enabled = true; o.projectile.damage = 20; o.projectile.tint = kMoradoProj; o.projectile.scale = 1.1f;
      o.projectile.origin = {56, -70}; }
    { const AttackDef& d = GetAttack(AttackId::RisingUppercut);
      Vis x(F); x.sar("ready", "punch", "ready", 0.7f).fx(ANTICIPA(1)).fx(RECOBRO(1)).key('A', 0.3f, -25, 8, 0, 1.05f, 1.06f)
          .key('R', 0.4f, -15).ghosts(4, kMorado, 0.025f).over(21, 1, 14, -60, 0.9f, 'A', kMoradoFx, -90);
      MoveOverride& o = add(Ov(AttackId::RisingUppercut, "GANCHO DE SOMBRA", x));
      // Sube 90 px (el del shoryu generico) y avanza 30 hacia el rival.
      Arc(o, LA(4, -78, 22, 44, -40, 95, 20)); o.root = d.root; o.root.dx = 30; }
    { Vis x(F); x.sar("ready", "kick", "ready", 0.6f).fx(GIRO(3)).fx(SALTITO(30)).fx(RECOBRO(1)).ghosts(4, kMorado, 0.025f)
          .over(23, 1, 0, -40, 0.9f, 'A', kMoradoFx);
      MoveOverride& o = add(Ov(AttackId::Hurricane, "CICLON MORADO", x));
      Arc(o, LA(0, -66, 24, 44, 0, 0, 20, true)); }
    { Vis x(F); x.seg('S', 0, 1, "ready").seg('A', 0, 1, "dash").seg('R', 0, 0.5f, "dash").seg('R', 0.5f, 1, "ready")
          .fx(ANTICIPA(1)).fx(GIRO(2)).fx(RECOBRO(1)).ghosts(4, kMorado, 0.025f)
          .over(42, 4, -30, -64, 0.8f, 'A', kMoradoFx);
      MoveOverride& o = add(Ov(AttackId::DrillKick, "TALADRO", x));
      Arc(o, LA(6, -64, 28, 50, -6, -6, 20)); }

    // ---- supers ----
    { Vis x(F); x.seg('S', 0, 1, "ready").seg('A', 0, 1, "release_orb").seg('R', 0, 0.5f, "release_orb")
          .seg('R', 0.5f, 1, "ready").fx(ANTICIPA(1.4f)).fx(LATIDO(0.05f, 6, '*')).fx(RECOBRO(1)).ghosts(4, kMorado)
          .over(24, 2, 40, -66, 1.0f, 'A', kMoradoFx).over(22, 1, 0, -10, 1.3f, 'A', kMoradoFx);
      MoveOverride& o = add(Ov(AttackId::RageAttack, "FURIA DE SOMBRA", x));
      Arc(o, LA(0, -70, 30, 60, -40, 40, 30, true)); }
    { Vis x(F); x.seg('S', 0, 1, "ready").seg('A', 0, 0.5f, "dash").seg('A', 0.5f, 1, "punch").seg('R', 0, 1, "ready")
          .fx(ANTICIPA(1.4f)).fx(IMPACTO(1.6f)).fx(RECOBRO(1.2f)).ghosts(5, kMorado, 0.02f)
          .over(13, 1, 44, -72, 0.9f, 'A', kMoradoFx).over(27, 2, 30, -70, 1.1f, 'R', kMoradoFx);
      MoveOverride& o = add(Ov(AttackId::Finisher, "GOLPE DEL ESPEJO", x));
      Arc(o, LA(6, -72, 30, 56, -10, 10, 30)); o.root = Estocada(60, GetAttack(AttackId::Finisher)); }

    // ---- firmas ----
    {   // QCB+J: Sombra espejo. Una sombra intensa avanza 160 px y golpea dash -> punch.
        AttackDef d = Row(AttackId::DashAttack, "SOMBRA ESPEJO", 14, 10, 12, 16);
        d.spCost = 15; d.knockback = 420; d.hitstun = 0.40f; d.launch = 0; d.heavy = true; d.priority = 3;
        d.cooldown = 0.2f; d.canCombo = false;
        Vis x(F); x.seg('S', 0, 1, "ready").seg('A', 0, 0.5f, "dash").seg('A', 0.5f, 1, "punch").seg('R', 0, 1, "ready")
            .fx(ANTICIPA(1.2f)).fx(IMPACTO(1.2f)).fx(RECOBRO(1)).ghosts(5, Color{150, 70, 255, 255}, 0.016f)
            .over(10, 3, 40, -72, 0.6f, 'A', kMoradoFx);
        MoveOverride& o = add(Ov(AttackId::Signature1, "SOMBRA ESPEJO", x));
        Replace(o, d);
        Arc(o, LA(6, -70, 28, 52, 0, 0, 22)); o.root = Estocada(160, d, Ease::InQuad);
        o.armorFrom = 0; o.armorTo = d.startup; o.armorHits = 1;
    }
    {   // >L: Doble orbe (carriles -12/+12).
        AttackDef d = Row(AttackId::EnergyWave, "DOBLE ORBE", 12, 12, 6, 18);
        d.spCost = 25; d.spawnsProjectile = true;
        Vis x(F); x.seg('S', 0, 1, "ready").seg('A', 0, 1, "release_orb").seg('R', 0, 0.6f, "release_orb")
            .seg('R', 0.6f, 1, "ready").fx(ANTICIPA(1.2f)).fx(IMPACTO(0.8f)).fx(RECOBRO(1))
            .over(2, 2, 34, -68, 0.7f, 'S', kMoradoFx).over(24, 2, 56, -68, 0.7f, 'A', kMoradoFx);
        MoveOverride& o = add(Ov(AttackId::Signature2, "DOBLE ORBE", x));
        Replace(o, d);
        o.projectile.enabled = true; o.projectile.damage = 12; o.projectile.count = 2; o.projectile.interval = 0.06f;
        o.projectile.spreadLane = 12; o.projectile.tint = kMoradoProj; o.projectile.origin = {56, -70};
    }
    {   // vL: Explosion oscura (anillo r110 que levanta).
        AttackDef d = Row(AttackId::RageAttack, "EXPLOSION OSCURA", 16, 8, 10, 18);
        d.spCost = 20; d.launch = 400; d.knockback = 380; d.hitstun = 0.45f; d.knockdown = true; d.cooldown = 0.3f;
        d.rageGain = 6; d.priority = 3;
        Vis x(F); x.seg('S', 0, 1, "ready").seg('A', 0, 1, "release_orb").seg('R', 0, 0.5f, "release_orb")
            .seg('R', 0.5f, 1, "ready").fx(ANTICIPA(1.2f)).fx(LATIDO(0.06f, 7, '*')).fx(RECOBRO(1))
            .over(22, 1, 0, -8, 1.6f, 'A', kMoradoFx).over(24, 2, 0, -64, 1.2f, 'A', kMoradoFx);
        MoveOverride& o = add(Ov(AttackId::Signature3, "EXPLOSION OSCURA", x));
        Replace(o, d);
        o.areaRadius = 110;
        o.armorFrom = 0; o.armorTo = d.startup + d.active; o.armorHits = 0;
    }

    // ---- Fase 2 (agarres, contra, levantarse atacando): solo el visual; enabled
    // queda como venga: sin replaceDef siguen apagados por requiresOverride hasta F2.1. ----
    { Vis x(F); x.sar("ready", "punch", "ready", 0.5f).key('A', 0.5f, 4, 6);
      MoveOverride o = Ov(AttackId::Grab, "AGARRE", x); add(o); }
    { Vis x(F); x.sar("ready", "kick", "ready", 0.5f).key('A', 0.5f, -10, 4).fx(SALTITO(6));
      MoveOverride o = Ov(AttackId::GrabKnee, "RODILLAZO", x); add(o); }
    { Vis x(F); x.sar("punch", "dash", "ready", 0.5f).fx(IMPACTO(1.4f));
      MoveOverride o = Ov(AttackId::ThrowForward, "PROYECCION", x); add(o); }
    { Vis x(F); x.sar("punch", "release_orb", "ready", 0.5f, true).fx(GIRO(1));
      MoveOverride o = Ov(AttackId::ThrowBack, "SUPLEX OSCURO", x); add(o); }
    { Vis x(F); x.sar("ready", "punch", "ready", 0.5f).fx(IMPACTO(1.4f)).ghosts(3, kMorado);
      MoveOverride o = Ov(AttackId::GuardCounter, "CONTRA", x); add(o); }
    { Vis x(F); x.sar("death", "kick", "ready", 0.5f).fx(BARRIDA('A'));
      MoveOverride o = Ov(AttackId::WakeupAttack, "BARRIDA AL LEVANTARSE", x); add(o); }
    return v;
}

MoveSet BuildClone() {
    static const std::vector<MoveOverride> kOv = CloneOverrides();
    MoveSet m;
    m.name = "clon antiguo";
    m.caps = CapAll;
    const int j[4] = {Id(AttackId::Punch1), Id(AttackId::Punch2), Id(AttackId::PunchHook), Id(AttackId::Punch3)};
    for (int i = 0; i < 4; ++i) m.chainJ[i] = j[i];
    m.chainJCount = 4;
    m.chainK[0] = Id(AttackId::Kick); m.chainK[1] = Id(AttackId::Kick2); m.chainK[2] = Id(AttackId::Kick3);
    m.chainKCount = 3;
    // Habilidades 1-5: Orbe oscuro, Sombra espejo, Gancho de sombra, Ciclon morado, Explosion oscura.
    m.skillSlots[0] = Id(AttackId::EnergyWave);
    m.skillSlots[1] = Id(AttackId::Signature1);
    m.skillSlots[2] = Id(AttackId::RisingUppercut);
    m.skillSlots[3] = Id(AttackId::Hurricane);
    m.skillSlots[4] = Id(AttackId::Signature3);
    m.overrides = kOv.data();
    m.overrideCount = static_cast<int>(kOv.size());
    // Estados (las poses las elige la tabla de Poses_CloneGrinder.cpp; aqui el PoseFx).
    { Vis x("rayder_clone"); x.fx(TEMBLOR(3, 28, 'A')).key('A', 0.0f, -6, -6).state(0.30f, false); m.hit = x.mv; }
    { Vis x("rayder_clone"); x.fx(LATIDO(0.03f, 2.0f, '*')).over(16, 3, 0, -2, 1.1f, '*', kMoradoFx)
          .over(26, 6, 0, -70, 1.0f, '*', Color{120, 60, 200, 255}).state(1.6f, false); m.victory = x.mv; }
    m.jumpVz = 720;
    return m;
}

// =====================================================================
// GRINDER: maquina pesada con sierra. Sin salto ni aereos (CapJump/CapAir
// apagados: I no hace nada y los aereos no se ofrecen). Golpes multiples de
// sierra, armadura en la embestida, ondas al golpear el suelo.
// =====================================================================
std::vector<MoveOverride> GrinderOverrides() {
    const char* F = "grinder";
    std::vector<MoveOverride> v;
    auto add = [&](MoveOverride o) -> MoveOverride& { v.push_back(o); return v.back(); };

    // ---- cadena J de sierra (multigolpe cada 5-6 f) ----
    {   AttackDef d = Row(AttackId::Punch1, "SIERRA", 5, 6, 10, 10);
        Vis x(F); x.sar("idle", "saw", "idle", 0.5f); Golpe(x, 1.0f); x.fx(TEMBLOR(1.5f, 30, 'A'))
            .over(30, 2, 40, -60, 0.5f, 'A', kChispa);
        MoveOverride& o = add(Ov(AttackId::Punch1, "SIERRA", x)); Replace(o, d); Multi(o, 2, 5 / 60.0f);
        Arc(o, LA(6, -74, 26, 44, -10, 0, 18)); o.root = Estocada(10, d); }
    {   AttackDef d = Row(AttackId::Punch2, "SIERRA CRUZADA", 4, 7, 12, 10);
        Vis x(F); x.sar("idle", "saw", "idle", 0.5f).fx(ANTICIPA(0.8f)).fx(RECOBRO(0.8f)).fx(TEMBLOR(1.5f, 30, 'A'))
            .key('A', 0.5f, 8, 8, 0, 1.06f, 0.97f).over(30, 2, 42, -64, 0.5f, 'A', kChispa);
        MoveOverride& o = add(Ov(AttackId::Punch2, "SIERRA CRUZADA", x)); Replace(o, d); Multi(o, 3, 4 / 60.0f);
        Arc(o, LA(6, -76, 26, 46, 10, -10, 18)); o.root = Estocada(12, d); }
    {   AttackDef d = Row(AttackId::PunchHook, "SIERRA LARGA", 4, 8, 16, 12);
        Vis x(F); x.sar("idle", "saw", "idle", 0.5f); Golpe(x, 1.0f); x.fx(TEMBLOR(1.5f, 30, 'A'))
            .over(39, 1, 44, -64, 0.6f, 'A', kChispa);
        MoveOverride& o = add(Ov(AttackId::PunchHook, "SIERRA LARGA", x)); Replace(o, d); Multi(o, 4, 4 / 60.0f);
        Arc(o, LA(6, -72, 28, 48, -4, 0, 18)); o.root = Estocada(40, d); }
    {   AttackDef d = Row(AttackId::Punch3, "SIERRA DESCENDENTE", 18, 10, 6, 18);
        d.overhead = true; d.knockdown = true; d.knockback = 520; d.launch = 220;
        Vis x(F); x.sar("idle", "saw", "idle", 0.5f).fx(ANTICIPA(1.2f)).fx(RECOBRO(1.2f))
            .key('S', 1.0f, -30, -4, -6).key('A', 1.0f, 30, 10, 2, 1.06f, 0.94f)
            .over(13, 1, 46, -30, 0.7f, 'A', kChispa).over(37, 1, 40, -6, 0.6f, 'R', WHITE);
        MoveOverride& o = add(Ov(AttackId::Punch3, "SIERRA DESCENDENTE", x)); Replace(o, d);
        Arc(o, LA(6, -96, 34, 52, 50, -40, 22)); }

    // ---- cadena K: golpes al suelo (bajos, onda r100) ----
    {   AttackDef d = Row(AttackId::Kick, "GOLPE AL SUELO", 15, 12, 6, 16);
        d.low = true; d.launch = 160;
        Vis x(F); x.sar("idle", "slam", "idle", 0.6f).fx(ANTICIPA(1.2f)).fx(RECOBRO(1)).key('S', 1, -6, 0, -6)
            .key('A', 0.3f, 6, 4, 4, 1.06f, 0.92f).over(22, 1, 30, -6, 1.0f, 'A', kChispa).over(37, 1, 40, -4, 0.7f, 'A', WHITE);
        MoveOverride& o = add(Ov(AttackId::Kick, "GOLPE AL SUELO", x)); Replace(o, d);
        o.areaRadius = 100; }
    {   AttackDef d = Row(AttackId::Kick2, "DOBLE GOLPE", 10, 10, 14, 16);
        d.low = true;
        Vis x(F); x.seg('S', 0, 1, "idle").seg('A', 0, 0.45f, "slam").seg('A', 0.45f, 1, "slam").seg('R', 0, 1, "idle")
            .fx(ANTICIPA(1)).fx(RECOBRO(1)).key('A', 0.2f, 6, 4, 4, 1.05f, 0.93f).key('A', 0.45f, -4, 0, -6)
            .key('A', 0.7f, 6, 8, 4, 1.06f, 0.92f).over(22, 1, 34, -6, 1.0f, 'A', kChispa);
        MoveOverride& o = add(Ov(AttackId::Kick2, "DOBLE GOLPE", x)); Replace(o, d); Multi(o, 2, 7 / 60.0f);
        o.areaRadius = 100; }
    {   AttackDef d = Row(AttackId::Kick3, "TERREMOTO", 20, 14, 8, 20);
        d.low = true; d.knockdown = true; d.launch = 300; d.heavy = true;
        Vis x(F); x.sar("overdrive", "slam", "idle", 0.6f).fx(ANTICIPA(1.5f)).fx(SALTITO(14, 'S')).fx(RECOBRO(1.2f))
            .key('A', 0.2f, 8, 6, 6, 1.08f, 0.90f).over(22, 1, 0, -6, 1.5f, 'A', kChispa).over(14, 1, 40, -6, 0.8f, 'A', kChispa);
        MoveOverride& o = add(Ov(AttackId::Kick3, "TERREMOTO", x)); Replace(o, d);
        o.areaRadius = 120; }

    // ---- lanzadores ----
    {   AttackDef d = Row(AttackId::KickLauncher, "LANZADOR", 14, 8, 6, 18);
        d.launch = 680;
        Vis x(F); x.sar("idle", "saw", "idle", 0.6f).fx(ANTICIPA(1)).fx(SALTITO(6)).fx(RECOBRO(1))
            .key('A', 0.5f, -40, 6, 0, 1.04f, 1.04f).over(30, 2, 34, -90, 0.6f, 'A', kChispa);
        MoveOverride& o = add(Ov(AttackId::KickLauncher, "LANZADOR", x)); Replace(o, d);
        Arc(o, LA(6, -70, 28, 48, -20, 70, 20)); }
    {   AttackDef d = Row(AttackId::Uppercut, "SIERRA ALTA", 14, 7, 6, 18);
        d.launch = 680;
        Vis x(F); x.sar("idle", "saw", "idle", 0.6f).fx(ANTICIPA(1)).fx(UPPER(8)).fx(RECOBRO(1))
            .over(30, 2, 30, -96, 0.6f, 'A', kChispa);
        MoveOverride& o = add(Ov(AttackId::Uppercut, "SIERRA ALTA", x)); Replace(o, d);
        Arc(o, LA(6, -70, 26, 46, -30, 85, 20)); }

    // ---- comando ----
    {   AttackDef d = Row(AttackId::FwdPunch, "EMBESTIDA", 16, 10, 10, 16);
        d.knockback = 520; d.heavy = true;
        Vis x(F); x.sar("idle", "ram", "idle", 0.5f).fx(ANTICIPA(1.4f)).fx(IMPACTO(0.8f)).fx(RECOBRO(1)).ghosts(3, kRojo)
            .over(37, 1, -30, -8, 0.8f, 'A', WHITE);
        MoveOverride& o = add(Ov(AttackId::FwdPunch, "EMBESTIDA", x)); Replace(o, d);
        Arc(o, LA(10, -64, 26, 46, 0, 0, 24)); o.root = Estocada(140, d);
        o.armorFrom = 0; o.armorTo = d.startup + d.active; o.armorHits = 2; }
    {   Vis x(F); x.sar("idle", "slam", "idle", 0.5f).fx(ANTICIPA(1)).fx(RECOBRO(1)).key('A', 0.3f, 8, 6, 2, 1.05f, 0.94f)
            .over(22, 1, 40, -6, 0.8f, 'A', kChispa);
        MoveOverride& o = add(Ov(AttackId::FwdKick, "PISADA DE AVANCE", x));
        Arc(o, LA(8, -30, 24, 46, -10, -20, 20)); o.timeScale = 1.15f; o.damageScale = 1.1f; }
    {   Vis x(F); x.sar("idle", "saw", "idle", 0.5f, true).fx(ANTICIPA(0.8f)).fx(RECOBRO(1)).fx(TEMBLOR(1.5f, 30, 'A'))
            .key('A', 0.5f, 6, 8, 0, 1.06f, 0.97f);
        MoveOverride& o = add(Ov(AttackId::BackElbow, "SIERRA TRASERA", x));
        Arc(o, LA(-6, -72, 26, 46, 180, 180, 18)); o.hitCount = 2; o.multiHitInterval = 5 / 60.0f; }
    {   Vis x(F); x.sar("idle", "saw", "idle", 0.5f).fx(ANTICIPA(0.8f)).fx(GIRO(1)).fx(RECOBRO(1)).ghosts(3, kChispa);
        MoveOverride& o = add(Ov(AttackId::SpinKick, "GIRO DE SIERRA", x));
        Arc(o, LA(0, -70, 26, 46, -10, 10, 20, true)); o.timeScale = 1.1f; }
    {   Vis x(F); x.sar("idle", "slam", "idle", 0.6f).fx(ANTICIPA(0.8f)).fx(BARRIDA('A')).fx(BARRIDA('R'))
            .over(37, 1, 34, -6, 0.7f, 'A', WHITE);
        MoveOverride& o = add(Ov(AttackId::Sweep, "BARRIDO", x));
        Arc(o, LA(4, -18, 24, 48, -4, -4, 18)); o.timeScale = 1.1f; }
    {   Vis x(F); x.sar("idle", "slam", "idle", 0.5f).fx(ANTICIPA(1.2f)).fx(RECOBRO(1)).key('A', 0.3f, 10, 6, 4, 1.06f, 0.92f)
            .over(14, 1, 30, -6, 0.6f, 'A', kChispa);
        MoveOverride& o = add(Ov(AttackId::Stomp, "APLASTAR", x)); o.damageScale = 1.4f; }

    // ---- dash ----
    {   Vis x(F); x.seg('S', 0, 1, "ram").seg('A', 0, 1, "ram").seg('R', 0, 0.5f, "ram").seg('R', 0.5f, 1, "idle")
            .fx(IMPACTO(1.2f)).fx(RECOBRO(1)).ghosts(3, kRojo);
        MoveOverride& o = add(Ov(AttackId::DashAttack, "CARGA", x));
        Arc(o, LA(10, -64, 26, 46, -4, 0, 24)); }
    {   Vis x(F); x.sar("ram", "slam", "idle", 0.5f).fx(BARRIDA('A')).fx(RECOBRO(1)).ghosts(3, kRojo)
            .over(37, 1, 20, -6, 0.8f, 'A', WHITE);
        MoveOverride& o = add(Ov(AttackId::DashKick, "ARRASTRE", x));
        Arc(o, LA(4, -20, 24, 48, -4, -4, 18)); }

    // ---- especiales ----
    {   AttackDef d = Row(AttackId::EnergyWave, "DISCO DE SIERRA", 16, 12, 6, 14);
        d.spawnsProjectile = true;
        Vis x(F); x.sar("idle", "saw", "idle", 0.6f).fx(ANTICIPA(1.2f)).fx(IMPACTO(0.8f)).fx(RECOBRO(1))
            .over(30, 2, 40, -60, 0.6f, 'A', kChispa);
        MoveOverride& o = add(Ov(AttackId::EnergyWave, "DISCO DE SIERRA", x)); Replace(o, d);
        o.projectile.enabled = true; o.projectile.damage = 16; o.projectile.tint = kChispa; o.projectile.speed = 520;
        o.projectile.scale = 0.9f; o.projectile.origin = {50, -60}; }
    {   AttackDef d = Row(AttackId::RisingUppercut, "SIERRA ASCENDENTE", 16, 4, 12, 22);
        d.launch = 760;
        Vis x(F); x.sar("idle", "saw", "idle", 0.7f).fx(ANTICIPA(1)).fx(RECOBRO(1)).fx(TEMBLOR(1.5f, 30, 'A'))
            .key('A', 0.3f, -28, 8, 0, 1.05f, 1.06f).ghosts(3, kChispa).over(21, 1, 20, -70, 1.0f, 'A', kChispa, -90);
        MoveOverride& o = add(Ov(AttackId::RisingUppercut, "SIERRA ASCENDENTE", x)); Replace(o, d);
        // Maquina pesada: sube solo 30 px (el shoryu generico 90) y avanza 30.
        Arc(o, LA(6, -80, 26, 48, -40, 95, 22)); o.root = d.root; o.root.dz = 30; o.root.dx = 30; }
    {   Vis x(F); x.sar("overdrive", "saw", "idle", 0.6f).fx(GIRO(3)).fx(RECOBRO(1)).ghosts(4, kChispa, 0.025f)
            .over(23, 1, 0, -30, 1.0f, 'A', kChispa);
        MoveOverride& o = add(Ov(AttackId::Hurricane, "TORBELLINO", x));
        Arc(o, LA(0, -70, 26, 46, 0, 0, 22, true)); o.root.dx = 90; o.root.t1 = 0.8f; o.root.ease = Ease::InOutSine; }
    {   Vis x(F); x.seg('S', 0, 1, "idle").seg('A', 0, 1, "ram").seg('R', 0, 0.5f, "saw").seg('R', 0.5f, 1, "idle")
            .fx(ANTICIPA(1)).fx(TEMBLOR(2, 30, 'A')).fx(RECOBRO(1)).ghosts(3, kRojo).over(30, 2, 46, -64, 0.6f, 'A', kChispa);
        MoveOverride& o = add(Ov(AttackId::DrillKick, "PERFORADORA", x));
        Arc(o, LA(10, -64, 26, 46, 0, 0, 22)); }

    // ---- supers ----
    {   Vis x(F); x.sar("idle", "overdrive", "idle", 0.6f).fx(ANTICIPA(1.4f)).fx(LATIDO(0.05f, 6, '*')).fx(RECOBRO(1))
            .ghosts(3, kAscua).over(22, 1, 0, -6, 1.4f, 'A', kAscua).over(13, 1, 40, -60, 1.0f, 'A', kChispa);
        MoveOverride& o = add(Ov(AttackId::RageAttack, "FURIA DE ACERO", x));
        Arc(o, LA(0, -70, 30, 60, -40, 40, 32, true)); }
    {   Vis x(F); x.seg('S', 0, 1, "overdrive").seg('A', 0, 0.5f, "ram").seg('A', 0.5f, 1, "saw").seg('R', 0, 1, "idle")
            .fx(ANTICIPA(1.4f)).fx(IMPACTO(1.4f)).fx(RECOBRO(1.2f)).ghosts(4, kRojo, 0.02f)
            .over(13, 1, 46, -66, 1.0f, 'A', kChispa);
        MoveOverride& o = add(Ov(AttackId::Finisher, "DESGUACE", x));
        Arc(o, LA(8, -70, 30, 56, -10, 10, 30)); o.root = Estocada(60, GetAttack(AttackId::Finisher)); }

    // ---- firmas ----
    {   // QCB+J: Molinillo (overdrive girando, a los dos lados).
        AttackDef d = Row(AttackId::Hurricane, "MOLINILLO", 4, 8, 36, 18);
        d.spCost = 15; d.root = RootCurve{}; d.knockback = 200;
        Vis x(F); x.sar("overdrive", "overdrive", "idle", 0.6f).fx(GIRO(4)).fx(TEMBLOR(1.5f, 30, 'A')).fx(RECOBRO(1))
            .ghosts(4, kChispa, 0.025f).over(23, 1, 0, -30, 1.1f, 'A', kChispa).over(30, 2, 0, -60, 0.8f, 'A', kChispa);
        MoveOverride& o = add(Ov(AttackId::Signature1, "MOLINILLO", x)); Replace(o, d); Multi(o, 6, 6 / 60.0f);
        Arc(o, LA(0, -64, 30, 50, 0, 0, 24, true));
        o.armorFrom = 0; o.armorTo = d.startup + d.active; o.armorHits = 0; }
    {   // >L: Embestida turbo (estela roja, 220 px, 3 golpes, armadura).
        AttackDef d = Row(AttackId::FwdPunch, "EMBESTIDA TURBO", 6, 10, 16, 20);
        d.spCost = 20; d.knockback = 480; d.heavy = true; d.priority = 3; d.overhead = false; d.knockdown = true;
        Vis x(F); x.sar("idle", "ram", "idle", 0.5f).fx(ANTICIPA(1.4f)).fx(IMPACTO(0.8f)).fx(RECOBRO(1))
            .ghosts(5, kRojo, 0.02f).over(37, 1, -40, -8, 0.9f, 'A', WHITE).over(11, 2, 50, -60, 0.7f, 'A', kRojo);
        MoveOverride& o = add(Ov(AttackId::Signature2, "EMBESTIDA TURBO", x)); Replace(o, d); Multi(o, 3, 5 / 60.0f);
        Arc(o, LA(10, -64, 26, 46, 0, 0, 24)); o.root = Estocada(220, d, Ease::InQuad);
        o.armorFrom = 0; o.armorTo = d.startup + d.active; o.armorHits = 0; }
    {   // vL: Sobrecarga (aura que dana, r120).
        AttackDef d = Row(AttackId::RageAttack, "SOBRECARGA", 5, 8, 40, 18);
        d.spCost = 25; d.knockback = 260; d.launch = 0; d.knockdown = false; d.cooldown = 0.3f; d.rageGain = 4;
        Vis x(F); x.sar("idle", "overdrive", "idle", 0.7f).fx(ANTICIPA(1)).fx(LATIDO(0.05f, 8, '*'))
            .fx(TEMBLOR(1.5f, 30, 'A')).fx(TINTE(Color{255, 200, 160, 255}, 0, 'A'))
            .over(22, 1, 0, -6, 1.6f, 'A', kAscua).over(16, 3, 0, -10, 1.3f, 'A', kAscua);
        MoveOverride& o = add(Ov(AttackId::Signature3, "SOBRECARGA", x)); Replace(o, d); Multi(o, 4, 10 / 60.0f);
        o.areaRadius = 120;
        o.armorFrom = 0; o.armorTo = d.startup + d.active; o.armorHits = 0; }

    // ---- Fase 2: solo el visual (apagados hasta F2.1) ----
    { Vis x(F); x.sar("idle", "saw", "idle", 0.5f).key('A', 0.5f, 4, 4);
      MoveOverride o = Ov(AttackId::Grab, "AGARRE", x); add(o); }
    { Vis x(F); x.sar("idle", "slam", "idle", 0.5f).key('A', 0.5f, 6, 4, 4);
      MoveOverride o = Ov(AttackId::GrabKnee, "PRENSA", x); add(o); }
    { Vis x(F); x.sar("saw", "ram", "idle", 0.5f).fx(IMPACTO(1.2f));
      MoveOverride o = Ov(AttackId::ThrowForward, "LANZAR CHATARRA", x); add(o); }
    { Vis x(F); x.sar("saw", "slam", "idle", 0.5f, true).fx(GIRO(1));
      MoveOverride o = Ov(AttackId::ThrowBack, "VOLTEO", x); add(o); }
    { Vis x(F); x.sar("idle", "ram", "idle", 0.5f).fx(IMPACTO(1.2f)).ghosts(3, kRojo);
      MoveOverride o = Ov(AttackId::GuardCounter, "CONTRA", x); add(o); }
    { Vis x(F); x.sar("death", "slam", "idle", 0.5f).fx(BARRIDA('A'));
      MoveOverride o = Ov(AttackId::WakeupAttack, "SACUDIDA", x); add(o); }
    return v;
}

MoveSet BuildGrinder() {
    static const std::vector<MoveOverride> kOv = GrinderOverrides();
    MoveSet m;
    m.name = "grinder";
    m.caps = CapAll & ~(CapJump | CapAir);   // maquina pesada: no salta
    const int j[4] = {Id(AttackId::Punch1), Id(AttackId::Punch2), Id(AttackId::PunchHook), Id(AttackId::Punch3)};
    for (int i = 0; i < 4; ++i) m.chainJ[i] = j[i];
    m.chainJCount = 4;
    m.chainK[0] = Id(AttackId::Kick); m.chainK[1] = Id(AttackId::Kick2); m.chainK[2] = Id(AttackId::Kick3);
    m.chainKCount = 3;
    // Habilidades 1-5: Disco de sierra, Molinillo, Embestida turbo, Sierra ascendente, Sobrecarga.
    m.skillSlots[0] = Id(AttackId::EnergyWave);
    m.skillSlots[1] = Id(AttackId::Signature1);
    m.skillSlots[2] = Id(AttackId::Signature2);
    m.skillSlots[3] = Id(AttackId::RisingUppercut);
    m.skillSlots[4] = Id(AttackId::Signature3);
    m.overrides = kOv.data();
    m.overrideCount = static_cast<int>(kOv.size());
    { Vis x("grinder"); x.fx(TEMBLOR(2, 28, 'A')).key('A', 0.0f, -5, -5).state(0.30f, false); m.hit = x.mv; }
    { Vis x("grinder"); x.fx(LATIDO(0.04f, 2.5f, '*')).fx(TEMBLOR(1.5f, 30, 'A'))
          .over(22, 1, 0, -4, 1.3f, '*', kAscua).over(30, 2, 0, -80, 0.9f, '*', kChispa).state(1.6f, false);
      m.victory = x.mv; }
    m.jumpVz = 560;
    m.walkSpeedScale = 1.0f;   // ya lento por la ficha (speedMultiplier 0.80)
    return m;
}

}  // namespace

const MoveSet* MovesetFor_CloneGrinder(int skin) {
    if (skin == 1) { static const MoveSet kClone = BuildClone(); return &kClone; }
    if (skin == 3) { static const MoveSet kGrinder = BuildGrinder(); return &kGrinder; }
    return nullptr;
}

}  // namespace district_fury
