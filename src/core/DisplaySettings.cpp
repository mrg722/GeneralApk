#include "core/DisplaySettings.h"
#include "core/Platform.h"
#include <algorithm>
#include <cstdlib>
#include <string>

namespace district_fury {
namespace display {
namespace {
int gWidth = kMaxWidthPercent;
constexpr const char* kFile = "display_settings.txt";
}  // namespace

int WidthPercent() { return gWidth; }

void SetWidthPercent(int percent) {
    gWidth = std::clamp(percent, kMinWidthPercent, kMaxWidthPercent);
    platform::SaveTextFile(kFile, std::to_string(gWidth) + "\n");
}

void Load() {
    std::string text;
    if (!platform::LoadTextFile(kFile, text)) return;
    const long v = std::strtol(text.c_str(), nullptr, 10);
    gWidth = v > 0 ? std::clamp(static_cast<int>(v), kMinWidthPercent, kMaxWidthPercent) : kMaxWidthPercent;
}

}  // namespace display
}  // namespace district_fury
