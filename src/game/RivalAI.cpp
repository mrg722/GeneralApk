#include "game/RivalAI.h"
#include "game/combat/AttackData.h"
#include "raylib.h"
#include <algorithm>
#include <cmath>

namespace district_fury {

namespace {
int Scaled(int basePct, float mult) {
    return std::clamp(static_cast<int>(basePct * mult), 0, 99);
}
bool Chance(int basePct, float mult) { return GetRandomValue(0, 99) < Scaled(basePct, mult); }

// Capacidades utiles del personaje (consultadas via el MoveSet efectivo).
bool CanAir(const Player& s) {
    const unsigned c = s.Moves().caps;
    return (c & CapJump) && (c & CapAir) &&
           (s.MoveEnabled(AttackId::AirPunch) || s.MoveEnabled(AttackId::AirKick) ||
            s.MoveEnabled(AttackId::AirSpike) || s.MoveEnabled(AttackId::DiveKick));
}
bool HasSp(const Player& s, AttackId id) { return s.sp >= GetAttack(id).spCost; }

int PickReadySkill(const Player& s) {
    const int start = GetRandomValue(0, 4);
    for (int k = 0; k < 5; ++k) {
        const int slot = (start + k) % 5;
        if (s.SkillReady(slot)) return slot;
    }
    return -1;
}
}  // namespace

void RivalAI::Reset() {
    script_.clear();
    scriptPos_ = 0;
    decide_ = 0.6f;
    blockTimer_ = 0.0f;
    retreatTimer_ = 0.0f;
    farTimer_ = 0.0f;
    mustClose_ = false;
    pageFlips_ = 0;
    lastPlay = AttackId::None;
    playChanged = false;
}

void RivalAI::Add(const PlayerInput& f, int frames) {
    for (int i = 0; i < frames && script_.size() < 96; ++i) script_.push_back(f);
}
void RivalAI::SetPlay(AttackId id) { lastPlay = id; playChanged = true; }

// IA 2.0 + F2.2: la forma de decidir es la misma para todos (bloquea, se acerca,
// elige una ruta, se retira), pero (a) el PERFIL de cada personaje cambia cuanto
// bloquea/se acerca/abusa de habilidades/le gustan los aereos o agarres, y (b)
// la RUTA elegida siempre esta dentro de lo que ESE personaje puede hacer: un
// pesado sin salto nunca intenta un aereo, uno sin especiales no hace QCF, etc.
// Las rutas de varios frames (especial por comando, salto con aereo, lanzador +
// persecucion, cadena) se guardan como un guion de entradas que se reproduce un
// frame por llamada, asi la secuencia de direcciones de un QCF sale bien.
PlayerInput RivalAI::Think(const Player& self, const Player& target, float dt) {
    playChanged = false;
    if (self.state == PlayerState::Defeat || target.state == PlayerState::Defeat) {
        ClearScript();
        return PlayerInput{};
    }
    // Reproduce el guion pendiente (una ruta ya decidida).
    if (HasScript()) {
        const PlayerInput f = script_[scriptPos_++];
        if (!HasScript()) ClearScript();
        return f;
    }

    const AiProfile profile = profileOverride ? *profileOverride : ProfileFor(self.skin);
    const float dx = target.position.x - self.position.x;
    const float dy = target.position.y - self.position.y;
    const float dist = std::fabs(dx);
    const float dir = dx >= 0.0f ? 1.0f : -1.0f;
    // "Remontada": con poca vida se vuelve algo mas agresivo (dificultad dinamica
    // suave que no depende de un contador de ronda externo).
    const float hpRatio = self.maxHp > 0 ? static_cast<float>(self.hp) / self.maxHp : 1.0f;
    const float urgency = hpRatio < 0.35f ? 1.25f : 1.0f;
    decide_ -= dt * profile.aggression * urgency;
    blockTimer_ -= dt;
    retreatTimer_ -= dt;

    // Tiempo seguido fuera de rango de pelea (cuenta aunque este atacando a rango).
    // Pasado un limite el rival se COMPROMETE a entrar (ignora las opciones a
    // distancia) hasta pegarse: asi un zoner no empuja al jugador eternamente con
    // proyectiles y siempre acaba habiendo intercambio (y es alcanzable).
    if (dist > profile.preferredRange) farTimer_ += dt; else farTimer_ = 0.0f;
    if (dist < 90.0f) mustClose_ = false;
    else if (farTimer_ > 1.8f) mustClose_ = true;

    PlayerInput in;

    // 1) Bloqueo reactivo cuando el jugador ataca de cerca (los defensivos mas,
    //    los pesados/agresivos casi nunca). Se guioniza unos frames.
    if (target.state == PlayerState::Attack && dist < 155.0f && decide_ <= 0.0f &&
        Chance(30, profile.blockChance / urgency)) {
        PlayerInput b;
        b.block = true;
        Add(b, GetRandomValue(14, 26));
        decide_ = 0.3f;
        const PlayerInput f = script_[scriptPos_++];
        if (!HasScript()) ClearScript();
        return f;
    }

    // 2) Retirada tras una ruta (no se queda pegado); los defensivos/soporte mas.
    if (retreatTimer_ > 0.0f) {
        in.moveX = -dir;
        in.moveY = dy > 0 ? -0.3f : 0.3f;
        return in;
    }

    // Alinea el carril con el jugador.
    if (std::fabs(dy) > 14.0f) in.moveY = dy > 0 ? 1.0f : -1.0f;
    if (self.state == PlayerState::Attack) return in;   // deja terminar la animacion

    // 3) Lejos: acercarse (a veces con dash o salto) y castigar a rango.
    if (dist > profile.preferredRange || mustClose_) {
        in.moveX = dir;
        // Comprometido a entrar: solo caminar/dash hacia el jugador, sin acciones a
        // rango que lo empujen mas lejos.
        if (mustClose_) {
            if (decide_ <= 0.0f && self.dashCooldown <= 0.0f && dist > 160.0f) {
                in.dash = true; decide_ = 0.35f;
            }
            return in;
        }
        if (decide_ <= 0.0f) {
            // Habilidad a rango (los zoners/soporte abusan de esto; es como peleaba
            // la IA 1.0). Mantiene el uso de habilidades que exige all_modes_check.
            if (dist < 560.0f && Chance(26, profile.skillChance)) {
                const int slot = PickReadySkill(self);
                if (slot >= 0) {
                    in.skill = slot; decide_ = 1.0f; SetPlay(AttackId::Skill);
                    return in;
                }
            }
            if (dist < 540.0f && Chance(20, profile.skillChance) && BuildSpecial(self, dir, true)) {
                decide_ = 0.9f;
                return TakeFront();
            }
            if (dist < 440.0f && Chance(16, profile.airChance) && BuildJumpIn(self, dir)) {
                decide_ = 0.9f;
                return TakeFront();
            }
            if ((self.Moves().caps & CapRun) == 0 && dist > 360.0f && Chance(25, profile.dashChance)) {
                in.dash = true;
                decide_ = 0.75f;
                return in;
            }
        }
        return in;
    }

    if (decide_ > 0.0f) {
        // Esperando el proximo gesto: mantiene una distancia de intercambio (~60-95
        // px) en vez de quedarse lejos, asi es alcanzable y puede castigar. Los
        // zoners/soporte prefieren mas aire (su preferredRange es grande).
        const float pocket = std::min(profile.preferredRange, 95.0f);
        if (dist > pocket) in.moveX = dir;
        else if (dist < 58.0f) in.moveX = -dir;
        return in;
    }

    // 4) Cerca y listo para decidir: una ruta del moveset del personaje.
    ChooseClosePlay(self, target, profile, dir, dy, dist);
    return HasScript() ? TakeFront() : in;
}

PlayerInput RivalAI::TakeFront() {
    if (!HasScript()) return PlayerInput{};
    const PlayerInput f = script_[scriptPos_++];
    if (!HasScript()) ClearScript();
    return f;
}

// Frames de acercamiento (sin boton) para cerrar hasta ~70 px antes de un cuerpo
// a cuerpo (asi la cadena/normal no sale en vacio). Devuelve cuantos anadio.
int RivalAI::AddApproach(float dir, float dist, float dy) {
    const int frames = static_cast<int>(std::clamp((dist - 72.0f) / 6.0f, 0.0f, 16.0f));
    if (frames <= 0) return 0;
    PlayerInput w;
    w.moveX = dir;
    if (std::fabs(dy) > 14.0f) w.moveY = dy > 0 ? 1.0f : -1.0f;
    Add(w, frames);
    return frames;
}

void RivalAI::ChooseClosePlay(const Player& self, const Player& target, const AiProfile& profile,
                              float dir, float dy, float dist) {
    (void)target;
    const int roll = GetRandomValue(0, 99);
    // Transformar (boton 6 de los KF): ocasional, si esta listo y no lo esta ya.
    if (roll < Scaled(7, profile.skillChance) && self.SkillReady(5) && !self.IsTransformed()) {
        PlayerInput in; in.skill = 5; Add(in);
        decide_ = 1.2f; SetPlay(AttackId::Skill);
        return;
    }
    // Habilidad de la pagina actual (1-5).
    if (roll < Scaled(28, profile.skillChance)) {
        const int slot = PickReadySkill(self);
        if (slot >= 0) {
            PlayerInput in; in.skill = slot; Add(in);
            decide_ = 1.0f; SetPlay(AttackId::Skill);
            return;
        }
    }
    // Cambio de pagina (personajes KF con varias paginas de habilidades).
    if (roll < 34 && self.SkillPageCount() > 1) {
        PlayerInput in; in.skillPage = true; Add(in);
        ++pageFlips_; decide_ = 0.2f;
        return;
    }
    // Especial por comando / Signature (si el personaje lo tiene y hay SP).
    if (Chance(28, profile.skillChance) && BuildSpecial(self, dir, false)) {
        decide_ = 0.9f;
        return;
    }
    // Lanzador J J K + persecucion aerea (o vJ si no hay lanzador de patada).
    if (Chance(24, profile.launcherChance) && BuildLauncher(self, dir, dist, dy)) {
        decide_ = 1.0f;
        return;
    }
    // Normal de comando (>J <K vJ vK...).
    if (Chance(26, profile.commandChance) && BuildCommandNormal(self, dir, dist, dy)) {
        decide_ = 0.7f;
        return;
    }
    // Salto con aereo (si puede). Solo desde algo de distancia: de cerca el rival
    // se queda a ras de suelo para intercambiar (asi tambien es alcanzable).
    if (dist > 120.0f && Chance(14, profile.airChance) && BuildJumpIn(self, dir)) {
        decide_ = 0.9f;
        return;
    }
    // Por defecto: cadena J o K (siempre disponible).
    BuildChain(self, dir, dy, dist, profile);
    decide_ = 0.55f;
}

// Cadena J (por defecto Punch1..Punch3+REMATE) o K. Acerca primero y luego
// pulsa el boton 2-4 veces (el buffer encadena cada golpe).
void RivalAI::BuildChain(const Player& self, float dir, float dy, float dist, const AiProfile& profile) {
    const bool useK = self.MoveEnabled(AttackId::Kick) && GetRandomValue(0, 99) < 34;
    AddApproach(dir, dist, dy);
    PlayerInput lane;
    if (std::fabs(dy) > 14.0f) lane.moveY = dy > 0 ? 1.0f : -1.0f;
    const int maxHits = useK ? 3 : 4;
    const int hits = GetRandomValue(2, maxHits);
    for (int i = 0; i < hits; ++i) {
        PlayerInput p = lane;
        if (useK) p.kick = true; else p.punch = true;
        Add(p, 1);
        Add(lane, 8);
    }
    SetPlay(useK ? AttackId::Kick : AttackId::Punch1);
    // Retirada corta y solo a veces: asi el rival no se va de rango cada combo
    // (quedaria inalcanzable para un jugador quieto) pero tampoco se pega siempre.
    if (GetRandomValue(0, 99) < 55) retreatTimer_ = 0.20f * profile.retreatBias;
}

// Normal de comando: elige una de las habilitadas (>J >K <K <J vJ vK).
bool RivalAI::BuildCommandNormal(const Player& self, float dir, float dist, float dy) {
    struct Opt { AttackId id; int axis; bool kick; };   // axis: +1 adelante, -1 atras, 0 abajo
    const Opt all[] = {
        {AttackId::FwdPunch, +1, false}, {AttackId::FwdKick, +1, true},
        {AttackId::SpinKick, -1, true},  {AttackId::BackElbow, -1, false},
        {AttackId::Uppercut,  0, false}, {AttackId::Sweep,     0, true},
    };
    Opt avail[6]; int n = 0;
    for (const Opt& o : all)
        if (self.MoveEnabled(o.id)) avail[n++] = o;
    if (n == 0) return false;
    const Opt o = avail[GetRandomValue(0, n - 1)];
    AddApproach(dir, dist, dy);
    if (o.axis == 0) {
        // vJ / vK: abajo puro 2 frames y luego el boton (sin salir un QCF).
        PlayerInput d; d.moveY = 1.0f;
        Add(d, 2);
        PlayerInput p = d;
        if (o.kick) p.kick = true; else p.punch = true;
        Add(p, 1);
    } else {
        PlayerInput p;
        p.moveX = o.axis > 0 ? dir : -dir;
        if (o.kick) p.kick = true; else p.punch = true;
        Add(p, 1);
    }
    Add(PlayerInput{}, 4);
    SetPlay(o.id);
    return true;
}

// Lanzador: J J K (si hay KickLauncher) o vJ (Uppercut) + persecucion aerea.
bool RivalAI::BuildLauncher(const Player& self, float dir, float dist, float dy) {
    const bool kickLauncher = self.MoveEnabled(AttackId::KickLauncher);
    const bool upper = self.MoveEnabled(AttackId::Uppercut);
    if (!kickLauncher && !upper) return false;
    AddApproach(dir, dist, dy);
    if (kickLauncher) {
        PlayerInput j; j.punch = true;
        Add(j, 1); Add(PlayerInput{}, 8);
        Add(j, 1); Add(PlayerInput{}, 8);
        PlayerInput k; k.kick = true;
        Add(k, 1);
        SetPlay(AttackId::KickLauncher);
    } else {
        PlayerInput d; d.moveY = 1.0f;
        Add(d, 2);
        PlayerInput u = d; u.punch = true;
        Add(u, 1);
        SetPlay(AttackId::Uppercut);
    }
    // Persecucion: tras el lanzador (10 f de ventana) saltar y golpear en el aire.
    if (self.Moves().caps & CapJump) {
        Add(PlayerInput{}, 3);
        PlayerInput jump; jump.jump = true; jump.moveX = dir;
        Add(jump, 2);
        PlayerInput rise; rise.moveX = dir;
        Add(rise, 6);
        PlayerInput atk; atk.moveX = dir;
        if (self.MoveEnabled(AttackId::AirKick) || self.MoveEnabled(AttackId::AirPunch)) {
            if (self.MoveEnabled(AttackId::AirPunch) || self.MoveEnabled(AttackId::AirSpike)) atk.punch = true;
            else atk.kick = true;
            Add(atk, 1);
            Add(PlayerInput{}, 8);
            if (self.MoveEnabled(AttackId::AirKick)) { PlayerInput k2; k2.kick = true; Add(k2, 1); }
        }
        Add(PlayerInput{}, 8);
    }
    return true;
}

// Salto de aproximacion/castigo + ataque aereo (>J martillo, vK picada, K o J).
bool RivalAI::BuildJumpIn(const Player& self, float dir) {
    if (!CanAir(self)) return false;
    PlayerInput j; j.jump = true; j.moveX = dir;
    Add(j, 1);
    PlayerInput rise; rise.moveX = dir;
    Add(rise, 8);
    PlayerInput atk;
    if (self.MoveEnabled(AttackId::AirSpike)) {
        atk.moveX = dir; atk.punch = true; SetPlay(AttackId::AirSpike);
    } else if (self.MoveEnabled(AttackId::DiveKick)) {
        atk.moveY = 1.0f; atk.kick = true; SetPlay(AttackId::DiveKick);
    } else if (self.MoveEnabled(AttackId::AirKick)) {
        atk.moveX = dir; atk.kick = true; SetPlay(AttackId::AirKick);
    } else {
        atk.moveX = dir; atk.punch = true; SetPlay(AttackId::AirPunch);
    }
    Add(atk, 1);
    Add(PlayerInput{}, 12);   // caer y aterrizar
    return true;
}

// Especial por comando o Signature. allowRanged amplia a los que vuelan lejos.
bool RivalAI::BuildSpecial(const Player& self, float dir, bool allowRanged) {
    struct Opt { AttackId id; int kind; };   // kind: 0 energy,1 rise,2 qcf-K,3 qcb-K,4 qcb-J,5 >L,6 vL
    Opt avail[7]; int n = 0;
    if (self.MoveEnabled(AttackId::EnergyWave) && HasSp(self, AttackId::EnergyWave))
        avail[n++] = {AttackId::EnergyWave, 0};
    if (self.MoveEnabled(AttackId::RisingUppercut))
        avail[n++] = {AttackId::RisingUppercut, 1};
    if (self.MoveEnabled(AttackId::DrillKick) && HasSp(self, AttackId::DrillKick))
        avail[n++] = {AttackId::DrillKick, 2};
    if (self.MoveEnabled(AttackId::Hurricane) && HasSp(self, AttackId::Hurricane))
        avail[n++] = {AttackId::Hurricane, 3};
    if (self.MoveEnabled(AttackId::Signature1))
        avail[n++] = {AttackId::Signature1, 4};
    if (self.MoveEnabled(AttackId::Signature2))
        avail[n++] = {AttackId::Signature2, 5};
    if (self.MoveEnabled(AttackId::Signature3))
        avail[n++] = {AttackId::Signature3, 6};
    if (n == 0) return false;
    const Opt o = avail[GetRandomValue(0, n - 1)];
    (void)allowRanged;
    switch (o.kind) {
        case 0: { PlayerInput e; e.energy = true; Add(e, 1); } break;        // L (onda), moveX neutro
        case 1: { PlayerInput r; r.specialRise = true; Add(r, 1); } break;   // DP+K (boton directo)
        case 2: AddMotion(dir, +1, true); break;                             // QCF + K (DrillKick)
        case 3: AddMotion(dir, -1, true); break;                             // QCB + K (Hurricane)
        case 4: AddMotion(dir, -1, false); break;                            // QCB + J (Signature1)
        case 5: { PlayerInput s; s.energy = true; s.moveX = dir; Add(s, 1); } break;   // >L (Signature2)
        case 6: { PlayerInput s; s.energy = true; s.moveY = 1.0f; Add(s, 1); } break;  // vL (Signature3)
    }
    SetPlay(o.id);
    return true;
}

// Secuencia de direcciones de un cuarto de circulo + boton (frame a frame), para
// que PollAttackInput reconozca el comando. side = +1 adelante (QCF), -1 atras
// (QCB); relativo al facing (que el Modo VS fuerza hacia el jugador).
void RivalAI::AddMotion(float dir, int side, bool kick) {
    PlayerInput d; d.moveY = 1.0f;          // abajo puro (>= 2 f para validar)
    Add(d, 3);
    PlayerInput f;
    f.moveX = side > 0 ? dir : -dir;         // adelante / atras + boton
    if (kick) f.kick = true; else f.punch = true;
    Add(f, 1);
}

}  // namespace district_fury
