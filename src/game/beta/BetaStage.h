#pragma once
#include "raylib.h"
#include <string>
#include <vector>

namespace district_fury {

// Escenario del modo BETA (data/beta/stages/<id>.txt, generado por
// tools/beta/build_beta_stages.py desde mapdata + tilesets de nuevosSprites).
// El mapa se dibuja a escala uniforme 1.25 (576 px -> 720), la misma de los
// personajes BETA; la franja caminable del mapa (capa de colision) cae sobre
// el carril del juego (kLaneMinY..kLaneMaxY).
struct BetaStageLayer {
    std::string file;
    float parallax = 0.0f;
    float y = 0.0f;
    bool repeat = true;
    float gap = 0.0f;
    Texture2D tex{};
};

struct BetaStageChunk {
    std::string file;
    float x = 0.0f;
    Texture2D tex{};
};

class BetaStage {
public:
    static constexpr float kScale = 1.25f;
    bool Load(const std::string& id);
    bool Loaded() const { return loaded; }

    // Mundo (coordenadas del juego) <-> mapa.
    float WorldLeft() const { return walkX0 * kScale; }
    float WorldRight() const { return walkX1 * kScale; }
    float WorldWidth() const { return width * kScale; }
    // Desplazamiento vertical de la camara (el mapa termina en el borde inferior).
    float CameraOffsetY() const { return cameraDy; }

    void DrawBack(float cameraX) const { DrawSky(cameraX); DrawMap(cameraX); }   // cielo + parallax + mapa
    void DrawSky(float cameraX) const;     // cielo y capas de parallax
    void DrawMap(float cameraX) const;     // el mapa de tiles (suelo, barandas...)
    void DrawWeather(float cameraX, float time) const;  // clima procedural en pantalla

    std::string id, name, music, weather;
    Color sky{20, 24, 30, 255};

private:
    bool loaded = false;
    int width = 0, height = 0;
    int walkY0 = 0, walkY1 = 0, walkX0 = 0, walkX1 = 0;
    float mapTop = 0.0f;   // y del borde superior del mapa en el mundo
    float cameraDy = 0.0f;
    std::vector<BetaStageLayer> layers;
    std::vector<BetaStageChunk> chunks;
};

}  // namespace district_fury
