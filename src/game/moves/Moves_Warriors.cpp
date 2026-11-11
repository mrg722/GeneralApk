// Moveset de RONIN (22), GUERRERO x4 armas (23-26) y RONIN-KRATOS x4 (38-41). F1.7.
//
// Son personajes KF/BETA (clipDriven): la animacion manda los golpes y la caja de
// ataque sale del cuadro dibujado (sin caja en los datos). Aqui solo van DATOS
// (MoveSet/MoveOverride de MoveTypes.h): que clip usa cada movimiento, numeros y
// FX. Los clips los derivan en memoria BetaDerive_Warriors.cpp (gk_dash_a,
// gk_habA_short, gk_habB, gk_habC, gk_combo2, gk_sweep; rk_fly, rk_launch, rk_sweep)
// y los .txt de data/kratos NO se tocan.
//
// La duracion de cada fila = la del clip elegido (x0.92; arranque 18 %, activo 22 %,
// recuperacion 60 %), como F1.6: asi la ventana activa (12-92 % del clip en
// clipDriven) cae sobre los cuadros que alcanzan al rival y el reloj de combate va
// con el visual. Por eso el MoveSet se arma con la ventana abierta (GetKfCharacter).
//
// Limites del motor para ataques por clip (clipDriven, PEDIDO AL MOTOR, igual que
// F1.6/F1.8): la caja no usa def.range/reachScale (sale del dibujo), no hay root
// motion ni golpe de area reales, dano x0.6, y EnergyWave/RageAttack/Finisher los
// resuelve Player::BeginAttack con los clips "special"/"super" del personaje (sus
// overrides de clip se ignoran: aqui solo se les pone nombre). Las habilidades 1-5
// (botones) salen de ref.abilityClips (los del .txt: 5 distintas). <J / <K
// (BackElbow/SpinKick) se apagan: la caja del cuadro solo cubre el frente.
#include "game/moves/MoveRegistry.h"
#include "game/beta/BetaDerive.h"
#include "game/CharacterVisual.h"
#include "game/lab/KfReference.h"
#include "game/player/PoseFxPresets.h"
#include <algorithm>
#include <map>
#include <memory>
#include <vector>

