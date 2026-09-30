#include "game/Campaign.h"
#include <fstream>

namespace district_fury {

Campaign& Campaign::Get() {
    static Campaign instance;
    return instance;
}

std::array<RewardOption, 3> Campaign::OptionsFor(int stageCleared) const {
    switch (stageCleared) {
        case 1:  // Brakk
            return {{{"SUERO DE RESISTENCIA", "+25 vida maxima y +15 escudo"},
                     {"SUERO DE FUERZA",      "+12% dano cuerpo a cuerpo"},
                     {"SUERO DE MOVILIDAD",   "+8% velocidad y dash mas rapido"}}};
        case 2:  // Grinder
            return {{{"SUERO DE FUERZA",      "+18% dano y mas empuje"},
                     {"SUERO DE RESISTENCIA", "+20 vida maxima"},
                     {"SUERO DE COMBO",       "ventana de combo mas larga"}}};
        case 3:  // Titan-X
            return {{{"SUERO DE ENERGIA",     "+25 SP maximo y +15% dano de energia"},
                     {"SUERO DE FURIA",       "barra de furia mas corta"},
                     {"SUERO DE MOVILIDAD",   "+8% velocidad y dash mas rapido"}}};
        default:  // Titan-X Mejorado
            return {{{"SUERO X COMPLETO",     "+30 vida, +20 SP y +10% dano"},
                     {"SUERO DE ENERGIA",     "+35 SP maximo y +20% dano de energia"},
                     {"SUERO DE FURIA",       "furia mas rapida y +15% dano"}}};
    }
}

void Campaign::ApplyReward(int stageCleared, int option) {
    const auto opts = OptionsFor(stageCleared);
    const std::string name = opts[static_cast<std::size_t>(option < 0 ? 0 : option > 2 ? 2 : option)].name;
    if (name == "SUERO DE RESISTENCIA") {
        upgrades.bonusMaxHp += stageCleared == 1 ? 25 : 20;
        upgrades.bonusMaxShield += 15;
    } else if (name == "SUERO DE FUERZA") {
        upgrades.attackDamageScale += stageCleared == 2 ? 0.18f : 0.12f;
    } else if (name == "SUERO DE MOVILIDAD") {
        upgrades.moveSpeedScale += 0.08f;
        upgrades.dashCooldownScale *= 0.85f;
    } else if (name == "SUERO DE COMBO") {
        upgrades.comboWindowBonus += 0.12f;
    } else if (name == "SUERO DE ENERGIA") {
        upgrades.bonusMaxSp += stageCleared == 3 ? 25 : 35;
        upgrades.energyDamageScale += stageCleared == 3 ? 0.15f : 0.20f;
    } else if (name == "SUERO DE FURIA") {
        upgrades.rageCapacityScale *= 0.85f;
        if (stageCleared >= 4) upgrades.attackDamageScale += 0.15f;
    } else {  // SUERO X COMPLETO
        upgrades.bonusMaxHp += 30;
        upgrades.bonusMaxSp += 20;
        upgrades.attackDamageScale += 0.10f;
    }
    stagesCompleted = stageCleared;
    Save();
}

void Campaign::ResetRun() {
    upgrades = PlayerUpgrades{};
    stagesCompleted = 0;
    totalScore = 0;
}

void Campaign::Load() {
    std::ifstream in(path);
    if (!in) return;
    in >> stagesCompleted >> totalScore >> upgrades.bonusMaxHp >> upgrades.bonusMaxSp
       >> upgrades.bonusMaxShield >> upgrades.rageCapacityScale >> upgrades.attackDamageScale
       >> upgrades.energyDamageScale >> upgrades.moveSpeedScale >> upgrades.dashCooldownScale
       >> upgrades.comboWindowBonus;
}

void Campaign::Save() const {
    std::ofstream out(path, std::ios::trunc);
    if (!out) return;
    out << stagesCompleted << ' ' << totalScore << ' ' << upgrades.bonusMaxHp << ' '
        << upgrades.bonusMaxSp << ' ' << upgrades.bonusMaxShield << ' '
        << upgrades.rageCapacityScale << ' ' << upgrades.attackDamageScale << ' '
        << upgrades.energyDamageScale << ' ' << upgrades.moveSpeedScale << ' '
        << upgrades.dashCooldownScale << ' ' << upgrades.comboWindowBonus << '\n';
}

}  // namespace district_fury
