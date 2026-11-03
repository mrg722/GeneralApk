// Tablas de poses sueltas por personaje (los que se dibujan con imagenes sueltas
// de assets/bosses/<carpeta>/ en vez de un atlas). Contrato creado en F0.1:
// CharacterPose (CharacterVisual.cpp) solo despacha por id a estas funciones, y
// cada bloque edita SOLO su archivo:
//   Poses_CloneGrinder.cpp -> PoseClone (id 1), PoseGrinder (id 3)   [F1.2]
//   Poses_Brakk.cpp        -> PoseBrakkLegacy (id 2)                 [F1.5]
//   Poses_TitanX.cpp       -> PoseTitanX (id 4), PoseTitanXM (id 5)  [F1.1]
// Todas reciben el estado YA normalizado (Airborne llega como Knockdown) y el
// reloj en segundos; devuelven el nombre de la pose ("idle1", "punch"...), que se
// busca como textura "<carpeta>_<pose>".
#pragma once
#include "game/Player.h"
#include "game/moves/MoveTypes.h"

namespace district_fury {

const char* PoseClone(PlayerState state, AttackType attack, bool rage, double time);
const char* PoseGrinder(PlayerState state, AttackType attack, bool rage, double time);
const char* PoseBrakkLegacy(PlayerState state, AttackType attack, bool rage, double time);
const char* PoseTitanX(PlayerState state, AttackType attack, bool rage, double time);
const char* PoseTitanXM(PlayerState state, AttackType attack, bool rage, double time);

// Orientacion nativa de UNA pose concreta (true = el dibujo mira a la derecha).
// Hoy todas devuelven el valor de la ficha (CharacterVisual::facesRightByDefault);
// F1.1/F1.2 la corrigen pose por pose. Todavia nadie la usa para dibujar.
// Cada archivo responde por sus carpetas: 1 = derecha, 0 = izquierda, -1 = no es
// una carpeta suya.
int PoseFacesRightCloneGrinder(const char* folder, const char* pose);   // rayder_clone, grinder
int PoseFacesRightBrakk(const char* folder, const char* pose);          // brakk
int PoseFacesRightTitanX(const char* folder, const char* pose);         // titanx, titanx_mejorado
// Pregunta a los tres archivos; si ninguno conoce la carpeta, devuelve el
// facesRightByDefault de la ficha con esa carpeta (o true si no hay ficha).
// Implementada en CharacterVisual.cpp.
bool PoseFacesRight(const char* folder, const char* pose);

// ---- Interfaz por cuadro (F0.3) ----
// Lo que el dibujo sabe del jugador en este instante. attack = static_cast<int>(AttackId).
// phase/v: fase del ataque ('S', 'A', 'R') y progreso en ella; fuera de un ataque,
// phase = 'A' y v = 0. stateTime = segundos en el estado actual. time = reloj de
// GetTime() (los ciclos de reposo de hoy lo usan).
struct PoseContext {
    PlayerState state = PlayerState::Idle;
    int attack = 0;
    AttackType attackType = AttackType::None;
    char phase = 'A';
    float v = 0;
    float stateTime = 0;
    float walkPhase = 0;
    bool rage = false;
    bool transformed = false;
    float z = 0;
    double time = 0;
    bool gettingUp = false;   // Knockdown con el clip "getup" en curso (levantandose)
};

// Cuadro a dibujar: 1-2 piezas (dibujadas a la vez, la 0 debajo) + su PoseFx.
// Cada pieza es la textura "<carpeta>_<pose>" (o el recorte `src` de ella),
// desplazada `offset` px de textura y con su propia orientacion `facesRight`:
// flip = pieza.facesRight ? facing == Left : facing == Right.
struct PoseFrame {
    PosePiece pieces[2];
    int pieceCount = 0;
    PoseFx fx;
    bool fromMoveSet = false;   // salio de MoveVisual.segs (no de la tabla)
};

// Resuelve el cuadro de poses sueltas del personaje `skin`: primero el MoveVisual
// del MoveSet (segmentos del ataque en curso o del estado), si no la tabla del
// bloque. False si `skin` no es un personaje de poses sueltas.
// Implementada en CharacterVisual.cpp.
bool ResolvePoseFrame(int skin, const PoseContext& ctx, PoseFrame& out);

// Tablas por cuadro de cada bloque (cada una en su Poses_*.cpp). Hoy devuelven
// EXACTAMENTE las poses de las funciones por nombre de arriba (1 pieza, con la
// orientacion de PoseFacesRight); F1.1/F1.2 cambian el contenido.
bool PoseFrameClone(const PoseContext& ctx, PoseFrame& out);       // id 1
bool PoseFrameGrinder(const PoseContext& ctx, PoseFrame& out);     // id 3
bool PoseFrameBrakkLegacy(const PoseContext& ctx, PoseFrame& out); // id 2 (sin atlas)
bool PoseFrameTitanX(const PoseContext& ctx, PoseFrame& out);      // id 4
bool PoseFrameTitanXM(const PoseContext& ctx, PoseFrame& out);     // id 5

// Una pieza simple con la orientacion de la carpeta (PoseFacesRight).
inline PosePiece SimplePiece(const char* folder, const char* pose) {
    PosePiece p; p.pose = pose; p.facesRight = PoseFacesRight(folder, pose); return p;
}
inline bool SinglePose(PoseFrame& out, const char* folder, const char* pose) {
    out.pieces[0] = SimplePiece(folder, pose); out.pieceCount = 1; return pose != nullptr;
}

}  // namespace district_fury
