#pragma once
#include "raylib.h"
#include <string>
#include <unordered_map>

namespace district_fury {

class AssetManager {
private:
    std::unordered_map<std::string, Texture2D> textures;

    AssetManager() {}
public:
    static AssetManager& Get() {
        static AssetManager instance;
        return instance;
    }

    void LoadAll();
    void UnloadAll();
    
    Texture2D GetTexture(const std::string& name);
};

}
