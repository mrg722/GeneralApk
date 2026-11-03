// Evaluador PoseFx (F0.3): convierte el PoseFx de un movimiento (presets V6 +
// claves FxKey + hop de RootCurve) en una transformacion visual FxXform para el
// instante actual, y resuelve las secuencias de clips de MoveVisual.
//
// La parte "pura" (EvalPoseFx, AttackClock, PickAtlasFrame...) no necesita
// ventana: la prueban tests/unit/PoseFxTests.cpp. Al final estan los ganchos
// que usa el dibujo del jugador (VisualMoveSet, EffectiveFeel).
#pragma once
#include "game/player/PoseFxPresets.h"
#include "game/moves/MoveRegistry.h"
#include "rendering/Animator.h"

namespace district_fury {

enum class PlayerState;

// Reloj de un movimiento: fase actual ('S', 'A', 'R'), v en [0,1] dentro de la
// fase, t = segundos dentro de la fase, u = progreso de todo el movimiento,
// total = segundos desde el inicio, dir = +1 derecha / -1 izquierda.
struct FxClock { char phase = 'A'; float v = 0, t = 0, u = 0, total = 0, dir = 1; };

// Ataque con arranque/activo/recuperacion (segundos) a `elapsed` segundos.
FxClock AttackClock(float elapsed, float startup, float active, float recovery, float dir);
// Estado no-ataque: todo es fase 'A'; v = t / duration (con loop, se repite).
FxClock StateClock(float stateTime, float duration, bool loop, float dir);

// Suma presets (los de la fase actual, o phase '*'), claves FxKey interpoladas
// con `keyEase` (con neutro implicito al inicio y al final del movimiento) y
// devuelve la transformacion. Desplazamientos en px de cuerpo estandar (112 px).
FxXform EvalPoseFx(const PoseFx& fx, const FxClock& c, Ease keyEase = Ease::OutCubic);
// Hop de RootCurve como desplazamiento visual: dy = -dz sin(pi u') en [t0, t1].
FxXform EvalRootHop(const RootCurve& r, float u);
// Cuadro (indice en efectos_azules.png) de la superposicion `o` en el reloj c, o
// -1 si no toca en esta fase.
int OverlayFrame(const OverlayFx& o, const FxClock& c);
// Estelas pedidas por el PoseFx (ghostCount, o 3 si usa GIRO/BARRIDA).
int EffectiveGhostCount(const PoseFx& fx);
bool HasFx(const PoseFx& fx);
bool HasVisual(const MoveVisual& v);

// Cuadro elegido de una secuencia de clips (MoveVisual.clips[0..2]).
struct AtlasPick {
    const Animator* anim = nullptr;    // de que animador sale el cuadro
    const AnimationClip* clip = nullptr;
    int frame = -1;                    // indice en anim->frames
    int step = 0;                      // indice dentro de clip->frames (escala/desfase por cuadro)
    bool fromSecondary = false;        // salio del animador base (forma transformada sin el clip)
};
// Natural = suma de duraciones de cada tramo (from..to, / speed). Con `loop`, el
// tiempo `seconds` se repite; si no, se reparte `u` en [0,1] sobre la secuencia
// (ataques: u = progreso del ataque). Busca cada clip en `primary`, luego
// clipFallback, luego en `secondary` (si no es nullptr). False si nada se encontro.
bool PickAtlasFrame(const MoveVisual& mv, float u, float seconds, bool useSeconds, const Animator& primary,
                    const Animator* secondary, AtlasPick& out);

// ---- Ganchos del dibujo del jugador ----
// MoveSet que usa el DIBUJO para `skin`: GetMoveSet(skin) salvo que un test o
// herramienta haya fijado otro con SetVisualMoveSetForTests (nullptr lo quita).
const MoveSet& VisualMoveSet(int skin);
void SetVisualMoveSetForTests(int skin, const MoveSet* set);
// Nivel de feel efectivo. clipCharacter = KF/BETA/RONIN (dibuja su clip tal cual):
// para ellos solo `Light` cuenta (Full/None = None). Para el resto, set.feel.
FeelLevel EffectiveFeel(const MoveSet& set, bool clipCharacter);
// Visual del estado no-ataque (hit, knockdown, getup, victory, jumpRise/Fall,
// walk, block) o nullptr si el MoveSet no lo define (HasVisual == false).
const MoveVisual* StateVisual(const MoveSet& set, PlayerState state, float velocityZ);

// ---- Cuerpo del jugador ya resuelto (lo comparten PlayerDraw.cpp, que toma las
// estelas en Update, y PlayerAtlasDraw/PlayerPoseDraw.cpp, que dibujan). ----
class Player;
struct BodyCmd {
    Vector2 feet{};             // pies en pantalla (ya con dx/dy del PoseFx)
    float scale = 1;            // px de pantalla por px de textura (sin sx/sy)
    float sx = 1, sy = 1, angle = 0;
    Color tint{255, 255, 255, 255};
    float bodyK = 1;            // escala de cuerpo estandar (112 px) -> pantalla
    float dir = 1;
    PoseFx fx; FxClock clock; bool hasFx = false;
    // Poses sueltas: 1-2 piezas.
    int pieceCount = 0;
    Texture2D tex[2]{}; Rectangle src[2]{}; Vector2 offset[2]{}; bool pieceFlip[2]{};
    // Atlas con MoveVisual propio (clips/fx del MoveSet).
    const Animator* anim = nullptr; const AnimationClip* clip = nullptr; int frame = -1; std::size_t step = 0;
    bool flip = false;
};
// Poses sueltas (skins con carpeta y sin atlas): false si el personaje no las usa
// o falta la textura. Implementada en PlayerPoseDraw.cpp.
bool ComputePoseBody(const Player& p, Vector2 screen, float depthScale, Color tint, BodyCmd& out);
// Atlas con visual propio del MoveSet para el ataque/estado actual; false = sin
// visual propio (se dibuja exactamente como antes de F0.3). PlayerAtlasDraw.cpp.
bool ComputeAtlasCustom(const Player& p, Vector2 screen, float spriteScale, Color tint, BodyCmd& out);
// Reloj PoseFx del jugador (ataque: S/A/R de su AttackDef; estado: stateClock).
FxClock PlayerFxClock(const Player& p, const MoveVisual* mv);
// Estelas del jugador (PlayerVisualState) y superposiciones FX del cuerpo.
// PlayerDraw.cpp.
void DrawBodyTrails(const Player& p, float spriteScale);
void DrawBodyOverlays(const BodyCmd& c);

}  // namespace district_fury
