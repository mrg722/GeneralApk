#pragma once
#include "raylib.h"
#include <vector>

namespace district_fury {

// Camara de scroll horizontal. Sigue al jugador con suavizado, respeta los
// limites del nivel y, con la arena bloqueada, se queda fija en lockX.
struct StageCamera {
    static constexpr float kHalfWidth = 640.0f;
    float x = 640.0f;
    float minX = 640.0f;
    float maxX = 5360.0f;

    void Follow(float playerX, float dt, bool locked, float lockX);
    float ViewLeft() const { return x - kHalfWidth; }
    float ViewRight() const { return x + kHalfWidth; }
    Camera2D Get(float shakeX = 0.0f, float shakeY = 0.0f) const {
        return Camera2D{{kHalfWidth, 360.0f}, {x + shakeX, 360.0f + shakeY}, 0.0f, 1.0f};
    }
};

// Oleadas por linea de activacion (estilo beat'em up arcade):
//  - el jugador cruza triggerX -> la oleada aparece y la camara se bloquea;
//  - todos sus enemigos derrotados -> se desbloquea y se muestra "GO >>".
class ArenaDirector {
public:
    static constexpr float kGoDuration = 3.2f;

    struct Wave {
        float triggerX = 0.0f;
        std::vector<int> enemyIndices;   // indices en el vector de enemigos del stage
        bool fired = false;
        bool cleared = false;
    };

    void Reset();
    void AddWave(float triggerX, std::vector<int> enemyIndices);

    // Devuelve el indice de la oleada que se activa este frame, o -1. Solo
    // activa una oleada si la anterior ya fue limpiada y no hay bloqueo.
    int CheckTrigger(float playerX);

    void Lock(float cameraX) { locked_ = true; lockX_ = cameraX; }
    void Unlock(bool showGo);
    void ClearGo() { goTimer_ = 0.0f; }
    // activeWaveCleared: todos los enemigos de la oleada activa derrotados.
    void Update(float dt, bool activeWaveCleared);

    bool Locked() const { return locked_; }
    float LockX() const { return lockX_; }
    int ActiveWave() const { return activeWave_; }
    const std::vector<Wave>& Waves() const { return waves_; }
    bool AllWavesCleared() const;
    float GoTimer() const { return goTimer_; }
    bool GoVisible() const;   // parpadeo 0.25 s encendido / 0.15 s apagado

private:
    std::vector<Wave> waves_;
    bool locked_ = false;
    float lockX_ = 0.0f;
    int activeWave_ = -1;
    float goTimer_ = 0.0f;
};

// Indicador "GO >>" en coordenadas de pantalla (fuera de BeginMode2D). Arte
// propio: texto con la fuente por defecto de raylib y chevrones dibujados.
void DrawGoArrow(float goTimer);

}  // namespace district_fury
