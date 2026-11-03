// Director de audio (contrato F0.1). Drena la cola de eventos de juego
// (game/events/GameEvents.h) y decide que sonar: el audio SOLO consume eventos,
// nunca toca archivos de combate. Stub vacio: F0.2 lo conecta al Sfx legado
// (llamado desde AudioSystem::UpdateAmbient) y F1.10 lo reescribe.
#pragma once

namespace district_fury {

void AudioDirectorUpdate(float dt);

}  // namespace district_fury
