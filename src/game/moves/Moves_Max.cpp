// Moveset de MAX (skin 6, ex-RAYDER; otro personaje, no es Rayden).
// Lo rellena SOLO el bloque F1.4. "Artista marcial de energia azul": cadena J de
// 5, cadena K de 4, normales de comando, aereos, especiales y 3 firmas con su
// propio arte (data/sprite_manifest.json, juegos "max" y "max_white").
//
// Arte: los clips nuevos los escribe tools/max/aplicar_manifiesto.py (celdas
// elegidas a ojo, nunca 153-203). Los arcos (LimbArc) salen de
// tools/max/derivar_arcos.py: punta del miembro dibujado en el cuadro de impacto,
// medida desde el hombro (0,-84) o la cadera (0,-58), con la escala del cuadro;
// el L1 de aqui = punta medida + 6..14 px (cubre el miembro + la mano).
//
// Orden de las cadenas: los ids siguen la posicion de la cadena por defecto
// (J: Punch1, Punch2, PunchHook, Punch3, Chain5; K: Kick, Kick2, Kick3, Chain6)
// para que moves_sheet y la IA encuentren cada eslabon; el CONTENIDO es el del
// brief: 4o J = media luna ascendente (lanza), 5o J = remate volador (el punch3
// historico), 4o K = la patada giratoria historica de 550c16e (derriba).
#include "game/moves/MoveRegistry.h"
#include "game/InputBuffer.h"
#include "game/player/PoseFxPresets.h"
#include <vector>

