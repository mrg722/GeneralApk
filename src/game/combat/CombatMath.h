// Geometria de combate (contrato F0.1; F0.2 la amplia y la usa en HitResolver).
// Header puro, sin estado: solo tipos de raylib (Vector2). Plano 2D = (X del
// mundo, altura); el carril se compara aparte (Hurt3::lane / halfDepth).
#pragma once
#include "raylib.h"
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

}  // namespace district_fury
