// Director de audio (ver AudioDirector.h).
//
// F1.10: la tabla de CueFor decide el sonido de cada evento segun HitKind, heavy,
// weight, fromEnemy y el ARQUETIPO del personaje (skin). Los nombres son los del
// contrato con F1.11 (assets/audio/sfx/combat/<nombre>_N.wav, arquetipos/...,
// rayder_v2/..., ronin_v2/...); si un archivo no existe, AudioSystem cae al WAV
// legado de LegacySfxFor (el mapa 1:1 de F0.2), asi que nunca queda en silencio.
// Regla: UN dueno por evento (los pares que se emiten juntos tienen uno mudo).
#include "audio/AudioDirector.h"
#include "audio/AudioSystem.h"
#include "audio/MusicPlayer.h"
#include "game/CharacterVisual.h"
#include "game/combat/AttackData.h"
#include "game/events/GameEvents.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace district_fury {
namespace {

AudioDirectorStats gStats;
GameMode gMode = GameMode::Boot;

const char* SkinName(int skin) {
    if (skin < 0 || skin >= CharacterCount()) return nullptr;
    return GetCharacterVisual(skin).name;
}

bool SkinNamed(int skin, const char* name) {
    const char* n = SkinName(skin);
    return n && std::strcmp(n, name) == 0;
}

bool Has(const char* s, const char* part) { return s && std::strstr(s, part) != nullptr; }
bool Starts(const char* s, const char* prefix) { return s && std::strncmp(s, prefix, std::strlen(prefix)) == 0; }

// Variante 0/1/2 del sonido de impacto segun el golpe de la cadena (antes
// comboStep % 3: Punch1 0, Punch2 1, Punch3 2, el resto 0).
int Variant(int attack) {
    switch (static_cast<AttackId>(attack)) {
        case AttackId::Punch2: return 1;
        case AttackId::Punch3: return 2;
        default:               return 0;
    }
}

// ---- Mapa legado (F0.2) -----------------------------------------------------
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

// Nombre del archivo del pack propio (rayder/ ronin/) para un Sfx legado.
const char* PackName(Sfx s) {
    switch (s) {
        case Sfx::RayderGolpe1: case Sfx::RoninGolpe1: return "golpe_1";
        case Sfx::RayderGolpe2: case Sfx::RoninGolpe2: return "golpe_2";
        case Sfx::RayderGolpe3: case Sfx::RoninGolpe3: return "golpe_3";
        case Sfx::RayderPatada1: case Sfx::RoninPatada1: return "patada_1";
        case Sfx::RayderPatada2: case Sfx::RoninPatada2: return "patada_2";
        case Sfx::RayderPatada3: return "patada_3";
        case Sfx::RayderImpactoFuerte: case Sfx::RoninImpactoFuerte: return "impacto_fuerte";
        case Sfx::RayderSilbido: return "silbido";
        case Sfx::RayderCargaPoder: return "carga_poder";
        case Sfx::RayderDisparoPoder: return "disparo_poder";
        case Sfx::RayderImpactoPoder: return "impacto_poder";
        case Sfx::RayderFuria: return "furia";
        case Sfx::RayderEsquiva: return "esquiva";
        case Sfx::RoninEspadaDesenvaine: return "espada_desenvaine";
        case Sfx::RoninEspadaTajo: return "espada_tajo";
        case Sfx::RoninEspadaImpacto: return "espada_impacto";
        case Sfx::RoninSalto: return "salto";
        case Sfx::RoninAterrizaje: return "aterrizaje";
        default: return nullptr;
    }
}

// ---- Constructores de cues --------------------------------------------------
SoundCue Base(const GameEvent& e, const char* name, Sfx fallback, SoundCat cat, int prio) {
    SoundCue c;
    c.name = name;
    c.fallback = fallback;
    c.cat = cat;
    c.priority = prio;
    c.arch = ArchetypeForSkin(e.skin);
    if (c.arch == Archetype::Max || c.arch == Archetype::Ronin) c.arch = Archetype::Brawler;   // sin carpeta
    c.bus = cat == SoundCat::Ui ? Bus::Ui : cat == SoundCat::Voice ? Bus::Voice : Bus::Sfx;
    const bool hit = cat == SoundCat::Hit;
    c.pitchJitter = hit ? 0.06f : (cat == SoundCat::Move || cat == SoundCat::Step) ? 0.05f : 0.0f;
    c.volJitterDb = (cat == SoundCat::Ui || cat == SoundCat::Jingle) ? 0.0f : 1.5f;
    const bool positional = !(e.x == 0.0f && e.y == 0.0f);
    c.hasX = positional && cat != SoundCat::Ui && cat != SoundCat::Jingle;
    c.x = e.x;
    return c;
}

// Pack propio de MAX (rayder_v2) o RONIN (ronin_v2) sobre un Sfx legado.
void UsePack(SoundCue& c, const GameEvent& e, Sfx legacy, float pitch = 1.0f) {
    const char* n = PackName(legacy);
    if (!n) return;
    c.pack = SkinNamed(e.skin, "MAX") ? "rayder_v2" : "ronin_v2";
    c.packName = n;
    c.packPitch = pitch;
    c.fallback = legacy;
    c.name = nullptr;   // el pack manda: si falta v2, suena rayder/ o ronin/ (legado)
}

void Duck(SoundCue& c, float db, float hold) { c.duckDb = db; c.duckHold = hold; }

// Impacto del golpe del jugador.
SoundCue ImpactCue(const GameEvent& e) {
    const bool heavy = e.heavy || e.kind == HitKind::Heavy;
    const char* name = e.kind == HitKind::Energy ? "energy_impact"
                     : e.kind == HitKind::Prop   ? "prop_wood"
                     : e.kind == HitKind::Slash  ? "slash"
                     : e.kind == HitKind::Kick   ? "kick"
                     : heavy                     ? "hit_heavy"
                     : e.kind == HitKind::Medium ? "hit_medium" : "hit_light";
    SoundCue c = Base(e, name, ImpactSfx(e), SoundCat::Hit, heavy || e.kind == HitKind::Energy ? kPrioHeavy : kPrioLight);
    if (heavy) { c.pitchJitter = 0.03f; Duck(c, 6.0f, 0.25f); }
    const Archetype a = ArchetypeForSkin(e.skin);
    const bool body = e.kind != HitKind::Kick && e.kind != HitKind::Energy && e.kind != HitKind::Prop;
    switch (a) {
        case Archetype::Agil:      c.pitch = e.kind == HitKind::Kick ? 1.12f : 1.06f; break;
        case Archetype::Cuchillas: if (body) c.alt = "slash"; break;
        case Archetype::Pesado:    c.pitch = 0.85f; break;
        case Archetype::Electrico: if (body) c.alt = heavy ? "energy_impact" : "energy"; break;
        case Archetype::Max:
        case Archetype::Ronin:     UsePack(c, e, ImpactSfx(e)); break;
        case Archetype::Brawler:   break;
    }
    return c;
}

SoundCue SwingCue(const GameEvent& e) {
    const bool heavy = e.heavy || e.weight >= 3;
    SoundCue c = Base(e, heavy ? "whoosh_heavy" : "whoosh_light", Sfx::Whoosh, SoundCat::Move, kPrioMove);
    switch (ArchetypeForSkin(e.skin)) {
        case Archetype::Agil:      c.pitch = 1.10f; break;
        case Archetype::Cuchillas: c.name = "whoosh_light"; c.pitch = 1.30f; break;   // "swish"
        case Archetype::Pesado:    c.name = "whoosh_heavy"; c.pitch = 0.90f; break;
        case Archetype::Electrico: c.alt = "energy_shot"; c.pitch = 1.40f; c.gainDb = -8.0f; break;   // "zap"
        case Archetype::Max:       UsePack(c, e, Sfx::RayderSilbido); break;
        default: break;
    }
    return c;
}

SoundCue SpecialCue(const GameEvent& e) {
    const AttackId id = static_cast<AttackId>(e.attack);
    const Archetype a = ArchetypeForSkin(e.skin);
    SoundCue c;
    if (id == AttackId::Skill) {
        if (e.value == 5) {   // transformar
            c = Base(e, "transform", SpecialSfx(e), SoundCat::Voice, kPrioTop);
            Duck(c, 9.0f, 0.4f);
        } else {
            c = Base(e, "energy_shot", SpecialSfx(e), SoundCat::Hit, kPrioHeavy);
        }
        return c;
    }
    if (a == Archetype::Max || a == Archetype::Ronin) {
        c = Base(e, nullptr, SpecialSfx(e), SoundCat::Voice, id == AttackId::RageAttack ? kPrioTop : kPrioHeavy);
        UsePack(c, e, SpecialSfx(e));
        if (id == AttackId::RageAttack) Duck(c, 9.0f, 0.4f);
        return c;
    }
    if (id == AttackId::RageAttack) {
        c = Base(e, "rage_start", Sfx::Rage, SoundCat::Voice, kPrioTop);
        Duck(c, 9.0f, 0.4f);
    } else {
        c = Base(e, "energy_charge", SpecialSfx(e), SoundCat::Move, kPrioHeavy);
        if (a == Archetype::Cuchillas && id != AttackId::EnergyWave) c.alt = "slash";   // corte + energia
    }
    if (a == Archetype::Agil) c.pitch = 1.15f;          // energia brillante
    if (a == Archetype::Pesado) c.pitch = 0.85f;
    return c;
}

void NoteListener(const GameEvent& e) {
    if (e.skin < 0 || e.fromEnemy) return;
    switch (e.type) {
        case GameEventType::Swing: case GameEventType::SpecialStart: case GameEventType::Dash:
        case GameEventType::Jump: case GameEventType::Land: case GameEventType::KnockdownLand:
        case GameEventType::PlayerHurt: case GameEventType::PlayerBlocked: case GameEventType::PlayerGuardBreak:
        case GameEventType::RageStart: case GameEventType::TransformStart: case GameEventType::Footstep:
        case GameEventType::ComboMilestone:
            AudioSystem::Get().NotePlayerX(e.x);
            break;
        default:
            break;
    }
}

}  // namespace

