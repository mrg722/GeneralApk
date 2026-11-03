// Presets procedurales de PoseFx (V6 del plan maestro), como funciones puras.
//
// Cada preset devuelve una transformacion visual FxXform {sx, sy, angle, dx, dy}
// para el instante v (normalizado a [0,1] dentro de su fase) o t (segundos).
// dir = +1 si el personaje mira a la derecha, -1 a la izquierda. Los
// desplazamientos estan en px de pantalla para un cuerpo "estandar" de 112 px:
// el dibujo los multiplica por la escala del cuerpo. Se combinan con Accumulate:
// sumando desplazamientos/angulos y multiplicando escalas.
// La rotacion siempre es alrededor de los pies (la aplica el dibujo).
// Header puro (sin ventana): lo prueban tests/unit/PoseFxTests.cpp.
#pragma once
#include "game/moves/MoveTypes.h"
#include <algorithm>
#include <cmath>

namespace district_fury {

struct FxXform {
    float sx = 1, sy = 1, angle = 0, dx = 0, dy = 0, alpha = 1;
    bool flip = false;          // espejo visual extra (GIRO)
    Color tint{255, 255, 255, 255};
};

inline bool IsNeutral(const FxXform& x, float eps = 1e-4f) {
    return std::fabs(x.sx - 1) < eps && std::fabs(x.sy - 1) < eps && std::fabs(x.angle) < eps &&
           std::fabs(x.dx) < eps && std::fabs(x.dy) < eps && std::fabs(x.alpha - 1) < eps && !x.flip;
}

inline Color MultiplyColor(Color a, Color b) {
    return {static_cast<unsigned char>(a.r * b.r / 255), static_cast<unsigned char>(a.g * b.g / 255),
            static_cast<unsigned char>(a.b * b.b / 255), static_cast<unsigned char>(a.a * b.a / 255)};
}

inline FxXform& Accumulate(FxXform& acc, const FxXform& k) {
    acc.sx *= k.sx; acc.sy *= k.sy; acc.angle += k.angle; acc.dx += k.dx; acc.dy += k.dy;
    acc.alpha *= k.alpha; acc.flip = acc.flip != k.flip; acc.tint = MultiplyColor(acc.tint, k.tint);
    return acc;
}

namespace posefx {

constexpr float kPi = 3.14159265358979f;

inline float Clamp01(float v) { return std::clamp(v, 0.0f, 1.0f); }
inline float OutBack(float u, float s = 1.70158f) { const float w = u - 1; return 1 + (s + 1) * w * w * w + s * w * w; }

// Curvas de V4 (u en [0,1]). Ease::Profile = OutCubic (curva por defecto).
inline float ApplyEase(Ease e, float u, float p1 = .3f, float p2 = .9f) {
    u = Clamp01(u);
    switch (e) {
        case Ease::Linear: return u;
        case Ease::InQuad: return u * u;
        case Ease::OutQuad: return 1 - (1 - u) * (1 - u);
        case Ease::InCubic: return u * u * u;
        case Ease::InOutSine: return -(std::cos(kPi * u) - 1) / 2;
        case Ease::OutBack: return OutBack(u);
        case Ease::Bezier: return 3 * (1 - u) * (1 - u) * u * p1 + 3 * (1 - u) * u * u * p2 + u * u * u;
        case Ease::OutCubic:
        case Ease::Profile:
        default: { const float w = 1 - u; return 1 - w * w * w; }
    }
}

// ANTICIPA(k), fase S: encoge y se echa atras antes del golpe.
inline FxXform Anticipa(float k, float v, float dir) {
    const float e = ApplyEase(Ease::OutQuad, v);
    FxXform x; x.sx = 1 - 0.06f * k * e; x.sy = 1 + 0.04f * k * e; x.angle = -4 * k * dir * e; x.dx = -6 * k * dir * e;
    return x;
}
// IMPACTO(k), fase A: estira hacia el frente con q = sin(pi v).
inline FxXform Impacto(float k, float v, float dir) {
    const float q = std::sin(kPi * Clamp01(v));
    FxXform x; x.sx = 1 + 0.10f * k * q; x.sy = 1 - 0.05f * k * q; x.angle = 6 * k * dir * q; x.dx = 8 * k * dir * q;
    return x;
}
// RECOBRO(k), fase R: un resto del impacto vuelve a neutro con outBack(1.4)
// (sobrepaso ~0.03k en sx); neutro en v = 1.
inline FxXform Recobro(float k, float v, float dir) {
    const float r = 1 - OutBack(Clamp01(v), 1.4f);
    FxXform x; x.sx = 1 + 0.03f * k * r; x.sy = 1 - 0.02f * k * r; x.angle = 3 * k * dir * r; x.dx = 4 * k * dir * r;
    return x;
}
// SALTITO(h): dy = -h sin(pi v).
inline FxXform Saltito(float h, float v) { FxXform x; x.dy = -h * std::sin(kPi * Clamp01(v)); return x; }
// UPPER(h): angulo +6 dir -> -18 dir (outCubic) y dy = -h sin(pi v).
inline FxXform Upper(float h, float v, float dir) {
    FxXform x; const float e = ApplyEase(Ease::OutCubic, v);
    x.angle = (6 + (-18 - 6) * e) * dir; x.dy = -h * std::sin(kPi * Clamp01(v));
    return x;
}
// GIRO(n): sx = |cos(2 pi n v)| con minimo 0.15, espejo visual cuando el coseno es
// negativo (n vueltas = 2n cambios de signo) y bamboleo de 4 grados.
inline FxXform Giro(float n, float v) {
    const float c = std::cos(2 * kPi * n * Clamp01(v));
    FxXform x; x.sx = std::max(std::fabs(c), 0.15f); x.flip = c < 0; x.angle = 4 * std::sin(2 * kPi * n * Clamp01(v));
    return x;
}
// BARRIDA: agachado a ras de suelo.
inline FxXform Barrida(float dir) { FxXform x; x.sy = 0.72f; x.dy = 18; x.angle = -10 * dir; return x; }
// CAIDA_ROT(a): rota alrededor de los pies 0 -> -a dir (inQuad); dy 0 -> +10.
inline FxXform CaidaRot(float a, float v, float dir) {
    const float e = ApplyEase(Ease::InQuad, v);
    FxXform x; x.angle = -a * dir * e; x.dy = 10 * e;
    return x;
}
// TEMBLOR(amp, f): dx = amp sin(2 pi f t) exp(-t/0.06).
inline FxXform Temblor(float amp, float f, float t) {
    FxXform x; x.dx = amp * std::sin(2 * kPi * f * t) * std::exp(-std::max(0.0f, t) / 0.06f); return x;
}
// LATIDO(a, hz): s = 1 + a sin(2 pi hz t) sobre sx y sy.
inline FxXform Latido(float a, float hz, float t) {
    FxXform x; const float s = 1 + a * std::sin(2 * kPi * hz * t); x.sx = s; x.sy = s; return x;
}
// TINTE(color, dur): multiplica el color del sprite mientras t < dur (dur <= 0 = siempre).
inline FxXform Tinte(Color c, float dur, float t) { FxXform x; if (dur <= 0 || t < dur) x.tint = c; return x; }

// Evalua un PresetUse. v y t son de la fase del preset (el llamador ya filtro la fase).
inline FxXform Eval(const PresetUse& p, float v, float t, float dir) {
    switch (p.preset) {
        case FxPreset::Anticipa: return Anticipa(p.a, v, dir);
        case FxPreset::Impacto: return Impacto(p.a, v, dir);
        case FxPreset::Recobro: return Recobro(p.a, v, dir);
        case FxPreset::Saltito: return Saltito(p.a, v);
        case FxPreset::Upper: return Upper(p.a, v, dir);
        case FxPreset::Giro: return Giro(p.a, v);
        case FxPreset::Barrida: return Barrida(dir);
        case FxPreset::CaidaRot: return CaidaRot(p.a, v, dir);
        case FxPreset::Temblor: return Temblor(p.a, p.b, t);
        case FxPreset::Latido: return Latido(p.a, p.b, t);
        case FxPreset::Tinte: return Tinte(p.color, p.a, t);
        case FxPreset::None:
        default: return FxXform{};
    }
}

// Constructores comodos para las tablas de Fase 1 (fase por defecto la de V6).
inline PresetUse ANTICIPA(float k) { PresetUse p; p.preset = FxPreset::Anticipa; p.a = k; p.phase = 'S'; return p; }
inline PresetUse IMPACTO(float k) { PresetUse p; p.preset = FxPreset::Impacto; p.a = k; p.phase = 'A'; return p; }
inline PresetUse RECOBRO(float k) { PresetUse p; p.preset = FxPreset::Recobro; p.a = k; p.phase = 'R'; return p; }
inline PresetUse SALTITO(float h, char ph = 'A') { PresetUse p; p.preset = FxPreset::Saltito; p.a = h; p.phase = ph; return p; }
inline PresetUse UPPER(float h) { PresetUse p; p.preset = FxPreset::Upper; p.a = h; p.phase = 'A'; return p; }
inline PresetUse GIRO(float n, char ph = 'A') { PresetUse p; p.preset = FxPreset::Giro; p.a = n; p.phase = ph; return p; }
inline PresetUse BARRIDA(char ph = 'A') { PresetUse p; p.preset = FxPreset::Barrida; p.phase = ph; return p; }
inline PresetUse CAIDA_ROT(float a, char ph = 'A') { PresetUse p; p.preset = FxPreset::CaidaRot; p.a = a; p.phase = ph; return p; }
inline PresetUse TEMBLOR(float amp, float f, char ph = 'A') { PresetUse p; p.preset = FxPreset::Temblor; p.a = amp; p.b = f; p.phase = ph; return p; }
inline PresetUse LATIDO(float a, float hz, char ph = '*') { PresetUse p; p.preset = FxPreset::Latido; p.a = a; p.b = hz; p.phase = ph; return p; }
inline PresetUse TINTE(Color c, float dur, char ph = '*') { PresetUse p; p.preset = FxPreset::Tinte; p.a = dur; p.color = c; p.phase = ph; return p; }

}  // namespace posefx
}  // namespace district_fury
