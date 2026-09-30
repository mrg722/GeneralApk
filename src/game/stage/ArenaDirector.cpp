#include "game/stage/ArenaDirector.h"
#include <algorithm>
#include <cmath>
#include <utility>

namespace district_fury {

void StageCamera::Follow(float playerX, float dt, bool locked, float lockX) {
    const float target = locked ? lockX : std::clamp(playerX, minX, maxX);
    x += (target - x) * (1.0f - std::pow(0.001f, dt));
    x = std::clamp(x, minX, maxX);
}

void ArenaDirector::Reset() {
    waves_.clear();
    locked_ = false;
    lockX_ = 0.0f;
    activeWave_ = -1;
    goTimer_ = 0.0f;
}

void ArenaDirector::AddWave(float triggerX, std::vector<int> enemyIndices) {
    Wave w;
    w.triggerX = triggerX;
    w.enemyIndices = std::move(enemyIndices);
    waves_.push_back(std::move(w));
}

int ArenaDirector::CheckTrigger(float playerX) {
    if (locked_) return -1;
    for (std::size_t i = 0; i < waves_.size(); ++i) {
        Wave& w = waves_[i];
        if (w.cleared) continue;
        if (w.fired) return -1;                 // en curso
        if (playerX < w.triggerX) return -1;    // en orden: la siguiente aun no
        w.fired = true;
        activeWave_ = static_cast<int>(i);
        goTimer_ = 0.0f;
        return activeWave_;
    }
    return -1;
}

void ArenaDirector::Unlock(bool showGo) {
    locked_ = false;
    if (showGo) goTimer_ = kGoDuration;
}

void ArenaDirector::Update(float dt, bool activeWaveCleared) {
    goTimer_ = std::max(0.0f, goTimer_ - dt);
    if (activeWave_ < 0) return;
    Wave& w = waves_[static_cast<std::size_t>(activeWave_)];
    if (w.fired && !w.cleared && activeWaveCleared) {
        w.cleared = true;
        activeWave_ = -1;
        Unlock(true);
    }
}

bool ArenaDirector::AllWavesCleared() const {
    return std::all_of(waves_.begin(), waves_.end(), [](const Wave& w) { return w.cleared; });
}

bool ArenaDirector::GoVisible() const {
    if (goTimer_ <= 0.0f) return false;
    return std::fmod(goTimer_, 0.40f) > 0.15f;
}

void DrawGoArrow(float goTimer) {
    if (goTimer <= 0.0f || std::fmod(goTimer, 0.40f) <= 0.15f) return;
    const int baseX = 1010, baseY = 250;
    const Color fill{255, 214, 72, 255};
    const Color edge{40, 20, 6, 255};
    const char* text = "GO";
    DrawText(text, baseX + 3, baseY + 3, 64, edge);
    DrawText(text, baseX, baseY, 64, fill);
    // Chevrones ">>" dibujados (animados hacia la derecha).
    const float slide = std::fmod(goTimer * 90.0f, 18.0f);
    for (int i = 0; i < 2; ++i) {
        const float x = static_cast<float>(baseX + 100 + i * 42) + slide;
        const float y = static_cast<float>(baseY + 32);
        DrawTriangle({x + 3, y - 26 + 3}, {x + 3, y + 26 + 3}, {x + 36 + 3, y + 3}, edge);
        DrawTriangle({x, y - 26}, {x, y + 26}, {x + 36, y}, fill);
        DrawTriangle({x - 2, y - 14}, {x - 2, y + 14}, {x + 16, y}, {150, 90, 20, 255});
    }
}

}  // namespace district_fury
