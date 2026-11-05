// F1.1: TITAN-X (skin 4) y TITAN-X MEJORADO (skin 5). Sin ventana: datos de las
// tablas de poses y del MoveSet + la maquina de estados del jugador.
// - toda pose referenciada (MoveSet y tabla de estados) existe como PNG;
// - orientacion por pose: solo TX `recoil` y TXM `shoot_orb` miran a la izquierda;
// - caminar en los dos sentidos: el dibujo mira hacia donde va;
// - el golpe recibido / derribo nunca usa `recoil` ni `cannon_aim`;
// - la carga de TX = punch1 + punch2 (2 piezas, pies alineados);
// - MARTILLO llega a >= 1.4x el alcance del JAB de Rayden;
// - movimientos habilitados >= 26 / 22, habilidades 1-5 fuera de las cadenas;
// - rutas: J J J J, K K K, J J K, >J (armadura) en TX; J J J J y K K en TXM.
#include "game/CharacterVisual.h"
#include "game/combat/HitResolver.h"
#include "game/moves/MoveRegistry.h"
#include "game/player/PlayerCommon.h"
#include "game/visual/PoseTables.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>

using namespace district_fury;

namespace {
constexpr float kDt = 1.0f / 60.0f;

const char* Folder(int skin) { return skin == 4 ? "titanx" : "titanx_mejorado"; }

bool PoseFileExists(int skin, const char* pose) {
    if (pose == nullptr) return false;
    return std::filesystem::exists(std::string("assets/bosses/") + Folder(skin) + "/" + pose + ".png");
}

void CheckPieces(int skin, const PosePiece* pieces, int count) {
    assert(count >= 1 && count <= 2);
    for (int i = 0; i < count; ++i) {
        if (!PoseFileExists(skin, pieces[i].pose)) std::fprintf(stderr, "falta pose %s/%s\n", Folder(skin), pieces[i].pose);
        assert(PoseFileExists(skin, pieces[i].pose));
    }
}

void CheckVisual(int skin, const MoveVisual& v) {
    for (int s = 0; s < v.segCount; ++s) CheckPieces(skin, v.segs[s].pieces, v.segs[s].pieceCount);
}

bool Is(const char* a, const char* b) { return a && b && std::strcmp(a, b) == 0; }

void TestPosesExistAndOrientation() {
    for (int skin : {4, 5}) {
        const MoveSet& ms = GetMoveSet(skin);
        assert(&ms != &GenericMoveSet() && ms.overrides != nullptr);
        for (int i = 0; i < ms.overrideCount; ++i) CheckVisual(skin, ms.overrides[i].visual);
        for (const MoveVisual* v : {&ms.hit, &ms.knockdown, &ms.getup, &ms.victory, &ms.jumpRise, &ms.jumpFall,
                                    &ms.walk, &ms.block})
            CheckVisual(skin, *v);
        // Tabla de estados: todas las poses existen, en cualquier instante.
        for (PlayerState st : {PlayerState::Idle, PlayerState::Walk, PlayerState::Dash, PlayerState::Block, PlayerState::Hit,
                               PlayerState::GuardBreak, PlayerState::Knockdown, PlayerState::Airborne, PlayerState::Defeat,
                               PlayerState::Recovery, PlayerState::Jump, PlayerState::Victory}) {
            for (float t = 0; t < 1.2f; t += 0.05f) {
                PoseContext ctx; ctx.state = st; ctx.stateTime = t; ctx.walkPhase = t * 9; ctx.time = t;
                ctx.z = st == PlayerState::Airborne ? 40.0f : 0.0f;
                PoseFrame f;
                assert(ResolvePoseFrame(skin, ctx, f));
                CheckPieces(skin, f.pieces, f.pieceCount);
                // Golpe recibido / derribo: nunca la pose de disparo ni la de apuntar.
                if (st == PlayerState::Hit || st == PlayerState::GuardBreak || st == PlayerState::Knockdown ||
                    st == PlayerState::Airborne)
                    for (int k = 0; k < f.pieceCount; ++k)
                        assert(!Is(f.pieces[k].pose, "recoil") && !Is(f.pieces[k].pose, "cannon_aim"));
            }
        }
    }
    assert(!PoseFacesRight("titanx", "recoil"));
    assert(!PoseFacesRight("titanx_mejorado", "shoot_orb"));
    for (const char* p : {"idle1", "idle2", "idle3", "idle4", "lean", "punch1", "punch2", "shoot_orb", "slam", "uppercut", "death"})
        assert(PoseFacesRight("titanx", p));
    for (const char* p : {"idle1", "idle2", "idle3", "idle4", "cannon_aim", "charge_orb", "dash", "rage_aura", "slam", "death"})
        assert(PoseFacesRight("titanx_mejorado", p));
}

// Caminar: la pieza mira a la derecha (arte) y el dibujo la espeja SOLO al ir a la
// izquierda -> la cara mira hacia donde camina en los dos sentidos. Y el ciclo
// avanza con walkPhase (no con el reloj).
void TestWalkFacing() {
    for (int skin : {4, 5}) {
        PoseContext ctx; ctx.state = PlayerState::Walk;
        const char* seen[4] = {};
        for (int q = 0; q < 4; ++q) {
            ctx.walkPhase = 0.3f + q * 1.5708f; ctx.time = 0;   // reloj quieto
            PoseFrame f;
            assert(ResolvePoseFrame(skin, ctx, f) && f.pieceCount == 1);
            // flip = facesRight ? facing == Left : facing == Right (PlayerPoseDraw)
            assert(f.pieces[0].facesRight && PoseFacesRight(Folder(skin), f.pieces[0].pose));
            seen[q] = f.pieces[0].pose;
        }
        assert(!Is(seen[0], seen[1]));   // cambia de pose con el paso
    }
}

void TestCargaTwoPieces() {
    const MoveSet& ms = GetMoveSet(4);
    const MoveOverride* m = FindOverride(ms, static_cast<int>(AttackId::Punch1));
    assert(m && m->visual.segCount >= 2);
    const PoseSegment& s = m->visual.segs[0];
    assert(s.phase == 'S' && s.pieceCount == 2 && Is(s.pieces[0].pose, "punch1") && Is(s.pieces[1].pose, "punch2"));
    assert(s.pieces[0].src.width > 0);                 // sin el puno suelto
    assert(s.pieces[1].offset.x > s.pieces[0].offset.x);   // punch2 a la derecha de punch1
    assert(s.pieces[0].offset.y == s.pieces[1].offset.y);  // pies alineados
    assert(Is(m->visual.segs[1].pieces[0].pose, "shoot_orb"));
}

// Alcance desde la X de partida: punta del arco + radio al final del activo.
float Reach(int skin) {
    Player p;
    p.ApplyCharacter(skin);
    p.position = {640, 560, 0};
    p.facing = Facing::Right;
    p.Update(kDt);
    const float x0 = p.position.x;
    p.inputBuffer.Push(InputCommand::Punch, 0, 0, false);
    for (int i = 0; i < 30 && p.state != PlayerState::Attack; ++i) p.Update(kDt);
    assert(p.state == PlayerState::Attack && p.currentAttack == AttackId::Punch1);
    float best = 0;
    for (int i = 0; i < 40 && p.state == PlayerState::Attack; ++i) {
        const AttackShape s = PlayerAttackShape(p);
        if (s.valid && s.arc) {
            const LimbPose pose = EvalLimbArc(s.limb, s.x, s.z, s.facing, s.scale, s.u);
            best = std::max(best, pose.tip.x + pose.radius - x0);
        }
        p.Update(kDt);
    }
    return best;
}

void TestMartilloReach() {
    const float rayden = Reach(0), titan = Reach(4);
    std::fprintf(stderr, "alcance JAB Rayden %.1f px, MARTILLO Titan-X %.1f px (x%.2f)\n", rayden, titan, titan / rayden);
    assert(rayden > 0 && titan >= 1.4f * rayden);
}

AttackId NextAfter(Player& p, AttackId from, InputCommand c, int relX = 0, bool down = false) {
    p.inputBuffer.Push(c, 0, relX, down);
    for (int i = 0; i < 70 && (p.state != PlayerState::Attack || p.currentAttack == from); ++i) p.Update(kDt);
    return p.state == PlayerState::Attack ? p.currentAttack : AttackId::None;
}

Player Make(int skin) { Player p; p.ApplyCharacter(skin); p.position = {640, 560, 0}; p.Update(kDt); return p; }

void TestMovesetAndRoutes() {
    for (int skin : {4, 5}) {
        Player p = Make(skin);
        int enabled = 0;
        for (int m = 1; m < kAttackIdCount; ++m) {
            const AttackId id = static_cast<AttackId>(m);
            if (id == AttackId::Skill || (id >= AttackId::Grab && id <= AttackId::WakeupAttack)) continue;
            if (p.MoveEnabled(id)) ++enabled;
        }
        std::printf("skin %d: %d movimientos habilitados\n", skin, enabled);
        assert(enabled >= (skin == 4 ? 26 : 22));
        const MoveSet& ms = p.Moves();
        for (int s = 0; s < 5; ++s) {
            const int id = ms.skillSlots[s];
            assert(id != 0 && p.MoveEnabled(static_cast<AttackId>(id)));
            for (int c = 0; c < ms.chainJCount; ++c) assert(ms.chainJ[c] != id);
            for (int c = 0; c < ms.chainKCount; ++c) assert(ms.chainK[c] != id);
        }
        assert(!p.MoveEnabled(AttackId::Hurricane));
    }
    {   // TX: J J J J
        Player p = Make(4);
        assert(NextAfter(p, AttackId::None, InputCommand::Punch) == AttackId::Punch1);
        assert(NextAfter(p, AttackId::Punch1, InputCommand::Punch) == AttackId::Punch2);
        assert(NextAfter(p, AttackId::Punch2, InputCommand::Punch) == AttackId::PunchHook);
        assert(NextAfter(p, AttackId::PunchHook, InputCommand::Punch) == AttackId::Punch3);
        assert(p.CurrentAttackDef().knockdown && p.CurrentAttackDef().low);
        assert(std::strcmp(p.CurrentAttackDef().name, "GOLPE SISMICO") == 0);
    }
    {   // TX: K K K (la embestida con armadura) y J J K (lanzador)
        Player p = Make(4);
        assert(NextAfter(p, AttackId::None, InputCommand::Kick) == AttackId::Kick);
        assert(NextAfter(p, AttackId::Kick, InputCommand::Kick) == AttackId::Kick2);
        assert(NextAfter(p, AttackId::Kick2, InputCommand::Kick) == AttackId::Kick3);
        assert(p.CurrentAttackDef().armorHits == 1 && p.CurrentAttackDef().wallBounce);
        Player l = Make(4);
        assert(NextAfter(l, AttackId::None, InputCommand::Punch) == AttackId::Punch1);
        assert(NextAfter(l, AttackId::Punch1, InputCommand::Punch) == AttackId::Punch2);
        assert(NextAfter(l, AttackId::Punch2, InputCommand::Kick) == AttackId::KickLauncher);
        assert(l.CurrentAttackDef().launcher && l.CurrentAttackDef().launch >= 700);
    }
    {   // TX: >J ARIETE con armadura y root motion hacia delante
        Player p = Make(4);
        p.facing = Facing::Right;
        const float x0 = p.position.x;
        assert(NextAfter(p, AttackId::None, InputCommand::Punch, +1) == AttackId::FwdPunch);
        assert(p.CurrentAttackDef().armorHits == 1 && p.CurrentAttackDef().armorTo > 0);
        for (int i = 0; i < 40; ++i) p.Update(kDt);
        assert(p.position.x - x0 > 120.0f);
    }
    {   // TXM: J J J J y K K (cadena K de 2)
        Player p = Make(5);
        assert(NextAfter(p, AttackId::None, InputCommand::Punch) == AttackId::Punch1);
        assert(NextAfter(p, AttackId::Punch1, InputCommand::Punch) == AttackId::Punch2);
        assert(p.CurrentAttackDef().hitCount == 3);
        assert(NextAfter(p, AttackId::Punch2, InputCommand::Punch) == AttackId::PunchHook);
        assert(NextAfter(p, AttackId::PunchHook, InputCommand::Punch) == AttackId::Punch3);
        Player k = Make(5);
        assert(NextAfter(k, AttackId::None, InputCommand::Kick) == AttackId::Kick);
        assert(NextAfter(k, AttackId::Kick, InputCommand::Kick) == AttackId::Kick2);
        assert(!k.MoveEnabled(AttackId::Kick3) && !k.MoveEnabled(AttackId::AirPunch));
    }
}
}  // namespace

int main() {
    TestPosesExistAndOrientation();
    TestWalkFacing();
    TestCargaTwoPieces();
    TestMartilloReach();
    TestMovesetAndRoutes();
    std::puts("pose_titanx_tests OK");
    return 0;
}
