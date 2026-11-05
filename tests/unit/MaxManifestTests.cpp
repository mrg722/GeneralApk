// F1.4: manifiesto y moveset de MAX (skin 6). Sin ventana: lee
// data/sprite_manifest.json y usa la maquina de estados del jugador.
// - el juego de clips max_white existe y es el del atlas max_white (antes se
//   llamaba rayder_kf_white y la transformacion no cargaba);
// - ningun clip de max/max_white usa 153-203 (otro personaje; unica excepcion el
//   clip "projectile" 176-180, solo orbes) ni 142 en max_white (texto);
// - todo indice dentro de la rejilla; knockdown sin 277/278; defeat termina en
//   298-302; las escalas por cuadro existen donde hay set grande;
// - cadenas: J de 5 y K de 4 (Chain5 / Chain6) y las habilidades 1-5.
#include "game/Player.h"
#include "game/moves/MoveRegistry.h"
#include "game/player/PlayerCommon.h"
#include "rendering/SpriteManifest.h"
#include <cassert>
#include <cstdio>
#include <cstring>

using namespace district_fury;

namespace {
constexpr float kDt = 1.0f / 60.0f;

void TestManifest() {
    SpriteManifest m;
    const bool ok = m.LoadFromFile("data/sprite_manifest.json");
    assert(ok);
    const AtlasProfile* base = m.FindAtlas("max");
    const AtlasProfile* white = m.FindAtlas("max_white");
    assert(base && white);
    assert(base->cellW == 270 && base->cellH == 175 && base->pivotX == 135 && base->pivotY == 168);
    assert(white->cellW == 270 && white->cellH == 175 && white->pivotX == 135 && white->pivotY == 168);
    const ClipMap* bc = m.FindClips(base->clipSet);
    const ClipMap* wc = m.FindClips(white->clipSet);
    assert(bc && wc);
    assert(white->clipSet == "max_white");
    assert(m.FindClips("rayder_kf_white") == nullptr);
    const int baseCells = base->columns * base->rows, whiteCells = white->columns * white->rows;
    assert(baseCells == 14 * 22 && whiteCells == 14 * 18);
    for (int pass = 0; pass < 2; ++pass) {
        const ClipMap& clips = pass == 0 ? *bc : *wc;
        const int cells = pass == 0 ? baseCells : whiteCells;
        for (const auto& kv : clips) {
            for (int f : kv.second.frames) {
                assert(f >= 0 && f < cells);
                if (kv.first != "projectile") assert(f < 153 || f > 203);
                if (pass == 1) assert(f != 142);
            }
            assert(kv.second.scales.empty() || kv.second.scales.size() == kv.second.frames.size());
        }
        for (int f : clips.at("knockdown").frames) assert(f != 277 && f != 278);
        assert(clips.count("victory") && clips.count("jump") && clips.count("punch_hook"));
    }
    const int lastDefeat = bc->at("defeat").frames.back();
    assert(lastDefeat >= 298 && lastDefeat <= 302);
    // Set grande (245-285) reducido para igualar el cuerpo del reposo.
    const ClipDef& kick = bc->at("kick");
    assert(!kick.scales.empty() && kick.scales[0] < 0.8f);
    // Los clips de ataque nuevos tambien en la forma blanca (celdas < 148).
    for (const char* c : {"kick2", "kick3", "sweep", "uppercut", "rising_uppercut", "signature1", "signature3"})
        assert(wc->count(c) == 1);
}

AttackId NextAfter(Player& p, AttackId from, InputCommand c) {
    p.inputBuffer.Push(c, 0, 0, false);
    for (int i = 0; i < 60 && (p.state != PlayerState::Attack || p.currentAttack == from); ++i) p.Update(kDt);
    return p.state == PlayerState::Attack ? p.currentAttack : AttackId::None;
}

void TestChains() {
    const MoveSet* ms = MovesetFor_Max(6);
    assert(ms && MovesetFor_Max(0) == nullptr && MovesetFor_Max(7) == nullptr);
    assert(ms->chainJCount == 5 && ms->chainKCount == 4);
    {
        Player p;
        p.ApplyCharacter(6);
        const AttackId j[5] = {AttackId::Punch1, AttackId::Punch2, AttackId::PunchHook, AttackId::Punch3, AttackId::Chain5};
        AttackId prev = AttackId::None;
        for (AttackId want : j) { assert(NextAfter(p, prev, InputCommand::Punch) == want); prev = want; }
        assert(p.CurrentAttackDef().knockdown);   // REMATE VOLADOR
    }
    {
        Player p;
        p.ApplyCharacter(6);
        const AttackId k[4] = {AttackId::Kick, AttackId::Kick2, AttackId::Kick3, AttackId::Chain6};
        AttackId prev = AttackId::None;
        for (AttackId want : k) { assert(NextAfter(p, prev, InputCommand::Kick) == want); prev = want; }
        assert(p.CurrentAttackDef().knockdown);   // PATADA GIRATORIA DE CIERRE
    }
    {   // 4o J lanza (media luna ascendente) y el 3er K rebota en la pared.
        Player p;
        p.ApplyCharacter(6);
        assert(p.EffectiveDef(AttackId::Punch3).launcher && p.EffectiveDef(AttackId::Punch3).launch >= 400);
        assert(p.EffectiveDef(AttackId::Kick3).wallBounce);
        assert(p.EffectiveDef(AttackId::KickLauncher).launch >= 700);
        assert(p.EffectiveDef(AttackId::RisingUppercut).invulnTo > 0);
        // Habilidades 1-5 distintas de las cadenas.
        const int want[5] = {static_cast<int>(AttackId::EnergyWave), static_cast<int>(AttackId::Signature1),
                             static_cast<int>(AttackId::RisingUppercut), static_cast<int>(AttackId::Signature2),
                             static_cast<int>(AttackId::Signature3)};
        for (int i = 0; i < 5; ++i) {
            assert(p.Moves().skillSlots[i] == want[i]);
            for (int c = 0; c < 5; ++c) assert(p.Moves().chainJ[c] != want[i]);
            for (int c = 0; c < 4; ++c) assert(p.Moves().chainK[c] != want[i]);
        }
        int enabled = 0;
        for (int i = 1; i < kAttackIdCount; ++i) enabled += p.MoveEnabled(static_cast<AttackId>(i)) ? 1 : 0;
        assert(enabled >= 34);   // 33 ataques + Skill
        assert(!p.MoveEnabled(AttackId::Grab));   // Fase 2: solo visual
        const MoveOverride* nova = FindOverride(*ms, static_cast<int>(AttackId::Signature3));
        assert(nova && nova->areaRadius >= 150);
        const MoveOverride* ray = FindOverride(*ms, static_cast<int>(AttackId::Signature1));
        assert(ray && ray->projectile.enabled && ray->projectile.count == 5);
    }
}
}  // namespace

int main() {
    TestManifest();
    TestChains();
    std::puts("MaxManifestTests OK");
    return 0;
}
