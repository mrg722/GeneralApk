// Geometria de combate (contrato F0.1; F0.2 la amplia y la usa en HitResolver).
// Header puro, sin estado: solo tipos de raylib (Vector2). Plano 2D = (X del
// mundo, altura); el carril se compara aparte (Hurt3::lane / halfDepth).
//
// Convencion F0.2: en el plano de colision la segunda coordenada es la ALTURA
// sobre el suelo, POSITIVA hacia arriba (h = z + altura). Los pivotes de LimbArc
// (MoveCurves.h) se escriben con ph negativo = arriba, como en pantalla; aqui se
// convierten con h = z - s * ph.
#pragma once
#include "raylib.h"
#include "game/moves/MoveCurves.h"   // LimbArc, Ease (header sin dependencias)
#include <algorithm>
#include <cmath>

namespace district_fury {

// Capsula = segmento [a, b] engordado por el radio r.
struct Capsule2 { Vector2 a, b; float r; };
// Rectangulo alineado a ejes, con x0 <= x1 y y0 <= y1.
struct Rect2 { float x0, y0, x1, y1; };
// Caja de dano 3D: X [x0, x1], carril lane +- halfDepth, altura [h0, h1].
struct Hurt3 { float x0, x1, lane, halfDepth, h0, h1; };

// Distancia del punto p al segmento [a, b].
inline float DistPointSegment2(Vector2 p, Vector2 a, Vector2 b) {
    const float abx = b.x - a.x, aby = b.y - a.y;
    const float len2 = abx * abx + aby * aby;
    float t = len2 > 0.0f ? ((p.x - a.x) * abx + (p.y - a.y) * aby) / len2 : 0.0f;
    t = std::clamp(t, 0.0f, 1.0f);
    const float dx = p.x - (a.x + abx * t), dy = p.y - (a.y + aby * t);
    return std::sqrt(dx * dx + dy * dy);
}

// Distancia del punto p al rectangulo (0 si esta dentro).
inline float DistPointRect2(Vector2 p, const Rect2& r) {
    const float dx = std::max({r.x0 - p.x, 0.0f, p.x - r.x1});
    const float dy = std::max({r.y0 - p.y, 0.0f, p.y - r.y1});
    return std::sqrt(dx * dx + dy * dy);
}

// True si los segmentos [p1, p2] y [q1, q2] se cruzan (incluye tocarse).
inline bool SegmentsIntersect2(Vector2 p1, Vector2 p2, Vector2 q1, Vector2 q2) {
    auto cross = [](Vector2 o, Vector2 a, Vector2 b) { return (a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x); };
    auto onSeg = [](Vector2 a, Vector2 b, Vector2 p) {
        return std::min(a.x, b.x) <= p.x && p.x <= std::max(a.x, b.x) && std::min(a.y, b.y) <= p.y && p.y <= std::max(a.y, b.y);
    };
    const float d1 = cross(q1, q2, p1), d2 = cross(q1, q2, p2), d3 = cross(p1, p2, q1), d4 = cross(p1, p2, q2);
    if (((d1 > 0 && d2 < 0) || (d1 < 0 && d2 > 0)) && ((d3 > 0 && d4 < 0) || (d3 < 0 && d4 > 0))) return true;
    return (d1 == 0 && onSeg(q1, q2, p1)) || (d2 == 0 && onSeg(q1, q2, p2)) ||
           (d3 == 0 && onSeg(p1, p2, q1)) || (d4 == 0 && onSeg(p1, p2, q2));
}

// Solape capsula-rectangulo: la distancia minima entre el segmento de la capsula
// y el rectangulo es <= r.
inline bool CapsuleRect(const Capsule2& c, const Rect2& r) {
    if (DistPointRect2(c.a, r) <= c.r || DistPointRect2(c.b, r) <= c.r) return true;
    const Vector2 k[4] = {{r.x0, r.y0}, {r.x1, r.y0}, {r.x1, r.y1}, {r.x0, r.y1}};
    for (int i = 0; i < 4; ++i) {
        if (DistPointSegment2(k[i], c.a, c.b) <= c.r) return true;
        if (SegmentsIntersect2(c.a, c.b, k[i], k[(i + 1) % 4])) return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// F0.2: curvas, arco del miembro (V5), carril, punto de contacto y formulas de
// sensacion (V7). Todo puro y testeable sin ventana.
// ---------------------------------------------------------------------------

constexpr float kCombatPi = 3.14159265358979f;
// Media profundidad del cuerpo (carril) que se suma a def.depth (V5).
constexpr float kLanePad = 18.0f;
// Altura de referencia de un personaje "estandar" (V3).
constexpr float kStandardBodyHeight = 112.0f;

// Curvas de V4 (u se acota a [0,1]). Profile se trata como OutCubic en combate.
inline float CombatEase(Ease e, float u, float p1 = 0.3f, float p2 = 0.9f) {
    u = std::clamp(u, 0.0f, 1.0f);
    switch (e) {
        case Ease::Linear:    return u;
        case Ease::InQuad:    return u * u;
        case Ease::OutQuad:   return 1.0f - (1.0f - u) * (1.0f - u);
        case Ease::InCubic:   return u * u * u;
        case Ease::InOutSine: return -(std::cos(kCombatPi * u) - 1.0f) * 0.5f;
        case Ease::OutBack: {
            const float s = 1.70158f, v = u - 1.0f;
            return 1.0f + (s + 1.0f) * v * v * v + s * v * v;
        }
        case Ease::Bezier: {
            const float a = 1.0f - u;
            return 3.0f * a * a * u * p1 + 3.0f * a * u * u * p2 + u * u * u;
        }
        case Ease::OutCubic:
        case Ease::Profile:
        default: {
            const float a = 1.0f - u;
            return 1.0f - a * a * a;
        }
    }
}

// Pose del miembro en el plano (X, altura): base del tramo que golpea
// (Pw + 0.35 L dir), punta y radio ya escalados.
struct LimbPose { Vector2 pivot, base, tip; float radius; };

// Evalua el arco V5 en u (fraccion del activo). x/z = pies del atacante,
// facing = +1/-1, scale = bodyScale. `side` = +1 lado normal, -1 lado espejo
// (para bothSides); behind invierte el lado.
inline LimbPose EvalLimbArc(const LimbArc& a, float x, float z, float facing, float scale, float u, float side = 1.0f) {
    const float e = CombatEase(a.ease, u);
    const float theta = (a.theta0 + (a.theta1 - a.theta0) * e) * kCombatPi / 180.0f;
    const float L = a.L0 + (a.L1 - a.L0) * e;
    const float f = facing * side * (a.behind ? -1.0f : 1.0f);
    const Vector2 dir{f * std::cos(theta), std::sin(theta)};
    LimbPose p;
    p.pivot = {x + f * scale * a.px, z - scale * a.ph};
    p.tip = {p.pivot.x + scale * L * dir.x, p.pivot.y + scale * L * dir.y};
    p.base = {p.pivot.x + scale * 0.35f * L * dir.x, p.pivot.y + scale * 0.35f * L * dir.y};
    p.radius = scale * a.r;
    return p;
}

// Punto del segmento [a, b] mas cercano a p.
inline Vector2 ClosestPointOnSegment2(Vector2 p, Vector2 a, Vector2 b) {
    const float abx = b.x - a.x, aby = b.y - a.y;
    const float len2 = abx * abx + aby * aby;
    float t = len2 > 0.0f ? ((p.x - a.x) * abx + (p.y - a.y) * aby) / len2 : 0.0f;
    t = std::clamp(t, 0.0f, 1.0f);
    return {a.x + abx * t, a.y + aby * t};
}

// Punto de contacto = punto del segmento de la capsula mas cercano al centro
// de la hurtbox (para chispas y numeros).
inline Vector2 ContactPoint(const Capsule2& c, const Rect2& r) {
    return ClosestPointOnSegment2({(r.x0 + r.x1) * 0.5f, (r.y0 + r.y1) * 0.5f}, c.a, c.b);
}

// Carril: abs(dLane) <= depth + kLanePad (V5).
inline bool LaneOverlap(float laneAttacker, float laneTarget, float depth, float pad = kLanePad) {
    return std::fabs(laneTarget - laneAttacker) <= depth + pad;
}

// Resultado de probar un arco contra una hurtbox: hit y punto de contacto.
struct ArcHit { bool hit = false; Vector2 contact{}; };

// Forma activa del arco = capsula del miembro en u UNION capsula del barrido
// [tip(uPrev), tip(u)] (anti-tunel). bothSides prueba tambien el lado espejo.
inline ArcHit LimbArcVsRect(const LimbArc& a, float x, float z, float facing, float scale,
                            float uPrev, float u, const Rect2& hurt) {
    ArcHit out;
    const int sides = a.bothSides ? 2 : 1;
    for (int s = 0; s < sides && !out.hit; ++s) {
        const float side = s == 0 ? 1.0f : -1.0f;
        const LimbPose now = EvalLimbArc(a, x, z, facing, scale, u, side);
        const LimbPose prev = EvalLimbArc(a, x, z, facing, scale, uPrev, side);
        const Capsule2 limb{now.base, now.tip, now.radius};
        const Capsule2 sweep{prev.tip, now.tip, now.radius};
        if (CapsuleRect(limb, hurt)) { out.hit = true; out.contact = ContactPoint(limb, hurt); }
        else if (CapsuleRect(sweep, hurt)) { out.hit = true; out.contact = ContactPoint(sweep, hurt); }
    }
    return out;
}

// Hurtbox en el plano a partir del centro X, ancho, altura de los pies (z) y alto.
inline Rect2 BodyRect(float cx, float width, float z, float height) {
    return {cx - width * 0.5f, z, cx + width * 0.5f, z + height};
}

// Caja de pantalla (x, yPantalla, w, h, con y hacia abajo) -> plano (X, altura)
// respecto al carril `laneY` (la linea de los pies en pantalla, sin z).
inline Rect2 ScreenBoxToPlane(float bx, float by, float bw, float bh, float laneY) {
    return {bx, laneY - (by + bh), bx + bw, laneY - by};
}

// Solape de dos rectangulos del plano.
inline bool RectOverlap(const Rect2& a, const Rect2& b) {
    return a.x0 < b.x1 && b.x0 < a.x1 && a.y0 < b.y1 && b.y0 < a.y1;
}

// Centro del solape de dos rectangulos (contacto de cajas).
inline Vector2 RectContact(const Rect2& a, const Rect2& b) {
    const float x0 = std::max(a.x0, b.x0), x1 = std::min(a.x1, b.x1);
    const float y0 = std::max(a.y0, b.y0), y1 = std::min(a.y1, b.y1);
    return {(x0 + x1) * 0.5f, (y0 + y1) * 0.5f};
}

// Amortiguacion "k por frame a 60 fps" independiente del frame rate (B11):
// integra exactamente x += v dt con v(t) = v0 k^(60 t).
inline void DampedStep(float& pos, float& vel, float k60, float dt) {
    if (dt <= 0.0f) return;
    const float lambda = -std::log(std::clamp(k60, 1e-4f, 0.9999f)) * 60.0f;
    const float decay = std::exp(-lambda * dt);
    pos += vel * (1.0f - decay) / lambda;
    vel *= decay;
}
// Solo el factor (para velocidades que no mueven posicion aqui).
inline float DampFactor(float k60, float dt) { return std::pow(k60, dt * 60.0f); }

// ---- V7: sensacion ----
// hitstop = clamp(0.035 + 0.0035 dano, 0.035, 0.20) x contra 1.5 x ultimo 1.35 x bloqueado 0.6 x escala.
inline float HitstopSeconds(int damage, bool counter, bool lastOfRoute, bool blocked, float scale = 1.0f) {
    float h = std::clamp(0.035f + 0.0035f * static_cast<float>(damage), 0.035f, 0.20f);
    if (counter) h *= 1.5f;
    if (lastOfRoute) h *= 1.35f;
    if (blocked) h *= 0.6f;
    return h * scale;
}
// Temblor de la victima: A sin(2 pi 28 t) exp(-t/0.06), A = 2 + 0.15 dano px.
inline float VictimShakeAmplitude(int damage) { return 2.0f + 0.15f * static_cast<float>(damage); }
inline float VictimShakeOffset(float amplitude, float t) {
    return amplitude * std::sin(2.0f * kCombatPi * 28.0f * t) * std::exp(-t / 0.06f);
}
// Camara: 1.5 + 0.22 dano px en la direccion del golpe, decae exp(-t/0.12).
inline float CameraKickAmplitude(int damage) { return 1.5f + 0.22f * static_cast<float>(damage); }
inline float CameraKickOffset(float amplitude, float t) { return amplitude * std::exp(-t / 0.12f); }
// Muerte del ultimo enemigo de la oleada o del jefe: camara lenta.
constexpr float kKillSlowMoScale = 0.35f;
constexpr float kKillSlowMoSeconds = 0.35f;

}  // namespace district_fury
