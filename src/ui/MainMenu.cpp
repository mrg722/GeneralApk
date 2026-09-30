#include "ui/MainMenu.h"
#include <cmath>

namespace district_fury {
namespace ui {
namespace {

// Posiciones medidas directamente sobre el arte fuente (1672x941, centroide
// de brillo por fila) y escaladas al canvas del juego (1280x720). El arte y
// el canvas comparten casi la misma proporcion (1.776 vs 1.778), asi que
// estirarlo a pantalla completa no lo distorsiona de forma perceptible.
constexpr float kItemY[kMainMenuItemCount] = {333.f, 367.f, 403.f, 437.f, 472.f, 506.f, 540.f};
constexpr float kTextLeft = 110.f;

const char* kItemLabels[kMainMenuItemCount] = {
    "NUEVA PARTIDA", "MODO VS", "DIFICULTAD", "CONTROLES", "OPCIONES", "CREDITOS", "SALIR",
};

// Bloque completo donde el arte trae los 7 renglones + el resaltado propio de
// "NUEVA PARTIDA" horneados en la imagen. Se cubre una sola vez con un parche
// oscuro y luego se redibujan los 7 labels a mano: asi el texto queda legible
// sin importar cual item este seleccionado (dibujarlo semi-transparente
// encima del arte original lo volvia ilegible).
constexpr Rectangle kMenuColumnPatch = {46.f, 308.f, 385.f, 253.f};

void DrawSelectorBar(float centerY) {
    const float height = 34.f;
    const float y = centerY - height * 0.5f;
    const float x = 79.f;
    const float width = 335.f;
    const float pulse = std::sin(static_cast<float>(GetTime()) * 5.5f) * 0.5f + 0.5f;

    const Color fill = {14, 62, 122, 200};
    const Color edge = {90, 205, 255, static_cast<unsigned char>(190 + 40 * pulse)};

    DrawRectangle(static_cast<int>(x), static_cast<int>(y), static_cast<int>(width),
                  static_cast<int>(height), fill);
    DrawRectangleLines(static_cast<int>(x), static_cast<int>(y), static_cast<int>(width),
                       static_cast<int>(height), edge);

    // Chevron doble al final de la barra, como en el arte original.
    for (int i = 0; i < 2; ++i) {
        const float cx = x + width + 6.f + i * 11.f;
        DrawTriangle({cx, y}, {cx, y + height}, {cx + 9.f, y + height * 0.5f}, edge);
    }

    // Flecha indicadora a la izquierda del texto (parpadeante).
    const float ax = kTextLeft - 24.f;
    const Color arrowColor = {220, 245, 255, static_cast<unsigned char>(200 + 55 * pulse)};
    DrawTriangle({ax, centerY - 8.f}, {ax, centerY + 8.f}, {ax + 12.f, centerY}, arrowColor);
}

}  // namespace

void DrawMainMenuArt(Texture2D art, int selectedIndex, const char* difficultyLabel) {
    DrawRectangle(0, 0, 1280, 720, {4, 7, 11, 255});

    if (art.id != 0) {
        const Rectangle source = {0, 0, static_cast<float>(art.width), static_cast<float>(art.height)};
        const Rectangle dest = {0, 0, 1280, 720};
        DrawTexturePro(art, source, dest, {0, 0}, 0.0f, WHITE);
    }

    selectedIndex = selectedIndex < 0 ? 0 : (selectedIndex >= kMainMenuItemCount
        ? kMainMenuItemCount - 1 : selectedIndex);

    // Un unico parche oscuro (con degrade suave via dos rectangulos) apaga
    // los 7 labels horneados y el resaltado por defecto del arte.
    DrawRectangle(static_cast<int>(kMenuColumnPatch.x), static_cast<int>(kMenuColumnPatch.y),
                  static_cast<int>(kMenuColumnPatch.width), static_cast<int>(kMenuColumnPatch.height),
                  {2, 7, 15, 214});

    DrawSelectorBar(kItemY[selectedIndex]);

    const Font font = GetFontDefault();
    for (int i = 0; i < kMainMenuItemCount; ++i) {
        const bool selected = (i == selectedIndex);
        const Color color = selected ? Color{255, 240, 200, 255} : Color{198, 212, 222, 235};
        const float fontSize = selected ? 21.f : 20.f;
        DrawTextEx(font, kItemLabels[i], {kTextLeft, kItemY[i] - fontSize * 0.5f}, fontSize, 3.0f, color);
    }

    if (selectedIndex == static_cast<int>(MainMenuItem::Difficulty) && difficultyLabel != nullptr) {
        const float y = kItemY[static_cast<int>(MainMenuItem::Difficulty)];
        const int labelWidth = MeasureText(difficultyLabel, 16);
        DrawText(difficultyLabel, 445, static_cast<int>(y) - 8, 16, {110, 220, 255, 255});
        DrawText("< >", 445 + labelWidth + 10, static_cast<int>(y) - 8, 16, {150, 190, 205, 200});
    }
}

}  // namespace ui
}  // namespace district_fury
