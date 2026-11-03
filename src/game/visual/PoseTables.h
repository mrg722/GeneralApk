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

}  // namespace district_fury
