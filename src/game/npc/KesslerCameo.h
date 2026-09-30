#pragma once
#include "raylib.h"
#include <array>

// DF-013.2 — Dr. Kessler como NPC narrativo (canon de la hoja del usuario:
// "sin habilidades de combate, solo animaciones narrativas").
//
// Aparece durante el BossIntro de los ultimos stages: entra caminando,
// dice sus lineas junto al boss y se va caminando, dejando al boss.
// NO tiene hurtbox ni hitbox y no se agrega a ninguna lista de combate,
// asi que es imposible golpearlo y el no puede dañar a nadie.
//
// Sprites: busca PNG transparentes en assets/npc/kessler/ (idle.png,
// walk1.png, walk2.png). Las laminas de referencia recibidas tienen fondo
// de panel opaco y no se pueden recortar limpias sin comerse el pantalon
// negro, asi que mientras no existan esos PNG se dibuja una silueta
// temporal (bata blanca, pelo gris, gafas). Ver docs/AUTONOMOUS_PROGRESS.md.
namespace district_fury {

class KesslerCameo {
public:
    enum class State { Hidden, Entering, Talking, Leaving, Done };

    // enterFromX: donde aparece; standX: donde se para (junto al boss);
    // exitX: hacia donde se va. groundY: linea de pies.
    void Start(float enterFromX, float standX, float exitX, float groundY,
               const std::array<const char*, 3>& lines);
    void Update(float dt);
    void Skip();
    void DrawWorld() const;      // dentro de BeginMode2D
    void DrawDialogue() const;   // en espacio de pantalla
    bool IsDone() const { return state == State::Done || state == State::Hidden; }
    bool IsActive() const { return state != State::Hidden && state != State::Done; }

private:
    State state{State::Hidden};
    float x{0}, groundY{0}, standX{0}, exitX{0};
    float timer{0};
    float walkTime{0};
    int line{0};
    bool facingRight{false};
    std::array<const char*, 3> lines{nullptr, nullptr, nullptr};
};

}  // namespace district_fury