// ---- Arquetipos -------------------------------------------------------------
Archetype ArchetypeForSkin(int skin) {
    const char* n = SkinName(skin);
    if (!n) return Archetype::Brawler;
    if (std::strcmp(n, "MAX") == 0) return Archetype::Max;
    if (std::strcmp(n, "RONIN") == 0) return Archetype::Ronin;
    if (Has(n, "(CADENA DEL RAYO)")) return Archetype::Electrico;
    if (Has(n, "(ESPADAS DEL CAOS)") || Has(n, "(GARRAS DE HADES)") || Starts(n, "KF NAVAJERA") ||
        Starts(n, "KF CUCHILLERO") || Starts(n, "KF JEFE GARRA"))
        return Archetype::Cuchillas;
    if (Has(n, "(CESTUS DE NEMEA)") || Starts(n, "GRINDER") || Starts(n, "TITAN-X") || Starts(n, "KF SOLDADO") ||
        Starts(n, "SOLDADO ESQUELETO") || Starts(n, "BESTIA ") || Starts(n, "MOMIA") || Starts(n, "AVE ") ||
        Starts(n, "CENTAURO") || Starts(n, "BRUTO DE LA BOLA") || Starts(n, "MEDUSA"))
        return Archetype::Pesado;
    if (Starts(n, "KF RUBIA") || Starts(n, "KF BUFONA") || Starts(n, "KF HEROINA") || Starts(n, "KF HEROE") ||
        Starts(n, "RAYDER CLON BETA"))
        return Archetype::Agil;
    return Archetype::Brawler;   // RAYDEN, CLON ANTIGUO, BRAKK, KF MATON/PELEADOR/GORRA, RAYDER CLON...
}

