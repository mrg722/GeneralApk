#include "core/Platform.h"
#include "raylib.h"
#if defined(PLATFORM_ANDROID)
#include <android/asset_manager.h>
#include <android_native_app_glue.h>
struct android_app* GetAndroidApp(void);
#endif

namespace district_fury {
namespace platform {

bool AssetExists(const std::string& path) {
    if (path.empty()) return false;
#if defined(PLATFORM_ANDROID)
    android_app* app = GetAndroidApp();
    if (app && app->activity && app->activity->assetManager) {
        AAsset* a = AAssetManager_open(app->activity->assetManager, path.c_str(), AASSET_MODE_UNKNOWN);
        if (a) { AAsset_close(a); return true; }
    }
    return false;
#else
    return FileExists(path.c_str());
#endif
}

bool LoadBinaryFile(const std::string& path, std::vector<unsigned char>& out) {
    int size = 0;
    unsigned char* data = LoadFileData(path.c_str(), &size);
    if (!data || size <= 0) { if (data) UnloadFileData(data); return false; }
    out.assign(data, data + size);
    UnloadFileData(data);
    return true;
}

bool LoadTextFile(const std::string& path, std::string& out) {
    char* text = LoadFileText(path.c_str());
    if (!text) return false;
    out = text;
    UnloadFileText(text);
    return true;
}

bool SaveTextFile(const std::string& path, const std::string& text) {
    return SaveFileText(path.c_str(), const_cast<char*>(text.c_str()));
}

}  // namespace platform
}  // namespace district_fury
