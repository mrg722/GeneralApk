// F0.4: framework de movimientos (BRIEF F0.4, tarea 11). Sin ventana: solo datos
// y la maquina de estados del jugador. Es el CONTRATO que heredan los bloques de
// Fase 1 (Moves_*.cpp): rutas por defecto, entradas direccionales, comandos,
// salto/aereos, persecucion, cadenas largas por MoveSet, capacidades y B18.
#include "game/combat/HitResolver.h"
#include "game/moves/MoveRegistry.h"
#include "game/player/PlayerCommon.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <vector>

using namespace district_fury;

namespace {
constexpr float kDt = 1.0f / 60.0f;

void Advance(Player& p, int frames) { for (int i = 0; i < frames; ++i) p.Update(kDt); }

// Encola un comando con direccion relativa (como lo haria PollAttackInput).
void Press(Player& p, InputCommand c, int relX = 0, bool down = false) { p.inputBuffer.Push(c, 0, relX, down); }

// Pulsa `c` y espera (hasta 60 f) a que arranque un ataque distinto de `from`.
AttackId NextAfter(Player& p, AttackId from, InputCommand c, int relX = 0, bool down = false) {
    Press(p, c, relX, down);
    for (int i = 0; i < 60 && (p.state != PlayerState::Attack || p.currentAttack == from); ++i) p.Update(kDt);
    return p.state == PlayerState::Attack ? p.currentAttack : AttackId::None;
}

// Entrada inyectada: una por frame.
void Feed(Player& p, PlayerInput& in, std::initializer_list<PlayerInput> frames) {
    p.scriptedInput = &in;
    for (const PlayerInput& f : frames) { in = f; p.Update(kDt); }
    in = PlayerInput{};
}

PlayerInput Dir(float x, float y) { PlayerInput i; i.moveX = x; i.moveY = y; return i; }
PlayerInput With(PlayerInput i, bool punch, bool kick) { i.punch = punch; i.kick = kick; return i; }

// Rutas J J J J y K K K (en vacio: se cancelan en la recuperacion).
void TestDefaultChains() {
    Player p;
    assert(NextAfter(p, AttackId::None, InputCommand::Punch) == AttackId::Punch1);
    assert(NextAfter(p, AttackId::Punch1, InputCommand::Punch) == AttackId::Punch2);
    assert(NextAfter(p, AttackId::Punch2, InputCommand::Punch) == AttackId::PunchHook);
    assert(NextAfter(p, AttackId::PunchHook, InputCommand::Punch) == AttackId::Punch3);
    assert(p.attackPhase == AttackPhase::Punch3 && p.CurrentAttackDef().knockdown);
    // B12: J tras el ultimo eslabon no corta el REMATE ni se pierde: sale al terminar.
    Press(p, InputCommand::Punch);
    Advance(p, 5);
    assert(p.currentAttack == AttackId::Punch3 && !p.inputBuffer.Empty());
    for (int i = 0; i < 60 && p.currentAttack == AttackId::Punch3; ++i) p.Update(kDt);
    assert(p.state == PlayerState::Attack && p.currentAttack == AttackId::Punch1);

    Player k;
    assert(NextAfter(k, AttackId::None, InputCommand::Kick) == AttackId::Kick);
    assert(NextAfter(k, AttackId::Kick, InputCommand::Kick) == AttackId::Kick2);
    assert(NextAfter(k, AttackId::Kick2, InputCommand::Kick) == AttackId::Kick3);
    assert(k.attackPhase == AttackPhase::Kick);

    Player l;   // J J K -> lanzador
    assert(NextAfter(l, AttackId::None, InputCommand::Punch) == AttackId::Punch1);
    assert(NextAfter(l, AttackId::Punch1, InputCommand::Punch) == AttackId::Punch2);
    assert(NextAfter(l, AttackId::Punch2, InputCommand::Kick) == AttackId::KickLauncher);
    assert(l.CurrentAttackDef().launcher && l.CurrentAttackDef().jumpCancel);
    // Envoltorio historico ResolveCancel (cadena generica, sin direccion).
    assert(ResolveCancel(AttackId::Punch1, InputCommand::Punch) == AttackId::Punch2);
    assert(ResolveCancel(AttackId::Punch2, InputCommand::Kick) == AttackId::KickLauncher);
    assert(ResolveCancel(AttackId::Punch3, InputCommand::Punch) == AttackId::None);
    assert(ResolveCancel(AttackId::Kick, InputCommand::Punch) == AttackId::Punch1);
    assert(ResolveCancel(AttackId::Punch1, InputCommand::SpecialWave) == AttackId::EnergyWave);
}

// Direcciones relativas al facing: >J, <J, vJ, vK, >K, <K.
void TestDirectional() {
    {   // >J avanza ~70 px (root motion outCubic).
        Player p;
        const float x0 = p.position.x;
        assert(NextAfter(p, AttackId::None, InputCommand::Punch, +1) == AttackId::FwdPunch);
        assert(p.CurrentAttackDef().overhead);
        Advance(p, 40);
        const float moved = p.position.x - x0;
        assert(moved > 60.0f && moved < 80.0f);
    }
    {   // <J: CODAZO detras SIN girar.
        Player p;
        p.facing = Facing::Right;
        assert(NextAfter(p, AttackId::None, InputCommand::Punch, -1) == AttackId::BackElbow);
        assert(p.facing == Facing::Right);
        const AttackDef& d = p.CurrentAttackDef();
        p.attackElapsed = d.startup + d.active * 0.5f;
        p.attackPrevElapsed = p.attackElapsed - kDt;
        const AttackShape s = PlayerAttackShape(p);
        assert(s.valid && s.arc);
        const LimbPose pose = EvalLimbArc(s.limb, s.x, s.z, s.facing, s.scale, s.u);
        assert(pose.tip.x < p.position.x - 30.0f);   // la punta esta a la espalda
        // <J J: el 2o gira y sigue con el 2o eslabon J.
        Press(p, InputCommand::Punch);
        for (int i = 0; i < 40 && p.currentAttack == AttackId::BackElbow; ++i) p.Update(kDt);
        assert(p.currentAttack == AttackId::Punch2 && p.facing == Facing::Left);
    }
    {   // vK barrida (baja, derriba), vJ gancho ascendente (lanzador).
        Player p;
        assert(NextAfter(p, AttackId::None, InputCommand::Kick, 0, true) == AttackId::Sweep);
        assert(p.CurrentAttackDef().low && p.CurrentAttackDef().knockdown);
        Player q;
        assert(NextAfter(q, AttackId::None, InputCommand::Punch, 0, true) == AttackId::Uppercut);
        Player r;
        r.stompTargetAhead = true;   // enemigo derribado delante -> PISOTON
        assert(NextAfter(r, AttackId::None, InputCommand::Punch, 0, true) == AttackId::Stomp);
        Player f;
        assert(NextAfter(f, AttackId::None, InputCommand::Kick, +1) == AttackId::FwdKick);
        Player s;
        assert(NextAfter(s, AttackId::None, InputCommand::Kick, -1) == AttackId::SpinKick);
        assert(s.CurrentAttackDef().arc.bothSides);
    }
    {   // En la cadena, sostener una direccion no rompe J J J J (cae a la base).
        Player p;
        assert(NextAfter(p, AttackId::None, InputCommand::Punch) == AttackId::Punch1);
        assert(NextAfter(p, AttackId::Punch1, InputCommand::Punch, -1) == AttackId::Punch2);
        assert(NextAfter(p, AttackId::Punch2, InputCommand::Punch, +1) == AttackId::PunchHook);
    }
}

// Comandos: DP+K, QCB+K, QCF+K, QCF+J (1 proyectil), diagonal + J no es onda.
void TestCommands() {
    PlayerInput in;
    const PlayerInput down = Dir(0, 1), df = Dir(1, 1), fwd = Dir(1, 0), db = Dir(-1, 1), back = Dir(-1, 0);
    {
        Player p;
        Feed(p, in, {fwd, fwd, down, down, With(df, false, true)});
        assert(p.state == PlayerState::Attack && p.currentAttack == AttackId::RisingUppercut);
        assert(p.dashInvulnerability > 0.0f);   // invulnerable f1-5
    }
    {
        Player p;
        p.facing = Facing::Right;
        Feed(p, in, {down, down, db, db, With(back, false, true)});
        assert(p.state == PlayerState::Attack && p.currentAttack == AttackId::Hurricane);
        assert(p.facing == Facing::Right);   // el huracan sale hacia donde miraba
        assert(p.sp == p.maxSp - 15);
    }
    {
        Player p;
        Feed(p, in, {down, down, df, df, With(fwd, false, true)});
        assert(p.state == PlayerState::Attack && p.currentAttack == AttackId::DrillKick);
        assert(p.CurrentAttackDef().hitCount == 3);
    }
    {   // QCF + J -> una sola onda.
        Player p;
        Feed(p, in, {down, down, df, df, With(fwd, true, false)});
        assert(p.currentAttack == AttackId::EnergyWave);
        int shots = 0;
        for (int i = 0; i < 90; ++i) { p.Update(kDt); if (p.ConsumeProjectileEvent()) ++shots; }
        assert(shots == 1);
    }
    {   // Caminar abajo (1 frame) y abajo-adelante + J: NO es onda (B14).
        Player p;
        Feed(p, in, {down, df, df, df, With(df, true, false)});
        assert(p.currentAttack != AttackId::EnergyWave);
        Player q;
        Feed(q, in, {df, df, df, df, With(fwd, true, false)});
        assert(q.currentAttack != AttackId::EnergyWave);
    }
}

// Salto + J J K y aterrizaje de 4 f; sin CapJump, I no salta.
void TestJumpAndAir() {
    PlayerInput in;
    PlayerInput jump; jump.jump = true;
    Player p;
    Feed(p, in, {jump});
    assert(p.state == PlayerState::Jump && p.position.z > 0.0f && !p.IsKnockedDown() && !p.CanAct());
    Advance(p, 6);
    assert(NextAfter(p, AttackId::None, InputCommand::Punch) == AttackId::AirPunch);
    assert(NextAfter(p, AttackId::AirPunch, InputCommand::Punch) == AttackId::AirPunch2);
    assert(NextAfter(p, AttackId::AirPunch2, InputCommand::Kick) == AttackId::AirKick);
    assert(p.CurrentAttackDef().air && p.position.z > 0.0f);
    bool landed = false;
    for (int i = 0; i < 120 && !landed; ++i) {
        p.Update(kDt);
        if (p.state == PlayerState::Recovery && p.position.z == 0.0f) landed = true;
    }
    assert(landed);
    assert(std::fabs(p.recoveryTimer - 4.0f / 60.0f) < 1.5f / 60.0f);
    Advance(p, 6);
    assert(p.state == PlayerState::Idle);

    // Apex ~173 px con vz0 = 720 (subida g 1500).
    Player a;
    Feed(a, in, {jump});
    float apex = 0;
    for (int i = 0; i < 90 && a.state == PlayerState::Jump; ++i) { a.Update(kDt); apex = std::max(apex, a.position.z); }
    assert(apex > 155.0f && apex < 185.0f);

    // Sin CapJump: I no salta.
    MoveSet noJump;
    noJump.caps = CapAll & ~CapJump;
    Player q;
    q.customMoveSet = &noJump;
    Feed(q, in, {jump});
    assert(q.state != PlayerState::Jump && q.position.z == 0.0f);
    assert(!q.MoveEnabled(AttackId::AirPunch));
    // Sin normales de comando: >J cae al J base (Punch1).
    MoveSet noCmd;
    noCmd.caps = CapAll & ~CapCommandNormals;
    Player r;
    r.customMoveSet = &noCmd;
    assert(NextAfter(r, AttackId::None, InputCommand::Punch, +1) == AttackId::Punch1);
}

// Persecucion: KickLauncher que conecta + SALTO en 10 f -> salto hacia el enemigo.
void TestJumpCancelAfterLauncher() {
    Player p;
    p.position = {1000, 565, 0};
    p.facing = Facing::Right;
    std::vector<StreetEnemy> enemies(1);
    enemies[0].Init({1075, 565, 0}, StreetEnemyType::Punk);
    enemies[0].active = true;
    enemies[0].facing = Facing::Left;
    StageHitStyle st;
    CombatFeedback fb;
    assert(NextAfter(p, AttackId::None, InputCommand::Punch) == AttackId::Punch1);
    assert(NextAfter(p, AttackId::Punch1, InputCommand::Punch) == AttackId::Punch2);
    Press(p, InputCommand::Kick);
    bool connected = false;
    for (int i = 0; i < 80 && !connected; ++i) {
        p.Update(kDt);
        if (p.currentAttack == AttackId::KickLauncher) {
            const HitReport r = ResolvePlayerVsEnemies(p, enemies, st, fb);
            connected = r.hits > 0;
        }
    }
    assert(connected && p.jumpCancelTimer > 0.0f);
    assert(enemies[0].position.z >= 0.0f);
    PlayerInput in;
    in.jump = true;
    p.scriptedInput = &in;
    p.Update(kDt);
    p.scriptedInput = nullptr;
    assert(p.state == PlayerState::Jump && p.velocity.x > 0.0f);
}

// Cadena de 6 con un MoveSet de prueba (Chain5/Chain6 con override completo).
void TestSixHitChain() {
    static MoveOverride ov[2];
    ov[0].id = static_cast<int>(AttackId::Chain5);
    ov[0].replaceDef = true;
    ov[0].def = GetAttack(AttackId::Punch2);
    ov[0].def.name = "CADENA 5";
    ov[1].id = static_cast<int>(AttackId::Chain6);
    ov[1].replaceDef = true;
    ov[1].def = GetAttack(AttackId::Punch3);
    ov[1].def.name = "CADENA 6";
    MoveSet ms;
    ms.name = "prueba-6";
    const AttackId chain[6] = {AttackId::Punch1, AttackId::Punch2, AttackId::PunchHook, AttackId::Kick2,
                               AttackId::Chain5, AttackId::Chain6};
    for (int i = 0; i < 6; ++i) ms.chainJ[i] = static_cast<int>(chain[i]);
    ms.chainJCount = 6;
    ms.overrides = ov;
    ms.overrideCount = 2;
    Player p;
    p.customMoveSet = &ms;
    AttackId prev = AttackId::None;
    for (AttackId want : chain) {
        assert(NextAfter(p, prev, InputCommand::Punch) == want);
        prev = want;
    }
    assert(std::strcmp(p.CurrentAttackDef().name, "CADENA 6") == 0);
    // Sin override, Chain5 no existe: la cadena por defecto se corta antes.
    Player q;
    assert(!q.MoveEnabled(AttackId::Chain5) && !q.MoveEnabled(AttackId::Signature1));
    assert(!q.MoveEnabled(AttackId::Grab));
}

// Override: escalas y la definicion efectiva.
void TestEffectiveDef() {
    static MoveOverride ov[1];
    ov[0].id = static_cast<int>(AttackId::FwdPunch);
    ov[0].damageScale = 2.0f;
    ov[0].timeScale = 0.5f;
    ov[0].hitCount = 2;
    MoveSet ms;
    ms.overrides = ov;
    ms.overrideCount = 1;
    Player p;
    p.customMoveSet = &ms;
    const AttackDef d = p.EffectiveDef(AttackId::FwdPunch);
    const AttackDef& g = GetAttack(AttackId::FwdPunch);
    assert(d.damage == g.damage * 2);
    assert(std::fabs(d.startup - g.startup * 0.5f) < 1e-5f);
    assert(d.hitCount == 2 && d.multiHitInterval > 0.0f);
    p.currentAttack = AttackId::FwdPunch;
    assert(p.CurrentAttackDef().damage == d.damage);
    // Ninguna fila historica cambia sus tiempos.
    assert(std::fabs(GetAttack(AttackId::Punch1).startup - 0.09f) < 1e-6f);
    assert(std::fabs(GetAttack(AttackId::Punch3).active - 0.15f) < 1e-6f);
}

// Finisher por los dos caminos (cola y cancel) en furia, combo >= 4, SP >= 35.
void TestFinisherBothPaths() {
    Player p;
    p.isRageMode = true;
    p.rageState = RageState::Active;
    p.comboCount = 5;
    Press(p, InputCommand::Kick);
    p.Update(kDt);
    assert(p.currentAttack == AttackId::Finisher);

    Player q;
    assert(NextAfter(q, AttackId::None, InputCommand::Punch) == AttackId::Punch1);
    q.isRageMode = true;
    q.rageState = RageState::Active;
    q.rageStateTimer = 9.0f;
    q.rage = q.maxRage;
    q.comboCount = 5;
    assert(NextAfter(q, AttackId::Punch1, InputCommand::Kick) == AttackId::Finisher);
}

// B18: ningun id 1..40 mapea a "idle"; familias y fases.
void TestClipNames() {
    for (int i = 1; i < kAttackIdCount; ++i) {
        const AttackId id = static_cast<AttackId>(i);
        assert(std::strcmp(ClipNameFor(id), "idle") != 0);
        for (const char* const* c = HistoricClipsFor(id); *c; ++c) assert(std::strcmp(*c, "idle") != 0);
        const ClipRange r = ClipFor(id);
        assert(!(r.cleanStart == 0 && r.cleanEnd == 3));   // nunca el rango de reposo
    }
    assert(std::strcmp(ClipNameFor(AttackId::PunchHook), "punch_hook") == 0);
    assert(std::strcmp(ClipNameFor(AttackId::Chain6), "chain6") == 0);
    assert(LegacyType(AttackId::Kick2) == AttackType::Kick && LegacyType(AttackId::PunchHook) == AttackType::Punch);
    assert(PhaseFor(AttackId::Sweep) == AttackPhase::Kick && PhaseFor(AttackId::Hurricane) == AttackPhase::Special);
    assert(std::strcmp(AttackIdName(AttackId::Chain6), "Chain6") == 0);
    // MoveSet generico: cadenas por defecto y botones 1-5.
    const MoveSet& g = GenericMoveSet();
    assert(g.chainJ[2] == static_cast<int>(AttackId::PunchHook) && g.chainJCount == 4);
    assert(g.chainK[2] == static_cast<int>(AttackId::Kick3) && g.chainKCount == 3);
    assert(g.skillSlots[1] == static_cast<int>(AttackId::Uppercut));
    // Habilitados por defecto (Rayden, sin ventana): >= 28 movimientos.
    Player p;
    int enabled = 0;
    for (int i = 1; i < kAttackIdCount; ++i) enabled += p.MoveEnabled(static_cast<AttackId>(i)) ? 1 : 0;
    assert(enabled >= 28);
}

// Victoria: no actua, no recibe golpes, 1.6 s y vuelve a Idle.
void TestVictory() {
    Player p;
    p.EnterVictory();
    assert(p.state == PlayerState::Victory && !p.CanAct());
    const int hp = p.hp;
    p.TakeDamage(30);
    assert(p.hp == hp && p.state == PlayerState::Victory);
    Press(p, InputCommand::Punch);
    Advance(p, 60);
    assert(p.state == PlayerState::Victory);
    Advance(p, 40);
    assert(p.state == PlayerState::Idle);
}

// Carrera (doble toque) y back-dash.
void TestRunAndBackDash() {
    PlayerInput in;
    const PlayerInput fwd = Dir(1, 0), none = Dir(0, 0);
    Player p;
    Feed(p, in, {fwd, none, fwd, fwd, fwd});
    assert(p.running);
    PlayerInput bd = Dir(-1, 0);
    bd.dash = true;
    Player q;
    q.facing = Facing::Right;
    q.position.x = 1000.0f;
    const float x0 = q.position.x;
    Feed(q, in, {bd});
    assert(q.state == PlayerState::Dash && q.dashDir == -1 && q.facing == Facing::Right);
    Advance(q, 14);
    assert(q.position.x < x0 - 90.0f && q.position.x > x0 - 130.0f);
}
}  // namespace

int main() {
    TestDefaultChains();
    TestDirectional();
    TestCommands();
    TestJumpAndAir();
    TestJumpCancelAfterLauncher();
    TestSixHitChain();
    TestEffectiveDef();
    TestFinisherBothPaths();
    TestClipNames();
    TestVictory();
    TestRunAndBackDash();
    std::puts("MovesetTests OK");
    return 0;
}