const char* ArchetypeFolder(Archetype a) {
    switch (a) {
        case Archetype::Agil:      return "agil";
        case Archetype::Cuchillas: return "cuchillas";
        case Archetype::Pesado:    return "pesado";
        case Archetype::Electrico: return "electrico";
        default:                   return nullptr;
    }
}

const char* ArchetypeName(Archetype a) {
    switch (a) {
        case Archetype::Brawler:   return "brawler";
        case Archetype::Agil:      return "agil";
        case Archetype::Cuchillas: return "cuchillas";
        case Archetype::Pesado:    return "pesado";
        case Archetype::Electrico: return "electrico";
        case Archetype::Max:       return "max";
        case Archetype::Ronin:     return "ronin";
    }
    return "?";
}

bool RainForMode(GameMode m) {
    return m == GameMode::Boot || m == GameMode::Menu || m == GameMode::CharacterSelect ||
           m == GameMode::Options || m == GameMode::Stage1;
}

// ---- Mudos intencionales ----------------------------------------------------
const char* MuteReason(const GameEvent& e) {
    switch (e.type) {
        case GameEventType::HitLanded:
        case GameEventType::HitBlocked:
        case GameEventType::HitArmored:
            return e.fromEnemy ? "golpe enemigo: suena PlayerHurt/PlayerBlocked" : nullptr;
        case GameEventType::CounterHit:
            return e.fromEnemy ? "contra al jugador: suena PlayerHurt" : "se emite junto a HitLanded, que suena";
        case GameEventType::PlayerKnockdown: return "suena PlayerHurt pesado del mismo golpe; la caida, KnockdownLand";
        case GameEventType::PlayerDefeat:    return "suena game_over con GameOver";
        case GameEventType::BossAttack:      return "el dano al jugador suena por PlayerHurt";
        case GameEventType::BossHit:
            return e.skin < 0 ? "onda al jefe: suena ProjectileHit del mismo frame" : nullptr;
        case GameEventType::ProjectileSpawn:
            if (e.fromEnemy) return nullptr;
            if (e.skin < 0) return "duplicado del ProjectileSpawn del jugador";
            if (SkinNamed(e.skin, "MAX") || SkinNamed(e.skin, "RONIN")) return "MAX/RONIN suenan al lanzar (SpecialStart)";
            return nullptr;
        case GameEventType::ProjectileHit:   return e.fromEnemy ? "dano al jugador: suena PlayerHurt" : nullptr;
        case GameEventType::ArenaLock:       return "solo musica (MusicPlayer)";
        case GameEventType::ArenaUnlock:     return e.value < 0 ? "stages 2-5: solo musica" : nullptr;
        case GameEventType::ModeEnter:       return "musica y lluvia (MusicPlayer / ambiente)";
        case GameEventType::Count:           return "sin tipo";
        default:                             return nullptr;
    }
}

