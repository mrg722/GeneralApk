#include "game/InputBuffer.h"
#include "game/Player.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <initializer_list>

using namespace district_fury;

namespace {
constexpr float kFrame = 1.0f / 60.0f;

void Advance(Player& p, int frames) { for (int i = 0; i < frames; ++i) p.Update(kFrame); }

void TestBufferExpiry() {
    InputBuffer b;
    b.Push(InputCommand::Punch);
    for (int i = 0; i < 15; ++i) b.Tick(kFrame + 0.0001f);
    assert(b.Size() == 1);             // 15 frames: todavia vigente
    b.Tick(kFrame + 0.0001f);
    assert(b.Empty());                 // 16 frames: expirado y eliminado
}

void TestBufferOrderAndCapacity() {
    InputBuffer b;
    b.Push(InputCommand::Punch);
    b.Push(InputCommand::Kick);
    InputCommand c;
    assert(b.Pop(c) && c == InputCommand::Punch);
    assert(b.Pop(c) && c == InputCommand::Kick);
    assert(!b.Pop(c));
    for (std::size_t i = 0; i < InputBuffer::kCapacity + 3; ++i) b.Push(InputCommand::Kick);
    assert(b.Size() == InputBuffer::kCapacity);   // desborda descartando lo mas viejo
}

void TestChainWithCancel() {
    Player p;
    p.QueueCommand(InputCommand::Punch);
    p.Update(kFrame);
    assert(p.state == PlayerState::Attack && p.attackPhase == AttackPhase::Punch1);
    assert(std::strcmp(p.AttackPhaseName(), "ATTACK_PUNCH_1") == 0);

    // Segundo J durante el startup: queda en buffer y NO cancela antes de la ventana.
    p.QueueCommand(InputCommand::Punch);
    p.Update(kFrame);
    assert(p.attackPhase == AttackPhase::Punch1 && !p.InCancelWindow());

    // Al abrirse la ventana (startup+active = 0.22 s) el golpe se corta y encadena.
    Advance(p, 14);
    assert(p.attackPhase == AttackPhase::Punch2);
    assert(p.inputBuffer.Empty());

    // K en cancel window de Punch2 -> patada.
    Advance(p, 2);
    p.QueueCommand(InputCommand::Kick);
    Advance(p, 20);
    assert(p.attackPhase == AttackPhase::Kick);
}

void TestRecoveryBeforeIdle() {
    Player p;
    p.QueueCommand(InputCommand::Punch);
    p.Update(kFrame);
    bool sawRecovery = false;
    for (int i = 0; i < 120 && p.state != PlayerState::Idle; ++i) {
        p.Update(kFrame);
        if (p.state == PlayerState::Recovery) {
            sawRecovery = true;
            assert(!p.CanAct());
            assert(p.attackPhase == AttackPhase::None);
        }
    }
    assert(sawRecovery);               // buffer vacio => recuperacion obligatoria
    assert(p.state == PlayerState::Idle);
}

void TestExpiredInputDoesNotFire() {
    Player p;
    p.QueueCommand(InputCommand::Kick);
    // Jugador ocupado (Hit) durante mas de 15 frames: el comando ya no debe ejecutarse.
    p.state = PlayerState::Recovery;
    p.recoveryTimer = 0.5f;
    Advance(p, 40);
    assert(p.state == PlayerState::Idle);
}

void TestMotionHistory() {
    MotionHistory m;
    int f = 0;
    m.Record(true, 0, f += 2);   // abajo
    m.Record(true, 1, f += 2);   // abajo-adelante
    m.Record(false, 1, f += 2);  // adelante
    assert(m.QuarterCircle(f) == 1);
    assert(m.DragonPunch(f) == 0);
    assert(m.QuarterCircle(f + 30) == 0);            // fuera de ventana

    MotionHistory dp;
    f = 0;
    dp.Record(false, -1, f += 2);  // adelante (izquierda)
    dp.Record(true, 0, f += 2);    // abajo
    dp.Record(true, -1, f += 2);   // abajo-adelante
    assert(dp.DragonPunch(f) == -1);

    MotionHistory walk;              // caminar adelante no es especial
    walk.Record(false, 1, 1);
    assert(walk.QuarterCircle(2) == 0 && walk.DragonPunch(2) == 0);
}

// Ejecuta una secuencia de entradas (una por frame) con entrada inyectada.
void Feed(Player& p, PlayerInput& in, std::initializer_list<PlayerInput> frames) {
    p.scriptedInput = &in;
    for (const PlayerInput& f : frames) { in = f; p.Update(kFrame); }
    in = PlayerInput{};
}

void TestSpecialCommands() {
    PlayerInput in, down, downFwd, fwd, fwdPunch, fwdKick, downFwdKick;
    down.moveY = 1; downFwd.moveY = 1; downFwd.moveX = 1; fwd.moveX = 1;
    fwdPunch = fwd; fwdPunch.punch = true;
    downFwdKick = downFwd; downFwdKick.kick = true;

    Player p;   // abajo, abajo-adelante, adelante + J -> Onda de Energia
    Feed(p, in, {down, down, downFwd, downFwd, fwdPunch});
    assert(p.state == PlayerState::Attack && p.currentAttack == AttackId::EnergyWave);

    Player q;   // adelante, abajo, abajo-adelante + K -> Punch3 (remate/uppercut)
    Feed(q, in, {fwd, fwd, down, down, downFwdKick});
    assert(q.state == PlayerState::Attack && q.currentAttack == AttackId::Punch3);

    Player r;   // special cancel: Punch1 -> Onda de Energia en la ventana
    r.QueueCommand(InputCommand::Punch);
    r.Update(kFrame);
    assert(r.currentAttack == AttackId::Punch1);
    r.QueueCommand(InputCommand::SpecialWave);
    for (int i = 0; i < 20 && r.currentAttack == AttackId::Punch1; ++i) r.Update(kFrame);
    assert(r.currentAttack == AttackId::EnergyWave);

    Player s;   // sin energia, el comando de onda sale como golpe normal
    s.sp = 0;
    Feed(s, in, {down, down, downFwd, downFwd, fwdPunch});
    assert(s.currentAttack == AttackId::Punch1);
}

void TestHitClearsBuffer() {
    Player p;
    p.QueueCommand(InputCommand::Punch);
    p.dashInvulnerability = 0;
    p.TakeDamage(10);
    assert(p.state == PlayerState::Hit && p.inputBuffer.Empty());
}
}  // namespace

int main() {
    TestBufferExpiry();
    TestBufferOrderAndCapacity();
    TestChainWithCancel();
    TestRecoveryBeforeIdle();
    TestExpiredInputDoesNotFire();
    TestHitClearsBuffer();
    TestMotionHistory();
    TestSpecialCommands();
    std::puts("input_buffer_tests OK");
    return 0;
}
