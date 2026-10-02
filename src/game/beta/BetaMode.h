#pragma once
#include "game/Player.h"
#include "game/RivalAI.h"
#include "game/beta/BetaStage.h"
#include "game/beta/Spine21.h"
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
    void StartStoryForTest(int character) { selCharacter = character; selMode = 3; StartStory(); }
    int StoryChapter() const { return storyChapter; }
    bool InStoryCard() const { return flow == Flow::StoryCard; }
    void StartForTest(int character, int stage, bool duel, int rival = 0) {
        selCharacter = character; selStage = stage; selMode = duel ? 1 : 0; selRival = rival; StartFight();
    }
    void StartBossForTest(int character, int stage, int bossIndex) {
        selCharacter = character; selStage = stage; selMode = 2; selBoss = bossIndex; StartFight();
    }
    bool BossActive() const { return boss.active; }
    bool QteActive() const { return qte.active; }
    bool QteWaiting() const { return qte.waiting; }
    int BossHp() const { return boss.hp; }
    float BossX() const { return boss.pos.x; }
    const char* BossName() const { return boss.name.c_str(); }
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
        bool qteDone = false;        // la cinematica de remate solo se ofrece una vez
        float gone = 0.0f;           // tiempo tras la derrota (se desvanece)
        std::map<std::string, std::string> sounds;
    };
    struct Fx {
        Animator anim;
        Vector2 pos{};
        bool flip = false;
        float scale = 1.25f;
    };
    // Jefe Spine horneado (Titan, Poseidon, tentaculos): no es un Player; ataca
    // con aviso de area (anim 99) y golpea en el evento del Spine original.
    struct BossAttack {
        std::string clip, warn, fx, fxClip, sound;
        float warnScale = 1.0f, reach = 0.0f;
        std::vector<float> hitTimes;   // segundos de los eventos Spine attackEffect/skillEffect
        int damage = 20;
        Rectangle area{};      // area del aviso (pies = 0,0; hacia adelante = +x)
    };
    struct BossFight {
        bool active = false;
        std::string id, name;
        spine21::Skeleton skel;
        float scale = 1.0f;    // escala del esqueleto (Titan 0.44)
        std::map<std::string, std::pair<std::string, bool>> clips;   // clip del juego -> animacion Spine
        std::string clip;
        Animator warnAnim;
        bool warnOn = false;
        Vector3D pos{};
        Facing facing = Facing::Left;
        int hp = 0, maxHp = 1, lastHp = 0;
        int phase = 0;          // 0 entrada, 1 quieto, 2 ataque, 3 herido, 4 vencido
        int attack = -1;
        float cooldown = 1.5f, flash = 0.0f, deadTime = 0.0f;
        int strikes = 0;        // golpes ya resueltos del ataque actual
        bool qteDone = false;
        // Dibujo (capturas del original): escala en pantalla, desplazamiento desde
        // la posicion del jefe y capa: 0 en el piso, 1 gigante delante del mapa,
        // 2 gigante detras del mapa (asoma por detras del escenario).
        float drawScale = 1.25f, drawDx = 0.0f, drawDy = 0.0f;
        int layer = 0;
        // En su arena original: la raiz del Spine va donde la ponia el juego.
        bool arena = false;
        Vector2 root{};
        float ground = 0.0f;    // pixeles del dibujo bajo el origen (Titan sale del suelo)
        bool Play(const std::string& c) {
            const auto it = clips.find(c);
            if (it == clips.end() || !skel.Play(it->second.first, it->second.second)) return false;
            clip = c;
            return true;
        }
        Rectangle body{};
        std::vector<BossAttack> attacks;
        std::map<std::string, std::string> sounds;
    };
    // Cinematica de remate (QTE) de nuevosSprites: vinetas Spine encadenadas con
    // boton a tiempo (data/beta/qte, data/QteControl.lua).
    struct QteSeg { std::string file, anim, fail; float prompt = -1.0f; };
    struct Qte {
        bool active = false;
        std::string dir;
        std::vector<QteSeg> segs;
        int seg = 0;
        bool waiting = false, resolved = false, failing = false;
        float timer = 0.0f;
        int fighter = -1;         // indice en fighters; -1 = el jefe
        spine21::Skeleton skel;
    };
    enum class Flow { Select, Fight, StoryCard };

    Flow flow{Flow::Select};
    int cursor{0};
    int selCharacter{0};   // 0..14 dentro del bloque BETA
    int selStage{0};
    int selMode{0};        // 0 oleadas, 1 1 VS 1, 2 jefe, 3 historia
    int selRival{13};      // MEDUSA por defecto
    int selBoss{0};        // 0 Poseidon, 1 tentaculos, 2 Titan
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
    BossFight boss;
    // Interfaz original: combo, numeros de dano y pociones (Z vida, X magia).
    struct DamageNumber { Vector2 pos{}; int value = 0; float t = 0.0f; bool hero = true; };
    std::vector<DamageNumber> numbers;
    int comboHits{0};
    float comboTimer{0.0f};
    int redPotions{5}, bluePotions{5};
    void OnHeroHit(Vector2 at, int damage);
    void SwitchWeapon();
    bool EnterArena(const std::string& bossId);   // escenario original del jefe
    Qte qte;
    // HISTORIA: 3 capitulos con tarjeta narrada (texto + jefe animado) antes de
    // cada uno y final que guarda las mejoras (district_fury_beta.dat).
    int storyChapter{0};       // 0..2; 3 = final
    bool qteExplained{false};  // primera vez: tarjeta que explica el remate
    float cardTime{0.0f};
    Animator cardAnim;
    spine21::Skeleton cardSkel;
    int storyClears{0};
    void StartStory();
    void ShowStoryCard();
    void LoadProgress();
    void SaveProgress() const;
    const std::vector<std::vector<int>>& CurrentWaves() const;
    void DrawStoryCard() const;

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
    bool SpawnBoss(const std::string& id, float x);
    void UpdateBoss(float dt);
    CombatBox BossHurtbox() const;
    void DrawBoss() const;
    void DrawBossWarning() const;
    bool StartQte(const std::string& key, int fighter);
    void QtePlaySegment();
    void UpdateQte(float dt);
    void EndQte(bool success);
    void DrawQte() const;
    void CheckQteTriggers();
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
