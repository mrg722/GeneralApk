// Director de audio (contrato F0.1). Drena la cola de eventos de juego
// (game/events/GameEvents.h) y decide que sonar: el audio SOLO consume eventos,
// nunca toca archivos de combate. Stub vacio: F0.2 lo conecta al Sfx legado
// (llamado desde AudioSystem::UpdateAmbient) y F1.10 lo reescribe.
#pragma once
#include "audio/AudioSystem.h"
#include "game/events/GameEvents.h"

namespace district_fury {

// Drena la cola de eventos (la llama AudioSystem::UpdateAmbient cada frame).
void AudioDirectorUpdate(float dt);
// F0.2: mapa unico evento -> Sfx legado (false = el evento no suena).
bool LegacySfxFor(const GameEvent& e, Sfx& out);
// Eventos que se reenvian a MusicPlayer (modo, arena, fase de jefe, fin).
bool IsMusicEvent(GameEventType t);

}  // namespace district_fury
