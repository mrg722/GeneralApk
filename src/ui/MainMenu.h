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

// OPCIONES (F1.10): SONIDO (mute global), MUSICA, EFECTOS, AMBIENTE (0-100 %,
// paso 10) y ANCHO DE PANTALLA. Los volumenes van a los buses de AudioSystem y
// se guardan solos (audio_settings.txt); el ancho, en display_settings.txt.
// Suenan los eventos de UI (UiNavigate/UiConfirm/UiCancel).
enum class OptionsRow { Sound = 0, Music, Effects, Ambient, ScreenWidth, Count };
constexpr int kOptionsRowCount = static_cast<int>(OptionsRow::Count);

// Lee la entrada de un frame (arriba/abajo elige, izq/der cambia, ENTER alterna,
// ESC sale). Devuelve true si se pidio volver al menu.
bool UpdateOptionsMenu(int& cursor);
// Dibuja la pantalla completa. `art` = fondo del menu (puede ser id 0);
// `difficultyLabel` se muestra como informacion (puede ser nullptr).
void DrawOptionsMenu(Texture2D art, int cursor, const char* difficultyLabel, bool touchHints);

}  // namespace ui
}  // namespace district_fury
