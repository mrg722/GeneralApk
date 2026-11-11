// F2.2: la IA de rivales (RivalAI) usa de verdad el moveset nuevo. Sin ventana:
// mueve un Player (el rival) con RivalAI contra un maniqui quieto y comprueba que
// - cada PERFIL (arquetipo) ejecuta >= 4 movimientos distintos y >= 1 especial en
//   30 s simulados;
// - la IA NUNCA pide ni ejecuta un movimiento que el personaje no tenga
//   habilitado (un pesado sin salto no hace aereos; sin CapSpecials no hay QCF;
//   los Signature/agarres sin override completo estan apagados);
// - un moveset de suelo (sin salto/aereos/especiales de comando/lanzador) jamas
//   produce un aereo ni un RisingUppercut/Hurricane/DrillKick.
#include "game/RivalAI.h"
#include "game/RivalAIProfile.h"
#include "game/combat/AttackData.h"
#include "raylib.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <set>

using namespace district_fury;

namespace {
constexpr float kDt = 1.0f / 60.0f;

struct Result {
    std::set<AttackId> moves;   // ataques distintos ejecutados
    int specials = 0;           // de ellos, especiales/Signature
    bool askedDisabled = false; // la IA pidio (intencion) un movimiento apagado
    bool ranDisabled = false;   // se ejecuto un movimiento apagado
};

// Simula `seconds` s de duelo: rival (IA con perfil `prof` y moveset `ms`) contra
// un maniqui quieto. Mimetiza al Modo VS: facing forzado hacia el objetivo, luego
// PumpInput + Update.
Result Simulate(const AiProfile& prof, const MoveSet& ms, unsigned seed, int seconds) {
    SetRandomSeed(seed);
    Player rival;
    rival.skin = 0;
    rival.customMoveSet = &ms;
    rival.position = {400.0f, 585.0f, 0.0f};
    Player target;
    target.skin = 0;
    target.inputEnabled = false;   // maniqui: no hace nada, solo existe como objetivo
    target.position = {560.0f, 585.0f, 0.0f};
    PlayerInput rin;
    rival.scriptedInput = &rin;

    RivalAI ai;
    ai.Reset();
    ai.profileOverride = &prof;

    Result res;
    AttackId prev = AttackId::None;
    for (int f = 0; f < 60 * seconds; ++f) {
        rival.sp = rival.maxSp;   // SP lleno: los especiales pueden salir
        rin = ai.Think(rival, target, kDt);
        // Intencion: la IA jamas se propone un movimiento deshabilitado.
        if (ai.playChanged && ai.lastPlay != AttackId::None && ai.lastPlay != AttackId::Skill &&
            !rival.MoveEnabled(ai.lastPlay))
            res.askedDisabled = true;
        if (rival.state != PlayerState::Attack)
            rival.facing = target.position.x > rival.position.x ? Facing::Right : Facing::Left;
        rival.PumpInput(kDt);
        rival.Update(kDt);
        if (rival.state == PlayerState::Attack && rival.currentAttack != prev &&
            rival.currentAttack != AttackId::None) {
            res.moves.insert(rival.currentAttack);
            if (!rival.MoveEnabled(rival.currentAttack)) res.ranDisabled = true;
            if (AttackIsSpecialMove(rival.currentAttack)) ++res.specials;
        }
        prev = rival.currentAttack;
    }
    return res;
}

MoveSet FullMoveSet() {
    MoveSet m;
    m.caps = CapAll;              // todo salvo lo que exige override (Signature, agarres)
    m.feel = FeelLevel::None;
    return m;
}

MoveSet GroundMoveSet() {
    MoveSet m;                    // pesado de suelo: ni salto, ni aereos, ni QCF/QCB/DP, ni lanzador
    m.caps = CapAll & ~(CapJump | CapAir | CapHeavyJump | CapSpecials | CapLauncher | CapGrab);
    m.feel = FeelLevel::None;
    return m;
}

const char* Name(AiArchetype a) {
    switch (a) {
        case AiArchetype::Balanced:   return "Balanced";
        case AiArchetype::Aggressive: return "Aggressive";
        case AiArchetype::Zoner:      return "Zoner";
        case AiArchetype::Heavy:      return "Heavy";
        case AiArchetype::Defensive:  return "Defensive";
        case AiArchetype::Fast:       return "Fast";
        case AiArchetype::Support:    return "Support";
    }
    return "?";
}

// Cada perfil, con un moveset completo, saca >= 4 movimientos distintos y >= 1
// especial en 30 s, sin pedir ni ejecutar nunca un movimiento deshabilitado.
void TestEachProfileHasVariety() {
    const MoveSet full = FullMoveSet();
    const AiArchetype arcs[] = {AiArchetype::Balanced, AiArchetype::Aggressive, AiArchetype::Zoner,
                                AiArchetype::Heavy, AiArchetype::Defensive, AiArchetype::Fast,
                                AiArchetype::Support};
    for (AiArchetype a : arcs) {
        const AiProfile prof = ProfileForArchetype(a);
        const Result r = Simulate(prof, full, 20261111u + static_cast<unsigned>(a), 30);
        std::printf("perfil %-11s movimientos=%zu especiales=%d pidio_apagado=%d ejecuto_apagado=%d\n",
                    Name(a), r.moves.size(), r.specials, r.askedDisabled ? 1 : 0, r.ranDisabled ? 1 : 0);
        assert(!r.askedDisabled);
        assert(!r.ranDisabled);
        assert(r.moves.size() >= 4);
        assert(r.specials >= 1);
    }
}

// Un moveset de suelo nunca produce aereos ni especiales por comando (QCF/QCB/DP),
// pero si variedad de suelo (>= 4) y la onda L (unico especial generico).
void TestGroundOnlyNeverFlies() {
    const MoveSet ground = GroundMoveSet();
    bool anyVariety = false;
    for (AiArchetype a : {AiArchetype::Heavy, AiArchetype::Aggressive, AiArchetype::Balanced}) {
        const AiProfile prof = ProfileForArchetype(a);
        const Result r = Simulate(prof, ground, 777u + static_cast<unsigned>(a), 30);
        std::printf("suelo  %-11s movimientos=%zu especiales=%d\n", Name(a), r.moves.size(), r.specials);
        assert(!r.askedDisabled && !r.ranDisabled);
        for (AttackId id : r.moves) {
            assert(!AttackIsAir(id));                      // sin aereos
            assert(id != AttackId::RisingUppercut);        // sin DP
            assert(id != AttackId::Hurricane);             // sin QCB
            assert(id != AttackId::DrillKick);             // sin QCF
            assert(id != AttackId::KickLauncher);          // sin lanzador de patada
        }
        if (r.moves.size() >= 4) anyVariety = true;
    }
    assert(anyVariety);
}

}  // namespace

int main() {
    TestEachProfileHasVariety();
    TestGroundOnlyNeverFlies();
    std::printf("RivalAITests: OK\n");
    return 0;
}
