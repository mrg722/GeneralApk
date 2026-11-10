// Movesets de los personajes KF de la APK (skins 7-21). F1.6.
//
// Todo es DATOS sobre el arte de la APK (sin rellenos): cada movimiento nombra un
// clip REAL del personaje. Los clips salen de src/game/lab/KfReference.cpp:
//   - atk1..atk4, special, super, skill1..5 y sus variantes por subrango/velocidad
//     ("<clip>.a" primera parte, ".b" ultima parte, ".f" rapida x1.4, ".s" lenta x0.75);
//   - "act<N>" = la accion N de la APK (heroes: 20 SOMBRAS, 21 ESTALLIDO, 22 PILAR,
//     23 LLAMARADA, 24 FENIX, 25 TORNADO, 27 BARRIDA, 30 CIRCULO FUEGO, 36 VOLTERETA,
//     37 GIRO, 38 RODADA, 39 PATADA AEREA, 40 CAIDA, 41 TORBELLINO, 45 MORTAL,
//     46 ERUPCION, 47 MEDIA LUNA, 50 TAJO, 51 INFIERNO, 57 SALTO TIGRE...). Se busca por
//     numero de accion (estable) y no por el indice abN, que cambia segun el personaje.
// El movimiento sigue siendo clipDriven (la animacion manda los golpes y la caja);
// aqui se elige QUE animacion usa cada entrada y se ajusta la duracion del ataque a la
// del clip (timeScale) para que el reloj visual y el de combate vayan juntos.
//
// Movimientos que el arte no puede mostrar -> deshabilitados (enabled=false):
//   * enemigos (9-17): sin salto ni aereos (no tienen animacion de salto/aire de ataque),
//     sin Rising/Hurricane/Drill (CapSpecials apagado), sin Stomp.
//   * heroes transformados (18, 19): sin salto/aereos (su arte no trae clip de salto).
// EnergyWave / RageAttack / Finisher los resuelve Player::BeginAttack con los clips
// "special" y "super" del personaje (PEDIDO AL MOTOR: que respete override/finisher).
#include "game/moves/MoveRegistry.h"
#include "game/CharacterVisual.h"
#include "game/lab/KfReference.h"
#include <algorithm>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace district_fury {

