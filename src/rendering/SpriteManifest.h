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
    // F0.3 (opcionales): escala por cuadro ("scale" = una para todos, "scales" =
    // una por cuadro) y desfase de los pies por cuadro ("offsets": [[dx,dy],...],
    // px de celda). El Animator los aplica al dibujar.
    std::vector<float> scales;
    std::vector<std::pair<float, float>> offsets;
    // Vino de un include (data/manifest/*.json): si apunta fuera de la rejilla se
    // descarta solo este clip (no invalida el personaje).
    bool fromInclude = false;
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
    // followIncludes: tambien carga los archivos de la clave "includes" (rutas
    // relativas a la raiz del repo, resueltas como el manifiesto principal).
    bool LoadFromString(const std::string& json, bool followIncludes = true);
    // F0.3: aplica un include {"clips": {"<juego>": {"<clip>": {...}}}}. Agrega
    // clips; uno que ya existe se ignora con aviso salvo "override": true, que esta
    // PROHIBIDO en el juego "rayden" (aviso y se ignora). Un clip mal formado se
    // descarta con aviso. False solo si el JSON no se puede leer.
    bool ApplyInclude(const std::string& json, const std::string& label);
    const std::string& Error() const { return error_; }
    // Avisos no fatales (includes, clips descartados), en orden.
    const std::vector<std::string>& Warnings() const { return warnings_; }

    const AtlasProfile* FindAtlas(const std::string& id) const;
    const ClipMap* FindClips(const std::string& set) const;

private:
    std::map<std::string, AtlasProfile> atlases_;
    std::map<std::string, ClipMap> clips_;
    std::string error_;
    std::vector<std::string> warnings_;
};

// Recorta la hoja en celdas cellW x cellH y asigna el pivote de la perfil a
// cada frame. Devuelve vacio si la textura no contiene ni una celda completa.
std::vector<SpriteFrame> BuildFrames(const AtlasProfile& profile, int textureWidth, int textureHeight);

}  // namespace district_fury
