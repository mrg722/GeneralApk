// Curvas de movimiento y arco del miembro (parte del contrato MoveTypes.h, F0.1).
//
// Viven en un header aparte SIN dependencias del juego para que
// combat/AttackData.h pueda incluirlo (F0.4 agrega `LimbArc arc; RootCurve root;`
// a AttackDef) sin un include circular con MoveTypes.h, que a su vez necesita
// AttackDef para MoveOverride. Incluir MoveTypes.h trae tambien este archivo.
// Los campos solo crecen al final, con default neutro.
#pragma once

namespace district_fury {

// Curvas de V4 (u en [0,1]). Profile = la curva del perfil de animacion actual.
enum class Ease { Linear, InQuad, OutQuad, OutCubic, InCubic, InOutSine, OutBack, Bezier, Profile };

// Desplazamiento real del cuerpo (root motion) durante [t0, t1] del ataque
// (normalizado a S+A+R): dx hacia el frente, dz hacia arriba (hop).
struct RootCurve {
    float dx = 0, dz = 0, t0 = 0, t1 = 1;
    Ease ease = Ease::OutCubic;
    float bezP1 = .3f, bezP2 = .9f;   // control de Ease::Bezier
    bool slowOnHit = true;            // frena x0.3 al conectar
};

// Arco del miembro (V5): pivote (px, ph) desde los pies (ph negativo = arriba),
// longitud L0->L1, angulo theta0->theta1 en grados (0 = al frente, + = arriba),
// radio r. behind = golpea hacia atras; bothSides = a los dos lados (giros).
struct LimbArc {
    float px = 8, ph = -84, L0 = 36, L1 = 74, theta0 = -8, theta1 = 0, r = 14;
    Ease ease = Ease::OutCubic;
    bool behind = false;
    bool bothSides = false;
};

}  // namespace district_fury