namespace {

int Id(AttackId a) { return static_cast<int>(a); }

// Duracion natural (s) de un clip del personaje (0.5 si no se conoce).
float Secs(const KfReference& kf, const std::string& clip) {
    auto it = kf.templ.namedClips.find(clip);
    if (it == kf.templ.namedClips.end() || it->second.frames.empty()) return 0.5f;
    float t = 0;
    for (size_t i = 0; i < it->second.frames.size(); ++i)
        t += i < it->second.durations.size() ? it->second.durations[i] : it->second.frameDuration;
    return std::max(0.2f, t);
}

struct Mv {
    AttackId id;
    const char* name;
    std::string clip;
    float dmg = 1.0f;
};

// Base generica de las filas completas (Signature / Chain).
AttackId BaseFor(AttackId id) {
    switch (id) {
        case AttackId::Signature1: return AttackId::Hurricane;
        case AttackId::Signature2: return AttackId::Uppercut;
        case AttackId::Signature3: return AttackId::DrillKick;
        case AttackId::Chain5: return AttackId::Punch3;
        case AttackId::Chain6: return AttackId::Kick3;
        default: return id;
    }
}
bool NeedsRow(AttackId id) {
    return id == AttackId::Signature1 || id == AttackId::Signature2 || id == AttackId::Signature3 ||
           id == AttackId::Chain5 || id == AttackId::Chain6;
}

MoveOverride MakeOverride(const Mv& m) {
    MoveOverride o;
    o.id = Id(m.id);
    o.displayName = m.name;
    o.damageScale = NeedsRow(m.id) ? 1.0f : m.dmg;
    AttackDef d = GetAttack(BaseFor(m.id));
    if (NeedsRow(m.id)) {
        d.name = m.name;
        d.damage = std::max(1, static_cast<int>(d.damage * m.dmg));
        d.requiresOverride = false;
        o.replaceDef = true;
        o.def = d;
    }
    o.visual.clips[0].clip = nullptr;   // se rellena abajo con una cadena estable
    return o;
}

// Cadena de nombres estables (los MoveOverride guardan const char*).
struct Built {
    MoveSet set;
    std::vector<MoveOverride> overrides;
    std::vector<std::unique_ptr<std::string>> strings;
    const char* Keep(const std::string& s) {
        strings.push_back(std::make_unique<std::string>(s));
        return strings.back()->c_str();
    }
};

void Add(Built& b, const KfReference& kf, const Mv& m) {
    MoveOverride o = MakeOverride(m);
    o.visual.clips[0].clip = b.Keep(m.clip);
    o.visual.clipCount = 1;
    // Duracion del ataque = la del clip (el motor usa max(def, clip*0.92)): arranque 18 %,
    // activo 22 %, recuperacion el resto; la ventana de cancelacion cae hacia el 40 %.
    AttackDef d = o.replaceDef ? o.def : GetAttack(BaseFor(m.id));
    const float total = std::max(0.2f, Secs(kf, m.clip) * 0.92f);
    d.startup = total * 0.18f; d.active = total * 0.22f; d.recovery = total * 0.60f;
    d.multiHitInterval = 0.0f;
    d.cancelFrom = -1.0f;
    d.name = m.name;
    d.requiresOverride = false;
    o.replaceDef = true;
    o.def = d;
    b.overrides.push_back(o);
}

void Off(Built& b, AttackId id) {
    MoveOverride o;
    o.id = Id(id);
    o.enabled = false;
    b.overrides.push_back(o);
}

// ---- generador de candidatos de clip a partir de los ataques reales ----
std::vector<std::string> Candidates(const std::vector<std::string>& bases) {
    std::vector<std::string> v;
    for (const char* suf : {"", ".f", ".b", ".a", ".s"})
        for (const std::string& b : bases) v.push_back(b + suf);
    return v;
}

struct Slot { AttackId id; const char* name; float dmg; };
// Orden fijo de huecos: los primeros reciben los clips "limpios" (sin sufijo).
const Slot kSlots[] = {
    {AttackId::Punch1, "GOLPE 1", 0.9f},       {AttackId::Punch2, "GOLPE 2", 1.0f},
    {AttackId::PunchHook, "GANCHO", 1.1f},     {AttackId::Punch3, "REMATE", 1.2f},
    {AttackId::Kick, "PATADA", 1.0f},          {AttackId::Kick2, "PATADA DOBLE", 1.05f},
    {AttackId::Kick3, "PATADA FINAL", 1.15f},  {AttackId::KickLauncher, "PATADA ALTA", 1.1f},
    {AttackId::FwdPunch, "PUNO DE AVANCE", 1.1f}, {AttackId::FwdKick, "PATADA DE AVANCE", 1.1f},
    {AttackId::BackElbow, "CODAZO", 1.0f},     {AttackId::SpinKick, "GIRO", 1.05f},
    {AttackId::Uppercut, "GANCHO ASCENDENTE", 1.15f}, {AttackId::Sweep, "BARRIDA", 1.0f},
    {AttackId::DashAttack, "EMBESTIDA", 1.15f}, {AttackId::DashKick, "DESLIZAMIENTO", 1.05f},
    {AttackId::Signature1, "TECNICA 1", 1.3f}, {AttackId::Signature2, "TECNICA 2", 1.35f},
    {AttackId::Signature3, "TECNICA 3", 1.4f}, {AttackId::RisingUppercut, "ASCENSO", 1.25f},
    {AttackId::Hurricane, "HURACAN", 1.25f},   {AttackId::DrillKick, "TALADRO", 1.25f},
    {AttackId::Chain5, "COMBO 5", 1.25f},      {AttackId::Chain6, "COMBO 6", 1.3f},
    {AttackId::Stomp, "PISOTON", 1.0f},
};

struct Spec {
    const char* name;
    std::vector<std::string> bases;       // ataques reales distintos
    int slotCount;                        // cuantos huecos de kSlots (en orden) se habilitan
    float dmg = 1.0f;                     // escala general de dano
    float walk = 1.0f;
    unsigned caps = CapAll;
    FeelLevel feel = FeelLevel::None;
    int jchain = 4, kchain = 3;
    std::vector<Mv> fixed;                // movimientos con clip elegido a mano (pisan al generador)
    std::vector<const char*> names;       // nombres propios opcionales (por id de kSlots), nullptr = generico
};

void ApplyChains(Built& b, const std::vector<AttackId>& j, const std::vector<AttackId>& k) {
    MoveSet& m = b.set;
    for (size_t i = 0; i < j.size() && i < 6; ++i) m.chainJ[i] = Id(j[i]);
    m.chainJCount = static_cast<int>(std::min<size_t>(j.size(), 6));
    for (size_t i = 0; i < k.size() && i < 4; ++i) m.chainK[i] = Id(k[i]);
    m.chainKCount = static_cast<int>(std::min<size_t>(k.size(), 4));
}

void FinishDisabled(Built& b, bool keepAirSpecials, const std::vector<AttackId>& enabledIds) {
    static const AttackId kAll[] = {
        AttackId::PunchHook, AttackId::Kick2, AttackId::Kick3, AttackId::KickLauncher, AttackId::FwdPunch,
        AttackId::FwdKick, AttackId::BackElbow, AttackId::SpinKick, AttackId::Sweep, AttackId::Uppercut,
        AttackId::Stomp, AttackId::DashKick, AttackId::AirPunch, AttackId::AirPunch2, AttackId::AirKick,
        AttackId::DiveKick, AttackId::AirSpike, AttackId::RisingUppercut, AttackId::Hurricane, AttackId::DrillKick};
    (void)keepAirSpecials;
    for (AttackId id : kAll)
        if (std::find(enabledIds.begin(), enabledIds.end(), id) == enabledIds.end()) Off(b, id);
}

// ---- construccion de un personaje a partir de una Spec (enemigos y transformados) ----
std::unique_ptr<Built> BuildGeneric(const KfReference& kf, const Spec& s, const std::vector<AttackId>& chainJ,
                                    const std::vector<AttackId>& chainK) {
    auto b = std::make_unique<Built>();
    b->set.name = s.name;
    b->set.caps = s.caps;
    b->set.feel = s.feel;
    b->set.walkSpeedScale = s.walk;
    const std::vector<std::string> cand = Candidates(s.bases);
    std::vector<AttackId> enabled;
    int ci = 0;
    for (int i = 0; i < s.slotCount && i < static_cast<int>(sizeof(kSlots) / sizeof(kSlots[0])); ++i) {
        const Slot& sl = kSlots[i];
        Mv m{sl.id, sl.name, cand[static_cast<size_t>(ci++) % cand.size()], sl.dmg * s.dmg};
        for (const Mv& f : s.fixed)
            if (f.id == sl.id) { m = f; m.dmg = f.dmg * s.dmg; if (!m.name) m.name = sl.name; }
        Add(*b, kf, m);
        enabled.push_back(sl.id);
    }
    FinishDisabled(*b, false, enabled);
    ApplyChains(*b, chainJ, chainK);
    return b;
}

// ---- HEROES de plantilla KF (7, 20, 21: sprite 0; 8: sprite 2) ----
std::unique_ptr<Built> BuildHero0(const KfReference& kf, const char* name, bool trails) {
    (void)trails;
    auto b = std::make_unique<Built>();
    b->set.name = name;
    b->set.caps = CapAll;
    b->set.feel = FeelLevel::None;
    b->set.jumpVz = 720;
    const Mv list[] = {
        {AttackId::Punch1, "GOLPE 1", "atk1", 0.9f},
        {AttackId::Punch2, "GOLPE 2", "atk2", 1.0f},
        {AttackId::PunchHook, "GOLPE 3", "atk3", 1.1f},
        {AttackId::Punch3, "REMATE", "atk4", 1.2f},
        {AttackId::Chain5, "SOMBRAS", "act20", 1.25f},
        {AttackId::Kick, "PATADA", "atk4.f", 1.0f},
        {AttackId::Kick2, "PATADA AEREA", "act39", 1.1f},
        {AttackId::Kick3, "MEDIA LUNA", "act47", 1.2f},
        {AttackId::KickLauncher, "ESTALLIDO", "act21", 1.15f},
        {AttackId::FwdPunch, "TAJO", "act50", 1.15f},
        {AttackId::FwdKick, "PATADA DE AVANCE", "act39", 1.1f},
        {AttackId::BackElbow, "CODAZO", "atk2.b", 1.0f},
        {AttackId::SpinKick, "GIRO", "act37", 1.1f},
        {AttackId::Uppercut, "PILAR", "act22", 1.2f},
        {AttackId::Sweep, "BARRIDA", "act27", 1.0f},
        {AttackId::Stomp, "CAIDA", "act40", 1.0f},
        {AttackId::DashAttack, "VOLTERETA", "act36", 1.15f},
        {AttackId::DashKick, "RODADA", "act38", 1.1f},
        {AttackId::RisingUppercut, "FENIX", "act24", 1.3f},
        {AttackId::Hurricane, "TORBELLINO", "act41", 1.25f},
        {AttackId::DrillKick, "TORNADO", "act25", 1.25f},
        {AttackId::Signature1, "ERUPCION", "act46", 1.35f},
        {AttackId::Signature2, "CIRCULO DE FUEGO", "act30", 1.4f},
        {AttackId::Signature3, "LLAMARADA", "act23", 1.4f},
        {AttackId::AirPunch, "PUNO AEREO", "atk1.f", 0.9f},
        {AttackId::AirPunch2, "PUNO AEREO 2", "atk2.f", 1.0f},
        {AttackId::AirKick, "PATADA EN SALTO", "atk4.f", 1.0f},
        {AttackId::DiveKick, "PATADA PICADA", "act39", 1.1f},
        {AttackId::AirSpike, "MORTAL", "act45", 1.15f},
    };
    std::vector<AttackId> enabled;
    for (const Mv& m : list) { Add(*b, kf, m); enabled.push_back(m.id); }
    FinishDisabled(*b, true, enabled);
    ApplyChains(*b, {AttackId::Punch1, AttackId::Punch2, AttackId::PunchHook, AttackId::Punch3, AttackId::Chain5},
                {AttackId::Kick, AttackId::Kick2, AttackId::Kick3});
    return b;
}

std::unique_ptr<Built> BuildHero2(const KfReference& kf, const char* name) {
    auto b = std::make_unique<Built>();
    b->set.name = name;
    b->set.caps = CapAll;
    b->set.feel = FeelLevel::None;
    b->set.walkSpeedScale = 1.08f;   // agil
    const Mv list[] = {
        {AttackId::Punch1, "GOLPE 1", "atk1", 0.9f},
        {AttackId::Punch2, "GOLPE 2", "atk3", 1.0f},
        {AttackId::PunchHook, "GOLPE 3", "atk1.b", 1.05f},
        {AttackId::Punch3, "ESTALLIDO", "act21", 1.2f},
        {AttackId::Kick, "PATADA", "atk2", 1.0f},
        {AttackId::Kick2, "PATADA 2", "atk4", 1.05f},
        {AttackId::Kick3, "PATADA AEREA", "act39", 1.2f},
        {AttackId::KickLauncher, "ERUPCION", "act46", 1.15f},
        {AttackId::FwdPunch, "PUNO DE AVANCE", "atk3.b", 1.1f},
        {AttackId::FwdKick, "PATADA DE AVANCE", "atk4.f", 1.1f},
        {AttackId::BackElbow, "CODAZO", "atk2.b", 1.0f},
        {AttackId::SpinKick, "GIRO", "atk4.s", 1.1f},
        {AttackId::Uppercut, "GANCHO ASCENDENTE", "atk4.b", 1.15f},
        {AttackId::Sweep, "GIRO BAJO", "act37", 1.0f},
        {AttackId::DashAttack, "RODADA", "act38", 1.15f},
        {AttackId::DashKick, "DESLIZAMIENTO", "act39", 1.05f},
        {AttackId::RisingUppercut, "FENIX", "act24", 1.3f},
        {AttackId::Hurricane, "VOLTERETA", "act36", 1.25f},
        {AttackId::DrillKick, "LLAMARADA", "act23", 1.25f},
        {AttackId::Signature1, "PILAR", "act22", 1.35f},
        {AttackId::Signature2, "TORMENTA", "atk3.f", 1.35f},
        {AttackId::Signature3, "SOMBRAS", "act20", 1.4f},
        {AttackId::Chain5, "COMBO 5", "atk2.f", 1.2f},
        {AttackId::AirPunch, "PUNO AEREO", "atk1.f", 0.9f},
        {AttackId::AirPunch2, "PUNO AEREO 2", "atk3.f", 1.0f},
        {AttackId::AirKick, "PATADA EN SALTO", "atk2.f", 1.0f},
        {AttackId::DiveKick, "PATADA PICADA", "act39", 1.1f},
        {AttackId::AirSpike, "VOLTERETA AEREA", "act36", 1.15f},
    };
    std::vector<AttackId> enabled;
    for (const Mv& m : list) { Add(*b, kf, m); enabled.push_back(m.id); }
    FinishDisabled(*b, true, enabled);
    Off(*b, AttackId::Stomp);
    ApplyChains(*b, {AttackId::Punch1, AttackId::Punch2, AttackId::PunchHook, AttackId::Punch3, AttackId::Chain5},
                {AttackId::Kick, AttackId::Kick2, AttackId::Kick3});
    return b;
}

// ---- enemigos (9-17): sin salto/aereos/especiales de comando ----
constexpr unsigned kEnemyCaps = CapLauncher | CapSweep | CapCommandNormals | CapRun | CapBackDash;

std::unique_ptr<Built> BuildEnemy(const KfReference& kf, const char* name, std::vector<std::string> bases, float dmg,
                                  float walk, std::vector<Mv> fixed = {}) {
    Spec s;
    s.name = name; s.bases = std::move(bases); s.dmg = dmg; s.walk = walk;
    s.caps = kEnemyCaps;
    s.slotCount = 18;   // J x4, K x3, lanzador, 4 de comando, gancho, barrida, dash x2, Signature1-2
    s.fixed = std::move(fixed);
    auto b = BuildGeneric(kf, s, {AttackId::Punch1, AttackId::Punch2, AttackId::PunchHook, AttackId::Punch3},
                          {AttackId::Kick, AttackId::Kick2, AttackId::Kick3});
    Off(*b, AttackId::Stomp);
    return b;
}

// ---- transformados (18, 19) ----
std::unique_ptr<Built> BuildTransformed(const KfReference& kf, const char* name, FeelLevel feel) {
    Spec s;
    s.name = name;
    s.bases = {"atk1", "atk2", "atk3", "atk4", "special"};
    s.slotCount = 25;
    s.caps = CapAll & ~(CapJump | CapAir | CapGrab);
    s.feel = feel;
    s.fixed = {
        {AttackId::DashAttack, "ESTOCADA", "skill5", 1.15f},
    };
    auto b = BuildGeneric(kf, s, {AttackId::Punch1, AttackId::Punch2, AttackId::PunchHook, AttackId::Punch3,
                                  AttackId::Chain5, AttackId::Chain6},
                          {AttackId::Kick, AttackId::Kick2, AttackId::Kick3});
    return b;
}

std::unique_ptr<Built> BuildFor(int skin, const KfReference& kf) {
    switch (skin) {
        case 7: return BuildHero0(kf, "heroe KF", false);
        case 20: return BuildHero0(kf, "clon beta", true);
        case 21: return BuildHero0(kf, "clon rojo", true);
        case 8: return BuildHero2(kf, "heroina KF");
        case 9: return BuildEnemy(kf, "maton", {"atk2", "atk3", "atk1"}, 1.15f, 0.92f,
                                  {{AttackId::Punch3, "CABEZAZO", "atk1.a", 1.2f},
                                   {AttackId::Kick, "PISOTON", "atk1", 1.2f},
                                   {AttackId::Signature1, "CARGA", "special", 1.3f},
                                   {AttackId::DashAttack, "EMBESTIDA", "atk1.f", 1.2f}});
        case 10: return BuildEnemy(kf, "navajera", {"atk1", "atk2"}, 0.95f, 1.1f,
                                   {{AttackId::Punch3, "ESTOCADA", "atk2.f", 1.1f},
                                    {AttackId::Signature1, "NAVAJA", "special", 1.2f}});
        case 11: return BuildEnemy(kf, "soldado", {"atk1", "atk3", "atk2"}, 1.05f, 1.0f,
                                   {{AttackId::Kick, "CULATAZO", "atk2.s", 1.15f},
                                    {AttackId::Signature1, "GRANADA", "special", 1.25f}});
        case 12: return BuildEnemy(kf, "rubia", {"atk1", "atk3", "atk2"}, 0.95f, 1.1f,
                                   {{AttackId::Signature1, "FENIX", "special", 1.3f},
                                    {AttackId::DashAttack, "ACROBACIA", "atk3.f", 1.1f}});
        case 13: return BuildEnemy(kf, "gorra", {"atk1", "atk2", "atk4"}, 1.0f, 1.0f,
                                   {{AttackId::Signature1, "ESPECIAL", "special", 1.25f}});
        case 14: return BuildEnemy(kf, "peleador", {"atk1", "atk2", "atk3"}, 1.1f, 1.0f,
                                   {{AttackId::Kick, "PATADA FUERTE", "atk4", 1.15f},
                                    {AttackId::Signature1, "ESPECIAL", "special", 1.3f}});
        case 15: return BuildEnemy(kf, "cuchillero", {"atk1", "atk3", "atk2"}, 1.0f, 1.05f,
                                   {{AttackId::Signature1, "CUCHILLO", "special", 1.2f}});
        case 16: return BuildEnemy(kf, "jefe garra", {"atk1", "atk2", "atk3", "atk4"}, 1.25f, 0.85f,
                                   {{AttackId::Kick, "PATADA", "atk4", 1.2f},
                                    {AttackId::Signature1, "ESPECIAL", "special", 1.35f}});
        case 17: return BuildEnemy(kf, "bufona", {"atk1", "atk2", "atk3", "atk4"}, 1.1f, 1.1f,
                                   {{AttackId::Kick, "PATADA", "atk4", 1.15f},
                                    {AttackId::DashAttack, "BARRIDA", "act20", 1.1f},
                                    {AttackId::Signature1, "ESPECIAL", "special", 1.3f}});
        case 18: return BuildTransformed(kf, "heroe transformado", FeelLevel::None);
        case 19: return BuildTransformed(kf, "heroina transformada", FeelLevel::Light);
        default: return nullptr;
    }
}

}  // namespace

