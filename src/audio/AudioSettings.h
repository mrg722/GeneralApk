// Ajustes de audio persistentes (OPCIONES, F1.10): volumen de MUSICA, EFECTOS y
// AMBIENTE (0-100 %, paso 10) y el silencio global. Se guardan en
// audio_settings.txt con el mismo mecanismo que display_settings.txt
// (platform::SaveTextFile: carpeta de datos de la app en Android, cwd en PC).
#pragma once
#include <string>

namespace district_fury {
namespace audio_settings {

constexpr int kStep = 10;
constexpr const char* kFile = "audio_settings.txt";

struct Values {
    int music = 100;     // %
    int sfx = 100;       // %
    int ambient = 100;   // %
    bool muted = false;
};

// Texto <-> valores (puro, testeable). Formato: "music=80\nsfx=100\nambient=60\nmuted=0\n".
std::string Serialize(const Values& v);
Values Parse(const std::string& text);
// Acota a [0, 100] y redondea al paso de 10.
int Snap(int percent);

// Lee el archivo (si existe) y aplica los valores a AudioSystem (buses + mute).
void Load();
// Guarda el estado actual de AudioSystem.
void Save();

}  // namespace audio_settings
}  // namespace district_fury
