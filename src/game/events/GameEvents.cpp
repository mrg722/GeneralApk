// Cola circular de eventos de juego (ver GameEvents.h).
#include "game/events/GameEvents.h"

namespace district_fury {
namespace {

GameEvent gQueue[kGameEventQueueSize];
int gHead = 0;      // indice del mas viejo
int gCount = 0;     // eventos pendientes
int gDropped = 0;   // descartados por cola llena

}  // namespace

void EmitGameEvent(const GameEvent& e) {
    if (gCount == kGameEventQueueSize) {   // llena: se pierde el mas viejo
        gHead = (gHead + 1) % kGameEventQueueSize;
        --gCount;
        ++gDropped;
    }
    gQueue[(gHead + gCount) % kGameEventQueueSize] = e;
    ++gCount;
}

bool PopGameEvent(GameEvent& out) {
    if (gCount == 0) return false;
    out = gQueue[gHead];
    gHead = (gHead + 1) % kGameEventQueueSize;
    --gCount;
    return true;
}

int PendingGameEvents() { return gCount; }
int DroppedGameEvents() { return gDropped; }

void EmitSimpleEvent(GameEventType type, int value) {
    GameEvent e;
    e.type = type;
    e.value = value;
    EmitGameEvent(e);
}

void ClearGameEvents() {
    gHead = 0;
    gCount = 0;
    gDropped = 0;
}

}  // namespace district_fury