// ---- Tabla evento -> sonido -------------------------------------------------
bool CueFor(const GameEvent& e, SoundCue& out) {
    if (MuteReason(e)) return false;
    const Archetype a = ArchetypeForSkin(e.skin);
    SoundCue c;
    switch (e.type) {
        case GameEventType::HitLanded:
        case GameEventType::HitArmored:
            c = ImpactCue(e);
            if (e.type == GameEventType::HitArmored) c.gainDb -= 2.0f;
            break;
        case GameEventType::HitBlocked:
            c = Base(e, "block", Sfx::Block, SoundCat::Hit, kPrioLight);
            c.pitchJitter = 0.04f;
            break;
        case GameEventType::BossHit:
            if (e.kind == HitKind::Block) {
                c = Base(e, "block", Sfx::Block, SoundCat::Hit, kPrioLight);
            } else if (a == Archetype::Max || a == Archetype::Ronin) {
                c = ImpactCue(e);   // conservan su identidad
                c.priority = kPrioTop;
            } else {
                c = Base(e, "boss_hit", ImpactSfx(e), SoundCat::Hit, kPrioTop);
                c.pitchJitter = 0.03f;
                Duck(c, 6.0f, 0.25f);
            }
            break;
        case GameEventType::BossPhase:
            c = Base(e, "boss_phase", Sfx::BossPhase, SoundCat::Voice, kPrioTop);
            Duck(c, 9.0f, 0.4f);
            break;
        case GameEventType::PlayerHurt:
            c = Base(e, "player_hurt", e.heavy ? Sfx::HeavyHit : Sfx::Hit, SoundCat::Hit, kPrioHeavy);
            c.pitchJitter = 0.04f;
            if (e.heavy) { c.pitch = 0.9f; c.gainDb = 2.0f; }
            Duck(c, 7.0f, 0.3f);
            break;
        case GameEventType::PlayerBlocked:
            c = Base(e, "block", Sfx::Block, SoundCat::Hit, kPrioLight);
            if (e.heavy) { c.pitch = 0.9f; c.gainDb = 2.0f; }
            break;
        case GameEventType::PlayerGuardBreak:
        case GameEventType::EnemyGuardBreak:
            c = Base(e, "guard_break", Sfx::HeavyHit, SoundCat::Hit, kPrioHeavy);
            c.pitchJitter = 0.03f;
            Duck(c, 6.0f, 0.3f);
            break;
        case GameEventType::EnemyKilled:
            c = Base(e, "ko_enemy", Sfx::EnemyDeath, SoundCat::Hit, kPrioTop);
            c.pitchJitter = 0.04f;
            Duck(c, 8.0f, 0.3f);
            break;
        case GameEventType::ProjectileSpawn:
            c = Base(e, "energy_shot", Sfx::EnergyShot, SoundCat::Hit, kPrioLight);
            c.pitch = e.fromEnemy ? 0.85f : a == Archetype::Agil ? 1.12f : a == Archetype::Pesado ? 0.85f : 1.0f;
            break;
        case GameEventType::ProjectileHit:
            c = Base(e, "energy_impact", Sfx::EnergyImpact, SoundCat::Hit, kPrioHeavy);
            c.pitchJitter = 0.03f;
            break;
        case GameEventType::Swing:
            c = SwingCue(e);
            break;
        case GameEventType::SpecialStart:
            c = SpecialCue(e);
            break;
        case GameEventType::RageStart:
        case GameEventType::TransformStart: {
            const bool rage = e.type == GameEventType::RageStart;
            c = Base(e, rage ? "rage_start" : "transform", Sfx::Rage, SoundCat::Voice, kPrioTop);
            if (a == Archetype::Max) UsePack(c, e, Sfx::RayderFuria);
            Duck(c, 9.0f, 0.4f);
            break;
        }
        case GameEventType::Dash:
            c = Base(e, "whoosh_light", Sfx::Dash, SoundCat::Move, kPrioMove);
            c.pitch = a == Archetype::Agil ? 1.25f : a == Archetype::Pesado ? 0.95f : 1.15f;
            if (a == Archetype::Max) UsePack(c, e, Sfx::RayderEsquiva);
            if (a == Archetype::Ronin) UsePack(c, e, Sfx::RoninEspadaDesenvaine, 1.3f);
            break;
        case GameEventType::Jump:
            c = Base(e, "jump", Sfx::Jump, SoundCat::Move, kPrioMove);
            c.pitch = a == Archetype::Agil ? 1.1f : a == Archetype::Pesado ? 0.85f : 1.0f;
            if (a == Archetype::Ronin) UsePack(c, e, Sfx::RoninSalto);
            break;
        case GameEventType::Land:
            c = Base(e, "land_light", Sfx::Land, SoundCat::Move, kPrioMove);
            if (a == Archetype::Pesado) c.pitch = 0.85f;
            if (a == Archetype::Ronin) UsePack(c, e, Sfx::RoninAterrizaje);
            break;
        case GameEventType::KnockdownLand:
            c = Base(e, "land_knockdown", Sfx::Land, SoundCat::Hit, kPrioLight);
            c.pitchJitter = 0.04f;
            break;
        case GameEventType::Footstep:
            c = Base(e, gMode == GameMode::Stage1 ? "step_wet" : "step", Sfx::Land,
                     SoundCat::Step, kPrioMove);
            c.pitchJitter = 0.08f;
            if (e.weight >= 3) c.pitch = 0.8f;
            c.gainDb = -2.0f;
            break;
        case GameEventType::Pickup:
            // value = PickupType: 0 vida; energia/furia/monedas/mejoras -> brillo.
            c = Base(e, e.value == 0 ? "pickup_health" : "pickup_energy", Sfx::Ui, SoundCat::Ui, kPrioLight);
            c.bus = Bus::Sfx;
            break;
        case GameEventType::PropHit:
            c = Base(e, e.value == 1 ? "prop_metal" : "prop_wood", Sfx::Hit, SoundCat::Hit, kPrioLight);
            break;
        case GameEventType::ComboMilestone:
            c = Base(e, e.value >= 30 ? "combo_30" : e.value >= 20 ? "combo_20" : "combo_10", Sfx::UiConfirm,
                     SoundCat::Ui, kPrioLight);
            c.bus = Bus::Sfx;
            c.hasX = false;
            c.pitch = e.value >= 30 ? 1.12f : e.value >= 20 ? 1.06f : 1.0f;   // solo al caer al legado
            break;
        case GameEventType::Telegraph:
            c = Base(e, "telegraph", Sfx::Ui, SoundCat::Move, kPrioMove);
            c.gainDb = -12.0f;
            c.pitchJitter = 0.0f;
            break;
        case GameEventType::UiNavigate: c = Base(e, nullptr, Sfx::Ui, SoundCat::Ui, kPrioUi); break;
        case GameEventType::UiOpen:     c = Base(e, nullptr, Sfx::UiOpen, SoundCat::Ui, kPrioUi); break;
        case GameEventType::UiPause:    c = Base(e, nullptr, Sfx::UiPause, SoundCat::Ui, kPrioUi); break;
        case GameEventType::UiConfirm:
            // skin >= 0: se eligio un personaje (seleccion de VS / historia).
            c = Base(e, e.skin >= 0 ? "ui_select_char" : nullptr, Sfx::UiConfirm, SoundCat::Ui, kPrioUi);
            c.arch = Archetype::Brawler;
            break;
        case GameEventType::UiCancel:
            // heavy = accion rechazada (ui_deny); si no, cancelar normal.
            c = Base(e, e.heavy ? "ui_deny" : nullptr, Sfx::UiCancel, SoundCat::Ui, kPrioUi);
            break;
        case GameEventType::WaveStart:
        case GameEventType::ArenaUnlock:
            c = Base(e, nullptr, Sfx::Ui, SoundCat::Ui, kPrioUi);
            break;
        case GameEventType::StageClear:
        case GameEventType::Victory:
            c = Base(e, "stage_clear", Sfx::StageClear, SoundCat::Jingle, kPrioTop);
            break;
        case GameEventType::GameOver:
            c = Base(e, "game_over", Sfx::GameOver, SoundCat::Jingle, kPrioTop);
            break;
        default:
            return false;
    }
    out = c;
    return true;
}

