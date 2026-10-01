// Stage 1: todo el dibujo (mundo, jefe, HUD, menus y pantallas).
#include "game/stage1/Stage1Common.h"
#include "core/DisplaySettings.h"
#include "ui/TouchControls.h"

namespace district_fury {

// STAGE 1 — ARTE (19-09 bloqueado; 30-09 el usuario pidio usar sus fondos por
// escenario: ver DrawScenarioArt y tools/build_backgrounds.py). Lo de abajo es
// la nota historica.
//
// El Stage 1 conserva EXACTAMENTE el fondo con el que ya estaba: el arte
// procedural por escenario mas la textura "bg_industrial"
// (assets/backgrounds/old_steel_yard_clean.png, 1280x720).
//
// NO conectar aqui los fondos stage1_scenarioNN.png ni ningun pack nuevo.
// Aunque lleguen fondos nuevos para los demas stages, este no se toca.
// Cualquier cambio de arte en Stage 1 debe pedirlo el usuario de forma
// explicita.
void Stage1StoryGame::DrawScenarioArt() const {
    int s = scenario;
    // DF-014 (pedido del usuario, 30-09): cada escenario usa su fondo pintado a
    // opacidad completa (el 1 es la calle de Barrio Bajo entregada por el
    // usuario, en HD). Se panea segun el avance dentro del escenario.
    {
        const Texture2D bgS = AssetManager::Get().GetTexture(TextFormat("bg_s1_%d", std::clamp(s, 1, 4)));
        if (bgS.id) {
            const float span = std::max(1.f, (float)(ScenarioEndX() - ScenarioStartX()) - 460.f);
            DrawPannedBackdrop(bgS, cameraX, (cameraX - (float)ScenarioStartX() - 460.f) / span);
            return;
        }
    }
    DrawRectangle((int)(cameraX - 700), 0, 1400, 720, {11, 16, 19, 255});
    if (s == 1) {
        for (int x = (int)(cameraX - 700); x < cameraX + 700; x += 170) {
            DrawRectangle(x, 285, 90, 250, {25, 29, 32, 255});
            DrawRectangle(x + 20, 330, 50, 8, {150, 50, 55, 160});
            DrawRectangle(x + 8, 385, 74, 5, {70, 80, 82, 180});
        }
    } else if (s == 2) {
        for (int x = (int)(cameraX - 700); x < cameraX + 700; x += 210) {
            DrawRectangle(x, 260, 145, 275, {34, 31, 29, 255});
            DrawRectangle(x + 12, 310, 38, 42, {190, 135, 65, 110});
            DrawRectangle(x + 65, 310, 58, 42, {65, 110, 120, 120});
            DrawLine(x + 145, 260, x + 145, 520, {105, 75, 50, 210});
        }
        DrawRectangle((int)(cameraX - 700), 515, 1400, 80, {30, 55, 55, 170});
    } else if (s == 3) {
        for (int x = (int)(cameraX - 700); x < cameraX + 700; x += 240) {
            DrawRectangle(x, 235, 20, 350, {38, 45, 48, 255});
            DrawRectangle(x - 15, 255, 50, 12, {110, 125, 130, 170});
            DrawRectangle(x + 35, 285, 110, 9, {90, 105, 110, 130});
            DrawLine(x + 10, 350, x + 155, 300, {85, 95, 100, 150});
        }
        DrawRectangle((int)(cameraX - 700), 410, 1400, 20, {60, 65, 65, 220});
    } else {
        for (int x = (int)(cameraX - 700); x < cameraX + 700; x += 190) {
            DrawRectangle(x, 210, 18, 370, {33, 38, 39, 255});
            DrawLine(x + 9, 230, x + 130, 380, {105, 105, 100, 190});
            DrawLine(x + 9, 285, x + 155, 430, {105, 105, 100, 150});
            DrawCircle(x + 145, 390, 14, {110, 105, 95, 200});
        }
        DrawRectangle((int)(cameraX - 700), 285, 1400, 12, {120, 110, 90, 170});
    }
    Texture2D bg = AssetManager::Get().GetTexture("bg_industrial");
    if (bg.id)
        DrawTexturePro(bg, {0, 0, (float)bg.width, (float)bg.height}, {cameraX - 640, 0, 1280, 720}, {0, 0},
                       0,
                       A(WHITE, s == 1   ? .42f
                                : s == 2 ? .30f
                                : s == 3 ? .34f
                                         : .28f));
}

void Stage1StoryGame::DrawArenaLock() const {
    const bool waveLock = arena.Locked() && !arenaLocked;
    if (!arenaLocked && !waveLock) return;
    const int gate = waveLock ? (int)(arena.LockX() + 612.f) : (int)ScenarioEndX() - 8;
    (void)
        gate; /* DF-014: sin barrotes celestes (se leian como un bug); el limite lo marca la camara bloqueada y el aviso. */
    const char* t = waveLock ? "ZONA BLOQUEADA - DERROTA A TODOS" : "ARENA BLOQUEADA - DERROTA AL GUARDIAN";
    DrawText(t, (int)camera.x - MeasureText(t, 18) / 2, 635, 18, {130, 225, 255, 220});
}

// DF-013.2 (19-09): Brakk pasa a usar los sprites de produccion entregados
// (assets/bosses/brakk/, lienzo 256x256 con la linea de suelo en y=247). Se
// respeta el diseno original: no se recorta, no se reescala por pose y no se
// inventa ningun frame. Cada pose del set se enlaza con el estado real que ya
// existia en UpdateBoss, sin tocar vida, fases ni ataques.
//   hurt        <- invulnerabilidad tras recibir dano
//   chain       <- ChainSwing        smash  <- GroundSmash
//   charge      <- Charge            chain_throw <- PowerWave
//   fury        <- Frenzy fase 2     explosive   <- Frenzy fase 3
//   walk / run  <- persecucion (run en fase 3)   idle <- reposo y bloqueo
// death/basic/heavy/grab quedan cargadas para el remate y futuros ataques.
void Stage1StoryGame::DrawBoss() const {
    if (!boss.position.x || boss.defeated) return;
    const Vector2 p = boss.position.ToScreen();
    const bool flip = player.position.x > boss.position.x;
    const float dx = player.position.x - boss.position.x;
    const char* pose = "idle";
    if (boss.invulnerability > 0.08f)
        pose = "hurt";
    else if (boss.blocking)
        pose = "idle";
    else if (boss.attack == StoryBossAttack::ChainSwing)
        pose = "chain";
    else if (boss.attack == StoryBossAttack::GroundSmash)
        pose = "smash";
    else if (boss.attack == StoryBossAttack::Charge)
        pose = "charge";
    else if (boss.attack == StoryBossAttack::PowerWave)
        pose = "chain_throw";
    else if (boss.attack == StoryBossAttack::Frenzy)
        pose = boss.phase >= 3 ? "explosive" : "fury";
    else if (std::abs(dx) > 190.f)
        pose = boss.phase >= 3 ? "run" : "walk";
    if (bossAnim.texture.id != 0 && !bossAnim.frames.empty()) {
        Color t = boss.invulnerability > 0 ? Color{255, 170, 170, 255}
                  : boss.blocking          ? Color{185, 215, 255, 255}
                  : boss.phase >= 3        ? Color{255, 150, 140, 255}
                                           : Color{235, 205, 195, 255};
        DrawEllipse((int)p.x, (int)p.y, 70, 14, {0, 0, 0, 150});
        bossAnim.Draw(p, 1.55f * DepthScaleFor(boss.position.y), !flip, t);
        if (boss.blocking) DrawCircleLines((int)p.x, (int)(p.y - 95), 96, {120, 190, 255, 170});
        (void)pose;
        return;
    }
    const Texture2D tex = AssetManager::Get().GetTexture(std::string("brakk_") + pose);
    Color tint = WHITE;
    if (boss.invulnerability > 0)
        tint = {255, 190, 190, 255};
    else if (boss.blocking)
        tint = {185, 215, 255, 255};
    // Escala fija para todas las poses: el sprite mide 188px de alto util y
    // Brakk debe leerse mas grande que Rayden (~115px) sin ser un titan.
    const float scale = 175.0f / 188.0f;
    DrawEllipse((int)p.x, (int)p.y, 82, 15, {0, 0, 0, 150});
    if (tex.id != 0)
        DrawSpriteUniform(tex, {p.x, p.y}, scale, flip, tint, 9.0f);
    else { // respaldo: la geometria original, si faltara el PNG
        const float sc = boss.phase == 3 ? 1.18f : boss.phase == 2 ? 1.08f : 1.f;
        const Color body = boss.phase == 3 ? Color{100, 45, 40, 255} : Color{55, 55, 58, 255};
        DrawRectangle((int)(p.x - 70 * sc), (int)(p.y - 125 * sc), (int)(140 * sc), (int)(105 * sc), body);
        DrawCircle((int)p.x, (int)(p.y - 150 * sc), (int)(38 * sc), {28, 30, 31, 255});
    }
    if (boss.blocking) DrawCircleLines((int)p.x, (int)(p.y - 95), 96, {120, 190, 255, 170});
}

void Stage1StoryGame::DrawWorld() const {
    Camera2D c{};
    c.offset = {640, 360};
    c.target = {cameraX, 360};
    c.zoom = 1;
    if (shake > 0) {
        c.target.x += GetRandomValue(-100, 100) * shake * 7;
        c.target.y += GetRandomValue(-100, 100) * shake * 4;
    }
    BeginMode2D(c);
    DrawScenarioArt();
    if (!AssetManager::Get().GetTexture(TextFormat("bg_s1_%d", std::clamp(scenario, 1, 4))).id) {
        DrawRectangle(0, 625, (int)kStageEnd, 95, {9, 12, 14, 255});
        for (int x = 0; x < (int)kStageEnd; x += 150) {
            DrawRectangle(x, 617, 110, 10, {60, 64, 63, 255});
            DrawRectangle(x + 30, 645, 65, 5, {90, 75, 50, 210});
        }
    }
    DrawArenaLock();
    combatWorld.DrawGround();
    std::vector<std::pair<float, int>> order{{player.position.y, -1}};
    for (size_t i = 0; i < enemies.size(); ++i)
        if (enemies[i].active) order.push_back({enemies[i].position.y, (int)i});
    std::sort(order.begin(), order.end(), [](auto& a, auto& b) { return a.first < b.first; });
    for (auto& o : order) {
        if (o.second < 0)
            player.Draw();
        else
            enemies[(size_t)o.second].Draw();
    }
    DrawBoss();
    for (const auto& p : projectiles) {
        Vector2 s = p.position.ToScreen();
        if (!p.fromBoss && player.DrawEnergyProjectile(s, p.velocity < 0, .9f - p.life)) continue;
        DrawCircle((int)s.x, (int)s.y, 20,
                   A(p.fromBoss ? Color{255, 65, 45, 255} : Color{40, 220, 255, 255}, .28f));
        DrawCircle((int)s.x, (int)s.y, 11, p.fromBoss ? Color{255, 100, 65, 255} : Color{100, 240, 255, 255});
    }
    for (const auto& p : particles) {
        Vector2 s = p.position.ToScreen();
        float a = p.life / p.maxLife;
        DrawCircle((int)s.x, (int)s.y, p.size * a, A(p.color, a));
    }
    combatWorld.DrawEffects();
    EndMode2D();
}

void Stage1StoryGame::DrawHUD() const {
    ui::PlayerVitals vitals{};
    vitals.player = &player;
    vitals.hp = player.hp;
    vitals.maxHp = player.maxHp;
    vitals.shield = player.shield;
    vitals.maxShield = player.maxShield;
    vitals.sp = player.sp;
    vitals.maxSp = player.maxSp;
    vitals.rage = player.rage;
    vitals.maxRage = player.maxRage;
    vitals.isRageMode = player.isRageMode;
    vitals.combo = combo;
    vitals.title = player.skin == 0 ? "RAYDEN CRUZ // DISTRICT FURY"
                                    : TextFormat("%s // DISTRICT FURY", GetCharacterVisual(player.skin).name);
    vitals.x = 14;
    vitals.y = 12;
    vitals.width = 555;
    vitals.panelHeight = 132;
    ui::DrawPlayerVitals(vitals);
    DrawText(TextFormat("ESCENARIO %d/4", scenario), 395, 94, 16, {255, 205, 80, 255});
    if (player.IsBlocking())
        DrawText("BLOQUEANDO", 395, 116, 15, {90, 210, 255, 255});
    else if (player.state == PlayerState::Attack && player.attackType == AttackType::Energy)
        DrawText("CARGANDO PODER", 395, 116, 15, {90, 210, 255, 255});
    DrawText(ScenarioName(), 18, 152, 18, {210, 225, 230, 235});
    DrawText(ScenarioObjective(), 18, 174, 14, {170, 190, 195, 220});
    DrawText(TextFormat("PUNTOS %d   TIEMPO %5.1fs   DIFICULTAD %s", CalculateScore(), stageTime,
                        DifficultyText()),
             18, 196, 14, WHITE);
    if (flow == StoryFlow::Boss || flow == StoryFlow::BossIntro) {
        DrawRectangle(285, 18, 710, 56, {5, 7, 9, 235});
        DrawText("BRAKK // LA CADENA", 455, 21, 24, {255, 205, 95, 255});
        DrawRectangle(350, 53, 580, 13, {30, 25, 25, 255});
        DrawRectangle(350, 53, (int)(580.f * boss.hp / boss.maxHp), 13, {220, 65, 55, 255});
        DrawText(TextFormat("FASE %d", boss.phase), 945, 51, 15, WHITE);
    }
    if (bannerTimer > 0) {
        const char* t = ScenarioName();
        DrawText(t, 640 - MeasureText(t, 30) / 2, 225, 30, {210, 235, 240, 235});
        DrawText(storyMessage.c_str(), 640 - MeasureText(storyMessage.c_str(), 16) / 2, 263, 16,
                 {175, 200, 205, 225});
    }
    DrawGoArrow(arena.GoTimer());
}

void Stage1StoryGame::DrawMenu() const {
    DrawRectangle(0, 0, 1280, 720, {4, 7, 11, 255});
    Texture2D bg = AssetManager::Get().GetTexture("bg_industrial");
    if (bg.id)
        DrawTexturePro(bg, {0, 0, (float)bg.width, (float)bg.height}, {0, 0, 1280, 720}, {0, 0}, 0,
                       {255, 255, 255, 45});
    DrawRectangle(0, 0, 1280, 720, {3, 7, 12, 175});
    DrawText("DISTRICT FURY", 340, 90, 70, {220, 230, 240, 255});
    DrawText("BEAT-EM-UP DE HISTORIA", 430, 165, 20, {90, 210, 235, 255});
    DrawText("CAPITULO 1 // LA RUTA DE LAS CADENAS", 350, 215, 23, {255, 195, 75, 255});
    const char* items[] = {"NUEVA PARTIDA", "CONTROLES", "DIFICULTAD"};
    for (int i = 0; i < 3; ++i) {
        bool sel = (i == 0);
        Color c = sel ? Color{255, 210, 80, 255} : Color{205, 215, 220, 255};
        DrawText(items[i], 480, 305 + i * 55, 25, c);
    }
    DrawText(TextFormat("DIFICULTAD ACTUAL: %s", DifficultyText()), 455, 490, 18, {150, 190, 200, 255});
    DrawText(TextFormat("MEJOR PUNTUACION: %d   MEJOR RANGO: %s", bestScore, RankName(bestRank)), 365, 535,
             16, {150, 170, 180, 230});
    DrawText("ENTER/J: JUGAR   C: CONTROLES   ↑/↓: DIFICULTAD", 365, 620, 15, {120, 150, 160, 230});
    DrawText("60 FPS // MODO HISTORIA PC", 500, 660, 13, {90, 120, 130, 220});
}

void Stage1StoryGame::DrawControls() const {
    DrawRectangle(0, 0, 1280, 720, {5, 8, 11, 255});
    DrawText("CONTROLES", 500, 75, 48, WHITE);
    DrawText("W A S D", 380, 165, 24, {185, 220, 240, 255});
    DrawText("Moverse y cambiar de carril", 610, 165, 20, WHITE);
    DrawText("J", 380, 210, 24, {255, 205, 80, 255});
    DrawText("Golpe / combo", 610, 210, 20, WHITE);
    DrawText("K", 380, 255, 24, {255, 205, 80, 255});
    DrawText("Patada fuerte", 610, 255, 20, WHITE);
    DrawText("L", 380, 300, 24, {80, 220, 255, 255});
    DrawText("Poder de energia", 610, 300, 20, WHITE);
    DrawText("B", 380, 345, 24, {80, 220, 255, 255});
    DrawText("Bloquear / reducir dano", 610, 345, 20, WHITE);
    DrawText("SHIFT", 380, 390, 24, {120, 200, 255, 255});
    DrawText("Dash / invulnerabilidad breve", 610, 390, 20, WHITE);
    DrawText("SPACE", 380, 435, 24, {100, 220, 255, 255});
    DrawText("Modo Furia", 610, 435, 20, WHITE);
    DrawText("ESC", 380, 480, 24, WHITE);
    DrawText("Pausa", 610, 480, 20, WHITE);
    DrawText("S, S+D, D + J", 380, 525, 22, {255, 150, 90, 255});
    DrawText("Onda de energia (comando; cancela golpes)", 610, 525, 20, WHITE);
    DrawText("D, S, S+D + K", 380, 560, 22, {255, 150, 90, 255});
    DrawText("Gancho ascendente (lanza al enemigo)", 610, 560, 20, WHITE);
    DrawText("C / ESC — VOLVER", 500, 630, 18, {180, 195, 200, 220});
}

void Stage1StoryGame::DrawOptions() const {
    DrawRectangle(0, 0, 1280, 720, {4, 7, 11, 255});
    Texture2D art = AssetManager::Get().GetTexture("menu_main_art");
    if (art.id)
        DrawTexturePro(art, {0, 0, (float)art.width, (float)art.height}, {0, 0, 1280, 720}, {0, 0}, 0,
                       {255, 255, 255, 60});
    DrawRectangle(0, 0, 1280, 720, {3, 7, 12, 190});
    DrawText("OPCIONES", 520, 110, 48, WHITE);
    const bool muted = AudioSystem::Get().IsMuted();
    const Color hi{255, 214, 72, 255}, lo{200, 220, 230, 255};
    if (optionsCursor == 0) DrawRectangle(430, 258, 520, 48, {30, 50, 70, 200});
    DrawText("SONIDO", 470, 270, 26, optionsCursor == 0 ? hi : lo);
    DrawText(muted ? "DESACTIVADO" : "ACTIVADO", 700, 270, 26,
             muted ? Color{255, 110, 100, 255} : Color{110, 240, 160, 255});
    if (optionsCursor == 1) DrawRectangle(430, 318, 520, 48, {30, 50, 70, 200});
    DrawText("ANCHO DE PANTALLA", 470, 330, 26, optionsCursor == 1 ? hi : lo);
    DrawText(TextFormat("< %d%% >", display::WidthPercent()), 790, 330, 26, {110, 240, 160, 255});
    const bool t = touch::Enabled();
    DrawText(t ? "CRUCETA ARRIBA/ABAJO: ELEGIR   IZQ/DER: CAMBIAR   OK: ALTERNAR"
               : "W/S ELEGIR   A/D CAMBIAR   ENTER/J ALTERNAR",
             t ? 330 : 420, 395, 18, {150, 190, 200, 230});
    DrawText("ANCHO: si tu celular es muy alargado y todo se ve ancho, bajalo (solo cambia lo horizontal).",
             200, 430, 16, {150, 175, 185, 220});
    DrawText(TextFormat("DIFICULTAD ACTUAL: %s", DifficultyText()), 440, 470, 18, {150, 190, 200, 230});
    DrawText(t ? "ATRAS - VOLVER" : "ESC - VOLVER", 540, 520, 20, {180, 195, 200, 220});
}

void Stage1StoryGame::DrawCredits() const {
    DrawRectangle(0, 0, 1280, 720, {4, 7, 11, 255});
    Texture2D art = AssetManager::Get().GetTexture("menu_main_art");
    if (art.id)
        DrawTexturePro(art, {0, 0, (float)art.width, (float)art.height}, {0, 0, 1280, 720}, {0, 0}, 0,
                       {255, 255, 255, 45});
    DrawRectangle(0, 0, 1280, 720, {3, 7, 12, 200});
    DrawText("CREDITOS", 520, 110, 48, WHITE);
    DrawText("DISTRICT FURY", 520, 215, 26, {90, 210, 235, 255});
    DrawText("Diseno, arte y programacion", 470, 265, 18, {170, 190, 200, 230});
    DrawText("Martin Reyes", 550, 295, 20, {255, 205, 80, 255});
    DrawText("\"La ciudad no perdona, pero aun quedan los que luchan\"", 300, 380, 16, {150, 175, 185, 220});
    DrawText("ESC / ENTER — VOLVER", 480, 470, 18, {180, 195, 200, 220});
}

void Stage1StoryGame::DrawPause() const {
    DrawRectangle(0, 0, 1280, 720, {0, 0, 0, 185});
    DrawRectangle(330, 165, 620, 390, {7, 12, 18, 245});
    DrawText("PAUSA", 535, 210, 54, {220, 230, 240, 255});
    DrawText("ESC — REANUDAR", 480, 300, 22, WHITE);
    DrawText("R — REINICIAR", 495, 345, 22, WHITE);
    DrawText("Q — VOLVER AL MENU", 450, 390, 22, WHITE);
    DrawText("El combate se detiene completamente.", 430, 465, 17, {140, 175, 190, 230});
}

void Stage1StoryGame::DrawGameOver() const {
    DrawRectangle(0, 0, 1280, 720, {0, 0, 0, 210});
    DrawRectangle(300, 145, 680, 430, {8, 10, 15, 245});
    DrawText("HAS CAIDO", 465, 220, 62, {235, 60, 65, 255});
    DrawText("La ruta se perdio.", 500, 295, 22, WHITE);
    DrawText("Tu mejor opcion es volver a intentarlo.", 405, 335, 18, {170, 190, 200, 240});
    DrawText("R — REINICIAR", 470, 405, 22, {255, 210, 80, 255});
    DrawText("Q — VOLVER AL MENU", 435, 450, 22, WHITE);
    DrawText(TextFormat("PUNTUACION: %d", CalculateScore()), 475, 500, 18, {150, 175, 185, 230});
}

void Stage1StoryGame::DrawScenarioClear() const {
    DrawRectangle(0, 0, 1280, 720, {4, 8, 10, 235});
    DrawText("RUTA ASEGURADA", 425, 160, 52, {90, 220, 150, 255});
    DrawText(TextFormat("ESCENARIO %d SUPERADO", scenario - 1), 455, 235, 28, WHITE);
    DrawText(storyMessage.c_str(), 220, 310, 18, {175, 200, 205, 235});
    DrawText("La siguiente ruta ha sido desbloqueada.", 400, 380, 19, {255, 200, 80, 255});
}

void Stage1StoryGame::DrawStageClear() const {
    DrawRectangle(0, 0, 1280, 720, {4, 7, 10, 245});
    DrawText("ETAPA 1 COMPLETADA", 360, 95, 58, {90, 225, 155, 255});
    DrawText("LA RUTA DE LAS CADENAS", 430, 165, 26, {255, 200, 80, 255});
    DrawText(TextFormat("TIEMPO       %6.1fs", stageTime), 400, 245, 22, WHITE);
    DrawText(TextFormat("ENEMIGOS     %6d", defeated), 400, 285, 22, WHITE);
    DrawText(TextFormat("DANO RECIBIDO %6d", damageTaken), 400, 325, 22, WHITE);
    DrawText(TextFormat("COMBO MAXIMO %6d", maxCombo), 400, 365, 22, WHITE);
    DrawText(TextFormat("PUNTUACION   %6d", CalculateScore()), 400, 405, 22, WHITE);
    DrawText(TextFormat("RANGO        %s", RankText()), 400, 455, 38, {255, 205, 70, 255});
    DrawText(TextFormat("XP %d   MONEDAS %d   GEMAS %d", xp, coins, gems), 400, 515, 18,
             {175, 200, 205, 255});
    DrawText("ENTER — STAGE 2: OLD STEEL YARD     R — MENU", 455, 625, 18, WHITE);
}

void Stage1StoryGame::Draw() const {
    if (flow == StoryFlow::CharacterSelect) {
        DrawCharacterSelect();
        return;
    }
    if (flow == StoryFlow::Menu) {
        ui::DrawMainMenuArt(AssetManager::Get().GetTexture("menu_main_art"), menuCursor, DifficultyText());
        return;
    }
    if (flow == StoryFlow::Controls) {
        DrawControls();
        return;
    }
    if (flow == StoryFlow::Options) {
        DrawOptions();
        return;
    }
    if (flow == StoryFlow::Credits) {
        DrawCredits();
        return;
    }
    DrawWorld();
    if (flow != StoryFlow::StageClear) DrawHUD();
    if (flow == StoryFlow::Intro) {
        DrawRectangle(0, 0, 1280, 720, {4, 8, 11, 145});
        DrawText("LA RUTA DE LAS CADENAS", 410, 280, 48, {220, 230, 235, 255});
        DrawText("ENTER / J — COMENZAR", 460, 350, 22, WHITE);
    }
    if (flow == StoryFlow::Pause) DrawPause();
    if (flow == StoryFlow::GameOver) DrawGameOver();
    if (flow == StoryFlow::ScenarioClear) DrawScenarioClear();
    if (flow == StoryFlow::StageClear) DrawStageClear();
}

// DF-014: pantalla de eleccion de luchador. Muestra el primer cuadro de cada
// atlas del manifiesto (mismo recorte y pivote que en el juego).
void Stage1StoryGame::DrawCharacterSelect() const {
    DrawRectangle(0, 0, 1280, 720, {6, 9, 14, 255});
    const char* title = "ELIGE TU LUCHADOR";
    DrawText(title, 640 - MeasureText(title, 44) / 2, 70, 44, {230, 235, 240, 255});
    constexpr int n = (int)(sizeof(kStoryCharacters) / sizeof(kStoryCharacters[0]));
    for (int i = 0; i < n; ++i) {
        const int id = kStoryCharacters[i];
        const CharacterVisual& cv = GetCharacterVisual(id);
        const bool sel = i == characterCursor;
        const int cx = 640 + (i - (n - 1) * 0.5f) * 420;
        const Rectangle box{(float)cx - 170, 170, 340, 400};
        DrawRectangleRec(box, sel ? Color{20, 40, 64, 255} : Color{14, 18, 26, 255});
        DrawRectangleLinesEx(box, sel ? 4.f : 2.f, sel ? Color{90, 200, 255, 255} : Color{60, 70, 85, 255});
        const char* atlas = cv.atlasId ? cv.atlasId : "rayden";
        const AtlasProfile* prof = SpriteManifest::Get().FindAtlas(atlas);
        Texture2D tex = prof ? AssetManager::Get().GetTextureByPath(prof->path) : Texture2D{0};
        if (prof && tex.id) {
            // Misma escala y ancho que en combate (Rayder: complexion de Rayden).
            const float scale = 2.6f * (cv.atlasId ? cv.scale : 1.0f);
            const float scaleX = scale * cv.widthScale;
            const Rectangle src{0, 0, prof->cellW, prof->cellH};
            const Rectangle dst{cx - prof->pivotX * scaleX, 520 - prof->pivotY * scale, prof->cellW * scaleX,
                                prof->cellH * scale};
            DrawTexturePro(tex, src, dst, {0, 0}, 0, WHITE);
        }
        const char* name = id == 0 ? "RAYDEN CRUZ" : cv.name;
        DrawText(name, cx - MeasureText(name, 28) / 2, 530, 28,
                 sel ? Color{255, 214, 72, 255} : Color{200, 205, 210, 255});
    }
    const char* hint = touch::Enabled() ? "CRUCETA  ELEGIR     OK  CONFIRMAR     ATRAS  VOLVER"
                                        : "A / D  ELEGIR     ENTER  CONFIRMAR     ESC  VOLVER";
    DrawText(hint, 640 - MeasureText(hint, 18) / 2, 640, 18, {150, 170, 185, 255});
}

} // namespace district_fury
