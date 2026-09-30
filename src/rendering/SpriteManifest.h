#pragma once
#include "rendering/SpriteFrame.h"
#include <map>
#include <string>
#include <vector>

namespace district_fury {

// Un clip indexado por nombre: secuencia explicita de frames con duraciones
// propias por cuadro (asincronas). Cualquier longitud.
struct ClipDef {
    std::vector<int> frames;
    std::vector<float> durations;   // mismo largo que frames
    bool loop = true;
};

using ClipMap = std::map<std::string, ClipDef>;

// Perfil de hoja de sprites. `cell` y `pivot` son propios de cada personaje u
// hoja: no hay tamano global. pivot esta en pixeles dentro de la celda
// (p. ej. [64,126] = centro inferior de una celda 128x128).
struct AtlasProfile {
    std::string id;
    std::string path;
    int columns = 0;        // 0 = derivar de la textura (ancho / cellW)
    int rows = 0;           // 0 = derivar de la textura (alto / cellH)
    float cellW = 0.0f;
    float cellH = 0.0f;
    float pivotX = 0.0f;
    float pivotY = 0.0f;
    std::string clipSet;    // nombre del conjunto de clips (por defecto = id)
};

class SpriteManifest {
public:
    // Manifiesto cargado una sola vez desde data/sprite_manifest.json.
    static const SpriteManifest& Get();

    bool LoadFromFile(const std::string& path);
    bool LoadFromString(const std::string& json);
    const std::string& Error() const { return error_; }

    const AtlasProfile* FindAtlas(const std::string& id) const;
    const ClipMap* FindClips(const std::string& set) const;

private:
    std::map<std::string, AtlasProfile> atlases_;
    std::map<std::string, ClipMap> clips_;
    std::string error_;
};

// Recorta la hoja en celdas cellW x cellH y asigna el pivote de la perfil a
// cada frame. Devuelve vacio si la textura no contiene ni una celda completa.
std::vector<SpriteFrame> BuildFrames(const AtlasProfile& profile, int textureWidth, int textureHeight);

}  // namespace district_fury
