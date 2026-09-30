#pragma once
#include "raylib.h"

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

}  // namespace ui
}  // namespace district_fury
