#include "ui/MainMenu.h"
#include "audio/AudioSystem.h"
#include "core/DisplaySettings.h"
#include "core/InputMap.h"
#include "game/events/GameEvents.h"
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
                  {2, 7, 15, 250});

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

namespace {
Bus BusForRow(int row) {
    return row == static_cast<int>(OptionsRow::Music)   ? Bus::Music
         : row == static_cast<int>(OptionsRow::Effects) ? Bus::Sfx
                                                        : Bus::Ambient;
}
bool IsVolumeRow(int row) {
    return row == static_cast<int>(OptionsRow::Music) || row == static_cast<int>(OptionsRow::Effects) ||
           row == static_cast<int>(OptionsRow::Ambient);
}
int VolumePercent(Bus b) { return static_cast<int>(std::lround(AudioSystem::Get().BusVolume(b) * 100.0f)); }
}  // namespace

bool UpdateOptionsMenu(int& cursor) {
    AudioSystem& audio = AudioSystem::Get();
    cursor = cursor < 0 ? 0 : cursor % kOptionsRowCount;
    if (input::Pressed(KEY_UP) || input::Pressed(KEY_W)) {
        cursor = (cursor + kOptionsRowCount - 1) % kOptionsRowCount;
        EmitSimpleEvent(GameEventType::UiNavigate, cursor);
    }
    if (input::Pressed(KEY_DOWN) || input::Pressed(KEY_S)) {
        cursor = (cursor + 1) % kOptionsRowCount;
        EmitSimpleEvent(GameEventType::UiNavigate, cursor);
    }
    const int dir = (input::Pressed(KEY_RIGHT) || input::Pressed(KEY_D)) ? 1
                  : (input::Pressed(KEY_LEFT) || input::Pressed(KEY_A)) ? -1 : 0;
    const bool enter = input::Pressed(KEY_ENTER) || input::Pressed(KEY_J);
    if (cursor == static_cast<int>(OptionsRow::Sound) && (dir != 0 || enter)) {
        const bool nowMuted = !audio.IsMuted();
        if (nowMuted) EmitSimpleEvent(GameEventType::UiCancel);   // (ya silenciado: no se oye)
        audio.SetMuted(nowMuted);
        if (!nowMuted) EmitSimpleEvent(GameEventType::UiConfirm);
    } else if (IsVolumeRow(cursor) && (dir != 0 || enter)) {
        const Bus b = BusForRow(cursor);
        int pct = VolumePercent(b);
        // ENTER recorre 100 -> 90 -> ... -> 0 -> 100 (tactil sin izq/der).
        pct = dir != 0 ? pct + dir * 10 : (pct <= 0 ? 100 : pct - 10);
        pct = pct < 0 ? 0 : (pct > 100 ? 100 : pct);
        audio.SetBusVolume(b, pct / 100.0f);
        EmitSimpleEvent(GameEventType::UiNavigate, cursor);   // se oye con el volumen nuevo (EFECTOS)
    } else if (cursor == static_cast<int>(OptionsRow::ScreenWidth)) {
        if (dir != 0) display::SetWidthPercent(display::WidthPercent() + dir * display::kWidthStep);
        if (enter)
            display::SetWidthPercent(display::WidthPercent() <= display::kMinWidthPercent ? display::kMaxWidthPercent
                                     : display::WidthPercent() - display::kWidthStep);
        if (dir != 0 || enter) EmitSimpleEvent(GameEventType::UiNavigate, cursor);
    }
    if (input::Pressed(KEY_ESCAPE)) {
        EmitSimpleEvent(GameEventType::UiCancel);
        return true;
    }
    return false;
}

void DrawOptionsMenu(Texture2D art, int cursor, const char* difficultyLabel, bool touchHints) {
    DrawRectangle(0, 0, 1280, 720, {4, 7, 11, 255});
    if (art.id)
        DrawTexturePro(art, {0, 0, (float)art.width, (float)art.height}, {0, 0, 1280, 720}, {0, 0}, 0,
                       {255, 255, 255, 60});
    DrawRectangle(0, 0, 1280, 720, {3, 7, 12, 190});
    DrawText("OPCIONES", 520, 90, 48, WHITE);
    const AudioSystem& audio = AudioSystem::Get();
    const Color hi{255, 214, 72, 255}, lo{200, 220, 230, 255}, val{110, 240, 160, 255};
    static const char* kLabels[kOptionsRowCount] = {"SONIDO", "MUSICA", "EFECTOS", "AMBIENTE", "ANCHO DE PANTALLA"};
    for (int i = 0; i < kOptionsRowCount; ++i) {
        const int y = 190 + i * 58;
        const bool sel = i == cursor;
        if (sel) DrawRectangle(380, y - 12, 620, 48, {30, 50, 70, 200});
        DrawText(kLabels[i], 420, y, 26, sel ? hi : lo);
        if (i == static_cast<int>(OptionsRow::Sound)) {
            const bool muted = audio.IsMuted();
            DrawText(muted ? "DESACTIVADO" : "ACTIVADO", 720, y, 26,
                     muted ? Color{255, 110, 100, 255} : val);
        } else if (IsVolumeRow(i)) {
            const int pct = VolumePercent(BusForRow(i));
            // Deslizador: 10 segmentos.
            for (int k = 0; k < 10; ++k) {
                const bool on = k < pct / 10;
                DrawRectangle(700 + k * 18, y + 2, 14, 22, on ? (audio.IsMuted() ? Color{120, 130, 135, 255} : val)
                                                              : Color{40, 55, 65, 255});
            }
            DrawText(TextFormat("%3d%%", pct), 900, y, 26, sel ? hi : lo);
        } else {
            DrawText(TextFormat("< %d%% >", display::WidthPercent()), 760, y, 26, val);
        }
    }
    DrawText(touchHints ? "CRUCETA ARRIBA/ABAJO: ELEGIR   IZQ/DER: CAMBIAR   OK: ALTERNAR"
                        : "W/S ELEGIR   A/D CAMBIAR   ENTER/J ALTERNAR",
             touchHints ? 330 : 420, 490, 18, {150, 190, 200, 230});
    DrawText("ANCHO: si tu celular es muy alargado y todo se ve ancho, bajalo (solo cambia lo horizontal).",
             200, 525, 16, {150, 175, 185, 220});
    if (difficultyLabel)
        DrawText(TextFormat("DIFICULTAD ACTUAL: %s", difficultyLabel), 440, 560, 18, {150, 190, 200, 230});
    DrawText(touchHints ? "ATRAS - VOLVER" : "ESC - VOLVER", 540, 610, 20, {180, 195, 200, 220});
}

}  // namespace ui
}  // namespace district_fury
