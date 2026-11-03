// Reproductor de musica (ver MusicPlayer.h). Stub vacio creado en F0.1.
#include "audio/MusicPlayer.h"

namespace district_fury {

MusicPlayer& MusicPlayer::Get() {
    static MusicPlayer instance;
    return instance;
}

void MusicPlayer::OnEvent(const GameEvent& e) { (void)e; }
void MusicPlayer::Update(float dt) { (void)dt; }
void MusicPlayer::SetDuck(float gain) { (void)gain; }
void MusicPlayer::SetVolume(float v) { (void)v; }

}  // namespace district_fury
