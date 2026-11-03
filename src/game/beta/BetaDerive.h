// Derivaciones en memoria de los personajes BETA para District Fury (contrato
// F0.1). Al cargar un personaje BETA (RONIN, GUERRERO, RONIN-KRATOS, bestias),
// KfReference::Load llama a DeriveBetaClipsForDistrict, que puede AGREGAR clips
// derivados (recortes, reversas, combinaciones de cuadros existentes) a la copia
// en memoria. Nunca escribe archivos: data/kratos/*.txt y sus PNG quedan intactos.
//
// GowApk (modo BETA) la apaga con SetBetaDeriveEnabled(false) para seguir viendo
// los personajes exactamente como en el juego original.
#pragma once
#include "game/lab/KfReference.h"

namespace district_fury {

void SetBetaDeriveEnabled(bool enabled);
bool BetaDeriveEnabled();   // true por defecto (District Fury)
// Llama a DeriveWarriors y DeriveBeasts; cada una solo toca los betaId suyos.
void DeriveBetaClipsForDistrict(KfReference& ref, const char* betaId);

void DeriveWarriors(KfReference& ref, const char* betaId);   // F1.7: BetaDerive_Warriors.cpp
void DeriveBeasts(KfReference& ref, const char* betaId);     // F1.8: BetaDerive_Beasts.cpp

}  // namespace district_fury
