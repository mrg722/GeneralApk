// F0.3: presets PoseFx (V6), evaluador, secuencias de clips y cuadros de poses
// sueltas. Sin ventana.
#include "game/player/PoseFxPresets.h"
#include "game/player/PoseFxEval.h"
#include "game/visual/PoseTables.h"
#include "game/CharacterVisual.h"
#include "game/Player.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>

using namespace district_fury;
using namespace district_fury::posefx;

static bool Near(float a, float b, float eps = 1e-4f) { return std::fabs(a - b) <= eps; }

static void TestPresetsNeutral() {
    for (float dir : {1.0f, -1.0f}) {
        assert(IsNeutral(Anticipa(1.5f, 0, dir)));
        assert(IsNeutral(Impacto(2, 0, dir)) && IsNeutral(Impacto(2, 1, dir)));
        assert(IsNeutral(Recobro(1, 1, dir)));
        assert(IsNeutral(Saltito(20, 0)) && IsNeutral(Saltito(20, 1)));
        assert(IsNeutral(Giro(1, 0)) && IsNeutral(Giro(2, 1)));
        assert(IsNeutral(CaidaRot(85, 0, dir)));
        assert(IsNeutral(Temblor(3, 28, 0)));
        assert(IsNeutral(Latido(0.05f, 2, 0)));
        assert(Near(Upper(10, 0, dir).dy, 0) && Near(Upper(10, 1, dir).dy, 0));
        // No neutros donde corresponde: el maximo de cada uno.
        assert(!IsNeutral(Anticipa(1, 1, dir)) && !IsNeutral(Impacto(1, 0.5f, dir)) && !IsNeutral(Barrida(dir)));
    }
    assert(Near(Upper(10, 0, 1).angle, 6) && Near(Upper(10, 1, 1).angle, -18));
    // Recobro: sobrepasa (cambia de signo) antes de quedar en neutro.
    bool overshoot = false;
    for (int i = 1; i < 100; ++i) if (Recobro(1, i / 100.0f, 1).sx < 1.0f) overshoot = true;
    assert(overshoot);
    // Tinte: color mientras dura.
    assert(Tinte(RED, 0.2f, 0.1f).tint.g == RED.g && Tinte(RED, 0.2f, 0.3f).tint.g == 255);
}

static void TestImpactoSymmetric() {
    for (int i = 0; i <= 20; ++i) {
        const float v = i / 20.0f;
        const FxXform r = Impacto(1.3f, v, 1), l = Impacto(1.3f, v, -1);
        assert(Near(r.sx, l.sx) && Near(r.sy, l.sy) && Near(r.angle, -l.angle) && Near(r.dx, -l.dx) && Near(r.dy, l.dy));
    }
}

static void TestGiroSignChanges() {
    for (int n = 1; n <= 3; ++n) {
        int changes = 0;
        bool prev = Giro(static_cast<float>(n), 0).flip;
        for (int i = 1; i <= 2000; ++i) {
            const FxXform x = Giro(static_cast<float>(n), i / 2000.0f);
            assert(x.sx >= 0.15f - 1e-6f && x.sx <= 1.0f + 1e-6f);
            if (x.flip != prev) ++changes;
            prev = x.flip;
        }
        assert(changes == 2 * n);
    }
}

static void TestCaidaRot() {
    assert(Near(CaidaRot(85, 1, 1).angle, -85) && Near(CaidaRot(85, 1, -1).angle, 85));
    assert(Near(CaidaRot(85, 1, 1).dy, 10));
    assert(CaidaRot(85, 0.5f, 1).angle > -85 && CaidaRot(85, 0.5f, 1).angle < 0);
}

