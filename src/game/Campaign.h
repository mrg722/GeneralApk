#pragma once
#include "game/Player.h"
#include <array>
#include <string>

// DF-013.2 (19-09) — progresion de campana entre stages.
//
// Reutiliza PlayerUpgrades, que YA existia en Player.h y se aplicaba en
// Player::Reset() pero que ningun modo alimentaba: la campana era una serie
// de stages sueltos. Ahora, al terminar cada stage, el jugador elige una
// mejora (el "suero" del documento de diseno) y esa mejora acompana al resto
// de la campana y se guarda en disco.
//
// El guardado va a district_fury_campaign.dat, un archivo NUEVO: no toca
// district_fury_save.dat, que sigue siendo del Stage 1.
namespace district_fury {

struct RewardOption {
    const char* name;
    const char* description;
};

class Campaign {
public:
    static Campaign& Get();

    PlayerUpgrades upgrades{};
    int stagesCompleted{0};
    int totalScore{0};

    // Recompensas canonicas del documento de diseno:
    //   Brakk -> vida/resistencia, Grinder -> dano/knockback, Titan-X -> SP.
    // La tercera opcion siempre es movilidad, para que la eleccion exista.
    std::array<RewardOption, 3> OptionsFor(int stageCleared) const;
    void ApplyReward(int stageCleared, int option);

    void ResetRun();          // nueva partida: limpia mejoras
    void Load();
    void Save() const;

private:
    Campaign() = default;
    std::string path{"district_fury_campaign.dat"};
};

}  // namespace district_fury
