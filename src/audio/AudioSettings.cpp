// Ajustes de audio persistentes (ver AudioSettings.h).
#include "audio/AudioSettings.h"
#include "audio/AudioSystem.h"
#include "core/Platform.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <sstream>

namespace district_fury {
namespace audio_settings {

int Snap(int percent) {
    const int p = std::clamp(percent, 0, 100);
    return ((p + kStep / 2) / kStep) * kStep;
}

std::string Serialize(const Values& v) {
    std::ostringstream o;
    o << "music=" << Snap(v.music) << "\nsfx=" << Snap(v.sfx) << "\nambient=" << Snap(v.ambient)
      << "\nmuted=" << (v.muted ? 1 : 0) << "\n";
    return o.str();
}

Values Parse(const std::string& text) {
    Values v;
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) {
        const std::size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        const std::string key = line.substr(0, eq);
        const int val = static_cast<int>(std::strtol(line.c_str() + eq + 1, nullptr, 10));
        if (key == "music") v.music = Snap(val);
        else if (key == "sfx") v.sfx = Snap(val);
        else if (key == "ambient") v.ambient = Snap(val);
        else if (key == "muted") v.muted = val != 0;
    }
    return v;
}

void Load() {
    std::string text;
    if (!platform::LoadTextFile(kFile, text)) return;
    const Values v = Parse(text);
    AudioSystem& a = AudioSystem::Get();
    a.SetBusVolume(Bus::Music, v.music / 100.0f);
    a.SetBusVolume(Bus::Sfx, v.sfx / 100.0f);
    a.SetBusVolume(Bus::Ambient, v.ambient / 100.0f);
    a.SetMuted(v.muted);
}

void Save() {
    const AudioSystem& a = AudioSystem::Get();
    Values v;
    v.music = static_cast<int>(std::lround(a.BusVolume(Bus::Music) * 100.0f));
    v.sfx = static_cast<int>(std::lround(a.BusVolume(Bus::Sfx) * 100.0f));
    v.ambient = static_cast<int>(std::lround(a.BusVolume(Bus::Ambient) * 100.0f));
    v.muted = a.IsMuted();
    platform::SaveTextFile(kFile, Serialize(v));
}

}  // namespace audio_settings
}  // namespace district_fury