namespace district_fury {
namespace {

using namespace posefx;

int Id(AttackId a) { return static_cast<int>(a); }

// Duracion natural (s) de un clip del personaje (0.5 si no se conoce).
float Secs(const KfReference& kf, const char* clip) {
    auto it = kf.templ.namedClips.find(clip);
    if (it == kf.templ.namedClips.end() || it->second.frames.empty()) return 0.5f;
    float t = 0;
    for (std::size_t i = 0; i < it->second.frames.size(); ++i)
        t += i < it->second.durations.size() ? it->second.durations[i] : it->second.frameDuration;
    return std::max(0.2f, t);
}

struct Set {
    MoveSet ms;
    std::vector<MoveOverride> ov;
};

// Fila completa (replaceDef) con clip propio y tiempos tomados del clip.
MoveOverride Row(const KfReference& kf, AttackId id, AttackId base, const char* name, const char* clip, int dmg,
                 int sp = 0, bool knock = false, float launch = 0.0f, bool heavy = false, float cd = 0.0f) {
    MoveOverride o;
    o.id = Id(id);
    o.displayName = name;
    o.replaceDef = true;
    AttackDef d = GetAttack(base);
    d.name = name;
    d.damage = dmg;
    const float total = std::max(0.22f, Secs(kf, clip) * 0.92f);
    d.startup = total * 0.18f;
    d.active = total * 0.22f;
    d.recovery = total * 0.60f;
    d.spCost = sp;
    d.knockdown = knock;
    d.launch = launch;
    d.launcher = launch > 0.0f;
    d.heavy = heavy;
    d.cooldown = cd;
    d.multiHitInterval = 0.0f;
    d.cancelFrom = -1.0f;
    d.requiresOverride = false;
    o.def = d;
    o.visual.clips[0].clip = clip;
    o.visual.clipCount = 1;
    return o;
}

// Solo nombre (clip y numeros los pone el motor: EnergyWave/RageAttack/Finisher KF).
MoveOverride Named(AttackId id, const char* name) {
    MoveOverride o;
    o.id = Id(id);
    o.displayName = name;
    return o;
}

MoveOverride OffMove(AttackId id) {
    MoveOverride o;
    o.id = Id(id);
    o.enabled = false;
    return o;
}

void Fx(MoveOverride& o, PresetUse p) {
    if (o.visual.fx.presetCount < 6) o.visual.fx.presets[o.visual.fx.presetCount++] = p;
}
void Ghosts(MoveOverride& o, int n, Color c) {
    o.visual.fx.ghostCount = n;
    o.visual.fx.ghostTint = c;
    o.visual.fx.ghostInterval = 0.03f;
}
MoveOverride& Swing(MoveOverride& o, float k) {
    Fx(o, ANTICIPA(0.6f * k));
    Fx(o, IMPACTO(k));
    Fx(o, RECOBRO(0.8f * k));
    return o;
}

// -------------------------------------------------------------------------
// RONIN (22): artista marcial de ki. Cadenas con RODILLAZO y patada voladora,
// firmas de espada/rafaga/tormenta. Clips derivados: rk_fly, rk_launch, rk_sweep.
// -------------------------------------------------------------------------
void BuildRonin(const KfReference& kf, Set& out) {
    std::vector<MoveOverride>& v = out.ov;
    const Color ki{150, 210, 255, 255};
    // Cadena J: punch1, punch2, punch3, RODILLAZO (remate).
    { MoveOverride o = Row(kf, AttackId::Punch1, AttackId::Punch1, "GOLPE 1", "punch1", 6);
      Swing(o, 0.9f); v.push_back(o); }
    { MoveOverride o = Row(kf, AttackId::Punch2, AttackId::Punch2, "GOLPE 2", "punch2", 7);
      Swing(o, 1.0f); v.push_back(o); }
    { MoveOverride o = Row(kf, AttackId::PunchHook, AttackId::PunchHook, "GOLPE 3", "punch3", 9);
      Swing(o, 1.1f); v.push_back(o); }
    { MoveOverride o = Row(kf, AttackId::Punch3, AttackId::Punch3, "RODILLAZO", "ab4", 12, 0, true, 0, true);
      Swing(o, 1.3f); Ghosts(o, 2, ki); v.push_back(o); }
    // Cadena K: patada, RODILLAZO, patada voladora.
    { MoveOverride o = Row(kf, AttackId::Kick, AttackId::Kick, "PATADA", "kick", 7);
      Swing(o, 1.0f); v.push_back(o); }
    { MoveOverride o = Row(kf, AttackId::Kick2, AttackId::Kick2, "RODILLAZO", "ab4", 10);
      Swing(o, 1.1f); v.push_back(o); }
    { MoveOverride o = Row(kf, AttackId::Kick3, AttackId::Kick3, "PATADA VOLADORA", "rk_fly", 12, 0, true, 300, true);
      Fx(o, ANTICIPA(0.6f)); Fx(o, SALTITO(10)); Fx(o, RECOBRO(0.9f)); Ghosts(o, 3, ki); v.push_back(o); }
    // J J K: lanzador (salto 0-2, Lch 700).
    { MoveOverride o = Row(kf, AttackId::KickLauncher, AttackId::KickLauncher, "LANZADOR", "rk_launch", 9, 0, true, 700, true);
      o.def.jumpCancel = true; Fx(o, ANTICIPA(0.8f)); Fx(o, UPPER(14)); Ghosts(o, 3, ki); v.push_back(o); }
    // Comandos: >J palma de avance, >K patada voladora, vK barrida.
    { MoveOverride o = Row(kf, AttackId::FwdPunch, AttackId::FwdPunch, "PALMA DE AVANCE", "finisher", 10);
      Swing(o, 1.2f); Ghosts(o, 2, ki); v.push_back(o); }
    { MoveOverride o = Row(kf, AttackId::FwdKick, AttackId::FwdKick, "PATADA VOLADORA", "rk_fly", 10);
      Fx(o, SALTITO(8)); Ghosts(o, 3, ki); v.push_back(o); }
    { MoveOverride o = Row(kf, AttackId::Sweep, AttackId::Sweep, "BARRIDA", "rk_sweep", 8, 0, true);
      Fx(o, ANTICIPA(0.5f)); Fx(o, BARRIDA('A')); Fx(o, RECOBRO(0.8f)); Ghosts(o, 2, ki); v.push_back(o); }
    // Dash.
    { MoveOverride o = Row(kf, AttackId::DashAttack, AttackId::DashAttack, "EMBESTIDA", "dash_attack", 11);
      Swing(o, 1.2f); Ghosts(o, 3, ki); v.push_back(o); }
    { MoveOverride o = Row(kf, AttackId::DashKick, AttackId::DashKick, "DESLIZAMIENTO", "rk_sweep", 9);
      Fx(o, BARRIDA('A')); Ghosts(o, 3, ki); v.push_back(o); }
    // Especial de comando: DP+K = ESPADA RAPIDA (corta, lanzador).
    { MoveOverride o = Row(kf, AttackId::RisingUppercut, AttackId::RisingUppercut, "ESPADA RAPIDA", "ab1", 12, 15, true, 760, true, 0.4f);
      o.def.invulnTo = 0.10f; Fx(o, ANTICIPA(0.5f)); Fx(o, UPPER(16)); Ghosts(o, 4, ki); v.push_back(o); }
    // Firmas: QCB+J RAFAGA, >L ESPADA RAPIDA completa, vL TORMENTA (area).
    { MoveOverride o = Row(kf, AttackId::Signature1, AttackId::Hurricane, "RAFAGA", "ab3", 11, 20, true, 0, true, 0.6f);
      Fx(o, ANTICIPA(0.6f)); Ghosts(o, 3, ki); v.push_back(o); }
    { MoveOverride o = Row(kf, AttackId::Signature2, AttackId::Uppercut, "ESPADA RAPIDA", "ab1", 13, 25, true, 520, true, 0.7f);
      Fx(o, ANTICIPA(0.6f)); Ghosts(o, 4, ki); v.push_back(o); }
    { MoveOverride o = Row(kf, AttackId::Signature3, AttackId::DrillKick, "TORMENTA", "ab2", 14, 30, true, 0, true, 0.8f);
      o.areaRadius = 120.0f; o.def.shake = 1.6f; Fx(o, ANTICIPA(0.7f)); Fx(o, TEMBLOR(1.5f, 30, 'A')); Ghosts(o, 3, ki);
      v.push_back(o); }
    // Aereos: aire J = puno, aire K = patada voladora (cuadro 3 del salto).
    { MoveOverride o = Row(kf, AttackId::AirPunch, AttackId::AirPunch, "PUNO AEREO", "punch1", 7);
      Swing(o, 0.9f); v.push_back(o); }
    { MoveOverride o = Row(kf, AttackId::AirKick, AttackId::AirKick, "PATADA AEREA", "rk_fly", 10);
      Fx(o, SALTITO(6)); Ghosts(o, 2, ki); v.push_back(o); }
    // L / furia / remate (clips special/super por el motor): solo nombre.
    v.push_back(Named(AttackId::EnergyWave, "ONDA DE KI"));
    v.push_back(Named(AttackId::RageAttack, "FURIA DE KI"));
    v.push_back(Named(AttackId::Finisher, "REMATE FINAL"));
    // Sin sentido para RONIN (o caja trasera no soportada): caen al movimiento base.
    for (AttackId id : {AttackId::BackElbow, AttackId::SpinKick, AttackId::Uppercut, AttackId::Stomp,
                        AttackId::Hurricane, AttackId::DrillKick, AttackId::AirPunch2, AttackId::DiveKick,
                        AttackId::AirSpike})
        v.push_back(OffMove(id));

    MoveSet& m = out.ms;
    m.name = "ronin";
    m.feel = FeelLevel::Light;
    m.caps = CapAll & ~CapGrab;
    m.jumpVz = 720;
    m.chainJ[0] = Id(AttackId::Punch1); m.chainJ[1] = Id(AttackId::Punch2);
    m.chainJ[2] = Id(AttackId::PunchHook); m.chainJ[3] = Id(AttackId::Punch3);
    m.chainJCount = 4;
    m.chainK[0] = Id(AttackId::Kick); m.chainK[1] = Id(AttackId::Kick2); m.chainK[2] = Id(AttackId::Kick3);
    m.chainKCount = 3;
    m.walkSpeedScale = 1.05f;
    m.block.fx.keys[0] = FxKey{'A', 0.0f, 1.0f, 1.0f, -3.0f, 0.0f, 0.0f};
    m.block.fx.keyCount = 1; m.block.duration = 0.3f; m.block.loop = true;
    m.victory.fx.presets[0] = SALTITO(7, 'A'); m.victory.fx.presetCount = 1;
    m.victory.duration = 0.6f; m.victory.loop = true;
}

// -------------------------------------------------------------------------
// GUERRERO / RONIN-KRATOS (23-26, 38-41): 4 armas. Mismo esquema; numeros por arma.
// -------------------------------------------------------------------------
struct Weapon {
    const char* name;
    float dmg;        // escala de dano del arma
    float reach;      // alcance (documental: clipDriven no lo usa; PEDIDO AL MOTOR)
    float hitstop;    // escala de hitstop
    int garrasHits;   // golpes extra del 3er golpe J (garras)
    float rayoReach;  // alcance extra de la firma 3 (rayo)
    Color tint;
};

void BuildKratos(const KfReference& kf, const Weapon& w, Set& out) {
    std::vector<MoveOverride>& v = out.ov;
    const float k = w.dmg;
    auto scale = [&](MoveOverride& o) { o.def.hitstopScale = w.hitstop; o.reachScale = w.reach; };
    // Cadena J: atk1, atk2, atk3 (barrido), 1a mitad del ataque largo (dash_attack 0-10).
    { MoveOverride o = Row(kf, AttackId::Punch1, AttackId::Punch1, "GOLPE 1", "punch1", static_cast<int>(7 * k));
      Swing(o, 1.0f); scale(o); v.push_back(o); }
    { MoveOverride o = Row(kf, AttackId::Punch2, AttackId::Punch2, "GOLPE 2", "punch2", static_cast<int>(8 * k));
      Swing(o, 1.1f); scale(o); v.push_back(o); }
    { MoveOverride o = Row(kf, AttackId::PunchHook, AttackId::PunchHook, "BARRIDO", "punch3", static_cast<int>(10 * k));
      Swing(o, 1.2f); Ghosts(o, 2, w.tint); scale(o);
      if (w.garrasHits > 0) { o.hitCount = 1 + w.garrasHits; o.multiHitInterval = 0.09f; }   // GARRAS: multigolpe
      v.push_back(o); }
    { MoveOverride o = Row(kf, AttackId::Punch3, AttackId::Punch3, "REMATE", "gk_dash_a", static_cast<int>(13 * k), 0, true, 0, true);
      Swing(o, 1.4f); Ghosts(o, 3, w.tint); scale(o); v.push_back(o); }
    // Cadena K: patada (atk4) que abre a HABILIDAD A corta (remate que derriba).
    { MoveOverride o = Row(kf, AttackId::Kick, AttackId::Kick, "PATADA", "kick", static_cast<int>(9 * k));
      Swing(o, 1.2f); scale(o); v.push_back(o); }
    { MoveOverride o = Row(kf, AttackId::Kick2, AttackId::Kick2, "GOLPE DE ARMA", "gk_habA_short", static_cast<int>(11 * k), 0, true, 0, true);
      Swing(o, 1.2f); Ghosts(o, 2, w.tint); scale(o); v.push_back(o); }
    // Comandos: >J HABILIDAD A, >K remate, vK barrida, vJ embate.
    { MoveOverride o = Row(kf, AttackId::FwdPunch, AttackId::FwdPunch, "HABILIDAD A", "ab0", static_cast<int>(12 * k), 0, true, 0, true);
      Swing(o, 1.3f); Ghosts(o, 3, w.tint); scale(o); v.push_back(o); }
    { MoveOverride o = Row(kf, AttackId::FwdKick, AttackId::FwdKick, "PATADA DE AVANCE", "kick", static_cast<int>(10 * k));
      Swing(o, 1.2f); scale(o); v.push_back(o); }
    { MoveOverride o = Row(kf, AttackId::Sweep, AttackId::Sweep, "BARRIDA", "gk_sweep", static_cast<int>(9 * k), 0, true);
      Fx(o, ANTICIPA(0.5f)); Fx(o, BARRIDA('A')); Fx(o, RECOBRO(0.8f)); scale(o); v.push_back(o); }
    { MoveOverride o = Row(kf, AttackId::Uppercut, AttackId::Uppercut, "EMBATE", "gk_dash_a", static_cast<int>(11 * k), 0, true, 420, true);
      Fx(o, ANTICIPA(0.6f)); Fx(o, UPPER(12)); Ghosts(o, 2, w.tint); scale(o); v.push_back(o); }
    // Dash.
    { MoveOverride o = Row(kf, AttackId::DashAttack, AttackId::DashAttack, "EMBESTIDA", "dash_attack", static_cast<int>(12 * k));
      Swing(o, 1.3f); Ghosts(o, 3, w.tint); scale(o); v.push_back(o); }
    { MoveOverride o = Row(kf, AttackId::DashKick, AttackId::DashKick, "DESLIZAMIENTO", "gk_sweep", static_cast<int>(10 * k));
      Fx(o, BARRIDA('A')); Ghosts(o, 3, w.tint); scale(o); v.push_back(o); }
    // Especiales de comando: DP+K HABILIDAD B, QCB+K HABILIDAD C.
    { MoveOverride o = Row(kf, AttackId::RisingUppercut, AttackId::RisingUppercut, "HABILIDAD B", "gk_habB", static_cast<int>(13 * k), 18, true, 640, true, 0.5f);
      o.def.invulnTo = 0.10f; Fx(o, ANTICIPA(0.6f)); Fx(o, UPPER(16)); Ghosts(o, 4, w.tint); scale(o); v.push_back(o); }
    { MoveOverride o = Row(kf, AttackId::Hurricane, AttackId::Hurricane, "HABILIDAD C", "gk_habC", static_cast<int>(13 * k), 20, true, 0, true, 0.6f);
      Fx(o, ANTICIPA(0.6f)); Fx(o, GIRO(1, 'A')); Ghosts(o, 3, w.tint); scale(o); v.push_back(o); }
    // Firmas: COMBO 1, COMBO 2, energy (RAYO: mas alcance en la 3).
    { MoveOverride o = Row(kf, AttackId::Signature1, AttackId::Hurricane, "COMBO 1", "ab3", static_cast<int>(12 * k), 20, true, 0, true, 0.6f);
      Fx(o, ANTICIPA(0.6f)); Ghosts(o, 3, w.tint); scale(o); v.push_back(o); }
    { MoveOverride o = Row(kf, AttackId::Signature2, AttackId::Uppercut, "COMBO 2", "gk_combo2", static_cast<int>(14 * k), 25, true, 520, true, 0.8f);
      Fx(o, ANTICIPA(0.7f)); Ghosts(o, 4, w.tint); scale(o); v.push_back(o); }
    { MoveOverride o = Row(kf, AttackId::Signature3, AttackId::DrillKick, "ENERGIA", "energy", static_cast<int>(13 * k), 25, true, 0, true, 0.7f);
      o.reachScale = w.rayoReach; o.def.hitstopScale = w.hitstop; Fx(o, ANTICIPA(0.6f)); Ghosts(o, 3, w.tint); v.push_back(o); }
    // Aereos (el .txt trae jump): aire J/K.
    { MoveOverride o = Row(kf, AttackId::AirPunch, AttackId::AirPunch, "PUNO AEREO", "punch1", static_cast<int>(8 * k));
      Swing(o, 1.0f); scale(o); v.push_back(o); }
    { MoveOverride o = Row(kf, AttackId::AirKick, AttackId::AirKick, "PATADA AEREA", "kick", static_cast<int>(9 * k));
      Swing(o, 1.0f); scale(o); v.push_back(o); }
    // L / furia / remate (clips special/super por el motor): solo nombre.
    v.push_back(Named(AttackId::EnergyWave, "HABILIDAD B"));
    v.push_back(Named(AttackId::RageAttack, "FURIA"));
    v.push_back(Named(AttackId::Finisher, "GOLPE FINAL"));
    for (AttackId id : {AttackId::BackElbow, AttackId::SpinKick, AttackId::Stomp, AttackId::KickLauncher,
                        AttackId::Kick3, AttackId::DrillKick, AttackId::AirPunch2, AttackId::DiveKick,
                        AttackId::AirSpike})
        v.push_back(OffMove(id));

    MoveSet& m = out.ms;
    m.name = w.name;
    m.feel = FeelLevel::Light;
    m.caps = CapAll & ~(CapGrab | CapLauncher);   // sin J J K propio (lanzador por DP+K)
    m.jumpVz = 640;                                // guerrero pesado
    m.chainJ[0] = Id(AttackId::Punch1); m.chainJ[1] = Id(AttackId::Punch2);
    m.chainJ[2] = Id(AttackId::PunchHook); m.chainJ[3] = Id(AttackId::Punch3);
    m.chainJCount = 4;
    m.chainK[0] = Id(AttackId::Kick); m.chainK[1] = Id(AttackId::Kick2);
    m.chainKCount = 2;
    m.walkSpeedScale = 0.95f;
    m.block.fx.keys[0] = FxKey{'A', 0.0f, 1.0f, 1.0f, -2.0f, 0.0f, 0.0f};   // bloqueo inclinado -2
    m.block.fx.keyCount = 1; m.block.duration = 0.3f; m.block.loop = true;
    m.victory.fx.presets[0] = SALTITO(5, 'A'); m.victory.fx.presetCount = 1;
    m.victory.duration = 0.6f; m.victory.loop = true;
}

const Weapon kWeapons[4] = {
    {"guerrero espadas", 1.0f, 1.15f, 1.0f, 0, 1.0f, {255, 220, 120, 255}},   // ESPADAS: alcance, corte
    {"guerrero cestus", 1.15f, 0.90f, 1.3f, 0, 1.0f, {255, 170, 90, 255}},    // CESTUS: dano, hitstop
    {"guerrero rayo", 1.0f, 1.0f, 1.0f, 0, 1.3f, {170, 210, 255, 255}},       // RAYO: alcance en firma 3
    {"guerrero garras", 1.0f, 1.0f, 1.0f, 1, 1.0f, {210, 120, 255, 255}},     // GARRAS: multigolpe J J J
};

std::unique_ptr<Set> BuildWarrior(int skin, const KfReference& kf) {
    auto s = std::make_unique<Set>();
    if (skin == 22) {
        BuildRonin(kf, *s);
    } else if (skin >= 23 && skin <= 26) {
        BuildKratos(kf, kWeapons[skin - 23], *s);
    } else if (skin >= 38 && skin <= 41) {
        BuildKratos(kf, kWeapons[skin - 38], *s);
        s->ms.name = "ronin-kratos";
    }
    s->ms.overrides = s->ov.data();
    s->ms.overrideCount = static_cast<int>(s->ov.size());
    return s;
}

}  // namespace

const MoveSet* MovesetFor_Warriors(int skin) {
    const bool mine = skin == 22 || (skin >= 23 && skin <= 26) || (skin >= 38 && skin <= 41);
    if (!mine || !BetaDeriveEnabled()) return nullptr;   // en BETA los clips son los originales
    static std::map<int, std::unique_ptr<Set>> cache;
    auto it = cache.find(skin);
    if (it != cache.end()) return &it->second->ms;
    // Los clips solo existen con la ventana abierta (y ya derivados); sin ella no hay
    // con que nombrar movimientos: se reintenta en la siguiente consulta.
    const int roster = GetCharacterVisual(skin).kfRoster;
    if (roster < 0 || !IsWindowReady()) return nullptr;
    const KfReference& kf = GetKfCharacter(roster);
    if (!kf.loaded) return nullptr;
    return &(cache[skin] = BuildWarrior(skin, kf))->ms;
}

}  // namespace district_fury