// Mapa unico evento -> Sfx legado de F0.2 (respaldo; false = no sonaba).
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
            out = e.skin >= 0 ? ImpactSfx(e) : Sfx::HeavyHit;
            return true;
        case GameEventType::ProjectileHit:
            if (e.fromEnemy) return false;
            out = Sfx::EnergyImpact;
            return true;
        case GameEventType::PlayerHurt:
            if (e.heavy) return false;
            out = Sfx::Hit;
            return true;
        case GameEventType::PlayerBlocked:    out = Sfx::Block; return true;
        case GameEventType::PlayerGuardBreak: out = Sfx::HeavyHit; return true;
        case GameEventType::PlayerKnockdown:  out = Sfx::HeavyHit; return true;
        case GameEventType::PlayerDefeat:     out = Sfx::GameOver; return true;
        case GameEventType::EnemyKilled:      out = Sfx::EnemyDeath; return true;
        case GameEventType::EnemyGuardBreak:  out = Sfx::HeavyHit; return true;
        case GameEventType::ProjectileSpawn:
            if (!e.fromEnemy && (max || SkinNamed(e.skin, "RONIN"))) return false;
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
        case GameEventType::WaveStart:        out = Sfx::Ui; return true;
        case GameEventType::ArenaUnlock:
            if (e.value < 0) return false;
            out = Sfx::Ui;
            return true;
        default:                              return false;
    }
}

