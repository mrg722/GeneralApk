// Moveset de las bestias BETA jugables (skins 27-37). F1.8.
//
// SOLDADO ESQUELETO 27, BESTIA EXCAVADORA 28, MOMIA CON ESCUDO 29, BESTIA ELEFANTE
// 30, AVE SANADORA 31, CENTAURO 32 / ROJO 33, BRUTO DE LA BOLA 34 / ROJO 35, MEDUSA
// 36 / ROJA 37.
//
// Los clips los derivan en memoria BetaDerive_Beasts.cpp con el NOMBRE CANONICO del
// movimiento (punch1, punch_hook, kick2, fwd_punch, sweep, signature1...), asi que
// aqui solo van numeros y FX: el motor elige el clip por nombre y la caja de ataque
// sale del cuadro (KF/BETA = ataque por clip).
//
// Limites del motor para ataques por clip (clipDriven, PEDIDO AL MOTOR): sin root
// motion, sin armadura por ventana, sin golpe multiple ni de area, dano x0.6 y
// habilidades 1-5 por la lista de habilidades del clip (no por skillSlots).
// El avance se hace en los cuadros (derivacion); la armadura total la dan las
// habilidades 1-5 y los especiales L / furia (superarmadura del motor).
#include "game/moves/MoveRegistry.h"
#include "game/player/PoseFxPresets.h"
#include <algorithm>
#include <vector>

