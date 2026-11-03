// F0.2: geometria pura de combate (CombatMath.h). Sin ventana.
#include "game/combat/CombatMath.h"
#include <cassert>
#include <cmath>
#include <cstdio>

using namespace district_fury;

namespace {
bool Near(float a, float b, float eps = 1e-3f) { return std::fabs(a - b) <= eps; }

void TestCapsuleRect() {
    const Rect2 r{0, 0, 10, 10};
    assert(CapsuleRect({{5, 5}, {6, 6}, 1}, r));            // dentro
    assert(CapsuleRect({{-5, 5}, {15, 5}, 0.5f}, r));       // cruza de lado a lado (extremos fuera)
    assert(CapsuleRect({{-2, -2}, {-2, -2}, 3}, r));        // esquina alcanzada por el radio
    assert(!CapsuleRect({{-2, -2}, {-2, -2}, 2.5f}, r));    // esquina a 2.83 > 2.5
    assert(!CapsuleRect({{20, 20}, {30, 30}, 2}, r));       // fuera
    assert(CapsuleRect({{12, -5}, {12, 15}, 2.0f}, r));     // lado a 2 px, radio 2
    assert(!CapsuleRect({{12.5f, -5}, {12.5f, 15}, 2.0f}, r));
}

void TestArcSymmetryAndScale() {
    const LimbArc jab{8, -84, 36, 74, -8, 0, 14};
    for (float u : {0.0f, 0.3f, 0.7f, 1.0f}) {
        const LimbPose right = EvalLimbArc(jab, 100.0f, 0.0f, 1.0f, 1.0f, u);
        const LimbPose left = EvalLimbArc(jab, 100.0f, 0.0f, -1.0f, 1.0f, u);
        assert(Near(right.tip.x - 100.0f, 100.0f - left.tip.x));   // espejo exacto en X
        assert(Near(right.tip.y, left.tip.y));
        assert(Near(right.radius, left.radius));
    }
    // u = 1: punta = pivote + L1 en theta1 (0 grados, al frente). Altura = -ph.
    const LimbPose end = EvalLimbArc(jab, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
    assert(Near(end.tip.x, 8.0f + 74.0f) && Near(end.tip.y, 84.0f));
    // Escala: todo crece lineal desde los pies (Titan-X x1.5).
    const LimbPose big = EvalLimbArc(jab, 0.0f, 0.0f, 1.0f, 1.5f, 1.0f);
    assert(Near(big.tip.x, 1.5f * end.tip.x) && Near(big.tip.y, 1.5f * end.tip.y) && Near(big.radius, 21.0f));
    // La altura del atacante (z) sube todo el arco.
    const LimbPose air = EvalLimbArc(jab, 0.0f, 50.0f, 1.0f, 1.0f, 1.0f);
    assert(Near(air.tip.y, 134.0f));
    // behind: golpea hacia atras.
    LimbArc back = jab;
    back.behind = true;
    assert(EvalLimbArc(back, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f).tip.x < 0.0f);
}

void TestArcVsRect() {
    const LimbArc jab{8, -84, 36, 74, -8, 0, 14};
    const Rect2 front = BodyRect(70.0f, 54.0f, 0.0f, 108.0f);    // Punk delante
    const Rect2 behind = BodyRect(-70.0f, 54.0f, 0.0f, 108.0f);  // Punk detras
    assert(LimbArcVsRect(jab, 0, 0, 1, 1, 0.4f, 0.5f, front).hit);
    assert(!LimbArcVsRect(jab, 0, 0, 1, 1, 0.4f, 0.5f, behind).hit);
    LimbArc both = jab;
    both.bothSides = true;
    assert(LimbArcVsRect(both, 0, 0, 1, 1, 0.4f, 0.5f, behind).hit);
    // El contacto esta sobre el miembro y dentro del alto del cuerpo.
    const ArcHit h = LimbArcVsRect(jab, 0, 0, 1, 1, 0.4f, 0.5f, front);
    assert(h.contact.x > 0.0f && h.contact.y > 60.0f && h.contact.y < 100.0f);
    // Anti-tunel: un arco que barre 180 grados de un frame a otro pasa por un
    // objetivo estrecho que ninguna de las dos poses toca; el barrido lo detecta.
    const LimbArc sweep{0, -80, 100, 100, 90, -90, 6, Ease::Linear};
    const Rect2 thin = BodyRect(100.0f, 6.0f, 75.0f, 10.0f);   // a la altura del pivote, a 100 px
    const LimbPose a = EvalLimbArc(sweep, 0, 0, 1, 1, 0.0f), b = EvalLimbArc(sweep, 0, 0, 1, 1, 1.0f);
    assert(!CapsuleRect({a.base, a.tip, a.radius}, thin) && !CapsuleRect({b.base, b.tip, b.radius}, thin));
    // (la cuerda arriba->abajo pasa por x = 0, no llega a 100: se usa un barrido
    // con pasos intermedios como hace el juego frame a frame)
    bool any = false;
    for (int i = 0; i < 10; ++i)
        any = any || LimbArcVsRect(sweep, 0, 0, 1, 1, i / 10.0f, (i + 1) / 10.0f, thin).hit;
    assert(any);
}

void TestLaneAndContact() {
    assert(LaneOverlap(565.0f, 585.0f, 42.0f));     // 20 px de carril: conecta
    assert(!LaneOverlap(565.0f, 635.0f, 42.0f));    // 70 px > 42 + 18
    assert(LaneOverlap(565.0f, 625.0f, 42.0f));     // 60 = limite
    const Capsule2 c{{0, 50}, {100, 50}, 5};
    const Vector2 p = ContactPoint(c, {40, 0, 60, 30});
    assert(Near(p.x, 50.0f) && Near(p.y, 50.0f));
    const Rect2 r = ScreenBoxToPlane(10, 500, 20, 40, 565);   // pantalla y 500..540 -> altura 25..65
    assert(Near(r.y0, 25.0f) && Near(r.y1, 65.0f) && Near(r.x0, 10.0f) && Near(r.x1, 30.0f));
}

void TestEaseAndFeel() {
    for (Ease e : {Ease::Linear, Ease::InQuad, Ease::OutQuad, Ease::OutCubic, Ease::InCubic, Ease::InOutSine,
                   Ease::OutBack, Ease::Bezier}) {
        assert(Near(CombatEase(e, 0.0f), 0.0f) && Near(CombatEase(e, 1.0f), 1.0f));
    }
    assert(CombatEase(Ease::OutCubic, 0.5f) > 0.5f && CombatEase(Ease::InQuad, 0.5f) < 0.5f);
    assert(Near(HitstopSeconds(0, false, false, false), 0.035f));
    assert(Near(HitstopSeconds(1000, false, false, false), 0.20f));
    assert(Near(HitstopSeconds(10, true, false, false), (0.035f + 0.035f) * 1.5f));
    assert(Near(HitstopSeconds(10, false, true, true, 2.0f), 0.07f * 1.35f * 0.6f * 2.0f));
    assert(Near(VictimShakeAmplitude(20), 5.0f) && Near(CameraKickAmplitude(10), 3.7f));
    assert(Near(VictimShakeOffset(5.0f, 0.0f), 0.0f) && std::fabs(VictimShakeOffset(5.0f, 0.5f)) < 0.01f);
    // Amortiguacion: a 60 fps identica al viejo v *= k por frame.
    float x = 0, v = 300, xo = 0, vo = 300;
    for (int i = 0; i < 20; ++i) {
        DampedStep(x, v, 0.84f, 1.0f / 60.0f);
        xo += vo / 60.0f;
        vo *= 0.84f;
    }
    assert(Near(x, xo, 0.01f) && Near(v, vo, 0.01f));
}
}  // namespace

int main() {
    TestCapsuleRect();
    TestArcSymmetryAndScale();
    TestArcVsRect();
    TestLaneAndContact();
    TestEaseAndFeel();
    std::printf("CombatMathTests OK\n");
    return 0;
}
