#pragma once
// Constantes y helpers internos del Stage 1 (compartidos por sus .cpp).
#include "core/InputMap.h"
#include "rendering/AssetManager.h"
#include "rendering/Backdrop.h"
#include "rendering/SpriteManifest.h"
#include "rendering/BossSprite.h"
#include "audio/AudioSystem.h"
#include "ui/MainMenu.h"
#include "ui/GameHUD.h"
#include "game/stage/ArenaDirector.h"
#include "game/CharacterVisual.h"
#include "raylib.h"
#include "core/Platform.h"
#include <sstream>
#include <algorithm>
#include <cmath>
#include <fstream>
#include "game/Stage1StoryGame.h"

namespace district_fury {
namespace {
constexpr float kStageEnd = 6000.0f, kLaneMin = 505.0f, kLaneMax = 625.0f, kPi = 3.14159265359f;
struct Spawn {
    float x;
    float y;
    StreetEnemyType type;
};
const Spawn kScenarioWaves[4][5] = {{{620, 570, StreetEnemyType::Punk},
                                     {790, 535, StreetEnemyType::Punk},
                                     {960, 610, StreetEnemyType::Charger},
                                     {1120, 545, StreetEnemyType::Punk},
                                     {0, 0, StreetEnemyType::Punk}},
                                    {{1620, 570, StreetEnemyType::Punk},
                                     {1770, 525, StreetEnemyType::Charger},
                                     {1930, 610, StreetEnemyType::Punk},
                                     {2090, 545, StreetEnemyType::Brute},
                                     {2240, 585, StreetEnemyType::Charger}},
                                    {{3020, 575, StreetEnemyType::Charger},
                                     {3180, 525, StreetEnemyType::Brute},
                                     {3350, 610, StreetEnemyType::Punk},
                                     {3520, 545, StreetEnemyType::Enforcer},
                                     {3670, 590, StreetEnemyType::Charger}},
                                    {{4420, 575, StreetEnemyType::Enforcer},
                                     {4580, 525, StreetEnemyType::Charger},
                                     {4740, 610, StreetEnemyType::Brute},
                                     {4900, 545, StreetEnemyType::Enforcer},
                                     {5060, 590, StreetEnemyType::Punk}}};
inline Color A(Color c, float a) {
    c.a = (unsigned char)(std::clamp(a, 0.f, 1.f) * 255);
    return c;
}
inline const char* RankName(int r) {
    static const char* n[] = {"D", "C", "B", "A", "S", "SS", "SSS", "SSS"};
    return n[std::clamp(r, 0, 7)];
}
} // namespace
} // namespace district_fury