static void TestClockAndEval() {
    // S 0.1 / A 0.1 / R 0.2
    FxClock c = AttackClock(0.05f, 0.1f, 0.1f, 0.2f, 1);
    assert(c.phase == 'S' && Near(c.v, 0.5f) && Near(c.u, 0.125f));
    c = AttackClock(0.15f, 0.1f, 0.1f, 0.2f, -1);
    assert(c.phase == 'A' && Near(c.v, 0.5f) && Near(c.t, 0.05f) && c.dir == -1);
    c = AttackClock(0.5f, 0.1f, 0.1f, 0.2f, 1);
    assert(c.phase == 'R' && Near(c.v, 1) && Near(c.u, 1));

    PoseFx fx;
    fx.presets[fx.presetCount++] = ANTICIPA(1);
    fx.presets[fx.presetCount++] = IMPACTO(1);
    // En A solo cuenta IMPACTO.
    c = AttackClock(0.15f, 0.1f, 0.1f, 0.2f, 1);
    FxXform x = EvalPoseFx(fx, c);
    const FxXform imp = Impacto(1, 0.5f, 1);
    assert(Near(x.sx, imp.sx) && Near(x.angle, imp.angle) && Near(x.dx, imp.dx));
    // '*' = todo el movimiento.
    PoseFx all; all.presets[all.presetCount++] = GIRO(1, '*');
    x = EvalPoseFx(all, AttackClock(0.2f, 0.1f, 0.1f, 0.2f, 1));   // u = 0.5 -> espaldas
    assert(x.flip);

    // Claves: neutro implicito al inicio y al final; en la clave vale la clave.
    PoseFx k; k.keys[0].phase = 'A'; k.keys[0].v = 0.5f; k.keys[0].dx = 10; k.keys[0].angle = 8; k.keyCount = 1;
    x = EvalPoseFx(k, AttackClock(0.15f, 0.1f, 0.1f, 0.2f, 1));
    assert(Near(x.dx, 10) && Near(x.angle, 8));
    x = EvalPoseFx(k, AttackClock(0.15f, 0.1f, 0.1f, 0.2f, -1));
    assert(Near(x.dx, -10) && Near(x.angle, -8));   // se refleja con el facing
    x = EvalPoseFx(k, AttackClock(0.0f, 0.1f, 0.1f, 0.2f, 1));
    assert(IsNeutral(x));
    x = EvalPoseFx(k, AttackClock(0.3f, 0.1f, 0.1f, 0.2f, 1));   // R v=0.5: entre la clave y el neutro final
    assert(x.dx > 0 && x.dx < 10);

    // Hop de RootCurve.
    RootCurve r; r.dz = 30; r.t0 = 0.2f; r.t1 = 0.6f;
    assert(Near(EvalRootHop(r, 0.4f).dy, -30) && Near(EvalRootHop(r, 0.1f).dy, 0));

    // Overlay: cuadros first..first+count-1 en su fase.
    OverlayFx o; o.first = 8; o.count = 4; o.phase = 'A';
    assert(OverlayFrame(o, AttackClock(0.05f, 0.1f, 0.1f, 0.2f, 1)) == -1);
    assert(OverlayFrame(o, AttackClock(0.1f, 0.1f, 0.1f, 0.2f, 1)) == 8);
    assert(OverlayFrame(o, AttackClock(0.199f, 0.1f, 0.1f, 0.2f, 1)) == 11);

    // Estelas: explicitas o 3 con GIRO/BARRIDA.
    PoseFx g; assert(EffectiveGhostCount(g) == 0);
    g.presets[g.presetCount++] = GIRO(1); assert(EffectiveGhostCount(g) == 3);
    g.ghostCount = 5; assert(EffectiveGhostCount(g) == 5);
}

static void TestPickAtlasFrame() {
    Animator a;
    AnimationClip c1{0, 0, 0.1f, false}; c1.frames = {10, 11, 12, 13}; c1.durations = {0.1f, 0.1f, 0.1f, 0.1f};
    AnimationClip c2{0, 0, 0.1f, false}; c2.frames = {20, 21}; c2.durations = {0.2f, 0.2f};
    a.namedClips["uno"] = c1; a.namedClips["dos"] = c2;
    MoveVisual mv;
    mv.clips[0].clip = "uno"; mv.clips[0].from = 1; mv.clips[0].to = 2;   // 0.2 s
    mv.clips[1].clip = "dos";                                            // 0.4 s
    mv.clipCount = 2;
    AtlasPick p;
    assert(PickAtlasFrame(mv, 0.0f, 0, false, a, nullptr, p) && p.frame == 11 && p.step == 1);
    assert(PickAtlasFrame(mv, 0.2f, 0, false, a, nullptr, p) && p.frame == 12);   // 0.12 s
    assert(PickAtlasFrame(mv, 0.5f, 0, false, a, nullptr, p) && p.frame == 20);   // 0.30 s
    assert(PickAtlasFrame(mv, 1.0f, 0, false, a, nullptr, p) && p.frame == 21);
    mv.clips[0].reverse = true;
    assert(PickAtlasFrame(mv, 0.0f, 0, false, a, nullptr, p) && p.frame == 12);
    // Clip inexistente: respaldo y luego el animador base (forma transformada).
    MoveVisual miss; miss.clips[0].clip = "no_esta"; miss.clipCount = 1;
    assert(!PickAtlasFrame(miss, 0.5f, 0, false, a, nullptr, p));
    miss.clipFallback = "dos";
    assert(PickAtlasFrame(miss, 0.0f, 0, false, a, nullptr, p) && p.frame == 20 && !p.fromSecondary);
    miss.clipFallback = nullptr;
    Animator base; base.namedClips["no_esta"] = c2;
    assert(PickAtlasFrame(miss, 0.0f, 0, false, a, &base, p) && p.anim == &base && p.fromSecondary);
    // Por segundos con loop.
    MoveVisual loop; loop.clips[0].clip = "dos"; loop.clipCount = 1; loop.loop = true;
    assert(PickAtlasFrame(loop, 0, 0.5f, true, a, nullptr, p) && p.frame == 20);   // 0.5 mod 0.4 = 0.1
}

