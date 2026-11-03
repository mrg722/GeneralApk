// Reproductor de musica original adaptativa por modo (contrato F0.1). Recibe los
// eventos de modo/arena/jefe (ModeEnter, ArenaLock, BossPhase, Victory...) que le
// pasa el AudioDirector. Stub vacio: lo implementa F1.12 (musica sintetizada
// propia, nada con copyright).
#pragma once
#include "game/events/GameEvents.h"

namespace district_fury {

class MusicPlayer {
public:
    static MusicPlayer& Get();
    void OnEvent(const GameEvent& e);
    void Update(float dt);
    void SetDuck(float gain);     // atenuacion temporal (0..1) bajo SFX fuertes
    void SetVolume(float v);      // volumen del bus de musica (0..1)
};

}  // namespace district_fury