const MoveSet* MovesetFor_Kf(int skin) {
    if (skin < 7 || skin > 21) return nullptr;
    static std::map<int, std::unique_ptr<Built>> cache;
    auto it = cache.find(skin);
    if (it != cache.end()) return &it->second->set;
    // Los clips solo existen con la ventana abierta: sin ella no hay arte con que
    // nombrar movimientos (se vuelve a intentar en la siguiente consulta).
    const int roster = GetCharacterVisual(skin).kfRoster;
    if (roster < 0 || !IsWindowReady()) return nullptr;
    const KfReference& kf = GetKfCharacter(roster);
    if (!kf.loaded) return nullptr;
    std::unique_ptr<Built> built = BuildFor(skin, kf);
    if (!built) return nullptr;
    // <J / <K golpean hacia atras: la caja de un clip KF (clipDriven) solo cubre el frente
    // (retroceso <= 30 px), asi que esos dos no pueden conectar: se apagan y <J / <K
    // caen a la cadena normal. PEDIDO AL MOTOR: caja trasera para clipDriven.
    for (MoveOverride& o : built->overrides)
        if (o.id == Id(AttackId::BackElbow) || o.id == Id(AttackId::SpinKick)) {
            const int id = o.id;
            o = MoveOverride{};
            o.id = id;
            o.enabled = false;
        }
    built->set.overrides = built->overrides.data();
    built->set.overrideCount = static_cast<int>(built->overrides.size());
    return &(cache[skin] = std::move(built))->set;
}

}  // namespace district_fury