static void TestFeelAndPoses() {
    MoveSet s;
    assert(EffectiveFeel(s, false) == FeelLevel::Full);
    assert(EffectiveFeel(s, true) == FeelLevel::None);   // KF/BETA: Full por defecto = nada
    s.feel = FeelLevel::Light;
    assert(EffectiveFeel(s, true) == FeelLevel::Light);
    assert(StateVisual(s, PlayerState::Hit, 0) == nullptr);

    PoseFrame f;
    PoseContext ctx;
    assert(!ResolvePoseFrame(0, ctx, f));   // Rayden: atlas
    assert(!ResolvePoseFrame(6, ctx, f));   // MAX: atlas
    assert(!ResolvePoseFrame(2, ctx, f));   // BRAKK: atlas brakk_v2
    // Poses de hoy (misma pose que CharacterPose) con la orientacion de la ficha.
    // F1.1: Titan-X (4, 5) tiene su propia tabla (orientacion por pose, caminar por
    // walkPhase, 1-2 piezas): lo prueba tests/unit/PoseTitanXTests.cpp.
    for (int skin : {1, 3}) {
        for (PlayerState st : {PlayerState::Idle, PlayerState::Walk, PlayerState::Dash, PlayerState::Hit,
                               PlayerState::Airborne, PlayerState::Knockdown, PlayerState::Defeat, PlayerState::Block}) {
            ctx = PoseContext{}; ctx.state = st; ctx.time = 0.4;
            assert(ResolvePoseFrame(skin, ctx, f) && f.pieceCount == 1);
            assert(std::strcmp(f.pieces[0].pose, CharacterPose(skin, st, AttackType::None, false, 0.4)) == 0);
            assert(f.pieces[0].facesRight == GetCharacterVisual(skin).facesRightByDefault);
        }
    }
    ctx = PoseContext{}; ctx.state = PlayerState::Airborne;   // F1.1: derribo = pose caida (no el disparo)
    assert(ResolvePoseFrame(4, ctx, f) && std::strcmp(f.pieces[0].pose, "death") == 0 && f.pieces[0].facesRight);
    // Clon: el golpe sigue la fase real del ataque.
    ctx = PoseContext{}; ctx.state = PlayerState::Attack; ctx.attackType = AttackType::Punch;
    ctx.phase = 'S'; assert(ResolvePoseFrame(1, ctx, f) && std::strcmp(f.pieces[0].pose, "ready") == 0);
    ctx.phase = 'A'; assert(ResolvePoseFrame(1, ctx, f) && std::strcmp(f.pieces[0].pose, "punch") == 0);
    ctx.phase = 'R'; assert(ResolvePoseFrame(1, ctx, f) && std::strcmp(f.pieces[0].pose, "idle1") == 0);
    ctx.attackType = AttackType::Kick; ctx.phase = 'A';
    assert(ResolvePoseFrame(1, ctx, f) && std::strcmp(f.pieces[0].pose, "kick") == 0);

    // Un MoveSet de prueba con segmentos de 2 piezas manda sobre la tabla.
    static MoveOverride ov[1];
    ov[0].id = static_cast<int>(AttackId::Punch1);
    PoseSegment& seg = ov[0].visual.segs[0];
    seg.phase = 'A'; seg.pieceCount = 2;
    seg.pieces[0].pose = "punch1"; seg.pieces[0].facesRight = true;
    seg.pieces[1].pose = "punch2"; seg.pieces[1].offset = {12, -40}; seg.pieces[1].facesRight = true;
    ov[0].visual.segCount = 1;
    ov[0].visual.fx.presets[ov[0].visual.fx.presetCount++] = GIRO(1);
    static MoveSet test; test.overrides = ov; test.overrideCount = 1;
    SetVisualMoveSetForTests(4, &test);
    ctx = PoseContext{}; ctx.state = PlayerState::Attack; ctx.attack = static_cast<int>(AttackId::Punch1); ctx.phase = 'A';
    assert(ResolvePoseFrame(4, ctx, f) && f.fromMoveSet && f.pieceCount == 2);
    assert(std::strcmp(f.pieces[1].pose, "punch2") == 0 && f.pieces[1].offset.x == 12 && f.fx.presetCount == 1);
    ctx.phase = 'S';   // sin segmento para S: la tabla de hoy (+ el PoseFx del override)
    assert(ResolvePoseFrame(4, ctx, f) && !f.fromMoveSet && std::strcmp(f.pieces[0].pose, "punch1") == 0 && f.fx.presetCount == 1);
    ctx.attack = static_cast<int>(AttackId::Kick); ctx.attackType = AttackType::Kick;
    assert(ResolvePoseFrame(4, ctx, f) && std::strcmp(f.pieces[0].pose, "slam") == 0 && f.fx.presetCount == 0);   // F1.1
    SetVisualMoveSetForTests(4, nullptr);
    assert(&VisualMoveSet(4) == &GetMoveSet(4));
}

int main() {
    TestPresetsNeutral();
    TestImpactoSymmetric();
    TestGiroSignChanges();
    TestCaidaRot();
    TestClockAndEval();
    TestPickAtlasFrame();
    TestFeelAndPoses();
    std::puts("pose_fx_tests OK");
    return 0;
}
