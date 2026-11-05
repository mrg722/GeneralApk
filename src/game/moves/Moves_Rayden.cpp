// Moveset de RAYDEN (ORIGINAL), skin 0 (BRIEF F1.3): SOLO se le AGREGAN movimientos.
//
// Regla del usuario (inviolable): su arte (rayden_128.png, rayden_clean.png) y sus
// 19 clips del juego `rayden` en data/sprite_manifest.json no se tocan, y los 9
// ataques historicos (Punch1-3, Kick, EnergyWave, DashAttack, RageAttack,
// Finisher, Skill) y sus estados historicos (idle, walk, dash, block, hit,
// airborne, knockdown, getup, defeat, recovery) se dibujan igual que antes:
// - ningun override para los 9 ids historicos (los prueba RaydenAddOnlyTests);
// - el MoveSet NO define hit/knockdown/getup/walk/block (sin visual propio el
//   dibujo de esos estados es el de siempre); solo los estados NUEVOS (salto y
//   victoria);
// - los clips nuevos viven en data/manifest/rayden_moves.json, con nombres nuevos
//   y celdas ya existentes del atlas (0-21), y ninguno se llama como un clip que
//   el motor busca por su cuenta (victory / transform / turn / run / jump): asi
//   la transformacion (boton 6) sigue usando "energy" como siempre.
//
// Estilo: peleador callejero rapido con electricidad azul. Celdas del atlas
// (4x6, 128 px, pivote (64,126)): 8 jab con chispa, 9 gancho con arco, 10 patada
// media con arco, 11 patada alta, 12 aura electrica, 14 dash con estela,
// 16 guardia, 20 guardia baja, 21 bloqueo.
#include "game/moves/MoveRegistry.h"
#include "game/player/PoseFxPresets.h"
#include <vector>

