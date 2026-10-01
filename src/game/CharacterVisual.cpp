#include "game/CharacterVisual.h"
#include "game/Player.h"
#include <array>

namespace district_fury {
namespace {

// name, folder, uniformCanvas, scale, targetHeight, footInset,
// facesRightByDefault, maxHp, damageMultiplier, speedMultiplier
const std::array<CharacterVisual, 21> kCharacters{{
    {"RAYDEN (ORIGINAL)", nullptr,           false, 1.00f,   0.0f, 0.0f, true,  100, 1.00f, 1.00f},
    {"RAYDEN CLON",       "rayder_clone",    false, 1.00f, 118.0f, 0.0f, true,  110, 1.05f, 1.05f},
    // Brakk jugable con la hoja mejorada (atlas brakk_v2); las poses sueltas siguen en assets/bosses/brakk.
    {"BRAKK",             "brakk",           true,  175.0f/188.0f, 0.0f, 9.0f, true, 150, 1.25f, 0.85f, "brakk_v2"},
    {"GRINDER",           "grinder",         true,  3.00f,   0.0f, 0.0f, true,  160, 1.30f, 0.80f},
    {"TITAN-X",           "titanx",          false, 1.00f, 170.0f, 0.0f, false, 140, 1.20f, 0.90f},
    {"TITAN-X MEJORADO",  "titanx_mejorado", false, 1.00f, 235.0f, 0.0f, false, 180, 1.40f, 0.75f},
    // DF-014: Rayder, hoja de 50 poses del usuario (tools/build_rayder_sheet.py).
    // Hoja de movimientos estilo KF (tools/build_rayder_kf_sheet.py): ya trae la
    // complexion de Rayden Cruz. El atlas anterior ("rayder") sigue en el manifiesto.
    {"RAYDER",            nullptr,           false, 0.83f,   0.0f, 0.0f, true,  100, 1.00f, 1.00f, "rayder_kf", -1, 1.00f, "rayder_kf_white"},
    // Laboratorio (temporales, solo Modo VS): personajes extraidos de la APK.
    {"KF HEROE (PELO BLANCO)", nullptr, false, 1.50f, 0.0f, 0.0f, true, 110, 1.00f, 1.05f, nullptr, 0, 1.0f, nullptr, 11},
    {"KF HEROINA (PELIRROJA)", nullptr, false, 1.50f, 0.0f, 0.0f, true, 100, 0.95f, 1.10f, nullptr, 1, 1.0f, nullptr, 12},
    {"KF MATON",               nullptr, false, 1.50f, 0.0f, 0.0f, true,  90, 1.05f, 0.95f, nullptr, 2},
    {"KF NAVAJERA",            nullptr, false, 1.50f, 0.0f, 0.0f, true,  80, 0.95f, 1.10f, nullptr, 3},
    {"KF SOLDADO",             nullptr, false, 1.50f, 0.0f, 0.0f, true,  95, 1.00f, 1.00f, nullptr, 4},
    {"KF RUBIA",               nullptr, false, 1.50f, 0.0f, 0.0f, true,  80, 0.95f, 1.10f, nullptr, 5},
    {"KF GORRA",               nullptr, false, 1.50f, 0.0f, 0.0f, true,  85, 1.00f, 1.05f, nullptr, 6},
    {"KF PELEADOR",            nullptr, false, 1.50f, 0.0f, 0.0f, true,  95, 1.05f, 1.00f, nullptr, 7},
    {"KF CUCHILLERO",          nullptr, false, 1.50f, 0.0f, 0.0f, true,  85, 1.00f, 1.05f, nullptr, 8},
    {"KF JEFE GARRA",          nullptr, false, 1.08f, 0.0f, 0.0f, true, 160, 1.25f, 0.85f, nullptr, 9},
    {"KF BUFONA",              nullptr, false, 1.50f, 0.0f, 0.0f, true, 140, 1.15f, 1.00f, nullptr, 10},
    {"KF HEROE TRANSFORMADO",        nullptr, false, 1.50f, 0.0f, 0.0f, true, 120, 1.05f, 1.05f, nullptr, 11},
    {"KF HEROINA TRANSFORMADA",             nullptr, false, 1.50f, 0.0f, 0.0f, true, 100, 1.00f, 1.10f, nullptr, 12},
    // PRUEBA: Rayder clon BETA = copia del heroe KF (todas sus piezas, cuadros,
    // habilidades y transformacion) vestida como el Rayder clon (morado electrico).
    // Escala 0.75: sus cuadros vienen ampliados x2 (Scale2x) para mas calidad.
    {"RAYDER CLON BETA",       nullptr, false, 0.75f, 0.0f, 0.0f, true, 115, 1.05f, 1.05f, nullptr, 13, 1.0f, nullptr, 14},
}};

}  // namespace

int CharacterCount() { return static_cast<int>(kCharacters.size()); }

const CharacterVisual& GetCharacterVisual(int id) {
    if (id < 0 || id >= static_cast<int>(kCharacters.size())) return kCharacters[0];
    return kCharacters[static_cast<std::size_t>(id)];
}

const char* CharacterPose(int id, PlayerState state, AttackType attack, bool rage, double time) {
    if (state == PlayerState::Airborne) state = PlayerState::Knockdown;
    const int idleCycle = static_cast<int>(time * (state == PlayerState::Walk ? 9.0 : 3.2)) % 4;
    switch (id) {
        case 1:  // Rayden clon
            if (state == PlayerState::Defeat) return "death";
            if (state == PlayerState::Hit || state == PlayerState::Knockdown || state == PlayerState::GuardBreak) return "hurt";
            if (state == PlayerState::Block) return "ready";
            if (state == PlayerState::Dash) return "dash";
            if (state == PlayerState::Attack)
                return attack == AttackType::Energy ? "release_orb" : attack == AttackType::Kick ? "kick" : "punch";
            return idleCycle == 0 ? "idle1" : idleCycle == 1 ? "idle2" : idleCycle == 2 ? "idle3" : "idle4";
        case 2:  // Brakk
            if (state == PlayerState::Defeat) return "death";
            if (state == PlayerState::Hit || state == PlayerState::Knockdown || state == PlayerState::GuardBreak) return "hurt";
            if (state == PlayerState::Dash) return "charge";
            if (state == PlayerState::Attack) {
                if (attack == AttackType::Energy) return "chain_throw";
                if (rage) return "fury";
                return attack == AttackType::Kick ? "heavy" : "basic";
            }
            if (state == PlayerState::Walk) return "walk";
            return "idle";
        case 3:  // Grinder
            if (state == PlayerState::Defeat) return "death";
            if (state == PlayerState::Hit || state == PlayerState::Knockdown || state == PlayerState::GuardBreak) return "hurt";
            if (state == PlayerState::Dash) return "ram";
            if (state == PlayerState::Attack) {
                if (attack == AttackType::Energy || rage) return "overdrive";
                return attack == AttackType::Kick ? "slam" : "saw";
            }
            return "idle";
        case 4:  // Titan-X
            if (state == PlayerState::Defeat) return "death";
            if (state == PlayerState::Hit || state == PlayerState::Knockdown || state == PlayerState::GuardBreak) return "recoil";
            if (state == PlayerState::Dash) return "lean";
            if (state == PlayerState::Attack) {
                if (attack == AttackType::Energy) return "shoot_orb";
                if (attack == AttackType::Kick) return "uppercut";
                return rage ? "slam" : "punch1";
            }
            return idleCycle == 0 ? "idle1" : idleCycle == 1 ? "idle2" : idleCycle == 2 ? "idle3" : "idle4";
        case 5:  // Titan-X Mejorado
            if (state == PlayerState::Defeat) return "death";
            if (state == PlayerState::Hit || state == PlayerState::Knockdown || state == PlayerState::GuardBreak) return "cannon_aim";
            if (state == PlayerState::Dash) return "dash";
            if (state == PlayerState::Attack) {
                if (attack == AttackType::Energy) return "shoot_orb";
                if (rage) return "rage_aura";
                return attack == AttackType::Kick ? "slam" : "charge_orb";
            }
            return idleCycle == 0 ? "idle1" : idleCycle == 1 ? "idle2" : idleCycle == 2 ? "idle3" : "idle4";
        default:
            return "idle1";
    }
}

}  // namespace district_fury
