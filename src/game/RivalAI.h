#pragma once
#include "game/Player.h"
#include "game/RivalAIProfile.h"
#include <vector>

namespace district_fury {

// Rival controlado por la maquina en el Modo VS: un Player completo (mismos
// movimientos, habilidades por paginas, transformacion y bloqueo que el
// jugador) manejado con entradas simuladas, como un bot de pelea.
//
// F2.2: la IA ya no se limita a J J J K + habilidades. Lee el MoveSet efectivo
// del personaje (`self.Moves()` y `self.MoveEnabled(id)`) y elige entre RUTAS
// que ESE personaje si puede hacer: cadenas J/K, lanzador J J K + persecucion
// aerea, normales de comando (>J <K vJ...), salto con aereos, especiales por
// comando (QCF/QCB + boton, por `scriptedInput` con la secuencia de direcciones
// frame a frame), Signature con SP, y bloqueo/retirada segun su perfil. Nunca
// pide un movimiento que el personaje no tenga habilitado (cae a nada).
class RivalAI {
public:
    void Reset();
    PlayerInput Think(const Player& self, const Player& target, float dt);

    // --- Tests / telemetria (no se usan en el juego salvo el log de VS) ---
    // Fuerza un perfil concreto (RivalAITests prueba cada arquetipo). nullptr =
    // el perfil derivado del personaje (ProfileFor(skin)).
    const AiProfile* profileOverride{nullptr};
    // Ultimo "play" (ruta) que la IA decidio ejecutar: lo rellena al elegir, para
    // que un log de depuracion o un test vea que movimientos saca el rival.
    AttackId lastPlay{AttackId::None};
    // true un solo frame: la IA acaba de decidir un play nuevo (flanco).
    bool playChanged{false};

private:
    // Guion de entradas: una lista de frames ya resueltos que se reproducen uno
    // por Think() (asi una ruta de varios frames —un especial por comando, un
    // salto con aereo, una cadena— se ejecuta de forma determinista).
    std::vector<PlayerInput> script_;
    std::size_t scriptPos_{0};

    float decide_{0.6f};       // espera hasta la proxima decision
    float blockTimer_{0.0f};   // bloqueo reactivo en curso
    float retreatTimer_{0.0f}; // retirada tras una ruta
    float farTimer_{0.0f};     // tiempo seguido fuera de rango de pelea
    bool mustClose_{false};    // comprometido a entrar a melee (hasta pegarse)
    int pageFlips_{0};

    bool HasScript() const { return scriptPos_ < script_.size(); }
    void ClearScript() { script_.clear(); scriptPos_ = 0; }
    // Anade `frames` copias de `f` al guion.
    void Add(const PlayerInput& f, int frames = 1);
    void SetPlay(AttackId id);
    // Devuelve el primer frame del guion recien construido (y libera si era 1).
    PlayerInput TakeFront();
    // Frames de acercamiento hasta ~70 px; devuelve cuantos anadio.
    int AddApproach(float dir, float dist, float dy);
    // Elige y guioniza una ruta de cuerpo a cuerpo segun perfil y capacidades.
    void ChooseClosePlay(const Player& self, const Player& target, const AiProfile& profile,
                         float dir, float dy, float dist);
    void BuildChain(const Player& self, float dir, float dy, float dist, const AiProfile& profile);
    bool BuildCommandNormal(const Player& self, float dir, float dist, float dy);
    bool BuildLauncher(const Player& self, float dir, float dist, float dy);
    bool BuildJumpIn(const Player& self, float dir);
    bool BuildSpecial(const Player& self, float dir, bool allowRanged);
    // Secuencia frame a frame de un cuarto de circulo (QCF side=+1 / QCB side=-1) + boton.
    void AddMotion(float dir, int side, bool kick);
};

}  // namespace district_fury
