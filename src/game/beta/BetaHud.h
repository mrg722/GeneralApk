#pragma once
#include "raylib.h"

namespace district_fury {
class Player;
namespace beta_hud {

// Interfaz de batalla original de nuevosSprites (assets/beta/ui): barra con
// forma de espada y retrato, barra de jefe de 3 capas, barritas de enemigos,
// contador de golpes HITS y numeros de dano con las fuentes del juego.

// Barra del jugador arriba a la izquierda. `portrait`: retrato del guerrero si
// es el heroe; si no, se dibuja el primer cuadro del personaje en el circulo.
void DrawPlayer(const Player& p, bool heroPortrait, const char* name);
// Barra de jefe arriba al centro (3 capas: verde, amarilla, roja; "x N").
void DrawBossBar(const char* name, int hp, int maxHp);
// Barrita sobre un enemigo (centro arriba en coordenadas de mundo/pantalla).
void DrawEnemyBar(Vector2 center, float ratio);
// Contador de golpes ("12 HITS") a la derecha.
void DrawCombo(int hits, float alpha);
// Numero con una hoja de digitos: sheet en assets/beta/ui, `cells` celdas
// iguales y el 0 en `zeroCell`. Centrado en `pos`, alto `h`.
void DrawDigits(const char* sheet, int cells, int zeroCell, int value, Vector2 pos, float h, Color tint);
// Pociones: contadores junto a sus botones (tactil) o abajo con su tecla.
void DrawPotions(int red, int blue, bool touch);

}  // namespace beta_hud
}  // namespace district_fury
