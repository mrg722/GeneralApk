// Director de audio (ver AudioDirector.h).
//
// F0.2: drena la cola de eventos de juego y la traduce 1:1 al Sfx LEGADO, para
// que suene lo mismo que antes de que el combate dejara de llamar a
// AudioSystem::Play, MAS lo que faltaba (dano ligero al jugador -> Hit, enemigo
// derrotado -> EnemyDeath, golpe de onda al jefe -> HeavyHit, onda lanzada ->
// EnergyShot). Los eventos de modo/arena/jefe se reenvian a MusicPlayer.
// F1.10 reescribe este archivo: todo el mapeo vive en LegacySfxFor.
#include "audio/AudioDirector.h"
#include "audio/AudioSystem.h"
#include "audio/MusicPlayer.h"
#include "game/CharacterVisual.h"
#include "game/combat/AttackData.h"
#include "game/events/GameEvents.h"
#include <cstring>

namespace district_fury {
namespace {

bool SkinNamed(int skin, const char* name) {
    if (skin < 0 || skin >= CharacterCount()) return false;
    const char* n = GetCharacterVisual(skin).name;
    return n && std::strcmp(n, name) == 0;
}

// Variante 0/1/2 del sonido de impacto segun el golpe de la cadena (antes
// comboStep % 3: Punch1 0, Punch2 1, Punch3 2, el resto 0).
int Variant(int attack) {
    switch (static_cast<AttackId>(attack)) {
        case AttackId::Punch2: return 1;
        case AttackId::Punch3: return 2;
        default:               return 0;
    }
}

// Impacto del golpe del jugador (lo que antes sonaba en PlayerDraw al conectar).
Sfx ImpactSfx(const GameEvent& e) {
    const bool skill = e.kind == HitKind::Energy;
    const bool kick = e.kind == HitKind::Kick;
    const int v = Variant(e.attack);
    if (SkinNamed(e.skin, "MAX")) {
        static const Sfx kGolpes[3] = {Sfx::RayderGolpe1, Sfx::RayderGolpe2, Sfx::RayderGolpe3};
        static const Sfx kPatadas[3] = {Sfx::RayderPatada1, Sfx::RayderPatada2, Sfx::RayderPatada3};
        return skill ? Sfx::RayderImpactoPoder : e.heavy ? Sfx::RayderImpactoFuerte : kick ? kPatadas[v] : kGolpes[v];
    }
    if (SkinNamed(e.skin, "RONIN")) {
        static const Sfx kGolpes[3] = {Sfx::RoninGolpe1, Sfx::RoninGolpe2, Sfx::RoninGolpe3};
        return skill ? Sfx::RoninEspadaImpacto
             : e.heavy ? (kick ? Sfx::RoninPatada2 : Sfx::RoninImpactoFuerte)
             : kick ? Sfx::RoninPatada1 : kGolpes[v];
    }
    return skill ? Sfx::EnergyImpact : e.heavy ? Sfx::HeavyHit : kick ? Sfx::Kick : Sfx::Punch;
}

// Carga/arranque de un especial (antes en Player::BeginAttack/BeginClipAttack).
Sfx SpecialSfx(const GameEvent& e) {
    const AttackId id = static_cast<AttackId>(e.attack);
    if (id == AttackId::Skill) return e.value == 5 ? Sfx::FuryCharge : Sfx::EnergyShot;   // 5 = transformar
    if (SkinNamed(e.skin, "MAX"))
        return id == AttackId::EnergyWave ? Sfx::RayderCargaPoder
             : id == AttackId::RageAttack ? Sfx::RayderFuria : Sfx::RayderDisparoPoder;
    if (SkinNamed(e.skin, "RONIN"))
        return id == AttackId::EnergyWave ? Sfx::RoninEspadaDesenvaine : Sfx::RoninEspadaTajo;
    return id == AttackId::EnergyWave ? Sfx::EnergyCharge : id == AttackId::RageAttack ? Sfx::Rage : Sfx::FuryCharge;
}

}  // namespace

// Mapa unico evento -> Sfx legado. Devuelve false si el evento no suena.
bool LegacySfxFor(const GameEvent& e, Sfx& out) {
    const bool max = SkinNamed(e.skin, "MAX");
    switch (e.type) {
        case GameEventType::HitLanded:
        case GameEventType::HitBlocked:
        case GameEventType::HitArmored:
            if (e.fromEnemy) return false;
            out = ImpactSfx(e);
            return true;
        case GameEventType::BossHit:
            // Golpe cuerpo a cuerpo: el impacto de siempre; onda (skin -1): HeavyHit.
            out = e.skin >= 0 ? ImpactSfx(e) : Sfx::HeavyHit;
            return true;
        case GameEventType::ProjectileHit:
            if (e.fromEnemy) return false;   // el dano al jugador suena por PlayerHurt
            out = Sfx::EnergyImpact;
            return true;
        case GameEventType::PlayerHurt:
            if (e.heavy) return false;       // el derribo suena por PlayerKnockdown
            out = Sfx::Hit;
            return true;
        case GameEventType::PlayerBlocked:    out = Sfx::Block; return true;
        case GameEventType::PlayerGuardBreak: out = Sfx::HeavyHit; return true;
        case GameEventType::PlayerKnockdown:  out = Sfx::HeavyHit; return true;
        case GameEventType::PlayerDefeat:     out = Sfx::GameOver; return true;
        case GameEventType::EnemyKilled:      out = Sfx::EnemyDeath; return true;
        case GameEventType::EnemyGuardBreak:  out = Sfx::HeavyHit; return true;
        case GameEventType::ProjectileSpawn:
            if (!e.fromEnemy && (max || SkinNamed(e.skin, "RONIN"))) return false;   // ya suenan al lanzar
            out = Sfx::EnergyShot;
            return true;
        case GameEventType::Swing:
            out = max ? Sfx::RayderSilbido : Sfx::Whoosh;
            return true;
        case GameEventType::SpecialStart:     out = SpecialSfx(e); return true;
        case GameEventType::RageStart:
        case GameEventType::TransformStart:   out = max ? Sfx::RayderFuria : Sfx::Rage; return true;
        case GameEventType::Dash:             out = max ? Sfx::RayderEsquiva : Sfx::Dash; return true;
        case GameEventType::Jump:             out = Sfx::Jump; return true;
        case GameEventType::Land:
        case GameEventType::KnockdownLand:    out = Sfx::Land; return true;
        case GameEventType::Pickup:           out = Sfx::Ui; return true;
        case GameEventType::PropHit:          out = Sfx::Hit; return true;
        case GameEventType::UiNavigate:
        case GameEventType::UiOpen:           out = Sfx::Ui; return true;
        case GameEventType::UiConfirm:        out = Sfx::UiConfirm; return true;
        case GameEventType::UiCancel:         out = Sfx::UiCancel; return true;
        case GameEventType::UiPause:          out = Sfx::UiPause; return true;
        case GameEventType::WaveStart:
        case GameEventType::ArenaUnlock:      out = Sfx::Ui; return true;
        default:                              return false;
    }
}

bool IsMusicEvent(GameEventType t) {
    return t == GameEventType::ModeEnter || t == GameEventType::ArenaLock || t == GameEventType::ArenaUnlock ||
           t == GameEventType::BossPhase || t == GameEventType::Victory || t == GameEventType::GameOver ||
           t == GameEventType::StageClear;
}

void AudioDirectorUpdate(float dt) {
    (void)dt;
    DrainGameEvents([](const GameEvent& e) {
        Sfx sfx;
        if (LegacySfxFor(e, sfx)) AudioSystem::Get().Play(sfx);
        if (IsMusicEvent(e.type)) MusicPlayer::Get().OnEvent(e);
    });
}

}  // namespace district_fury
