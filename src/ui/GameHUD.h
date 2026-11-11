#pragma once
#include "raylib.h"
#include "game/Types.h"
#include <string>
#include <vector>

// DF-013.2 — HUD compartido.
//
// Antes de este archivo, Stage1StoryGame, Stage2Game, Stage3Game y VSMode
// dibujaban CADA UNO a mano su propio panel de HP/Shield/SP/Rage/Combo, con
// formulas y layout casi identicos copiados y ligeramente distintos entre si
// (ver docs/AUTONOMOUS_PROGRESS.md). Solo VSMode incluia el retrato de
// Rayden que pide el diseno.
//
// Este modulo NO reemplaza el HUD completo de cada stage (titulo de stage,
// caja de boss, banner de escenario siguen siendo responsabilidad de cada
// uno, porque su contenido es real y distinto por stage). Reemplaza
// unicamente el panel de vitales del jugador (HP/Shield/SP/Rage/Combo), que
// SI era codigo duplicado, y le agrega el retrato que faltaba en Campana.
//
// Deliberadamente conserva los mismos colores, formulas y proporciones que
// ya tenia cada stage (verificado leyendo cada DrawHUD original) para no
// introducir un cambio visual que no se pueda verificar en este entorno sin
// display: la unica adicion real es el retrato.

namespace district_fury {
class Player;
class StreetEnemy;
namespace ui {

struct PlayerVitals {
    int hp = 0;
    int maxHp = 1;
    int shield = 0;
    int maxShield = 1;
    int sp = 0;
    int maxSp = 1;
    int rage = 0;
    int maxRage = 1;
    bool isRageMode = false;
    int combo = 0;

    // Panel de encabezado (titulo del stage, ej. "RAYDEN CRUZ // SLUM DISTRICT").
    const char* title = "RAYDEN CRUZ";
    // Si se indica, el retrato es el del personaje que se esta usando.
    const district_fury::Player* player = nullptr;

    // Posicion/tamano del panel. Los valores por defecto reproducen el
    // panel de Stage1StoryGame (el mas completo de los tres).
    int x = 14;
    int y = 12;
    int width = 555;
    int panelHeight = 96;  // alto del rectangulo de fondo del panel
};

// Dibuja el retrato de Rayden + panel de HP/Shield/SP/Rage/Combo. Debe
// llamarse fuera de BeginMode2D (coordenadas de pantalla, no de mundo).
void DrawPlayerVitals(const PlayerVitals& v);

// ---------------------------------------------------------------------------
// F2.3 - HUD de combate: contador de golpes con dano acumulado y escalado,
// numeros de dano flotantes, avisos (CONTRA!, JUGGLE xN, PERFECTO...) y el nombre
// del especial al lanzarlo (0.8 s).
//
// No toca el resolvedor ni la cola de eventos (que consume el audio): observa la
// vida de los enemigos entre dos Update (cada caida de vida = un golpe del
// jugador) y el ataque en curso del jugador. Un solo estado global (CombatHud());
// los stages solo llaman a Update (en su Update, ya fuera de la pausa), a
// DrawWorld (DENTRO de BeginMode2D) y a DrawScreen (en el HUD).
// ---------------------------------------------------------------------------

// Escalado de dano de un combo de n golpes (misma formula que StreetEnemy: -8 %
// por golpe a partir del 4o, minimo 35 %). 1.0 = sin escalado.
float ComboDamageScale(int hits);
// Color del contador segun el escalado (blanco -> amarillo -> naranja -> rojo).
Color ComboScaleColor(float scale);

class CombatHud {
public:
    // stageCombo: contador del stage (<= 0 = usa el propio del HUD, p. ej. en VS).
    void Update(float dt, const Player& player, const std::vector<StreetEnemy>& enemies, int stageCombo = -1);
    // Un objetivo extra (jefe): lo mismo que un enemigo. key distinto de 0..n-1.
    void ObserveTarget(int key, int hp, Vector3D pos, bool inStartup = false);
    // Aviso breve en pantalla (F2.1: "PERFECTO" del JustDefend, "GUARDIA!"...).
    void Notice(const char* text, Color color, float seconds = 0.9f, int fontSize = 30);
    // Golpe del jugador en un punto del mundo (numero flotante + cuenta de combo).
    void AddHit(int damage, Vector3D at, bool counter = false);
    void Reset();
    // Dentro de BeginMode2D (coordenadas de mundo).
    void DrawWorld() const;
    // Fuera de la camara (pantalla 1280x720).
    void DrawScreen() const;

    // Consulta (tests): estado actual.
    int Hits() const { return hits_; }
    int ComboDamage() const { return comboDamage_; }
    int FloatingCount() const { return static_cast<int>(floaters_.size()); }
    int NoticeCount() const { return static_cast<int>(notices_.size()); }
    const std::string& SpecialName() const { return special_; }
    float SpecialTime() const { return specialT_; }

private:
    struct Floater { std::string text; Vector3D at; float t = 0, life = 0.8f; Color color; int size = 20; };
    struct NoticeItem { std::string text; Color color; float t = 0, life = 0.9f; int size = 30; };
    struct Seen { int key = -1; int hp = 0; bool startup = false; bool valid = false; };
    void Observe(int key, int hp, Vector3D pos, bool startup, bool airborne, int airHits);
    std::vector<Floater> floaters_;
    std::vector<NoticeItem> notices_;
    std::vector<Seen> seen_;
    int hits_ = 0, comboDamage_ = 0, stageCombo_ = -1;
    float comboT_ = 0, shownT_ = 0;   // tiempo desde el ultimo golpe / pulso del contador
    float pulse_ = 0;
    std::string special_;
    float specialT_ = 0;
    int lastAttack_ = 0, lastState_ = -1, lastSkill_ = -1;
    int lastJuggle_ = 0;
};

CombatHud& Combat();

}  // namespace ui
}  // namespace district_fury