namespace district_fury {
namespace {

using posefx::ANTICIPA;
using posefx::IMPACTO;
using posefx::RECOBRO;
using posefx::SALTITO;
using posefx::UPPER;
using posefx::GIRO;
using posefx::BARRIDA;
using posefx::TEMBLOR;
using posefx::LATIDO;
using posefx::TINTE;

constexpr Color kSpark{120, 200, 255, 255};     // estela azul electrica
constexpr Color kBolt{170, 225, 255, 255};      // superposicion (relampago)
constexpr float F(int frames) { return frames / 60.0f; }
int Id(AttackId a) { return static_cast<int>(a); }

FxKey Key(char ph, float v, float angle, float dx = 0, float dy = 0, float sx = 1, float sy = 1) {
    FxKey k; k.phase = ph; k.v = v; k.angle = angle; k.dx = dx; k.dy = dy; k.sx = sx; k.sy = sy;
    return k;
}

OverlayFx Overlay(int first, int count, float x, float y, float scale, char phase, float rot = 0, Color tint = kBolt) {
    OverlayFx o; o.first = first; o.count = count; o.x = x; o.y = y; o.scale = scale; o.phase = phase; o.rot = rot;
    o.tint = tint;
    return o;
}

// Constructor fluido de un override (visual + ajustes) para que la tabla se lea
// como la del brief.
struct Ov {
    MoveOverride o;
    explicit Ov(AttackId id, const char* name) { o.id = Id(id); o.displayName = name; }
    Ov& Clip(const char* c, float speed = 1) {
        ClipSeg s; s.clip = c; s.speed = speed; o.visual.clips[o.visual.clipCount++] = s; return *this;
    }
    Ov& Fx(PresetUse p) { PoseFx& f = o.visual.fx; if (f.presetCount < 6) f.presets[f.presetCount++] = p; return *this; }
    Ov& K(FxKey k) { PoseFx& f = o.visual.fx; if (f.keyCount < 8) f.keys[f.keyCount++] = k; return *this; }
    Ov& Ghost(int n, float interval = .03f, Color c = kSpark) {
        o.visual.fx.ghostCount = n; o.visual.fx.ghostInterval = interval; o.visual.fx.ghostTint = c; return *this;
    }
    Ov& Over(OverlayFx x) { PoseFx& f = o.visual.fx; if (f.overlayCount < 2) f.overlay[f.overlayCount++] = x; return *this; }
    Ov& Arc(LimbArc a) { o.hasArc = true; o.arc = a; return *this; }
    Ov& Root(float dx, float dz, float t0, float t1, Ease e) {
        o.root.dx = dx; o.root.dz = dz; o.root.t0 = t0; o.root.t1 = t1; o.root.ease = e; return *this;
    }
    Ov& Hits(int n, float interval) { o.hitCount = n; o.multiHitInterval = interval; return *this; }
    Ov& Def(const AttackDef& d) { o.replaceDef = true; o.def = d; return *this; }
};

// Fraccion del movimiento (S+A) sobre S+A+R, para curvas de root motion.
float UpTo(const AttackDef& d, bool includeActive = true) {
    const float total = d.startup + d.active + d.recovery;
    return total > 0 ? (d.startup + (includeActive ? d.active : 0)) / total : 1.0f;
}

// ---- Signatures (no tienen fila generica: se definen completas) ----

// QCB + J: PUNO RELAMPAGO. Se lanza 180 px como un rayo (outBack) y descarga
// 3 golpes de 6 + remate de 10 (4 golpes, 28 en total), SP 15, derriba.
AttackDef Sig1Def() {
    AttackDef d = GetAttack(AttackId::FwdPunch);
    d.name = "PUNO RELAMPAGO";
    d.damage = 7;
    d.startup = F(10); d.active = F(16); d.recovery = F(16);
    d.knockback = 460; d.hitstun = .40f; d.launch = 160;
    d.spCost = 15; d.rageGain = 10; d.priority = 3; d.cooldown = .30f;
    d.heavy = true; d.breaksGuard = true; d.knockdown = true; d.overhead = false;
    d.hitCount = 4; d.multiHitInterval = F(4);
    d.arc = LimbArc{8, -86, 56, 100, -4, 2, 22}; d.hasArc = true;
    // Anticipo quieto y embestida de 180 px durante el activo (outBack): frena
    // x0.3 al conectar, asi los 4 golpes caen sobre el rival.
    d.root = RootCurve{}; d.root.dx = 180; d.root.t0 = UpTo(d, false); d.root.t1 = UpTo(d); d.root.ease = Ease::OutBack;
    d.hitstop = .10f; d.shake = .14f;
    return d;
}

// >L: ONDA TRIPLE FASE. Anticipo -> aura que late -> empuje: una onda grande
// (x1.4) de 24, SP 25, rompe guardia.
AttackDef Sig2Def() {
    AttackDef d = GetAttack(AttackId::EnergyWave);
    d.name = "ONDA TRIPLE FASE";
    d.damage = 24;
    d.startup = F(18); d.active = F(12); d.recovery = F(14);
    d.spCost = 25; d.priority = 4; d.cooldown = .40f;
    d.knockback = 560; d.hitstun = .38f;
    d.spawnsProjectile = true; d.heavy = true; d.breaksGuard = true;
    return d;
}

// vL: DESCARGA. Estalla en aura electrica: golpe de area (r 110) de 14 que
// levanta (Lch 400), SP 20.
AttackDef Sig3Def() {
    AttackDef d = GetAttack(AttackId::SpinKick);
    d.name = "DESCARGA";
    d.damage = 14;
    d.startup = F(10); d.active = F(8); d.recovery = F(16);
    d.knockback = 380; d.hitstun = .50f; d.launch = 400;
    d.spCost = 20; d.priority = 4; d.cooldown = .40f;
    d.heavy = true; d.breaksGuard = true; d.launcher = true; d.juggleCost = 2;
    LimbArc a{0, -70, 40, 96, 0, 0, 34}; a.bothSides = true;
    d.arc = a; d.hasArc = true;
    d.hitsBehind = true;
    d.hitstop = .12f; d.shake = .20f;
    return d;
}

std::vector<MoveOverride> BuildOverrides() {
    std::vector<MoveOverride> v;
    auto add = [&](Ov& x) { v.push_back(x.o); };

    // ---------------- Cadena J: P1, P2 (historicos) -> GANCHO ELECTRICO -> P3 ----------------
    { Ov x(AttackId::PunchHook, "GANCHO ELECTRICO");
      x.Clip("punch_hook").Fx(ANTICIPA(.8f)).Fx(RECOBRO(.8f))
       .K(Key('A', 0, -8)).K(Key('A', 1, 10, 6)).K(Key('R', .4f, 5, 3))
       .Ghost(3).Over(Overlay(8, 3, 70, -84, .7f, 'A'));
      add(x); }

    // ---------------- Cadena K: Kick (historico) -> MEDIA LUNA -> TALON GIRATORIO ----------------
    { Ov x(AttackId::Kick2, "MEDIA LUNA");
      x.Clip("kick2").Fx(ANTICIPA(.8f)).Fx(GIRO(.5f, 'S')).Fx(RECOBRO(.8f))
       .K(Key('A', 0, -10)).K(Key('A', 1, 12, 6)).Ghost(3)
       .Over(Overlay(36, 1, 70, -70, .9f, 'A', -10));
      add(x); }
    { Ov x(AttackId::Kick3, "TALON GIRATORIO");
      x.Clip("kick3").Fx(ANTICIPA(1)).Fx(GIRO(1, 'S')).Fx(SALTITO(10)).Fx(IMPACTO(.8f)).Fx(RECOBRO(1))
       .Ghost(4).Over(Overlay(26, 4, 74, -104, .8f, 'A'));
      add(x); }

    // J J K: PATADA AL CIELO (lanzador, salto de persecucion).
    { Ov x(AttackId::KickLauncher, "PATADA AL CIELO");
      x.Clip("kick_launcher").Fx(ANTICIPA(1)).Fx(UPPER(8)).Fx(RECOBRO(1))
       .K(Key('A', 1, -6, 0, -4)).Ghost(3).Over(Overlay(21, 1, 50, -120, .7f, 'A', -60));
      add(x); }

    // ---------------- Normales de comando ----------------
    { Ov x(AttackId::FwdPunch, "DIRECTO EN CARRERA");   // root 70 px de la fila generica
      x.Clip("fwd_punch").Fx(ANTICIPA(1.2f)).Fx(IMPACTO(1)).Fx(RECOBRO(1))
       .K(Key('S', 1, -4, -4, 0, .96f, 1.02f)).Ghost(3).Over(Overlay(8, 4, 82, -84, .8f, 'A'));
      add(x); }
    { Ov x(AttackId::FwdKick, "PATADA DE AVANCE");      // root 60 px de la fila generica
      x.Clip("fwd_kick").Fx(ANTICIPA(1)).Fx(IMPACTO(.8f)).Fx(RECOBRO(1))
       .K(Key('A', 0, -6)).K(Key('A', 1, 6, 4)).Ghost(2);
      add(x); }
    // <J: codazo a la espalda SIN girar el facing: el dibujo gira, golpea hacia
    // atras y vuelve (GIRO de 1.25 vueltas sobre todo el movimiento: espejado
    // desde el 20 % hasta el 60 %, que cubre el activo).
    { Ov x(AttackId::BackElbow, "CODAZO TRASERO");
      x.Clip("back_elbow").Fx(GIRO(1.25f, '*')).Fx(IMPACTO(.6f));
      add(x); }
    { Ov x(AttackId::SpinKick, "PATADA GIRATORIA");     // bothSides de la fila generica
      x.Clip("spin_kick").Fx(ANTICIPA(.8f)).Fx(GIRO(1, 'A')).Fx(RECOBRO(.8f)).Ghost(3)
       .Over(Overlay(36, 1, 0, -66, 1.2f, 'A'));
      add(x); }
    { Ov x(AttackId::Uppercut, "GANCHO ASCENDENTE");    // Lch 680
      x.Clip("uppercut").Fx(ANTICIPA(1.2f)).Fx(UPPER(12)).Fx(RECOBRO(1))
       .K(Key('S', 1, 0, 0, 6, 1.04f, .9f)).Ghost(3).Over(Overlay(26, 4, 46, -124, .7f, 'A'));
      add(x); }
    { Ov x(AttackId::Sweep, "BARRIDA");                 // baja, derriba
      x.Clip("sweep").Fx(ANTICIPA(.6f)).Fx(BARRIDA('A')).Fx(RECOBRO(.6f))
       .K(Key('R', 0, -8, 0, 14, 1, .8f)).Over(Overlay(15, 1, 66, -6, .9f, 'A'));
      add(x); }
    { Ov x(AttackId::Stomp, "PISOTON");                 // OTG sobre un caido
      x.Clip("stomp").Fx(TEMBLOR(3, 28, 'A')).Fx(RECOBRO(.6f))
       .K(Key('S', 1, -4, 0, -12, .96f, 1.06f)).K(Key('A', .15f, 2, 2, 3, 1.06f, .9f))
       .Over(Overlay(15, 1, 48, -4, 1.0f, 'A')).Over(Overlay(10, 3, 48, -14, .6f, 'A'));
      add(x); }
    { Ov x(AttackId::DashKick, "DESLIZAMIENTO");        // root 120 px, bajo
      x.Clip("dash_kick").Fx(BARRIDA('A')).Fx(RECOBRO(.6f))
       .K(Key('S', 1, -6, 0, 6, 1.04f, .86f)).K(Key('R', 0, -6, 0, 10, 1, .8f))
       .Ghost(4, .025f).Over(Overlay(42, 4, -40, -24, .9f, 'A'));
      add(x); }

    // ---------------- Aire ----------------
    { Ov x(AttackId::AirPunch, "PUNO AEREO");
      // Golpes aereos en diagonal hacia abajo (alcanzan al rival desde el salto).
      x.Clip("air_punch").Arc(LimbArc{10, -60, 60, 96, -40, -55, 20}).K(Key('S', 1, 10)).K(Key('A', .3f, 28, 4)).K(Key('R', .5f, 16))
       .Over(Overlay(8, 2, 76, -84, .6f, 'A'));
      add(x); }
    { Ov x(AttackId::AirPunch2, "PUNO AEREO 2");
      x.Clip("air_punch2").Arc(LimbArc{10, -60, 64, 100, -35, -60, 20}).K(Key('S', 1, 12)).K(Key('A', .3f, 32, 4)).K(Key('R', .5f, 18)).Ghost(2)
       .Over(Overlay(9, 2, 66, -90, .6f, 'A'));
      add(x); }
    { Ov x(AttackId::AirKick, "PATADA VOLADORA");
      x.Clip("air_kick").K(Key('S', 1, 14)).K(Key('A', .2f, 25, 6)).K(Key('A', 1, 25, 6)).Ghost(2)
       .Over(Overlay(36, 1, 70, -54, .8f, 'A', 20));
      add(x); }
    { Ov x(AttackId::DiveKick, "PATADA PICADA");        // activa hasta tocar el suelo
      x.Clip("dive_kick").K(Key('S', 1, 30)).K(Key('A', .05f, 45, 4)).K(Key('A', 1, 45, 4)).Ghost(4, .025f)
       .Over(Overlay(42, 4, -30, -110, .8f, 'A', -45));
      add(x); }
    { Ov x(AttackId::AirSpike, "MARTILLO");             // groundBounce
      // Martillo de arriba abajo: el arco baja de +40 a -75 grados.
      x.Clip("air_spike").K(Key('S', 1, -30, 0, -6)).K(Key('A', 0, 35)).K(Key('A', 1, 70, 4))
       .Arc(LimbArc{8, -70, 80, 96, 40, -75, 22})
       .Ghost(3).Over(Overlay(26, 4, 54, -40, .8f, 'A'));
      add(x); }

    // ---------------- Especiales de comando ----------------
    // DP + K: PUNO TRUENO. Sube como un rayo con un pilar electrico (Lch 760,
    // invulnerable f1-5 de la fila generica).
    { Ov x(AttackId::RisingUppercut, "PUNO TRUENO");
      x.Clip("rising_uppercut").Fx(ANTICIPA(1)).Fx(UPPER(60)).Fx(RECOBRO(1)).Ghost(5, .025f)
       .Fx(TINTE(Color{200, 230, 255, 255}, 0, 'A'))
       .Over(Overlay(14, 1, 18, -70, 1.5f, 'A', 180)).Over(Overlay(26, 6, 40, -130, .9f, 'A'));
      add(x); }
    // QCB + K: CICLON. 5 golpes (6) a los dos lados, avanza 160 px con un salto de 40.
    { Ov x(AttackId::Hurricane, "CICLON");
      AttackDef g = GetAttack(AttackId::Hurricane);
      x.Clip("hurricane").Fx(ANTICIPA(.8f)).Fx(GIRO(3, 'A')).Fx(RECOBRO(.8f)).Ghost(4)
       .Root(160, 40, 0, UpTo(g), Ease::InOutSine)
       .Over(Overlay(36, 1, 0, -70, 1.3f, 'A')).Over(Overlay(22, 1, 0, -20, 1.0f, 'A'));
      add(x); }
    // QCF + K: TALADRO ELECTRICO. 8 x 3 avanzando 140 px girando como un taladro.
    { Ov x(AttackId::DrillKick, "TALADRO ELECTRICO");
      x.Clip("drill_kick").Fx(ANTICIPA(1)).Fx(GIRO(2, 'A')).Fx(RECOBRO(.8f))
       .K(Key('A', 0, 30, 0, -10)).K(Key('A', 1, 30, 0, -10)).Ghost(4, .025f)
       .Over(Overlay(34, 1, 70, -90, .9f, 'A')).Over(Overlay(42, 4, -36, -80, .9f, 'A'));
      x.Hits(3, 0);   // la fila generica ya reparte los 3 golpes
      add(x); }

    // ---------------- Signatures ----------------
    { Ov x(AttackId::Signature1, "PUNO RELAMPAGO");
      x.Def(Sig1Def()).Clip("sig1").Fx(ANTICIPA(1.4f)).Fx(IMPACTO(1)).Fx(RECOBRO(1)).Ghost(5, .025f)
       .K(Key('A', .1f, 8, 0, 0, 1.06f, .96f))
       .Over(Overlay(42, 4, -50, -80, 1.0f, 'A')).Over(Overlay(8, 6, 86, -86, .9f, 'A'));
      add(x); }
    { Ov x(AttackId::Signature2, "ONDA TRIPLE FASE");
      x.Def(Sig2Def()).Clip("sig2").Fx(ANTICIPA(1)).Fx(LATIDO(.05f, 6, 'S')).Fx(IMPACTO(1.2f)).Fx(RECOBRO(1))
       .Over(Overlay(24, 2, 0, -70, 1.1f, 'S')).Over(Overlay(26, 6, 70, -80, .9f, 'A'));
      x.o.projectile.enabled = true;
      x.o.projectile.damage = 24;
      x.o.projectile.scale = 1.4f;
      x.o.projectile.speed = 680;
      x.o.projectile.life = 1.3f;
      x.o.projectile.tint = Color{170, 220, 255, 255};
      add(x); }
    { Ov x(AttackId::Signature3, "DESCARGA");
      x.Def(Sig3Def()).Clip("sig3").Fx(LATIDO(.05f, 6, '*')).Fx(ANTICIPA(.6f)).Fx(RECOBRO(.8f))
       .Fx(TINTE(Color{190, 225, 255, 255}, 0, 'A'))
       .Over(Overlay(22, 1, 0, -18, 1.4f, 'A')).Over(Overlay(24, 2, 0, -70, 1.3f, 'A'));
      x.o.areaRadius = 110;
      add(x); }

    // ---------------- Fase 2 (F2.1 los activa; hoy solo el visual) ----------------
    // Sin replaceDef siguen apagados (requiresOverride): solo dejan su dibujo listo.
    { Ov x(AttackId::Grab, "AGARRE"); x.Clip("grab").Fx(ANTICIPA(.4f)); add(x); }
    { Ov x(AttackId::GrabKnee, "RODILLAZO"); x.Clip("grab_knee").Fx(SALTITO(6)).Fx(IMPACTO(.6f)); add(x); }
    { Ov x(AttackId::ThrowForward, "PROYECCION");
      x.Clip("throw_forward").K(Key('A', 0, -10)).K(Key('A', 1, 25, 6)).Ghost(3); add(x); }
    { Ov x(AttackId::ThrowBack, "SUPLEX");
      x.Clip("throw_back").Fx(GIRO(.5f, 'A')).K(Key('A', 1, -20, -6)).Ghost(3); add(x); }
    { Ov x(AttackId::GuardCounter, "CONTRA ELECTRICA");
      x.Clip("guard_counter").Fx(IMPACTO(1)).Ghost(3).Over(Overlay(8, 4, 80, -84, .7f, 'A')); add(x); }
    { Ov x(AttackId::WakeupAttack, "BARRIDA AL LEVANTARSE"); x.Clip("wakeup_attack").Fx(BARRIDA('A')); add(x); }
    return v;
}

const std::vector<MoveOverride>& Overrides() {
    static const std::vector<MoveOverride> kOverrides = BuildOverrides();
    return kOverrides;
}

MoveVisual StateClipVisual(const char* clip, float duration, bool loop) {
    MoveVisual mv;
    mv.clips[0].clip = clip; mv.clipCount = 1;
    mv.duration = duration; mv.loop = loop;
    return mv;
}

MoveSet Build() {
    MoveSet m;
    m.name = "rayden original";
    m.caps = CapAll;
    m.feel = FeelLevel::Full;
    // Cadena J de 4: P1, P2 (historicos), GANCHO ELECTRICO, REMATE (Punch3 historico).
    m.chainJ[0] = Id(AttackId::Punch1); m.chainJ[1] = Id(AttackId::Punch2);
    m.chainJ[2] = Id(AttackId::PunchHook); m.chainJ[3] = Id(AttackId::Punch3);
    m.chainJCount = 4;
    // Cadena K de 3: PATADA (historica), MEDIA LUNA, TALON GIRATORIO.
    m.chainK[0] = Id(AttackId::Kick); m.chainK[1] = Id(AttackId::Kick2); m.chainK[2] = Id(AttackId::Kick3);
    m.chainKCount = 3;
    // Habilidades 1-5 (antes repetian normales): ONDA (como siempre), PUNO
    // RELAMPAGO, PUNO TRUENO, CICLON, DESCARGA. El 6 (transformar) no cambia.
    m.skillSlots[0] = Id(AttackId::EnergyWave);
    m.skillSlots[1] = Id(AttackId::Signature1);
    m.skillSlots[2] = Id(AttackId::RisingUppercut);
    m.skillSlots[3] = Id(AttackId::Hurricane);
    m.skillSlots[4] = Id(AttackId::Signature3);
    m.jumpVz = 720;
    const std::vector<MoveOverride>& ov = Overrides();
    m.overrides = ov.data();
    m.overrideCount = static_cast<int>(ov.size());

    // Estados NUEVOS (no historicos): salto y victoria.
    // Salto: impulso agachado -> estirado (sy .9 al despegar, -10 grados); en la
    // caida, la guardia inclinada +8.
    m.jumpRise = StateClipVisual("jump_rise", .48f, false);
    m.jumpRise.fx.keys[0] = Key('A', 0, -10, 0, 0, 1.04f, .9f);
    m.jumpRise.fx.keys[1] = Key('A', .5f, -6);
    m.jumpRise.fx.keys[2] = Key('A', 1, -2);
    m.jumpRise.fx.keyCount = 3;
    m.jumpRise.fx.ghostCount = 2; m.jumpRise.fx.ghostTint = kSpark;
    m.jumpFall = StateClipVisual("jump_fall", .40f, false);
    m.jumpFall.fx.keys[0] = Key('A', 0, 4);
    m.jumpFall.fx.keys[1] = Key('A', 1, 8);
    m.jumpFall.fx.keyCount = 2;
    // Victoria: aura electrica con latido y luego la guardia, con un rebote leve.
    m.victory = StateClipVisual("victoria", 1.6f, false);
    m.victory.fx.presets[0] = SALTITO(6, 'A');
    m.victory.fx.presets[1] = LATIDO(.02f, 3, 'A');
    m.victory.fx.presetCount = 2;
    m.victory.fx.overlay[0] = Overlay(24, 2, 0, -70, 1.1f, 'A');
    m.victory.fx.overlayCount = 1;
    return m;
}

}  // namespace

const MoveSet* MovesetFor_Rayden(int skin) {
    static const MoveSet kSet = Build();
    return skin == 0 ? &kSet : nullptr;
}

}  // namespace district_fury
