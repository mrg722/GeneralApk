// Director de audio (contrato F0.1, tabla de F1.10). Drena la cola de eventos de
// juego (game/events/GameEvents.h) y decide que sonar: el audio SOLO consume
// eventos, nunca toca archivos de combate.
//
// Una tabla clara: cada GameEventType -> SoundCue (nombre del banco F1.11 por
// arquetipo del personaje + WAV legado de respaldo) o "mudo intencional" con su
// motivo (MuteReason). Los eventos de modo/arena/jefe/fin van ademas a MusicPlayer.
#pragma once
#include "audio/AudioSystem.h"
#include "game/events/GameEvents.h"

namespace district_fury {

// Drena la cola de eventos (la llama AudioSystem::UpdateAmbient cada frame).
void AudioDirectorUpdate(float dt);

// Arquetipo sonoro por personaje (por el nombre de CharacterVisual).
Archetype ArchetypeForSkin(int skin);
// Carpeta de arquetipos/<...> (nullptr para Brawler, MAX y RONIN).
const char* ArchetypeFolder(Archetype a);
const char* ArchetypeName(Archetype a);

// Tabla evento -> sonido. false = mudo intencional (ver MuteReason).
bool CueFor(const GameEvent& e, SoundCue& out);
// Motivo del silencio de un evento (nullptr si el evento suena).
const char* MuteReason(const GameEvent& e);

// F0.2: mapa evento -> Sfx legado (false = el evento no sonaba). Es el respaldo
// de CueFor cuando no existe ningun archivo nuevo del banco.
bool LegacySfxFor(const GameEvent& e, Sfx& out);
// Eventos que se reenvian a MusicPlayer (modo, arena, fase de jefe, fin).
bool IsMusicEvent(GameEventType t);
// La lluvia ambiente suena en este modo (solo Menu y Stage1).
bool RainForMode(GameMode m);

// Registro de depuracion (DF_AUDIO_LOG=1): eventos por tipo, cuantos sonaron,
// cuantos mudos intencionales y de que fuente salio el sonido.
struct AudioDirectorStats {
    int events[static_cast<int>(GameEventType::Count)] = {};
    int cued[static_cast<int>(GameEventType::Count)] = {};
    int muted[static_cast<int>(GameEventType::Count)] = {};
    int played[static_cast<int>(GameEventType::Count)] = {};
};
const AudioDirectorStats& GetAudioDirectorStats();
void ResetAudioDirectorStats();
void AudioDirectorPrintStats();
const char* GameEventName(GameEventType t);

}  // namespace district_fury
