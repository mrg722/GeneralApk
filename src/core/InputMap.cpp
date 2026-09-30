#include "core/InputMap.h"
#include "raylib.h"
#include <array>
#include <cstddef>

namespace district_fury {
namespace input {
namespace {
std::array<bool, kKeyCount> gNext{}, gCur{}, gPrev{};
constexpr int kLastRaylibKey = 348;   // KEY_KB_MENU
bool Valid(int key) { return key >= 0 && key < kKeyCount; }
}  // namespace

void ClearNext() { gNext.fill(false); }
void SetVirtual(int key, bool down) { if (Valid(key) && down) gNext[(std::size_t)key] = true; }
void Commit() { gPrev = gCur; gCur = gNext; }

bool Pressed(int key) {
    const bool kb = key > 0 && key <= kLastRaylibKey && IsKeyPressed(key);
    return kb || (Valid(key) && gCur[(std::size_t)key] && !gPrev[(std::size_t)key]);
}

bool Down(int key) {
    const bool kb = key > 0 && key <= kLastRaylibKey && IsKeyDown(key);
    return kb || (Valid(key) && gCur[(std::size_t)key]);
}

}  // namespace input
}  // namespace district_fury
