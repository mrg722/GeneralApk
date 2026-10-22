#pragma once
#include "raylib.h"

// DF-013: menu principal unico, construido sobre el arte final del juego.
// Sustituye la duplicacion historica (main.cpp::DrawMenuPrincipal +
// Stage1StoryGame::DrawMenu dibujaban dos menus distintos y solo uno de los
// dos llegaba a verse). Este modulo no incluye Player.h ni ui/SpanishText.h:
// no hereda la macro global DrawText (ver DF-013 D1), por lo que dibuja sus
// textos (ya en espanol) directamente con la API de raylib.

namespace district_fury {
namespace ui {

// Los siete items visibles en el arte, de arriba hacia abajo.
enum class MainMenuItem {
    NewGame = 0,
    VsMode,
    Difficulty,
    Controls,
    Options,
    Credits,
    Exit,
    Count
};

constexpr int kMainMenuItemCount = static_cast<int>(MainMenuItem::Count);

// Dibuja el fondo ilustrado a pantalla completa, los 7 items con el estilo
// del arte y el selector sobre el item activo. `difficultyLabel` se muestra
// junto a DIFICULTAD cuando esta seleccionado.
void DrawMainMenuArt(Texture2D art, int selectedIndex, const char* difficultyLabel);

}  // namespace ui
}  // namespace district_fury