bool IsMusicEvent(GameEventType t) {
    return t == GameEventType::ModeEnter || t == GameEventType::ArenaLock || t == GameEventType::ArenaUnlock ||
           t == GameEventType::BossPhase || t == GameEventType::Victory || t == GameEventType::GameOver ||
           t == GameEventType::StageClear;
}

void AudioDirectorUpdate(float dt) {
    (void)dt;   // MusicPlayer::Update y el ducking los avanza AudioSystem::UpdateAmbient
    static const bool log = std::getenv("DF_AUDIO_LOG") != nullptr;
    DrainGameEvents([](const GameEvent& e) {
        const int t = static_cast<int>(e.type);
        const bool valid = t >= 0 && t < static_cast<int>(GameEventType::Count);
        if (valid) ++gStats.events[t];
        if (e.type == GameEventType::ModeEnter) {
            gMode = static_cast<GameMode>(e.value);
            AudioSystem::Get().SetRainWanted(RainForMode(gMode));
        }
        NoteListener(e);
        SoundCue cue;
        if (CueFor(e, cue)) {
            if (valid) ++gStats.cued[t];
            const CueSource src = AudioSystem::Get().PlayCue(cue);
            if (valid && src != CueSource::None) ++gStats.played[t];
            if (log)
                std::fprintf(stderr, "AUDIO %-16s skin=%d -> %s%s%s (%s)\n", GameEventName(e.type), e.skin,
                             cue.pack ? cue.pack : "combat", cue.pack ? "/" : "/",
                             cue.pack ? cue.packName : (cue.alt ? cue.alt : (cue.name ? cue.name : "(legado)")),
                             src == CueSource::Pack ? "pack" : src == CueSource::Archetype ? "arquetipo"
                             : src == CueSource::Generic ? "generico" : src == CueSource::Legacy ? "legado" : "no sono");
        } else if (valid) {
            ++gStats.muted[t];
        }
        if (IsMusicEvent(e.type)) MusicPlayer::Get().OnEvent(e);
    });
}

