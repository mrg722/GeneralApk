// Moveset de BRAKK (skin 2) — BRIEF F1.5: "bruto de cadena: lento, alcance
// largo, sin salto". Usa los clips de jefe que el jugador nunca usaba (chain,
// chain_throw, smash, fury, explosive, charge, run, victory) y celdas sueltas del
// atlas brakk_v2 a traves de clips NUEVOS de data/manifest/brakk_moves.json.
//
// Reglas propias:
// - Los 8 ids historicos (Punch1-3, Kick, EnergyWave, DashAttack, RageAttack,
//   Finisher) conservan su perfil de animacion (AttackAnimation.h, skin 2): sin
//   replaceDef de tiempos (EnergyWave copia la fila tal cual y solo cambia el
//   alcance / multigolpe), y su clip es el suyo de siempre (rage_attack ya sin el
//   cuadro 88, arreglado en el manifiesto con override).
// - Sin salto ni aereos (caps sin CapJump/CapAir), sin Hurricane/DrillKick
//   genericos: los reemplazan sus Signature.
// - Alcance de cadena x1.3 sobre la fila generica (arcos medidos en px de cuerpo
//   estandar de 112 px; el motor los escala por bodyScale).
#include "game/moves/MoveRegistry.h"
#include "game/player/PoseFxPresets.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace district_fury {
namespace {

using namespace posefx;

constexpr float F(int frames) { return frames / 60.0f; }
constexpr int I(AttackId a) { return static_cast<int>(a); }
constexpr float kChainReach = 1.3f;
const Color kRust{255, 120, 90, 255};    // estela: cadena al rojo
const Color kEmber{255, 170, 120, 255};  // superposiciones

struct Mv {
    MoveOverride o;
    // full = replaceDef con la fila de `base` (o la del propio id).
    Mv(AttackId id, const char* name, bool full = true, AttackId base = AttackId::None) {
        o.id = I(id);
        o.displayName = name;
        if (full) {
            o.replaceDef = true;
            o.def = GetAttack(base == AttackId::None ? id : base);
            o.def.name = name;
            o.def.requiresOverride = false;
        }
    }
    Mv& row(int dmg, int s, int a, int r, float kb, float hs, float lch = 0) {
        AttackDef& d = o.def;
        d.damage = dmg; d.startup = F(s); d.active = F(a); d.recovery = F(r);
        d.knockback = kb; d.hitstun = hs; d.launch = lch;
        d.heavy = d.heavy || dmg >= 14; d.priority = std::max(d.priority, dmg >= 16 ? 2 : 1);
        d.rageGain = std::max(4, dmg * 2 / 3);
        return *this;
    }
    Mv& arc(float px, float ph, float L0, float L1, float t0, float t1, float r, bool both = false, bool behind = false) {
        LimbArc a{px, ph, L0, L1, t0, t1, r};
        a.bothSides = both; a.behind = behind;
        o.hasArc = true; o.arc = a;
        AttackDef& d = o.def;
        d.arc = a; d.hasArc = true;
        const float L = std::max(L0, L1);
        d.range = std::fabs(px) + L + r + 20.0f;
        d.boxWidth = L + r; d.boxForward = px + 0.5f * L; d.boxCenterOffsetY = ph;
        d.hitsBehind = d.hitsBehind || behind || both;
        return *this;
    }
    Mv& lunge(float dx, Ease e = Ease::OutCubic) {
        const float total = std::max(1e-4f, o.def.startup + o.def.active + o.def.recovery);
        o.root = RootCurve{dx, 0, 0, (o.def.startup + o.def.active) / total, e};
        o.def.root = o.root;
        return *this;
    }
    Mv& clip(const char* c, int from = -1, int to = -1) {
        if (o.visual.clipCount < 3) { ClipSeg s; s.clip = c; s.from = from; s.to = to; o.visual.clips[o.visual.clipCount++] = s; }
        return *this;
    }
    Mv& fx(const PresetUse& p) { if (o.visual.fx.presetCount < 6) o.visual.fx.presets[o.visual.fx.presetCount++] = p; return *this; }
    Mv& key(char ph, float v, float sx, float sy, float ang, float dx = 0, float dy = 0) {
        if (o.visual.fx.keyCount >= 8) return *this;
        FxKey k; k.phase = ph; k.v = v; k.sx = sx; k.sy = sy; k.angle = ang; k.dx = dx; k.dy = dy;
        o.visual.fx.keys[o.visual.fx.keyCount++] = k;
        return *this;
    }
    Mv& ghost(int n, float interval = .03f, Color c = kRust) {
        o.visual.fx.ghostCount = n; o.visual.fx.ghostInterval = interval; o.visual.fx.ghostTint = c; return *this;
    }
    Mv& over(int first, int count, float x, float y, float scale, char ph = 'A', float rot = 0) {
        if (o.visual.fx.overlayCount >= 2) return *this;
        OverlayFx f; f.first = first; f.count = count; f.x = x; f.y = y; f.scale = scale; f.phase = ph; f.rot = rot; f.tint = kEmber;
        o.visual.fx.overlay[o.visual.fx.overlayCount++] = f;
        return *this;
    }
    Mv& hits(int n, float interval) {
        o.hitCount = n; o.multiHitInterval = interval;
        if (o.replaceDef) { o.def.hitCount = n; o.def.multiHitInterval = interval; }
        return *this;
    }
    Mv& armor(int n) {
        o.armorFrom = 0; o.armorTo = o.def.startup + o.def.active; o.armorHits = n;
        o.def.armorFrom = o.armorFrom; o.def.armorTo = o.armorTo; o.def.armorHits = n;
        return *this;
    }
    Mv& area(float r) { o.areaRadius = r; return *this; }
    Mv& pull(float px) { o.pullPx = px; return *this; }
    Mv& sp(int c) { o.def.spCost = c; return *this; }
    Mv& with(void (*f)(AttackDef&)) { f(o.def); return *this; }
};

MoveOverride Off(AttackId id) { MoveOverride o; o.id = I(id); o.enabled = false; return o; }

std::vector<MoveOverride> BuildOverrides() {
    std::vector<MoveOverride> v;
    auto add = [&](Mv& m) { v.push_back(m.o); };

    // ================= Cadena J (5) =================
    // 1-2: latigazos historicos (punch1 23-25, punch2 26-29), mismo clip y perfil;
    // solo el alcance de cadena (x1.3).
    { Mv m(AttackId::Punch1, "LATIGAZO", false); m.o.reachScale = kChainReach; m.clip("punch1"); add(m); }
    { Mv m(AttackId::Punch2, "LATIGAZO DE VUELTA", false); m.o.reachScale = kChainReach; m.clip("punch2"); add(m); }
    // 3: CADENA GIRATORIA (chain 23-30): 2 golpes, la cadena barre por delante.
    { Mv m(AttackId::PunchHook, "CADENA GIRATORIA");
      m.row(8, 9, 10, 12, 260, .30f).hits(2, F(5))
       .arc(4, -84, 40, 100, 30, -20, 18).lunge(14)
       .clip("cadena_giro").fx(ANTICIPA(.9f)).fx(IMPACTO(.8f)).fx(RECOBRO(.8f)).ghost(3)
       .over(8, 3, 96, -80, .8f);
      add(m); }
    // 4: GOLPE AL SUELO historico (punch3, derriba), reach x1.3; el 5o remata con la
    // onda r160 (alcanza al caido: area).
    { Mv m(AttackId::Punch3, "GOLPE AL SUELO", false); m.o.reachScale = kChainReach; m.clip("punch3"); add(m); }
    // 5: GRAN GOLPE (10 cadena sobre la cabeza -> 48 -> 49/51/53 golpe al suelo):
    // onda r160 y derribo.
    { Mv m(AttackId::Chain5, "GRAN GOLPE DE CADENA", true, AttackId::Punch3);
      m.row(22, 16, 8, 24, 520, .48f, 260)
       .with([](AttackDef& d) { d.knockdown = true; d.breaksGuard = true; d.low = true; d.hitstopScale = 1.4f; d.shakeScale = 1.6f; d.priority = 3; })
       .arc(10, -110, 40, 120, 70, -30, 24).area(160)
       .clip("golpe_suelo").fx(ANTICIPA(1.4f)).fx(SALTITO(8, 'S')).fx(TEMBLOR(4, 24, 'A')).fx(RECOBRO(.8f))
       .over(14, 2, 70, -8, 1.8f).over(22, 1, 40, -10, 1.6f, 'R');
      add(m); }

    // ================= Cadena K (3) =================
    // 1: LATIGAZO LARGO historico (kick = lanzar cadena), reach x1.5.
    { Mv m(AttackId::Kick, "LATIGAZO LARGO", false); m.o.reachScale = 1.5f; m.clip("kick"); add(m); }
    // 2: EMBESTIDA DE HOMBRO (charge 13-17), armadura 1.
    { Mv m(AttackId::Kick2, "EMBESTIDA DE HOMBRO");
      m.row(14, 10, 8, 16, 460, .34f).armor(1)
       .arc(10, -76, 30, 60, 0, 0, 24).lunge(70)
       .clip("hombro").fx(ANTICIPA(1.2f)).fx(IMPACTO(1.2f)).fx(RECOBRO(1)).ghost(3)
       .over(42, 4, -40, -60, .9f);
      add(m); }
    // 3: GOLPE BAJO (smash 58-66 aplastado), derriba (trip).
    { Mv m(AttackId::Kick3, "GOLPE BAJO DE CADENA");
      m.row(16, 12, 7, 20, 300, .40f, 120)
       .with([](AttackDef& d) { d.low = true; d.knockdown = true; })
       .arc(8, -40, 40, 110, 10, -25, 20)
       .clip("smash").fx(ANTICIPA(1)).fx(IMPACTO(1)).fx(RECOBRO(.8f)).key('A', 0, 1.02f, .9f, 0, 0, 8).key('A', 1, 1.02f, .9f, 0, 0, 8)
       .over(15, 1, 90, -6, 1.2f);
      add(m); }
    // J J K: GANCHO DE CADENA AL CIELO (Lch 680).
    { Mv m(AttackId::KickLauncher, "GANCHO AL CIELO");
      m.row(13, 11, 7, 20, 90, .55f, 680)
       .with([](AttackDef& d) { d.launcher = true; d.jumpCancel = false; d.juggleCost = 2; })
       .arc(6, -80, 40, 104, -20, 70, 18)
       .clip("gancho_cielo").fx(ANTICIPA(1)).fx(UPPER(10)).fx(RECOBRO(1)).ghost(3)
       .over(21, 1, 60, -130, .8f, 'A', -60);
      add(m); }

    // ================= Normales de comando =================
    // >J JALON: lanza la cadena y atrae al enemigo 80 px (abre combo).
    { Mv m(AttackId::FwdPunch, "JALON");
      m.row(10, 12, 8, 14, -60, .42f)
       .with([](AttackDef& d) { d.canCombo = true; d.canCancel = true; d.comboWindow = .5f; d.heavy = false; })
       .arc(6, -82, 50, 140, -4, -4, 16).pull(80)
       .clip("jalon").fx(ANTICIPA(.8f)).fx(IMPACTO(.6f)).key('R', .2f, 1, 1, -6, -6, 0).ghost(2)
       .over(9, 3, 150, -82, .7f);
      add(m); }
    // >K HOMBRO: carga con ESTOCADA(90), armadura 1.
    { Mv m(AttackId::FwdKick, "HOMBRO");
      m.row(15, 12, 8, 16, 500, .36f).armor(1)
       .arc(10, -76, 30, 62, 0, 0, 26).lunge(90)
       .clip("charge").fx(ANTICIPA(1.2f)).fx(IMPACTO(1.3f)).fx(RECOBRO(1)).ghost(4, .025f)
       .over(42, 4, -50, -60, 1.0f);
      add(m); }
    // <J CADENA TRASERA: la cadena se dibuja detras del cuerpo (24-26, 31).
    { Mv m(AttackId::BackElbow, "CADENA TRASERA");
      m.row(12, 10, 8, 14, 360, .30f)
       .arc(-4, -84, 40, 110, 10, -20, 18, false, true)
       .clip("cadena_atras").fx(ANTICIPA(.6f)).fx(RECOBRO(.8f)).key('A', .5f, 1.04f, .97f, -6, -6, 0).ghost(3)
       .over(8, 3, -100, -80, .8f);
      add(m); }
    // <K REMOLINO CORTO: chain + GIRO(1), a los dos lados.
    { Mv m(AttackId::SpinKick, "REMOLINO CORTO");
      m.row(13, 10, 12, 16, 380, .32f)
       .arc(0, -78, 40, 104, 0, 0, 22, true)
       .clip("chain").fx(ANTICIPA(.8f)).fx(GIRO(1, 'A')).fx(RECOBRO(.8f)).ghost(3)
       .over(36, 1, 0, -70, 1.4f);
      add(m); }
    // vJ CADENA ASCENDENTE (33/35/76 arco hacia arriba), Lch 680.
    { Mv m(AttackId::Uppercut, "CADENA ASCENDENTE");
      m.row(14, 11, 7, 20, 80, .55f, 680)
       .with([](AttackDef& d) { d.launcher = true; d.juggleCost = 2; })
       .arc(6, -84, 40, 100, -30, 80, 18)
       .clip("cadena_alta").fx(ANTICIPA(1)).fx(UPPER(12)).fx(RECOBRO(1)).ghost(3)
       .over(26, 4, 50, -130, .8f);
      add(m); }
    // vK BARRIDA DE CADENA (smash sy .9 + 61-63 a ras de suelo), baja.
    { Mv m(AttackId::Sweep, "BARRIDA DE CADENA");
      m.row(12, 10, 8, 18, 220, .40f, 100)
       .with([](AttackDef& d) { d.low = true; d.knockdown = true; })
       .arc(0, -14, 50, 130, 0, 0, 18)
       .clip("barrida_cadena").fx(ANTICIPA(.6f)).fx(RECOBRO(.6f)).key('A', 0, 1.03f, .9f, -4, 0, 6).key('A', 1, 1.03f, .9f, -4, 0, 6)
       .over(15, 1, 110, -6, 1.2f).over(23, 1, 60, -4, 1.0f);
      add(m); }
    // vJ sobre un caido: PISOTON (cadena al suelo, OTG).
    { Mv m(AttackId::Stomp, "MARTILLAZO AL SUELO");
      m.row(14, 12, 6, 18, 120, .30f)
       .with([](AttackDef& d) { d.otg = true; d.low = true; })
       .arc(10, -30, 30, 90, 20, -40, 22)
       .clip("pisoton").fx(SALTITO(6, 'S')).fx(TEMBLOR(4, 26, 'A')).fx(RECOBRO(.6f))
       .over(15, 1, 60, -4, 1.3f);
      add(m); }
    // Dash: EMBESTIDA historica (dash_attack) y DashKick = charge + ESTOCADA(160), armadura 2.
    { Mv m(AttackId::DashAttack, "EMBESTIDA", false); m.clip("dash_attack"); add(m); }
    { Mv m(AttackId::DashKick, "EMBESTIDA DE CADENA");
      m.row(16, 8, 12, 18, 540, .36f).armor(2)
       .arc(-30, -76, 40, 110, 0, 0, 28).lunge(160)
       .clip("charge").fx(ANTICIPA(.8f)).fx(IMPACTO(1.2f)).fx(RECOBRO(1)).ghost(5, .025f)
       .over(42, 4, -60, -60, 1.1f);
      add(m); }

    // ================= Especiales =================
    // L / QCF+J: CADENA CON SIERRA (energy historico 42-44): misma fila y tiempos
    // (perfil skin 2 intacto). El motor resuelve EnergyWave SOLO por proyectil
    // (attackType Energy), asi que la sierra es una rafaga de 3 dientes cortos que
    // muerden hasta ~260 px (420 px/s x 0.62 s) en vez de la onda larga.
    { Mv m(AttackId::EnergyWave, "CADENA CON SIERRA", false);
      ProjectileSpec& p = m.o.projectile;
      p.enabled = true; p.count = 3; p.interval = F(5); p.speed = 420; p.life = .62f; p.damage = 8;
      p.origin = {70, -80}; p.scale = .8f; p.tint = Color{255, 140, 110, 255};
      m.clip("energy").fx(ANTICIPA(.8f)).over(8, 4, 120, -80, .9f);
      add(m); }
    // DP+K: GANCHO DE CADENA (chain ang -40 + UPPER(16)), Lch 760.
    { Mv m(AttackId::RisingUppercut, "GANCHO DE CADENA");
      m.row(16, 6, 12, 22, 100, .60f, 760)
       .with([](AttackDef& d) { d.launcher = true; d.juggleCost = 2; d.invulnTo = F(5); d.root = RootCurve{}; })
       .arc(6, -84, 40, 110, -40, 85, 20)
       .clip("gancho_cadena").fx(ANTICIPA(1)).fx(UPPER(16)).fx(RECOBRO(1)).ghost(4, .025f)
       .over(26, 4, 60, -140, .9f);
      m.o.root = RootCurve{};
      add(m); }
    v.push_back(Off(AttackId::Hurricane));   // lo reemplaza REMOLINO DE CADENA (QCB+J)
    v.push_back(Off(AttackId::DrillKick));   // no tiene patadas: lo reemplaza ANCLA (>L)

    // ================= Signatures =================
    // QCB+J: REMOLINO DE CADENA: 5 x 6, area r150, a los dos lados.
    { Mv m(AttackId::Signature1, "REMOLINO DE CADENA", true, AttackId::SpinKick);
      m.row(6, 10, 30, 18, 320, .30f, 160).hits(5, F(6)).sp(20)
       .with([](AttackDef& d) { d.cooldown = .4f; d.priority = 3; d.knockdown = true; d.breaksGuard = true; })
       .arc(0, -78, 50, 130, 0, 0, 26, true).area(150)
       .clip("remolino").fx(ANTICIPA(1)).fx(GIRO(3, 'A')).fx(RECOBRO(.8f)).ghost(4)
       .over(36, 1, 0, -70, 1.8f).over(22, 1, 0, -20, 1.4f);
      add(m); }
    // >L: ANCLA: atrae a todos en 220 px (pullPx 120) y remata con smash.
    { Mv m(AttackId::Signature2, "ANCLA", true, AttackId::Punch3);
      m.row(20, 14, 14, 22, 460, .50f, 240).sp(25)
       .with([](AttackDef& d) { d.cooldown = .5f; d.priority = 4; d.knockdown = true; d.breaksGuard = true; })
       .arc(6, -80, 60, 180, 10, -30, 24).area(220).pull(120)
       .clip("chain_throw").clip("smash", 2)
       .fx(ANTICIPA(1.2f)).fx(TEMBLOR(4, 24, 'A')).fx(RECOBRO(1)).ghost(3)
       .over(14, 2, 70, -8, 1.8f).over(9, 3, 160, -80, .9f, 'S');
      add(m); }
    // vL: EXPLOSIVO: area r170, 22, Lch 500, SP 30.
    { Mv m(AttackId::Signature3, "EXPLOSIVO", true, AttackId::Finisher);
      m.row(22, 16, 10, 22, 440, .50f, 500).sp(30)
       .with([](AttackDef& d) { d.cooldown = .6f; d.knockdown = true; d.launcher = true; d.juggleCost = 2; })
       .arc(0, -70, 40, 120, 0, 0, 34, true).area(170)
       .clip("explosivo").fx(LATIDO(.05f, 8, 'S')).fx(TINTE(Color{255, 150, 120, 255}, 0, 'A')).fx(RECOBRO(.8f))
       .over(23, 1, 0, -6, 2.8f).over(24, 2, 0, -70, 2.2f);
      add(m); }
    // Furia de siempre (rage_attack arreglado: 85, 86, 82) con temblor.
    { Mv m(AttackId::RageAttack, "FURIA DE CADENA", false); m.clip("rage_attack").fx(TEMBLOR(2, 28, 'A')); add(m); }
    // Finisher: APLASTAMIENTO FINAL (smash + explosivo), 44 de la fila.
    { Mv m(AttackId::Finisher, "APLASTAMIENTO FINAL", false);
      m.clip("smash").clip("explosivo", 1).fx(TEMBLOR(4, 24, 'A')).over(23, 1, 0, -6, 2.6f);
      add(m); }

    // ================= Fase 2 (solo el visual; F2.1 los activa) =================
    { Mv m(AttackId::Grab, "ESTRANGULAR CON CADENA", false); m.clip("estrangular").fx(ANTICIPA(.4f)); add(m); }
    { Mv m(AttackId::GrabKnee, "TIRON DE CADENA", false); m.clip("latigo").fx(IMPACTO(.6f)); add(m); }
    { Mv m(AttackId::ThrowForward, "LANZAR", false); m.clip("chain").key('A', 0, 1, 1, -10).key('A', 1, 1, 1, 25, 6).ghost(3); add(m); }
    { Mv m(AttackId::ThrowBack, "LANZAR ATRAS", false); m.clip("cadena_atras").fx(GIRO(.5f, 'A')).ghost(3); add(m); }
    { Mv m(AttackId::GuardCounter, "CONTRA DE CADENA", false); m.clip("latigo_largo").fx(IMPACTO(1)).ghost(3); add(m); }
    { Mv m(AttackId::WakeupAttack, "BARRIDA AL LEVANTARSE", false); m.clip("levantarse_cadena").fx(BARRIDA('A')); add(m); }
    return v;
}

MoveVisual StateClip(const char* clip, bool loop = false, float duration = 0) {
    MoveVisual m;
    m.clips[m.clipCount++] = ClipSeg{clip};
    m.loop = loop; m.duration = duration;
    return m;
}

MoveSet Build() {
    static const std::vector<MoveOverride> kOverrides = BuildOverrides();
    MoveSet m;
    m.name = "brakk";
    m.caps = CapAll & ~(CapJump | CapAir | CapHeavyJump);
    m.feel = FeelLevel::Full;
    const AttackId j[5] = {AttackId::Punch1, AttackId::Punch2, AttackId::PunchHook, AttackId::Punch3, AttackId::Chain5};
    for (int i = 0; i < 5; ++i) m.chainJ[i] = I(j[i]);
    m.chainJCount = 5;
    m.chainK[0] = I(AttackId::Kick); m.chainK[1] = I(AttackId::Kick2); m.chainK[2] = I(AttackId::Kick3);
    m.chainKCount = 3;
    // Habilidades 1-5: Cadena con sierra, Remolino, Ancla, Gancho de cadena, Explosivo.
    m.skillSlots[0] = I(AttackId::EnergyWave);
    m.skillSlots[1] = I(AttackId::Signature1);
    m.skillSlots[2] = I(AttackId::Signature2);
    m.skillSlots[3] = I(AttackId::RisingUppercut);
    m.skillSlots[4] = I(AttackId::Signature3);
    m.jumpVz = 560;   // sin CapJump no se usa; pesado por si acaso
    m.walkSpeedScale = 0.92f;   // lento
    m.overrides = kOverrides.data();
    m.overrideCount = static_cast<int>(kOverrides.size());
    // Victoria: el clip "victory" del jefe (79 -> 80) con un rebote y latido.
    m.victory = StateClip("victory", false, 1.2f);
    m.victory.fx.presets[m.victory.fx.presetCount++] = SALTITO(6, 'A');
    m.victory.fx.presets[m.victory.fx.presetCount++] = LATIDO(.02f, 3, 'A');
    return m;
}

}  // namespace

const MoveSet* MovesetFor_Brakk(int skin) {
    static const MoveSet kSet = Build();
    return skin == 2 ? &kSet : nullptr;
}

}  // namespace district_fury
