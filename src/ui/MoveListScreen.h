#pragma once
// F2.3 - Lista de movimientos del personaje (pantalla de pausa).
//
// BuildMoveList(player) lee los movimientos REALES del personaje (MoveSet,
// MoveEnabled, EffectiveDef y displayName; no define nada propio) y los agrupa en
// paginas: Cadenas, Direccionales, Aire, Especiales, Furia, Defensa/Agarre y
// Habilidades. Solo aparecen los movimientos habilitados para ESE personaje.
//
// Entradas: texto compacto que la pantalla dibuja con iconos.
//   ">" adelante, "<" atras, "v" abajo, "^" arriba (relativos al facing)
//   "J" "K" "L" botones; "QCF" "QCB" "DP" comandos (flechas); "+" a la vez;
//   " " en secuencia; " / " alternativa; cualquier otro texto = tecla (SHIFT, B...).
#include "game/combat/AttackData.h"
#include "raylib.h"
#include <string>
#include <vector>

namespace district_fury {
class Player;
namespace ui {

struct MoveRow {
    AttackId id = AttackId::None;   // None = fila que no es un ataque (salto, guardia...)
    std::string name;               // nombre en espanol (displayName o fila de AttackData)
    std::string input;              // entrada (ver arriba)
    std::string note;               // contexto breve ("en el aire", "con furia")
    int damage = -1;                // dano base (-1 = no aplica)
    int hits = 1;                   // golpes por activacion
    int spCost = 0;                 // coste de energia
};

struct MovePage {
    std::string title;
    std::vector<MoveRow> rows;
};

// Paginas no vacias del personaje. Toca `skillPage` de los KF de forma temporal
// (lo deja como estaba).
std::vector<MovePage> BuildMoveList(Player& player);

// Medidas (en px, a 1280x720) que comparten la pantalla y el test.
constexpr int kMoveNameFont = 16;
constexpr int kMoveNameColW = 290;
constexpr int kMoveInputColW = 420;
constexpr int kMoveNoteColW = 215;
constexpr int kMoveMinFont = 11;
// Ancho de `text` con la fuente por defecto de raylib (o una estimacion
// conservadora si no hay ventana: tests sin display).
int MeasureUiText(const char* text, int fontSize);
// Mayor tamano de fuente <= fontSize (y >= kMoveMinFont) con que `text` cabe en maxWidth.
int FitFontSize(const char* text, int maxWidth, int fontSize);
// Ancho que ocupa la entrada dibujada con iconos.
int MoveInputWidth(const std::string& input);

class MoveListScreen {
public:
    // Abre la lista del personaje de `player`.
    void Open(Player& player);
    void Close() { open_ = false; }
    bool IsOpen() const { return open_; }
    // Teclado y toque (flechas/cruceta, pestanas, ESC/M/ENTER cierran). Llamar cada
    // frame mientras IsOpen(), ANTES de la logica de pausa (ESC cierra la lista, no
    // reanuda).
    void Update();
    void Draw() const;
    // Pruebas.
    int PageCount() const { return static_cast<int>(pages_.size()); }
    int PageIndex() const { return page_; }
    int Cursor() const { return cursor_; }
    const std::vector<MovePage>& Pages() const { return pages_; }
    void SelectPage(int p);

private:
    bool open_ = false;
    std::vector<MovePage> pages_;
    std::string title_;
    int page_ = 0, cursor_ = 0, scroll_ = 0;
};

MoveListScreen& MoveList();

// Desde un menu de pausa: true si el jugador pide la lista (tecla M, ENTER si
// allowEnter, o toque/clic dentro de `tapArea`, coordenadas virtuales).
bool MoveListRequested(Rectangle tapArea, bool allowEnter = true);
// Patron comun de la pausa de un stage: si la lista esta abierta la actualiza y
// devuelve true (el stage debe salir de su Update sin tratar ESC como "reanudar");
// si `paused` es false y estaba abierta, la cierra.
bool UpdateMoveListInPause(bool paused);

}  // namespace ui
}  // namespace district_fury