namespace district_fury {
namespace {

using namespace posefx;

int Id(AttackId a) { return static_cast<int>(a); }

struct Spec {
    int skin;
    const char* name;
    const char* j[4];       // cadena J
    const char* k[3];       // cadena K
    const char* dash;       // DashAttack
    const char* sig[3];     // firmas
    float dmg;              // escala de dano (peso)
    float walk;
    Color tint;             // estelas / proyectil
    int proj;               // firma 2: 0 = golpe, 1 = proyectil
    int projCount;
    float projSpeed, projScale;
    bool arc;
};

const Spec kSpecs[] = {
    {27, "esqueleto", {"TAJO", "SEGUNDO TAJO", "TAJO ASCENDENTE", "TAJO FINAL"}, {"TAJO AL REVES", "ESTOCADA", "REMATE DE HUESO"},
     "TAJO DE CARRERA", {"TRES TAJOS", "ABANICO DE HUESOS", "LLUVIA DE TAJOS"}, 1.0f, 1.0f, {200, 150, 255, 255}, 1, 3, 560, 0.8f, false},
    {28, "excavadora", {"GUADANAZO", "GUADANAZO DOBLE", "SEGADA", "SEGADA FINAL"}, {"GUADANA AL REVES", "ZARPAZO", "DESGARRO"},
     "EMERGER", {"TORBELLINO ROJO", "DOBLE EMERSION", "TEMBLOR DE TIERRA"}, 1.1f, 1.0f, {255, 90, 70, 255}, 0, 1, 0, 1, false},
    {29, "momia", {"ESCUDAZO", "SEGUNDO ESCUDAZO", "EMPUJON", "TAJO PESADO"}, {"EMPUJON DE ESCUDO", "ESTOCADA", "GOLPE DE ESCUDO"},
     "CARGA CON ESCUDO", {"MURO DE ESCUDO", "VENDAS", "TAJO CRUZADO"}, 1.2f, 0.9f, {230, 210, 150, 255}, 1, 1, 320, 1.0f, false},
    {30, "elefante", {"MAZAZO", "SEGUNDO MAZAZO", "GOLPE DE LADO", "PISOTON"}, {"EMPUJON", "BARRIDO", "GOLPE DEMOLEDOR"},
     "EMBESTIDA", {"ESTAMPIDA", "DOBLE PISOTON", "TERREMOTO"}, 1.6f, 0.85f, {255, 200, 90, 255}, 0, 1, 0, 1, false},
    {31, "ave", {"PICOTAZO", "SEGUNDO PICOTAZO", "ALETEO", "ALETEO FINAL"}, {"ALETAZO", "PICADA", "CICLON DE PLUMAS"},
     "PICADA EN CARRERA", {"PICADA", "TORNADO DE PLUMAS", "VENDAVAL"}, 0.9f, 1.1f, {170, 255, 170, 255}, 1, 2, 520, 1.0f, false},
    {32, "centauro", {"ESTOCADA", "SEGUNDA ESTOCADA", "BARRIDO DE LANZA", "LANZAZO"}, {"COZ", "CARGA CORTA", "EMBATE"},
     "CARGA DE LANZA", {"LANZA DOBLE", "LLUVIA DE LANZAS", "PISOTON MAGICO"}, 1.3f, 1.05f, {255, 220, 120, 255}, 1, 3, 600, 0.9f, true},
    {33, "centauro rojo", {"ESTOCADA", "SEGUNDA ESTOCADA", "BARRIDO DE LANZA", "LANZAZO"}, {"COZ", "CARGA CORTA", "EMBATE"},
     "CARGA DE LANZA", {"LANZA DOBLE", "LLUVIA DE LANZAS", "PISOTON MAGICO"}, 1.4f, 1.05f, {255, 120, 90, 255}, 1, 3, 600, 0.9f, true},
    {34, "bruto", {"BOLAZO", "SEGUNDO BOLAZO", "BOLA AL SUELO", "AZOTE"}, {"BOLA AL SUELO", "ARRASTRE", "MARTILLAZO"},
     "ARRASTRE DE BOLA", {"REMOLINO DE BOLA", "LANZAMIENTO DE BOLA", "TERREMOTO"}, 1.5f, 0.85f, {200, 200, 210, 255}, 1, 1, 380, 1.4f, false},
    {35, "bruto rojo", {"BOLAZO", "SEGUNDO BOLAZO", "BOLA AL SUELO", "AZOTE"}, {"BOLA AL SUELO", "ARRASTRE", "MARTILLAZO"},
     "ARRASTRE DE BOLA", {"REMOLINO DE BOLA", "LANZAMIENTO DE BOLA", "TERREMOTO"}, 1.6f, 0.85f, {255, 130, 110, 255}, 1, 1, 380, 1.4f, false},
    {36, "medusa", {"COLETAZO", "COLETAZO RAPIDO", "LATIGAZO", "COLETAZO FINAL"}, {"AZOTE", "ESTOCADA", "ZARPAZO"},
     "COLETAZO CON ESTOCADA", {"MIRADA EN CONO", "COLETAZO GIRATORIO", "MIRADA PETRIFICANTE"}, 1.1f, 1.0f, {150, 255, 160, 255}, 0, 1, 0, 1, false},
    {37, "medusa roja", {"COLETAZO", "COLETAZO RAPIDO", "LATIGAZO", "COLETAZO FINAL"}, {"AZOTE", "ESTOCADA", "ZARPAZO"},
     "COLETAZO CON ESTOCADA", {"MIRADA EN CONO", "COLETAZO GIRATORIO", "MIRADA PETRIFICANTE"}, 1.2f, 1.0f, {255, 130, 120, 255}, 0, 1, 0, 1, false},
};

struct Set {
    MoveSet ms;
    std::vector<MoveOverride> ov;
};

// Tiempos de la fila = los de su clip derivado (golpe a `s`, activo 0.12, total `total`).
MoveOverride Ov(AttackId id, const char* name, float dmgScale, float s = 0.12f, float total = 0.55f) {
    MoveOverride o;
    o.id = Id(id);
    o.displayName = name;
    o.damageScale = dmgScale;
    o.replaceDef = true;
    o.def = GetAttack(id);
    o.def.name = name;
    o.def.startup = s; o.def.active = 0.10f; o.def.recovery = std::max(0.05f, total - s - 0.10f);
    o.def.requiresOverride = false;
    // Solo los remates derriban/lanzan; el resto deja al rival en pie para seguir la cadena.
    if (id != AttackId::Punch3 && id != AttackId::Kick3) { o.def.launch = 0; o.def.knockdown = false; }
    return o;
}
void Fx(MoveOverride& o, PresetUse p) { if (o.visual.fx.presetCount < 6) o.visual.fx.presets[o.visual.fx.presetCount++] = p; }
void Ghosts(MoveOverride& o, int n, Color c) { o.visual.fx.ghostCount = n; o.visual.fx.ghostTint = c; o.visual.fx.ghostInterval = 0.03f; }
void Key(MoveOverride& o, char ph, float v, float ang, float dx, float dy, float sx, float sy) {
    if (o.visual.fx.keyCount < 8) o.visual.fx.keys[o.visual.fx.keyCount++] = FxKey{ph, v, sx, sy, ang, dx, dy};
}
MoveOverride& Swing(MoveOverride& o, float k) { Fx(o, ANTICIPA(0.6f * k)); Fx(o, IMPACTO(k)); Fx(o, RECOBRO(0.8f * k)); return o; }

// Fila completa para una firma: parte de `base`, S/A/R en segundos.
AttackDef Row(AttackId base, const char* name, int dmg, float s, float a, float r) {
    AttackDef d = GetAttack(base);
    d.name = name; d.damage = dmg; d.startup = s; d.active = a; d.recovery = r;
    d.requiresOverride = false;
    return d;
}

void Build(const Spec& sp, Set& out) {
    std::vector<MoveOverride>& v = out.ov;
    const float k = sp.dmg;
    // ---- cadena J: A, B, C y el remate D (derriba) ----
    { MoveOverride o = Ov(AttackId::Punch1, sp.j[0], 1.6f * k); Swing(o, 1.0f); v.push_back(o); }
    { MoveOverride o = Ov(AttackId::Punch2, sp.j[1], 1.7f * k); Swing(o, 1.1f); v.push_back(o); }
    { MoveOverride o = Ov(AttackId::PunchHook, sp.j[2], 1.8f * k); Swing(o, 1.2f); Fx(o, SALTITO(5)); v.push_back(o); }
    { MoveOverride o = Ov(AttackId::Punch3, sp.j[3], 2.4f * k); Swing(o, 1.5f); Ghosts(o, 2, sp.tint); v.push_back(o); }
    // ---- cadena K ----
    { MoveOverride o = Ov(AttackId::Kick, sp.k[0], 1.7f * k); Swing(o, 1.0f); Fx(o, GIRO(1)); v.push_back(o); }
    { MoveOverride o = Ov(AttackId::Kick2, sp.k[1], 1.8f * k); Swing(o, 1.1f); Ghosts(o, 2, sp.tint); v.push_back(o); }
    { MoveOverride o = Ov(AttackId::Kick3, sp.k[2], 2.4f * k); Swing(o, 1.4f); v.push_back(o); }
    // ---- dash, comando, barrida ----
    { MoveOverride o = Ov(AttackId::DashAttack, sp.dash, 2.0f * k, 0.12f, 0.50f); Swing(o, 1.3f); Ghosts(o, 4, sp.tint); v.push_back(o); }
    { MoveOverride o = Ov(AttackId::DashKick, "DESLIZAMIENTO", 1.9f * k, 0.12f, 0.50f); Fx(o, BARRIDA('A')); Fx(o, RECOBRO(1)); Ghosts(o, 3, sp.tint); v.push_back(o); }
    { MoveOverride o = Ov(AttackId::FwdPunch, "ESTOCADA", 1.9f * k); Swing(o, 1.3f); Ghosts(o, 3, sp.tint); v.push_back(o); }
    { MoveOverride o = Ov(AttackId::FwdKick, "EMBATE", 1.9f * k); Swing(o, 1.2f); v.push_back(o); }
    { MoveOverride o = Ov(AttackId::SpinKick, "GIRO", 1.8f * k); Fx(o, ANTICIPA(0.6f)); Fx(o, GIRO(1)); Fx(o, RECOBRO(0.8f)); Ghosts(o, 3, sp.tint); v.push_back(o); }
    { MoveOverride o = Ov(AttackId::Sweep, "BARRIDA", 1.7f * k); Fx(o, ANTICIPA(0.6f)); Key(o, 'A', 0.5f, 0, 0, 0, 1.06f, 0.85f);
      Fx(o, RECOBRO(0.8f)); v.push_back(o); }
    // Sin sentido para una bestia de suelo: se apagan (caen al movimiento base).
    for (AttackId id : {AttackId::BackElbow, AttackId::Stomp}) {
        MoveOverride o; o.id = Id(id); o.enabled = false; v.push_back(o);
    }
    // ---- firmas (QCB+J, >L, vL) ----
    {   AttackDef d = Row(AttackId::Punch3, sp.sig[0], static_cast<int>(10 * k), 0.20f, 0.60f, 0.60f);
        d.spCost = 15; d.knockdown = true; d.heavy = true; d.cooldown = 0.5f;
        MoveOverride o = Ov(AttackId::Signature1, sp.sig[0], 1.4f * k); o.replaceDef = true; o.def = d;
        Swing(o, 1.3f); Ghosts(o, 3, sp.tint); v.push_back(o); }
    {   AttackDef d = Row(sp.proj ? AttackId::EnergyWave : AttackId::Punch3, sp.sig[1], static_cast<int>(11 * k), 0.25f, 0.20f, 0.25f);
        d.spCost = 20; d.cooldown = 0.6f; d.heavy = true; d.knockdown = !sp.proj;
        d.spawnsProjectile = sp.proj != 0;
        MoveOverride o = Ov(AttackId::Signature2, sp.sig[1], 1.5f * k); o.replaceDef = true; o.def = d;
        if (sp.proj) {
            o.projectile.enabled = true; o.projectile.count = sp.projCount; o.projectile.interval = 0.09f;
            o.projectile.speed = sp.projSpeed; o.projectile.scale = sp.projScale; o.projectile.tint = sp.tint;
            o.projectile.damage = static_cast<int>(9 * k); o.projectile.arc = sp.arc; o.projectile.origin = {70, -76};
        }
        Swing(o, 1.2f); Ghosts(o, 2, sp.tint); v.push_back(o); }
    {   AttackDef d = Row(AttackId::Punch3, sp.sig[2], static_cast<int>(13 * k), 0.25f, 0.30f, 0.35f);
        d.spCost = 25; d.knockdown = true; d.heavy = true; d.cooldown = 0.7f; d.shake = 1.5f;
        MoveOverride o = Ov(AttackId::Signature3, sp.sig[2], 1.6f * k); o.replaceDef = true; o.def = d;
        Swing(o, 1.4f); Fx(o, TEMBLOR(1.5f, 30, 'A')); Ghosts(o, 3, sp.tint);
        if (sp.skin == 36 || sp.skin == 37) Fx(o, TINTE(Color{150, 160, 150, 255}, 0, 'A'));   // mirada: tinte gris
        v.push_back(o); }

    MoveSet& m = out.ms;
    m.name = sp.name;
    m.caps = (CapAll & ~(CapJump | CapAir | CapHeavyJump | CapLauncher | CapSpecials | CapGrab | CapBackDash));
    m.feel = FeelLevel::Light;
    const AttackId j[4] = {AttackId::Punch1, AttackId::Punch2, AttackId::PunchHook, AttackId::Punch3};
    for (int i = 0; i < 4; ++i) m.chainJ[i] = Id(j[i]);
    m.chainJCount = 4;
    m.chainK[0] = Id(AttackId::Kick); m.chainK[1] = Id(AttackId::Kick2); m.chainK[2] = Id(AttackId::Kick3);
    m.chainKCount = 3;
    // Habilidades 1-5 (L, firma 1, firma 2, >J, furia) las da la lista de habilidades del clip (derivacion).
    m.skillSlots[0] = Id(AttackId::EnergyWave);
    m.skillSlots[1] = Id(AttackId::Signature1);
    m.skillSlots[2] = Id(AttackId::Signature2);
    m.skillSlots[3] = Id(AttackId::FwdPunch);
    m.skillSlots[4] = Id(AttackId::RageAttack);
    m.overrides = v.data();
    m.overrideCount = static_cast<int>(v.size());
    m.walkSpeedScale = sp.walk;
    // Bloqueo: la guardia ligeramente baja; victoria: rebote en bucle de 0.6 s.
    m.block.fx.keys[0] = FxKey{'A', 0.0f, 1.0f, 0.97f, 0.0f, 0.0f, 0.0f};
    m.block.fx.keyCount = 1; m.block.duration = 0.3f; m.block.loop = true;
    m.victory.fx.presets[0] = SALTITO(6, 'A'); m.victory.fx.presetCount = 1;
    m.victory.duration = 0.6f; m.victory.loop = true;
}

}  // namespace

const MoveSet* MovesetFor_Beasts(int skin) {
    static std::vector<Set*> sets = [] {
        std::vector<Set*> out;
        for (const Spec& sp : kSpecs) {
            Set* s = new Set();
            Build(sp, *s);
            s->ms.overrides = s->ov.data();
            out.push_back(s);
        }
        return out;
    }();
    if (skin < 27 || skin > 37) return nullptr;
    return &sets[static_cast<std::size_t>(skin - 27)]->ms;
}

}  // namespace district_fury
