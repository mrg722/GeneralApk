#pragma once

#include "rendering/SpriteFrame.h"
#include <string>
#include <vector>

namespace district_fury {

struct SpriteAtlas {
    std::string id;
    std::string texturePath;
    int columns = 0;
    int rows = 0;
    std::vector<SpriteFrame> frames;

    bool IsValid() const {
        if (id.empty() || texturePath.empty() || columns <= 0 || rows <= 0 || frames.empty()) {
            return false;
        }
        for (const SpriteFrame& frame : frames) {
            if (frame.source.width <= 0.0f || frame.source.height <= 0.0f ||
                frame.width <= 0.0f || frame.height <= 0.0f || frame.duration <= 0.0f ||
                frame.pivotX < 0.0f || frame.pivotX > frame.width ||
                frame.pivotY < 0.0f || frame.pivotY > frame.height) {
                return false;
            }
        }
        return true;
    }
};

}