const AudioDirectorStats& GetAudioDirectorStats() { return gStats; }
void ResetAudioDirectorStats() { gStats = AudioDirectorStats{}; }

const char* GameEventName(GameEventType t) {
    static const char* kNames[] = {
        "HitLanded", "HitBlocked", "HitArmored", "CounterHit",
        "PlayerHurt", "PlayerBlocked", "PlayerGuardBreak", "PlayerKnockdown", "PlayerDefeat",
        "EnemyKilled", "EnemyGuardBreak",
        "BossHit", "BossPhase", "BossAttack",
        "ProjectileSpawn", "ProjectileHit",
        "Swing", "SpecialStart", "RageStart", "TransformStart",
        "Dash", "Jump", "Land", "KnockdownLand", "Footstep",
        "Pickup", "PropHit", "ComboMilestone", "Telegraph",
        "UiNavigate", "UiConfirm", "UiCancel", "UiOpen", "UiPause",
        "ModeEnter", "ArenaLock", "ArenaUnlock", "WaveStart", "StageClear", "GameOver", "Victory",
    };
    static_assert(sizeof(kNames) / sizeof(kNames[0]) == static_cast<std::size_t>(GameEventType::Count),
                  "GameEventName: falta un tipo de evento");
    const int i = static_cast<int>(t);
    return i >= 0 && i < static_cast<int>(GameEventType::Count) ? kNames[i] : "?";
}

void AudioDirectorPrintStats() {
    std::fprintf(stderr, "AUDIO evento            eventos  con_sonido  sonaron  mudos\n");
    for (int i = 0; i < static_cast<int>(GameEventType::Count); ++i) {
        if (gStats.events[i] == 0) continue;
        std::fprintf(stderr, "AUDIO %-18s %7d %11d %8d %6d\n", GameEventName(static_cast<GameEventType>(i)),
                     gStats.events[i], gStats.cued[i], gStats.played[i], gStats.muted[i]);
    }
}

}  // namespace district_fury
