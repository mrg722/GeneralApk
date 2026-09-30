#pragma once
#include "game/Types.h"

// DF-013.2 (19-09) — personajes jugables del Modo VS.
//
// El Rayden ORIGINAL es el id 0 y NO se toca: mantiene su atlas, sus stats y
// su ruta de dibujo de siempre. Los demas ids reutilizan los sets de poses que
// ya existen en assets/ (clon y bosses) y se dibujan con el mismo Player: la
// mecanica, los hitboxes y los ataques son los del jugador.
//
// Lo que SI cambia por personaje: sprite, escala, vida maxima y multiplicador
// de dano/velocidad, tomados de su BossDefinition cuando existe.
//
// Lo que NO esta implementado (declarado, no inventado): los ataques propios
// de cada boss (chain throw, sierra, shockwave...) no son movimientos nuevos
// del jugador; se usan sus poses sobre los ataques que Rayden ya tiene.
namespace district_fury {

enum class PlayerState;   // game/Player.h
enum class AttackType;    // game/Types.h

struct CharacterVisual {
    const char* name;
    const char* folder;       // carpeta en assets/bosses/, nullptr = atlas de Rayden
    bool uniformCanvas;       // true: escala fija (Brakk/Grinder); false: por altura
    float scale;              // pixeles por pixel de sprite (uniformCanvas)
    float targetHeight;       // altura en pantalla (no uniformCanvas)
    float footInset;          // pixeles bajo los pies dentro del lienzo
    bool facesRightByDefault; // orientacion nativa del arte
    int maxHp;
    float damageMultiplier;
    float speedMultiplier;
};

int CharacterCount();
const CharacterVisual& GetCharacterVisual(int id);
// Devuelve la pose del set para el estado actual del jugador.
const char* CharacterPose(int id, PlayerState state, AttackType attack, bool rage, double time);

}  // namespace district_fury
