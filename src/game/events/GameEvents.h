// Cola de eventos de juego (contrato F0.1 del plan maestro de movimientos).
//
// El combate, los stages y la UI EMITEN eventos ("golpe conectado", "salto",
// "entro al modo VS"...). El audio (AudioDirector, MusicPlayer) y cualquier otro
// oyente solo los CONSUMEN: ningun bloque de audio toca archivos de combate.
//
// Cola circular de 256 eventos, de un solo hilo. Si se llena, se descarta el mas
// viejo y se cuenta en DroppedGameEvents(). Los tipos solo crecen al final (antes
// de Count) para no mover valores.
#pragma once

namespace district_fury {

enum class GameEventType {
    HitLanded, HitBlocked, HitArmored, CounterHit,
    PlayerHurt, PlayerBlocked, PlayerGuardBreak, PlayerKnockdown, PlayerDefeat,
    EnemyKilled, EnemyGuardBreak,
    BossHit, BossPhase, BossAttack,
    ProjectileSpawn, ProjectileHit,
    Swing, SpecialStart, RageStart, TransformStart,
    Dash, Jump, Land, KnockdownLand, Footstep,
    Pickup, PropHit, ComboMilestone, Telegraph,
    UiNavigate, UiConfirm, UiCancel, UiOpen, UiPause,
    ModeEnter, ArenaLock, ArenaUnlock, WaveStart, StageClear, GameOver, Victory,
    Count
};

// Familia de un golpe (elige el sonido / la chispa). Tambien la usa HitTypes.h.
enum class HitKind { Light, Medium, Heavy, Kick, Slash, Energy, Block, GuardBreak, Prop };

// Modo de juego activo (para ModeEnter y la musica por modo).
enum class GameMode { Boot, Menu, CharacterSelect, Options, Reward, Stage1, Stage2, Stage3, Stage4, Stage5, Boss, VS, Beta };

struct GameEvent {
    GameEventType type = GameEventType::Count;   // Count = sin tipo (no emitir asi)
    float x = 0, y = 0;          // posicion en el mundo (x, carril y) si aplica
    int skin = -1;               // personaje que lo origina (-1 = ninguno / enemigo)
    HitKind kind = HitKind::Light;
    int weight = 0;              // peso: 1 normal, 2 medio, 3 pesado (pasos, golpes)
    int value = 0;               // dato libre: dano, combo, GameMode, oleada...
    int attack = 0;              // static_cast<int>(AttackId) si aplica
    bool heavy = false;
    bool fromEnemy = false;
};

// Encola un evento (descarta el mas viejo si la cola esta llena).
void EmitGameEvent(const GameEvent& e);
// Saca el evento mas viejo; false si la cola esta vacia. Lo usa DrainGameEvents.
bool PopGameEvent(GameEvent& out);
// Eventos pendientes en la cola.
int PendingGameEvents();
// Eventos descartados por cola llena desde el ultimo ClearGameEvents().
int DroppedGameEvents();
// Vacia la cola y pone a 0 el contador de descartados (tests, cambio de modo).
void ClearGameEvents();

constexpr int kGameEventQueueSize = 256;

// Atajo (F0.2): evento sin posicion ni personaje (UI, modo, oleada...).
void EmitSimpleEvent(GameEventType type, int value = 0);
// Atajo: entrada a un modo de juego (ModeEnter con value = GameMode).
inline void EmitModeEnter(GameMode mode) { EmitSimpleEvent(GameEventType::ModeEnter, static_cast<int>(mode)); }

// Entrega a `f(const GameEvent&)` todos los eventos pendientes, del mas viejo al
// mas nuevo, y deja la cola vacia.
template <class F>
void DrainGameEvents(F&& f) {
    GameEvent e;
    while (PopGameEvent(e)) f(static_cast<const GameEvent&>(e));
}

}  // namespace district_fury
