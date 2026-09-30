#pragma once
// Acceso a archivos igual en PC y Android. En Android los assets viven dentro
// del APK (AAssetManager) y los guardados en el almacenamiento interno; las
// funciones de archivo de raylib ya redirigen ahi, std::ifstream no.
#include <string>
#include <vector>

namespace district_fury {
namespace platform {

bool AssetExists(const std::string& path);
bool LoadBinaryFile(const std::string& path, std::vector<unsigned char>& out);
bool LoadTextFile(const std::string& path, std::string& out);
bool SaveTextFile(const std::string& path, const std::string& text);

}  // namespace platform
}  // namespace district_fury
