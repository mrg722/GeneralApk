// F2.3: lista de movimientos de la pausa y HUD de combate. Sin ventana.
// - Para los 42 personajes la lista se genera sin crash, sin entradas duplicadas
//   y solo con movimientos HABILITADOS para ese personaje (lee displayName,
//   entradas y capacidades reales del MoveSet; no define movimientos propios).
// - El texto cabe en su caja: ancho medido (fuente por defecto de raylib; sin
//   ventana, una cota conservadora) <= ancho de la columna.
// - CombatHud: cuenta golpes por caida de vida, numeros flotantes, escalado,
//   nombre del especial 0.8 s y avisos.
#include "game/CharacterVisual.h"
#include "game/Player.h"
#include "game/StreetEnemy.h"
#include "game/moves/MoveRegistry.h"
#include "ui/GameHUD.h"
#include "ui/MoveListScreen.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <set>
#include <string>

using namespace district_fury;

namespace {

bool HasRow(const std::vector<ui::MovePage>& pages, AttackId id) {
    for (const auto& pg : pages)
        for (const auto& r : pg.rows)
            if (r.id == id) return true;
    return false;
}

bool HasInput(const std::vector<ui::MovePage>& pages, const char* input) {
    for (const auto& pg : pages)
        for (const auto& r : pg.rows)
            if (r.input == input) return true;
    return false;
}

void CheckCharacter(int skin, int* totalRows) {
    Player p;
    p.ApplyCharacter(skin);
    std::vector<ui::MovePage> pages = ui::BuildMoveList(p);
    assert(!pages.empty());
    // Siempre: Furia, Defensa y Habilidades (con TRANSFORMAR al final).
    bool furia = false, defensa = false, habil = false;
    std::set<std::string> titles;
    for (const ui::MovePage& pg : pages) {
        assert(!pg.rows.empty());                       // no hay paginas vacias
        assert(titles.insert(pg.title).second);         // ni paginas repetidas
        furia |= pg.title == "FURIA";
        defensa |= pg.title == "DEFENSA Y AGARRE";
        habil |= pg.title == "HABILIDADES";
        std::set<std::string> keys;
        std::set<int> ids;
        for (const ui::MoveRow& r : pg.rows) {
            ++*totalRows;
            assert(!r.name.empty() && !r.input.empty());
            // Sin duplicados: ni la misma entrada+contexto ni el mismo ataque dos veces.
            assert(keys.insert(r.input + "|" + r.note).second);
            if (r.id != AttackId::None) {
                assert(ids.insert(static_cast<int>(r.id)).second || pg.title == "HABILIDADES");
                // Solo movimientos habilitados para ESTE personaje.
                assert(p.MoveEnabled(r.id));
                assert(r.damage == p.EffectiveDef(r.id).damage);
            }
            // El texto cabe en su caja.
            assert(r.name.size() <= 30);
            const int nfs = ui::FitFontSize(r.name.c_str(), ui::kMoveNameColW, ui::kMoveNameFont);
            assert(nfs >= 12 && nfs <= ui::kMoveNameFont);
            assert(ui::MeasureUiText(r.name.c_str(), nfs) <= ui::kMoveNameColW);
            assert(ui::MoveInputWidth(r.input) <= ui::kMoveInputColW);
            if (!r.note.empty()) {
                const int fs = ui::FitFontSize(r.note.c_str(), ui::kMoveNoteColW, 13);
                assert(fs >= ui::kMoveMinFont && ui::MeasureUiText(r.note.c_str(), fs) <= ui::kMoveNoteColW);
            }
        }
    }
    assert(furia && defensa && habil);
    // Sin ventana ni Grab (Fase 2 apagada o encendida): lo que aparece esta habilitado.
    if (!p.MoveEnabled(AttackId::Grab)) assert(!HasRow(pages, AttackId::Grab));
    // Una lista real no puede ser un esqueleto: cadenas J con al menos un golpe.
    assert(pages.front().title == "CADENAS" && pages.front().rows.size() >= 2);
}

void TestAllCharacters() {
    const int n = CharacterCount();
    assert(n == 42);
    int total = 0;
    for (int skin = 0; skin < n; ++skin) CheckCharacter(skin, &total);
    std::printf("move_list: %d personajes, %d filas\n", n, total);
}

void TestRaydenContent() {
    Player p;
    p.ApplyCharacter(0);
    const auto pages = ui::BuildMoveList(p);
    // Cadenas J (hasta 4) y K, lanzador, direccionales, aire y los 3 comandos.
    assert(HasInput(pages, "J") && HasInput(pages, "J J J J") && HasInput(pages, "K K K"));
    assert(HasInput(pages, "J J K"));
    assert(HasRow(pages, AttackId::FwdPunch) && HasRow(pages, AttackId::Sweep) && HasRow(pages, AttackId::Uppercut));
    assert(HasRow(pages, AttackId::AirKick) && HasRow(pages, AttackId::DiveKick) && HasInput(pages, "I / SALTO"));
    assert(HasInput(pages, "QCF+J / L") && HasInput(pages, "DP+K") && HasInput(pages, "QCB+K") && HasInput(pages, "QCF+K"));
    assert(HasInput(pages, "ESPACIO") && HasInput(pages, "6"));
    // El nombre sale del displayName cuando el personaje lo define.
    const MoveSet& ms = p.Moves();
    for (int i = 0; i < ms.overrideCount; ++i) {
        const MoveOverride& o = ms.overrides[i];
        if (!o.displayName || !p.MoveEnabled(static_cast<AttackId>(o.id))) continue;
        for (const auto& pg : pages)
            for (const auto& r : pg.rows)
                if (static_cast<int>(r.id) == o.id) assert(r.name == o.displayName);
    }
}

void TestGatedByCaps() {
    // BRAKK (skin 2): sin salto ni aereos -> sin pagina AIRE.
    Player b;
    b.ApplyCharacter(2);
    const auto pages = ui::BuildMoveList(b);
    for (const auto& pg : pages) assert(pg.title != "AIRE");
    assert(!HasRow(pages, AttackId::AirKick) && !HasRow(pages, AttackId::Hurricane));
    assert(HasRow(pages, AttackId::Signature1) && HasRow(pages, AttackId::Signature3));
}

void TestScreenState() {
    Player p;
    p.ApplyCharacter(6);
    ui::MoveListScreen s;
    assert(!s.IsOpen());
    s.Open(p);
    assert(s.IsOpen() && s.PageCount() >= 5 && s.PageIndex() == 0);
    s.SelectPage(-1);
    assert(s.PageIndex() == s.PageCount() - 1);
    s.SelectPage(s.PageCount());
    assert(s.PageIndex() == 0);
    s.Close();
    assert(!s.IsOpen());
}

void TestKfSkillPages() {
    // Un KF con habilidades de clip: las habilidades se listan por paginas y el
    // skillPage del jugador queda como estaba.
    Player p;
    p.ApplyCharacter(7);
    p.skillPage = 0;
    const auto pages = ui::BuildMoveList(p);
    assert(p.skillPage == 0);
    assert(pages.back().title == "HABILIDADES");
    assert(pages.back().rows.back().input == "6");
}

void TestCombatHud() {
    using ui::Combat;
    Combat().Reset();
    Player p;
    std::vector<StreetEnemy> enemies(1);
    enemies[0].Init({400, 580, 0}, StreetEnemyType::Punk);
    enemies[0].Activate();
    enemies[0].hp = enemies[0].maxHp = 100;
    Combat().Update(0.016f, p, enemies, 0);
    assert(Combat().Hits() == 0 && Combat().FloatingCount() == 0);
    // Un golpe de 12: contador, dano acumulado y numero flotante.
    enemies[0].hp = 88;
    Combat().Update(0.016f, p, enemies, 1);
    assert(Combat().Hits() == 1 && Combat().ComboDamage() == 12 && Combat().FloatingCount() == 1);
    // Segundo golpe: sigue el combo (dentro de la ventana).
    enemies[0].hp = 70;
    Combat().Update(0.1f, p, enemies, 2);
    assert(Combat().Hits() == 2 && Combat().ComboDamage() == 30 && Combat().FloatingCount() == 2);
    // Curar no cuenta como golpe.
    enemies[0].hp = 100;
    Combat().Update(0.016f, p, enemies, 2);
    assert(Combat().Hits() == 2);
    // Pasado el tiempo, los numeros mueren y el siguiente golpe reinicia el combo.
    for (int i = 0; i < 200; ++i) Combat().Update(0.016f, p, enemies, 0);
    assert(Combat().FloatingCount() == 0);
    enemies[0].hp = 90;
    Combat().Update(0.016f, p, enemies, 1);
    assert(Combat().Hits() == 1 && Combat().ComboDamage() == 10);
    // Counter-hit: el enemigo estaba en su arranque de ataque -> aviso CONTRA!.
    Combat().AddHit(15, {400, 580, 0}, true);
    assert(Combat().NoticeCount() >= 1);
    // Escalado de dano: sin escalado hasta 3 golpes, -8 % por golpe despues, minimo 35 %.
    assert(std::fabs(ui::ComboDamageScale(3) - 1.0f) < 1e-5f);
    assert(std::fabs(ui::ComboDamageScale(6) - 0.76f) < 1e-5f);
    assert(std::fabs(ui::ComboDamageScale(40) - 0.35f) < 1e-5f);
    const Color white = ui::ComboScaleColor(1.0f), red = ui::ComboScaleColor(0.35f);
    assert(white.g > 200 && white.b > 200 && red.g < 100 && red.b < 100);
    // Nombre del especial al lanzarlo (0.8 s).
    Combat().Reset();
    p.state = PlayerState::Attack;
    p.currentAttack = AttackId::EnergyWave;
    Combat().Update(0.016f, p, enemies, 0);
    assert(!Combat().SpecialName().empty() && Combat().SpecialTime() > 0.7f);
    for (int i = 0; i < 60; ++i) Combat().Update(0.016f, p, enemies, 0);
    assert(Combat().SpecialTime() <= 0.0f);
    // Un golpe normal no anuncia nada.
    Combat().Reset();
    p.currentAttack = AttackId::Punch1;
    p.state = PlayerState::Idle;
    Combat().Update(0.016f, p, enemies, 0);
    p.state = PlayerState::Attack;
    Combat().Update(0.016f, p, enemies, 0);
    assert(Combat().SpecialName().empty());
    // Aviso manual (F2.1: PERFECTO); el mismo texto se renueva, no se apila.
    Combat().Notice("PERFECTO", {120, 240, 255, 255});
    Combat().Notice("PERFECTO", {120, 240, 255, 255});
    assert(Combat().NoticeCount() == 1);
    Combat().Reset();
}

}  // namespace

int main() {
    TestAllCharacters();
    TestRaydenContent();
    TestGatedByCaps();
    TestScreenState();
    TestKfSkillPages();
    TestCombatHud();
    std::puts("MoveListTests OK");
    return 0;
}
