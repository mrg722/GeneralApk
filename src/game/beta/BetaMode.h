#pragma once
#include "game/Player.h"
#include "game/RivalAI.h"
#include "game/beta/BetaStage.h"
#include "rendering/Animator.h"
#include "raylib.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace district_fury {

// MODO BETA: experiencia jugable aparte con el contenido de nuevosSprites
// (personajes, efectos, escenarios, sonidos y musica originales). Reutiliza
// Player (movimientos, habilidades por paginas, cajas de golpe por cuadro) y
// RivalAI (enemigos y rival manejados por la maquina); no toca los otros modos.
class BetaMode {
public:
    // Pantalla actual para los controles tactiles: 0 combate, 1 menu, 2 fin.
    int TouchContext() const;
    void Init();
    void Update(float dt);
    void Draw() const;
    bool ShouldExit() const { return exitRequested; }
    void ClearExit() { exitRequested = false; }
    void Shutdown();   // corta la musica al salir
    Player& PlayerRef() { return fighters.empty() ? placeholder : fighters[0]->p; }

    // Automatizacion (tests).
    void StartForTest(int character, int stage, bool duel, int rival = 0) {
        selCharacter = character; selStage = stage; selDuel = duel; selRival = rival; StartFight();
    }
    int AliveEnemies() const;
    int WaveIndex() const { return wave; }
    bool Won() const { return result == 1; }
    bool Lost() const { return result == -1; }
    const BetaStage& Stage() const { return stage; }
    float CameraX() const { return cameraX; }
    std::vector<const Player*> EnemiesForTest() const {
        std::vector<const Player*> v;
        for (const auto& f : fighters) if (f->enemy && f->p.state != PlayerState::Defeat) v.push_back(&f->p);
        return v;
    }

private:
    struct Fighter {
        Player p;
        RivalAI ai;
        PlayerInput in;
        int character = 0;           // indice en CharacterVisual
        bool enemy = false;
        int lastHp = 0;
        std::string lastClip;
        std::size_t lastFrame = static_cast<std::size_t>(-1);
        bool deathFx = false;
        float gone = 0.0f;           // tiempo tras la derrota (se desvanece)
        std::map<std::string, std::string> sounds;
    };
    struct Fx {
        Animator anim;
        Vector2 pos{};
        bool flip = false;
        float scale = 1.25f;
    };
    enum class Flow { Select, Fight };

    Flow flow{Flow::Select};
    int cursor{0};
    int selCharacter{0};   // 0..14 dentro del bloque BETA
    int selStage{0};
    bool selDuel{false};
    int selRival{13};      // MEDUSA por defecto
    bool exitRequested{false};

    BetaStage stage;
    std::vector<std::unique_ptr<Fighter>> fighters;   // [0] = jugador
    std::vector<Fx> effects;
    int wave{0};
    int waveCount{0};
    bool waveActive{false};
    float lockLeft{0.0f}, lockRight{0.0f};
    float cameraX{0.0f};
    float time{0.0f};
    float banner{0.0f};
    std::string bannerText;
    int result{0};         // 1 victoria, -1 derrota
    float resultTimer{0.0f};
    float shake{0.0f};
    float hitstop{0.0f};
    Player placeholder;

    std::map<std::string, Sound> soundCache;
    Music music{};
    bool musicReady{false};
    std::string musicName;

    void StartFight();
    void SpawnWave();
    Fighter& AddFighter(int character, bool enemy, float x, float y);
    void UpdateFight(float dt);
    void UpdateFighter(Fighter& f, float dt);
    void ResolveHits(Fighter& attacker, Fighter& target, float damageScale);
    void UpdateCamera(float dt);
    void PollSounds(Fighter& f);
    void SpawnFx(const std::string& id, const std::string& clip, Vector2 pos, bool flip);
    void PlaySound(const std::string& name);
    void PlayMusic(const std::string& file);
    void StopMusic();

    void DrawSelection() const;
    void DrawFight() const;
    void DrawHud() const;
};

}  // namespace district_fury
