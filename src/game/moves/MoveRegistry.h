// Registro de movesets por personaje (contrato F0.1).
//
// GetMoveSet(skin) pregunta, en este orden, a cada bloque de Fase 1; el primero
// que devuelve un MoveSet distinto de nullptr gana. Si nadie responde, devuelve
// el MoveSet generico (estatico, valores por defecto de MoveTypes.h).
// Cada MovesetFor_<Bloque> vive en su propio Moves_<Bloque>.cpp y solo lo edita
// su bloque; debe devolver nullptr para los skins que no son suyos.
#pragma once
#include "game/moves/MoveTypes.h"

namespace district_fury {

const MoveSet& GetMoveSet(int skin);
// Override del ataque `id` (static_cast<int>(AttackId)) o nullptr si no hay.
const MoveOverride* FindOverride(const MoveSet& set, int id);
// MoveSet generico (el que devuelve GetMoveSet cuando ningun bloque responde).
// F0.4: trae las cadenas por defecto (J: Punch1, Punch2, PunchHook, Punch3;
// K: Kick, Kick2, Kick3) y los botones 1-5 por defecto.
const MoveSet& GenericMoveSet();
// F0.4: rellena los huecos de un MoveSet con los valores por defecto (cadena J/K
// con el primer eslabon en 0, botones de habilidad en 0). Player lo aplica a su
// copia del MoveSet; un bloque puede llamarlo al construir el suyo.
void FillDefaultChains(MoveSet& m);
// Boton de habilidad i (0..4) por defecto: EnergyWave, Uppercut, RageAttack,
// DashAttack, Finisher (como int de AttackId).
int DefaultSkillSlot(int i);

const MoveSet* MovesetFor_TitanX(int skin);         // F1.1: skins 4 y 5
const MoveSet* MovesetFor_CloneGrinder(int skin);   // F1.2: skins 1 y 3
const MoveSet* MovesetFor_Rayden(int skin);         // F1.3: skin 0 (solo AGREGAR)
const MoveSet* MovesetFor_Max(int skin);            // F1.4: skin 6
const MoveSet* MovesetFor_Brakk(int skin);          // F1.5: skin 2
const MoveSet* MovesetFor_Kf(int skin);             // F1.6: skins 7-21
const MoveSet* MovesetFor_Warriors(int skin);       // F1.7: skins 22-26 y 38-41
const MoveSet* MovesetFor_Beasts(int skin);         // F1.8: skins 27-37

}  // namespace district_fury
