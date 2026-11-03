// Tipos del framework de movimientos (contrato F0.1 del plan maestro).
//
// Cada personaje puede declarar un MoveSet (src/game/moves/Moves_<Bloque>.cpp)
// con cadenas, capacidades, overrides por ataque y su visual. Lo consulta
// GetMoveSet(skin) (MoveRegistry.h). F0.3 llena la parte visual (PoseFx...),
// F0.4 la de combate; los bloques de Fase 1 SOLO escriben datos.
//
// Reglas del contrato:
// - Los ids de ataque se guardan como int = static_cast<int>(AttackId) para no
//   acoplar este header al enum (0 = AttackId::None = hueco / sin definir).
// - Todos los campos tienen un valor por defecto NEUTRO (no cambia nada).
// - Despues de Fase 0 solo se AGREGAN campos al final de cada struct.
#pragma once
#include "raylib.h"
#include "game/combat/AttackData.h"
#include "game/moves/MoveCurves.h"   // Ease, RootCurve, LimbArc

namespace district_fury {

// Clave de transformacion visual en la fase `phase` ('S' arranque, 'A' activo,
// 'R' recuperacion) en el instante normalizado v de esa fase.
struct FxKey { char phase = 'A'; float v = 0; float sx = 1, sy = 1, angle = 0, dx = 0, dy = 0; };

// Presets procedurales de V6 (los implementa F0.3 en PoseFxPresets.h).
enum class FxPreset { None, Anticipa, Impacto, Recobro, Saltito, Upper, Giro, Barrida, CaidaRot, Temblor, Latido, Tinte };

// Uso de un preset: a/b = parametros (k, h, n, amp/f, a/hz...) segun el preset.
// phase: 'S', 'A', 'R' (fase del ataque; en estados no-ataque todo el estado es
// 'A') o '*' = todo el movimiento (v sobre S+A+R, t = segundos desde el inicio).
// color (F0.3): color de TINTE (a = duracion en s, <= 0 = toda la fase).
struct PresetUse { FxPreset preset = FxPreset::None; float a = 1, b = 0; char phase = 'A'; Color color{255, 255, 255, 255}; };

// Superposicion de cuadros first..first+count-1 de assets/fx/efectos_azules.png
// (rejilla 8x5) anclada en (x, y) relativo a los pies.
struct OverlayFx { int first = -1, count = 0; float x = 0, y = -70, scale = 1, rot = 0; char phase = 'A'; Color tint = WHITE; };

// Visual procedural de un movimiento: presets + claves + estelas + overlays.
struct PoseFx {
    PresetUse presets[6]; int presetCount = 0;
    FxKey keys[8]; int keyCount = 0;
    int ghostCount = 0; float ghostInterval = .03f; Color ghostTint{110, 190, 255, 255};
    OverlayFx overlay[2]; int overlayCount = 0;
};

// Pieza de una pose suelta: textura "<carpeta>_<pose>" (o recorte src de ella).
struct PosePiece { const char* pose = nullptr; Rectangle src{}; Vector2 offset{}; bool facesRight = true; };

// Tramo [v0, v1] de la fase `phase` dibujado con 1-2 piezas.
struct PoseSegment { char phase = 'A'; float v0 = 0, v1 = 1; PosePiece pieces[2]; int pieceCount = 1; };

// Tramo de un clip del atlas: cuadros from..to (-1 = todo el clip).
struct ClipSeg { const char* clip = nullptr; int from = -1, to = -1; float speed = 1; bool reverse = false; };

// Visual completo de un movimiento o estado: hasta 3 clips seguidos (atlas) o
// hasta 4 segmentos de poses sueltas, mas su PoseFx.
struct MoveVisual {
    ClipSeg clips[3]; int clipCount = 0;
    const char* clipFallback = nullptr;
    PoseSegment segs[4]; int segCount = 0;
    PoseFx fx;
    // F0.3: estados no-ataque (MoveSet.hit/walk/...): duracion en s de la
    // secuencia (0 = la natural de sus clips) y si se repite (walk, block...).
    float duration = 0;
    bool loop = false;
};

// Proyectil propio de un movimiento (disparado una vez por activacion).
struct ProjectileSpec {
    bool enabled = false;
    float speed = 620, life = 1.2f;
    int damage = 0;
    Vector2 origin{60, -76};
    float scale = 1;
    Color tint = WHITE;
    int count = 1;
    float spreadLane = 0, interval = 0;
    bool arc = false;
    float arcRange = 260;
};

// Ajuste de un ataque para un personaje. id = static_cast<int>(AttackId).
// replaceDef = usa `def` entero en vez de la fila generica de AttackData.
struct MoveOverride {
    int id = 0;
    bool enabled = true;
    bool replaceDef = false;
    AttackDef def{};
    float damageScale = 1, timeScale = 1, reachScale = 1;
    bool hasArc = false;
    LimbArc arc;
    RootCurve root;
    int hitCount = 1;
    float multiHitInterval = 0;
    float armorFrom = -1, armorTo = -1;   // ventana de superarmadura (s), -1 = sin
    int armorHits = 0;
    float areaRadius = 0;                 // > 0 = golpe de area (todos en el radio)
    float pullPx = 0;                     // atrae a la victima hacia el atacante
    ProjectileSpec projectile;
    MoveVisual visual;
    const char* displayName = nullptr;    // nombre en la lista de movimientos
};

// Capacidades del personaje: lo no soportado cae al movimiento base (V2).
enum MoveCaps : unsigned {
    CapJump = 1u << 0, CapAir = 1u << 1, CapLauncher = 1u << 2, CapSweep = 1u << 3, CapCommandNormals = 1u << 4,
    CapSpecials = 1u << 5, CapGrab = 1u << 6, CapRun = 1u << 7, CapBackDash = 1u << 8, CapHeavyJump = 1u << 9,
    CapAll = 0x3FF
};

// Cuanto "feel" procedural recibe el personaje (KF/BETA usan Light o None).
// Atlas propios y poses sueltas: Full (por defecto) = respiracion, paso, dash,
// impacto, retroceso, aterrizaje; None = sin nada. KF/BETA/RONIN (dibujan su clip
// tal cual): SOLO `Light` enciende algo (inclinacion y temblor al recibir golpe,
// caida rotada si el clip de derribo tiene <= 2 cuadros, rebote de victoria);
// Full o None = nada (pixel identico), asi un MoveSet de KF que no toque `feel`
// no cambia su dibujo.
enum class FeelLevel { None, Light, Full };

struct CancelRule;   // F0.4: cancelaciones en datos (definida al final del archivo)

// Moveset de un personaje. Las cadenas y los botones de habilidad se guardan como
// ids (int); 0 = hueco. Los valores por defecto de las cadenas los pone F0.4
// (chainJ = Punch1, Punch2, PunchHook, Punch3; chainK = Kick, Kick2, Kick3) cuando
// existan esos ids: hasta entonces nadie lee estos campos.
struct MoveSet {
    const char* name = "generico";
    unsigned caps = CapAll;
    FeelLevel feel = FeelLevel::Full;
    float bodyScale = 1;
    int chainJ[6] = {}; int chainJCount = 4;
    int chainK[4] = {}; int chainKCount = 3;
    int skillSlots[5] = {};
    const MoveOverride* overrides = nullptr; int overrideCount = 0;
    MoveVisual hit, knockdown, getup, victory, jumpRise, jumpFall, walk, block;
    float jumpVz = 720;
    // ---- F0.4 ----
    // Reglas de cancelacion propias (se prueban ANTES de la tabla generica de
    // player/PlayerCommon.h; replaceCancels = solo las propias).
    const CancelRule* cancels = nullptr; int cancelCount = 0;
    bool replaceCancels = false;
    float walkSpeedScale = 1;   // multiplica la velocidad de caminar (y la carrera x1.45)
};

// ---- F0.4: contrato de combate de los MoveSet (lo leen Player y MovesetTests) ----
//
// Cadenas: chainJ[0..chainJCount-1] / chainK[0..chainKCount-1]. Una cadena con su
// primer eslabon en 0 usa la de por defecto (J: Punch1, Punch2, PunchHook, Punch3;
// K: Kick, Kick2, Kick3). Un eslabon que no se puede usar (requiresOverride sin
// override, o enabled=false) corta la cadena ahi.
// skillSlots[0..4]: todos en 0 = EnergyWave, Uppercut, RageAttack, DashAttack,
// Finisher (no-KF). Un hueco en 0 = el de por defecto de ese boton.
// Habilitacion de un ataque (MoveEnabled): override.enabled manda; sin override,
// los ids con requiresOverride (Signature1-3, Chain5-6, Fase 2) estan apagados y
// el resto depende de caps (CapCommandNormals: >J >K <J <K; CapSweep: vK, Stomp;
// CapLauncher: J J K, vJ; CapSpecials: DP/QCB/QCF+K; CapAir/CapJump: salto y
// aereos; CapRun / CapBackDash). Lo no habilitado cae al movimiento base (V2).

// Fuentes/destinos especiales de una CancelRule (ademas de un AttackId > 0).
enum CancelToken : int {
    kCancelAnyNormal = -1,    // from: normal de suelo (cadenas, de comando, dash)
    kCancelChainJ = -2,       // from: cualquier eslabon J; to: siguiente eslabon J (ultimo -> nada, B12)
    kCancelChainK = -3,       // from: cualquier eslabon K; to: siguiente eslabon K
    kCancelChainJ1 = -4,      // from: 1er eslabon J
    kCancelChainJ2 = -5,      // from: 2o eslabon J (J J K)
    kCancelSpecialMove = -6,  // from: especial o signature
    kCancelChainJStart = -7,  // to: 1er eslabon J (K J reinicia)
    kCancelChainKStart = -8,  // to: 1er eslabon K
    kCancelRequested = -9,    // to: lo que pide la entrada (comando especial o direccion)
    kCancelTurnChainJ2 = -10, // to: girar y seguir con el 2o eslabon J (<J J)
    kCancelAnyAttack = -11,   // from: cualquier ataque
    kCancelAir = -12,         // from: ataque aereo
};
// Comando de una regla: static_cast<int>(InputCommand) (Punch 0, Kick 1, ...) o
// kCmdAnySpecial = cualquier comando especial (onda, shoryu, huracan, ...).
constexpr int kCmdAnySpecial = -1;
// Direccion RELATIVA al facing al pulsar (V2). Neutral = sin horizontal ni abajo.
enum class CancelDir { Any, Neutral, Fwd, Back, Down };
// Desde `from`, el comando `cmd` con direccion `dir` cancela en `to` si el golpe
// conecto (onHit) o fallo (onWhiff). Ventana: en hit desde startup + 1 f (o
// AttackDef::cancelFrom), en fallo desde la recuperacion. Gana la primera que
// cumple; to = 0 bloquea explicitamente.
struct CancelRule {
    int from = kCancelAnyAttack;
    int cmd = 0;
    CancelDir dir = CancelDir::Any;
    bool onHit = true, onWhiff = true;
    int to = 0;
};

}  // namespace district_fury