namespace district_fury {
namespace {

using namespace posefx;

constexpr float Fr(int frames) { return frames / 60.0f; }
constexpr int Id(AttackId a) { return static_cast<int>(a); }

const Color kBlue{110, 190, 255, 255};
const Color kPale{170, 220, 255, 255};

// Override completo: copia la fila `from` y la renombra.
MoveOverride Make(AttackId id, AttackId from, const char* name) {
    MoveOverride o;
    o.id = Id(id);
    o.replaceDef = true;
    o.def = GetAttack(from);
    o.def.name = name;
    o.def.requiresOverride = false;
    o.displayName = name;
    return o;
}
void Times(MoveOverride& o, int s, int a, int r) {
    o.def.startup = Fr(s); o.def.active = Fr(a); o.def.recovery = Fr(r);
}
void Hit(MoveOverride& o, int dmg, float kb, float hs, float lch) {
    o.def.damage = dmg; o.def.knockback = kb; o.def.hitstun = hs; o.def.launch = lch;
    o.def.heavy = dmg >= 16 || o.def.heavy;
    o.def.rageGain = dmg * 2 / 3 > 4 ? dmg * 2 / 3 : 4;
}
void Arc(MoveOverride& o, float px, float ph, float l0, float l1, float t0, float t1, float r,
         bool behind = false, bool both = false) {
    LimbArc a{px, ph, l0, l1, t0, t1, r};
    a.behind = behind; a.bothSides = both;
    o.hasArc = true; o.arc = a;
    o.def.arc = a; o.def.hasArc = true;
    const float reach = (px < 0 ? -px : px) + (l0 > l1 ? l0 : l1) + r;
    o.def.range = reach + 20.0f;
    o.def.boxWidth = (l0 > l1 ? l0 : l1) + r;
    o.def.boxForward = px + 0.5f * (l0 > l1 ? l0 : l1);
    o.def.boxCenterOffsetY = ph;
}
// Root motion hacia delante sobre S+A (fraccion del total), con salto dz.
void Root(MoveOverride& o, float dx, float dz = 0, Ease e = Ease::OutCubic) {
    const float total = o.def.startup + o.def.active + o.def.recovery;
    RootCurve r;
    r.dx = dx; r.dz = dz; r.t0 = 0; r.t1 = total > 0 ? (o.def.startup + o.def.active) / total : 1; r.ease = e;
    o.root = r; o.def.root = r;
}
void Clip(MoveOverride& o, const char* c, const char* fallback = nullptr) {
    o.visual.clips[o.visual.clipCount++] = ClipSeg{c};
    o.visual.clipFallback = fallback;
}
void Fx(MoveOverride& o, PresetUse p) { o.visual.fx.presets[o.visual.fx.presetCount++] = p; }
void Ghost(MoveOverride& o, int n, float interval = .03f, Color c = kBlue) {
    o.visual.fx.ghostCount = n; o.visual.fx.ghostInterval = interval; o.visual.fx.ghostTint = c;
}
void Overlay(MoveOverride& o, int first, int count, float x, float y, float scale = 1, char phase = 'A', float rot = 0) {
    OverlayFx& f = o.visual.fx.overlay[o.visual.fx.overlayCount++];
    f.first = first; f.count = count; f.x = x; f.y = y; f.scale = scale; f.phase = phase; f.rot = rot;
}

std::vector<MoveOverride> BuildOverrides() {
    std::vector<MoveOverride> v;

    // ================= Cadena J (5) =================
    {   // 1. JAB AZUL (punch1 de siempre, 43-46): tiempos historicos.
        MoveOverride o = Make(AttackId::Punch1, AttackId::Punch1, "JAB AZUL");
        Arc(o, 6, -84, 28, 50, -10, -8, 14);
        Clip(o, "punch1");
        Fx(o, IMPACTO(0.5f)); Fx(o, RECOBRO(0.5f));
        v.push_back(o);
    }
    {   // 2. CRUZADO (47-50).
        MoveOverride o = Make(AttackId::Punch2, AttackId::Punch2, "CRUZADO");
        Arc(o, 6, -84, 30, 62, -6, -14, 15);
        Clip(o, "punch2");
        Fx(o, ANTICIPA(0.5f)); Fx(o, IMPACTO(0.8f));
        v.push_back(o);
    }
    {   // 3. GANCHO DE ENERGIA (122-124 + 135): carga el guante y entra un paso.
        MoveOverride o = Make(AttackId::PunchHook, AttackId::PunchHook, "GANCHO DE ENERGIA");
        Times(o, 7, 5, 11);
        Hit(o, 13, 320, .34f, 0);
        Arc(o, 4, -88, 30, 56, -45, 10, 18);
        Root(o, 22);
        Clip(o, "punch_hook");
        Fx(o, ANTICIPA(0.8f)); Fx(o, IMPACTO(1.0f));
        Overlay(o, 8, 3, 50, -86, 0.6f);
        v.push_back(o);
    }
    {   // 4. MEDIA LUNA ASCENDENTE (113 -> 53 -> 58): lanza (Lch 420).
        MoveOverride o = Make(AttackId::Punch3, AttackId::KickLauncher, "MEDIA LUNA ASCENDENTE");
        Times(o, 9, 6, 16);
        Hit(o, 15, 120, .50f, 420);
        // Sin persecucion (jumpCancel): el J siguiente es el 5o eslabon, que
        // remata al enemigo en el aire.
        o.def.launcher = true; o.def.jumpCancel = false; o.def.juggleCost = 2;
        o.def.knockdown = false; o.def.canCombo = true; o.def.canCancel = true;
        o.def.hitstopScale = 1.1f;
        Arc(o, 0, -70, 40, 62, -40, 75, 20);
        Clip(o, "media_luna");
        Fx(o, ANTICIPA(0.7f)); Fx(o, UPPER(10)); Ghost(o, 2);
        Overlay(o, 36, 1, 40, -96, 0.8f, 'A', -30);
        v.push_back(o);
    }
    {   // 5. REMATE VOLADOR (punch3 historico 255-259, set grande x0.64): derriba.
        MoveOverride o = Make(AttackId::Chain5, AttackId::Punch3, "REMATE VOLADOR");
        Hit(o, 18, 520, .45f, 260);
        o.def.knockdown = true;
        Arc(o, 6, -80, 36, 64, 15, -20, 22);
        Root(o, 40);
        Clip(o, "punch3");
        Fx(o, ANTICIPA(1.0f)); Fx(o, IMPACTO(1.2f)); Fx(o, SALTITO(8)); Fx(o, RECOBRO(1.0f));
        Ghost(o, 3);
        Overlay(o, 26, 3, 58, -74, 0.8f);
        v.push_back(o);
    }

    // ================= Cadena K (4) =================
    {   // 1. PATADA FRONTAL (52 -> 118 -> 54): rapida, abre la cadena.
        MoveOverride o = Make(AttackId::Kick, AttackId::Kick2, "PATADA FRONTAL");
        Times(o, 7, 5, 10);
        Hit(o, 10, 280, .30f, 0);
        o.def.knockdown = false; o.def.wallBounce = false; o.def.canCombo = true; o.def.canCancel = true;
        Arc(o, 0, -58, 30, 56, -5, 12, 16);
        Clip(o, "kick_front");
        Fx(o, IMPACTO(0.7f));
        v.push_back(o);
    }
    {   // 2. PATADA ALTA (51 -> 57 -> 56).
        MoveOverride o = Make(AttackId::Kick2, AttackId::Kick2, "PATADA ALTA");
        Times(o, 8, 6, 12);
        Hit(o, 14, 360, .34f, 0);
        Arc(o, 0, -60, 40, 68, 5, 30, 18);
        Clip(o, "kick2");
        Fx(o, ANTICIPA(0.6f)); Fx(o, IMPACTO(0.9f));
        v.push_back(o);
    }
    {   // 3. MEDIA LUNA GIRATORIA (52 -> 53 -> 58): sube (Lch 300) y rebota en la pared.
        MoveOverride o = Make(AttackId::Kick3, AttackId::Kick3, "MEDIA LUNA GIRATORIA");
        Times(o, 11, 6, 18);
        Hit(o, 18, 560, .40f, 300);
        o.def.knockdown = false; o.def.wallBounce = true; o.def.canCombo = true; o.def.canCancel = true;
        Arc(o, 0, -70, 40, 62, 70, -10, 22);
        Clip(o, "kick3");
        Fx(o, ANTICIPA(0.8f)); Fx(o, IMPACTO(1.1f)); Ghost(o, 2);
        Overlay(o, 36, 1, 36, -90, 0.9f);
        v.push_back(o);
    }
    {   // 4. PATADA GIRATORIA DE CIERRE (kick historico 249-254 x0.64): derriba.
        MoveOverride o = Make(AttackId::Chain6, AttackId::Kick, "PATADA GIRATORIA DE CIERRE");
        Hit(o, 20, 520, .42f, 180);
        o.def.knockdown = true; o.def.hitstopScale = 1.3f;
        Arc(o, 0, -62, 38, 62, -10, 12, 22);
        Root(o, 18);
        Clip(o, "kick");
        Fx(o, ANTICIPA(1.0f)); Fx(o, IMPACTO(1.3f)); Fx(o, RECOBRO(1.0f));
        Overlay(o, 10, 2, 56, -64, 0.8f);
        v.push_back(o);
    }

    // ================= Normales de comando =================
    {   // J J K: MEDIA LUNA AL CIELO (56 -> 60 -> 77), Lch 720 + persecucion.
        MoveOverride o = Make(AttackId::KickLauncher, AttackId::KickLauncher, "MEDIA LUNA AL CIELO");
        Times(o, 8, 5, 16);
        Hit(o, 14, 80, .55f, 720);
        Arc(o, 0, -58, 40, 64, -40, 90, 20);
        Clip(o, "kick_launcher");
        Fx(o, ANTICIPA(0.6f)); Fx(o, UPPER(14)); Ghost(o, 3);
        Overlay(o, 36, 1, 30, -110, 1.0f, 'A', -60);
        v.push_back(o);
    }
    {   // >J: DIRECTO DE ENERGIA (121 -> 45 -> 46) + ESTOCADA(70).
        MoveOverride o = Make(AttackId::FwdPunch, AttackId::FwdPunch, "DIRECTO DE ENERGIA");
        Arc(o, 8, -84, 30, 56, -4, -8, 18);
        Root(o, 70);
        Clip(o, "fwd_punch");
        Fx(o, ANTICIPA(1.0f)); Fx(o, IMPACTO(1.2f)); Ghost(o, 3);
        Overlay(o, 10, 2, 56, -80, 0.7f);
        v.push_back(o);
    }
    {   // >K: PATADA VOLADORA (27 -> 30 -> 33) + ESTOCADA(90) con saltito.
        MoveOverride o = Make(AttackId::FwdKick, AttackId::FwdKick, "PATADA VOLADORA");
        Hit(o, 15, 430, .34f, 0);
        Arc(o, 0, -70, 40, 62, 25, 5, 20);
        Root(o, 90);
        Clip(o, "fwd_kick");
        Fx(o, SALTITO(16)); Fx(o, IMPACTO(0.8f)); Ghost(o, 3);
        v.push_back(o);
    }
    {   // <J: CODAZO CON MEDIA LUNA (54 -> 55 -> 56): la media luna sale a la espalda.
        MoveOverride o = Make(AttackId::BackElbow, AttackId::BackElbow, "CODAZO CON MEDIA LUNA");
        Arc(o, -6, -84, 30, 56, 170, 190, 18);
        o.def.boxForward = -40.0f;
        Clip(o, "back_elbow");
        Fx(o, IMPACTO(0.6f));
        v.push_back(o);
    }
    {   // <K: GIRATORIA (52 -> 86 -> 77) + GIRO(1), golpea a los dos lados.
        MoveOverride o = Make(AttackId::SpinKick, AttackId::SpinKick, "GIRATORIA AZUL");
        Arc(o, 0, -62, 40, 62, -10, 10, 20, false, true);
        Clip(o, "spin_kick");
        Fx(o, GIRO(1)); Ghost(o, 3);
        v.push_back(o);
    }
    {   // vJ: GANCHO BAJO-ALTO (61 -> 63 -> 60), Lch 680.
        MoveOverride o = Make(AttackId::Uppercut, AttackId::Uppercut, "GANCHO BAJO-ALTO");
        Hit(o, 14, 60, .60f, 680);
        Arc(o, 6, -70, 30, 60, -30, 85, 18);
        Clip(o, "uppercut");
        Fx(o, UPPER(10)); Ghost(o, 2);
        Overlay(o, 36, 1, 34, -100, 0.8f, 'A', -70);
        v.push_back(o);
    }
    {   // vK: BARRIDA AZUL (71 -> 75 -> 76): baja, derriba.
        MoveOverride o = Make(AttackId::Sweep, AttackId::Sweep, "BARRIDA AZUL");
        Arc(o, 0, -18, 30, 58, -4, -4, 16);
        Clip(o, "sweep");
        Fx(o, IMPACTO(0.6f)); Ghost(o, 2);
        Overlay(o, 21, 2, 46, -14, 0.7f);
        v.push_back(o);
    }
    {   // vJ con enemigo en el suelo: PISOTON (22 -> 39 -> 40), caja a ras de suelo.
        MoveOverride o = Make(AttackId::Stomp, AttackId::Stomp, "PISOTON");
        Clip(o, "stomp");
        Fx(o, SALTITO(10, 'S')); Fx(o, IMPACTO(0.6f));
        Overlay(o, 14, 1, 30, -10, 0.8f);
        v.push_back(o);
    }
    {   // Shift+J: EMBESTIDA (dash_attack 282-285 x0.64).
        MoveOverride o = Make(AttackId::DashAttack, AttackId::DashAttack, "EMBESTIDA AZUL");
        Arc(o, 6, -72, 36, 62, -5, 0, 22);
        Clip(o, "dash_attack");
        Ghost(o, 3); Fx(o, IMPACTO(0.8f));
        Overlay(o, 42, 4, -40, -60, 1.0f, '*');
        v.push_back(o);
    }
    {   // Shift+K: DESLIZAMIENTO (72 -> 74 -> 73): bajo, derriba.
        MoveOverride o = Make(AttackId::DashKick, AttackId::DashKick, "DESLIZAMIENTO");
        Arc(o, 0, -22, 30, 66, -5, -5, 16);
        Clip(o, "dash_kick");
        Ghost(o, 3);
        Overlay(o, 37, 1, -20, -10, 0.8f);
        v.push_back(o);
    }

    // ================= Aereos =================
    {
        MoveOverride o = Make(AttackId::AirPunch, AttackId::AirPunch, "PUNO AEREO");
        Arc(o, 10, -70, 78, 84, -50, -58, 18);
        Clip(o, "air_punch");
        Fx(o, IMPACTO(0.5f));
        v.push_back(o);
    }
    {
        MoveOverride o = Make(AttackId::AirPunch2, AttackId::AirPunch2, "PUNO AEREO 2");
        Arc(o, 10, -70, 80, 88, -52, -62, 18);
        Clip(o, "air_punch2");
        Fx(o, IMPACTO(0.6f));
        v.push_back(o);
    }
    {
        MoveOverride o = Make(AttackId::AirKick, AttackId::AirKick, "PATADA AEREA");
        Arc(o, 0, -50, 84, 90, -28, -32, 20);
        Clip(o, "air_kick");
        Ghost(o, 2);
        v.push_back(o);
    }
    {   // aire + vK: PICADA AZUL (32 -> 33) con ESTELA(4).
        MoveOverride o = Make(AttackId::DiveKick, AttackId::DiveKick, "PICADA AZUL");
        Arc(o, 0, -40, 36, 54, -45, -45, 16);
        Clip(o, "dive_kick");
        Ghost(o, 4, .03f);
        v.push_back(o);
    }
    {   // aire + >J: MARTILLO (23 -> 38).
        MoveOverride o = Make(AttackId::AirSpike, AttackId::AirSpike, "MARTILLO AZUL");
        Arc(o, 8, -70, 50, 96, 30, -80, 24);
        Clip(o, "air_spike");
        Fx(o, IMPACTO(0.8f));
        v.push_back(o);
    }

    // ================= Especiales =================
    {   // QCF+J / L: ONDA AZUL (energy + proyectil 176-180, de siempre).
        MoveOverride o = Make(AttackId::EnergyWave, AttackId::EnergyWave, "ONDA AZUL");
        Clip(o, "energy");
        Fx(o, ANTICIPA(0.6f));
        v.push_back(o);
    }
    {   // DP+K: SHORYU AZUL (129 -> 133, pilar de energia). Lch 780, invulnerable f1-5.
        MoveOverride o = Make(AttackId::RisingUppercut, AttackId::RisingUppercut, "SHORYU AZUL");
        Hit(o, 16, 200, .50f, 780);
        Arc(o, 6, -80, 40, 70, -40, 95, 20);
        Clip(o, "rising_uppercut");
        Ghost(o, 3);
        Overlay(o, 26, 3, 20, -120, 0.9f);
        v.push_back(o);
    }
    {   // QCB+K: HURACAN AEREO (30 -> 34) + hop, 6 golpes x5.
        MoveOverride o = Make(AttackId::Hurricane, AttackId::Hurricane, "HURACAN AEREO");
        Hit(o, 5, 260, .30f, 0);
        o.hitCount = 6; o.multiHitInterval = Fr(5);
        Arc(o, 0, -70, 40, 60, 0, 0, 20, false, true);
        Root(o, 150, 40, Ease::InOutSine);
        Clip(o, "hurricane");
        Fx(o, GIRO(2)); Ghost(o, 3);
        v.push_back(o);
    }
    {   // QCF+K: TALADRO (31 -> 86 -> 33) + GIRO(2), 8 golpes x3.
        MoveOverride o = Make(AttackId::DrillKick, AttackId::DrillKick, "TALADRO AZUL");
        Times(o, 10, 24, 18);
        Hit(o, 3, 300, .30f, 0);
        o.hitCount = 8; o.multiHitInterval = Fr(3);
        o.def.knockdown = true;
        Arc(o, 0, -60, 40, 62, -10, -10, 18);
        Root(o, 140, 0, Ease::InOutSine);
        Clip(o, "drill_kick");
        Fx(o, GIRO(2)); Ghost(o, 3);
        v.push_back(o);
    }

    // ================= Firmas =================
    {   // QCB+J: RAYO AZUL (126 -> 139 -> 140): rafaga de 5 orbes cada 4 f, 420 px.
        MoveOverride o = Make(AttackId::Signature1, AttackId::EnergyWave, "RAYO AZUL");
        Times(o, 12, 22, 14);
        Hit(o, 5, 260, .26f, 0);
        o.def.spCost = 20; o.def.cooldown = 0.25f; o.def.breaksGuard = true;
        o.projectile.enabled = true; o.projectile.count = 5; o.projectile.interval = Fr(4);
        o.projectile.speed = 950; o.projectile.life = 0.45f; o.projectile.damage = 5;
        o.projectile.origin = {58, -78}; o.projectile.scale = 0.8f; o.projectile.tint = kPale;
        Clip(o, "signature1");
        Fx(o, ANTICIPA(0.8f)); Fx(o, TEMBLOR(2, 28, 'A'));
        Overlay(o, 4, 1, 70, -78, 0.9f);
        v.push_back(o);
    }
    {   // >L: PILARES DE ENERGIA (130 -> 134): golpe de suelo de 120 a 320 px, 3 x10, Lch 500.
        MoveOverride o = Make(AttackId::Signature2, AttackId::RisingUppercut, "PILARES DE ENERGIA");
        Times(o, 14, 24, 16);
        Hit(o, 10, 120, .45f, 500);
        o.def.spCost = 25; o.def.cooldown = 0.3f; o.def.invulnTo = 0; o.def.root = RootCurve{0, 0, 0, 1};
        o.def.launcher = true; o.def.jumpCancel = false;
        o.hitCount = 3; o.multiHitInterval = Fr(8);
        Arc(o, 120, -60, 20, 200, 0, 0, 40);
        Clip(o, "signature2");
        Fx(o, LATIDO(0.03f, 6, 'A'));
        Overlay(o, 21, 2, 160, -30, 1.2f);
        Overlay(o, 26, 4, 280, -60, 1.3f);
        v.push_back(o);
    }
    {   // vL: NOVA (143 -> 145): area r150, 22, Lch 450.
        MoveOverride o = Make(AttackId::Signature3, AttackId::Finisher, "NOVA");
        Times(o, 16, 12, 18);
        Hit(o, 22, 420, .50f, 450);
        o.def.spCost = 30; o.def.cooldown = 0.6f; o.def.knockdown = true;
        o.areaRadius = 150;
        Clip(o, "signature3");
        Fx(o, LATIDO(0.04f, 8, 'S'));
        Overlay(o, 23, 1, 0, -6, 2.6f);
        Overlay(o, 24, 2, 0, -70, 2.0f);
        v.push_back(o);
    }
    {   // Furia y finisher de siempre (escala igualada en el manifiesto).
        MoveOverride o = Make(AttackId::RageAttack, AttackId::RageAttack, "FURIA AZUL");
        Clip(o, "rage_attack");
        Fx(o, TEMBLOR(2, 28, 'A'));
        v.push_back(o);
    }
    {
        MoveOverride o = Make(AttackId::Finisher, AttackId::Finisher, "GOLPE FINAL");
        Clip(o, "finisher");
        Fx(o, ANTICIPA(1.0f)); Fx(o, IMPACTO(1.2f));
        v.push_back(o);
    }

    // ================= Fase 2 (solo el visual; F2.1 los activa) =================
    // Agarre = cuadros de MAX (112-124 / 43-50), NUNCA 181-188 (otro personaje).
    const struct { AttackId id; const char* clip; } kPhase2[] = {
        {AttackId::Grab, "grab"}, {AttackId::GrabKnee, "grab_knee"}, {AttackId::ThrowForward, "throw_forward"},
        {AttackId::ThrowBack, "throw_back"}, {AttackId::GuardCounter, "guard_counter"},
        {AttackId::WakeupAttack, "wakeup_attack"},
    };
    for (const auto& p : kPhase2) {
        MoveOverride o;
        o.id = Id(p.id);
        Clip(o, p.clip);
        v.push_back(o);
    }
    return v;
}

const CancelRule kCancels[] = {
    // >K PATADA VOLADORA en hit -> K MEDIA LUNA GIRATORIA (rebota en la pared).
    {Id(AttackId::FwdKick), static_cast<int>(InputCommand::Kick), CancelDir::Any, true, false, Id(AttackId::Kick3)},
    // DESLIZAMIENTO en hit -> K MEDIA LUNA AL CIELO (lanzador).
    {Id(AttackId::DashKick), static_cast<int>(InputCommand::Kick), CancelDir::Any, true, false, Id(AttackId::KickLauncher)},
    // PATADA ALTA en hit -> >J DIRECTO DE ENERGIA.
    {Id(AttackId::Kick2), static_cast<int>(InputCommand::Punch), CancelDir::Fwd, true, false, Id(AttackId::FwdPunch)},
    // GIRATORIA en hit -> vK BARRIDA.
    {Id(AttackId::SpinKick), static_cast<int>(InputCommand::Kick), CancelDir::Down, true, false, Id(AttackId::Sweep)},
};

MoveVisual StateClip(const char* clip, bool loop = false) {
    MoveVisual m;
    m.clips[m.clipCount++] = ClipSeg{clip};
    m.loop = loop;
    return m;
}

MoveSet Build() {
    static const std::vector<MoveOverride> kOverrides = BuildOverrides();
    MoveSet m;
    m.name = "max";
    m.caps = CapAll;
    m.feel = FeelLevel::Full;
    const AttackId j[5] = {AttackId::Punch1, AttackId::Punch2, AttackId::PunchHook, AttackId::Punch3, AttackId::Chain5};
    for (int i = 0; i < 5; ++i) m.chainJ[i] = Id(j[i]);
    m.chainJCount = 5;
    const AttackId k[4] = {AttackId::Kick, AttackId::Kick2, AttackId::Kick3, AttackId::Chain6};
    for (int i = 0; i < 4; ++i) m.chainK[i] = Id(k[i]);
    m.chainKCount = 4;
    // Habilidades 1-5: Onda, Rayo azul, Shoryu azul, Pilares de energia, Nova.
    m.skillSlots[0] = Id(AttackId::EnergyWave);
    m.skillSlots[1] = Id(AttackId::Signature1);
    m.skillSlots[2] = Id(AttackId::RisingUppercut);
    m.skillSlots[3] = Id(AttackId::Signature2);
    m.skillSlots[4] = Id(AttackId::Signature3);
    m.overrides = kOverrides.data();
    m.overrideCount = static_cast<int>(kOverrides.size());
    m.cancels = kCancels;
    m.cancelCount = static_cast<int>(sizeof(kCancels) / sizeof(kCancels[0]));
    // Salto: 21-24 al subir, 27 (pose neutra) al caer; aterrizaje = clip landing.
    m.jumpRise = StateClip("jump_rise");
    m.jumpFall = StateClip("jump_fall", true);
    m.jumpVz = 720;
    return m;
}

}  // namespace

const MoveSet* MovesetFor_Max(int skin) {
    static const MoveSet kSet = Build();
    return skin == 6 ? &kSet : nullptr;
}

}  // namespace district_fury
