#include "rendering/Animator.h"

namespace district_fury {

bool Animator::ApplyProfile(const AtlasProfile& profile, const ClipMap& clips, int textureWidth, int textureHeight) {
    std::vector<SpriteFrame> built = BuildFrames(profile, textureWidth, textureHeight);
    if (built.empty()) return false;

    // Un clip que apunta fuera de la hoja invalida todo el perfil: es preferible
    // caer al respaldo historico que dibujar celdas inexistentes.
    const int frameCount = static_cast<int>(built.size());
    std::unordered_map<std::string, AnimationClip> loaded;
    for (const auto& entry : clips) {
        const ClipDef& def = entry.second;
        bool outside = def.frames.empty();
        for (int index : def.frames) if (index < 0 || index >= frameCount) outside = true;
        if (outside) {
            // Clip de un include (data/manifest/*.json): se descarta solo, con aviso.
            if (def.fromInclude) {
                TraceLog(LOG_WARNING, "manifiesto: clip '%s' fuera de la rejilla (%d cuadros), descartado",
                         entry.first.c_str(), frameCount);
                continue;
            }
            return false;
        }
        AnimationClip clip{def.frames.front(), def.frames.back(), def.durations.empty() ? 0.1f : def.durations.front(), def.loop};
        clip.frames = def.frames;
        clip.durations = def.durations;
        clip.scales = def.scales;
        for (const auto& o : def.offsets) clip.offsets.push_back({o.first, o.second});
        loaded[entry.first] = std::move(clip);
    }

    cols = profile.columns > 0 ? profile.columns : std::max(1, static_cast<int>(textureWidth / profile.cellW));
    rows = profile.rows > 0 ? profile.rows : std::max(1, static_cast<int>(textureHeight / profile.cellH));
    frames = std::move(built);
    namedClips = std::move(loaded);
    currentClipName.clear();
    currentFrame = 0;
    normalizedAtlas = true;   // misma escala de dibujo que los atlas normalizados
    return true;
}

bool Animator::InitFromManifest(const std::string& atlasId, Texture2D tex) {
    if (tex.id == 0) return false;
    const SpriteManifest& manifest = SpriteManifest::Get();
    const AtlasProfile* profile = manifest.FindAtlas(atlasId);
    if (!profile) return false;
    const ClipMap* clips = manifest.FindClips(profile->clipSet);
    if (!clips) return false;
    Init(tex, 1, 1, true);
    if (!ApplyProfile(*profile, *clips, tex.width, tex.height)) {
        texture = Texture2D{0};   // deja el animador limpio para el respaldo
        return false;
    }
    return true;
}

bool Animator::PlayNamed(const std::string& name) {
    const auto it = namedClips.find(name);
    if (it == namedClips.end()) return false;
    Play(it->second);
    currentClipName = name;
    return true;
}

}  // namespace district_fury
