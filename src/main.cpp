#include "raylib.h"
#include "audio/AudioSystem.h"
#include "game/Stage1StoryGame.h"
#include "game/Stage2Game.h"
#include "game/Stage3Game.h"
#include "game/Stage4Game.h"
#include "game/Stage5Game.h"
#include "game/VSMode.h"
#include "core/ApplicationState.h"
#include "game/Campaign.h"
#include "rendering/AssetManager.h"

namespace {
void DrawMenuPrincipal() {
    DrawRectangle(0,0,1280,720,{4,7,11,255});
    Texture2D bg=district_fury::AssetManager::Get().GetTexture("bg_industrial");
    if(bg.id) DrawTexturePro(bg,{0,0,(float)bg.width,(float)bg.height},{0,0,1280,720},{0,0},0,{255,255,255,68});
    DrawRectangle(0,0,1280,720,{2,6,11,188});
    DrawRectangle(250,70,780,560,{4,9,14,238});
    DrawRectangleLines(250,70,780,560,{55,80,95,160});
    DrawRectangle(250,70,7,560,{60,205,240,230});
    DrawRectangle(1023,70,7,560,{255,205,75,210});
    DrawText("DISTRICT FURY",340,105,62,{225,235,240,255});
    DrawText("BEAT-EM-UP DE HISTORIA",445,172,20,{75,215,240,255});
    DrawText("CAPITULO 1 // LA RUTA DE LAS CADENAS",370,214,22,{255,205,75,255});
    DrawRectangle(410,280,460,58,{10,19,27,235});
    DrawRectangleLines(410,280,460,58,{255,205,75,190});
    DrawText("ENTER / J",440,300,18,{255,225,135,255});
    DrawText("NUEVA PARTIDA",595,300,20,WHITE);
    DrawRectangle(410,355,460,48,{8,16,23,220});
    DrawText("C",440,369,18,{80,215,240,255});
    DrawText("CONTROLES",595,369,18,{210,220,225,255});
    DrawRectangle(410,417,460,48,{8,16,23,220});
    DrawText("↑ / ↓",440,431,18,{255,205,75,255});
    DrawText("DIFICULTAD",595,431,18,{210,220,225,255});
    DrawRectangle(410,479,460,58,{8,16,23,235});
    DrawRectangleLines(410,479,460,58,{70,200,230,180});
    DrawText("V",440,498,20,{80,220,245,255});
    DrawText("MODO VS // LABORATORIO",595,498,20,{220,230,235,255});
    DrawText("ENTER/J  JUGAR     C  CONTROLES     ↑/↓  DIFICULTAD     V  MODO VS",342,585,13,{150,180,190,235});
    DrawText("El Modo VS permite probar stages, escenarios y hasta 4 enemigos.",350,608,12,{125,160,175,230});
    DrawText("PC // 60 FPS",965,682,11,{105,130,140,210});
}
}

int main(){
    constexpr int windowWidth=1280,windowHeight=720;
    InitWindow(windowWidth,windowHeight,"District Fury");
    SetExitKey(KEY_NULL); SetTargetFPS(60);
    district_fury::AssetManager::Get().LoadAll();
    district_fury::AudioSystem::Get().Init();
    district_fury::Stage1StoryGame stage1; district_fury::Stage2Game stage2; district_fury::Stage3Game stage3; district_fury::Stage4Game stage4; district_fury::Stage5Game stage5; district_fury::VSMode vsMode;
    stage1.Init(); stage2.Init(); stage3.Init(); stage4.Init(); stage5.Init(); vsMode.Init();
    int activeStage=1; bool vsActive=false;
    // DF-013.2 (19-09): pantalla de recompensa entre stages (el "suero" del
    // documento de diseno). rewardStage = stage recien superado, 0 = inactiva.
    int rewardStage=0;
    district_fury::Campaign::Get().Load();
    district_fury::core::ApplicationState state=district_fury::core::ApplicationState::Running;
    while(district_fury::core::shouldContinue(state)){
        if(WindowShouldClose()){state=district_fury::core::ApplicationState::ExitRequested;continue;}
        if(vsActive){vsMode.Update(GetFrameTime());if(vsMode.ShouldExit()){vsMode.ClearExit();vsActive=false;stage1.ReturnToMenu();}}
        else{
            // DF-013: MODO VS se puede pedir con la tecla directa V (atajo
            // historico) o eligiendolo en el menu con cursor+ENTER; ambas
            // rutas terminan aqui, en el unico lugar que sabe crear VSMode.
            if(stage1.IsMenu()&&(IsKeyPressed(KEY_V)||stage1.ConsumeVsRequest())){vsMode.Init();vsActive=true;}
            // Nueva partida: la campana arranca sin mejoras acumuladas.
            if(!vsActive&&stage1.ConsumeNewGame()){
                auto& campaign=district_fury::Campaign::Get();
                campaign.ResetRun();campaign.Save();
                activeStage=1;rewardStage=0;stage1.PlayerRef().upgrades=campaign.upgrades;
            }
            if(!vsActive&&rewardStage>0){
                // Eleccion de mejora: 1/2/3. Al elegir, se aplica y arranca el
                // stage siguiente con las mejoras ya puestas.
                int choice=IsKeyPressed(KEY_ONE)?0:IsKeyPressed(KEY_TWO)?1:IsKeyPressed(KEY_THREE)?2:-1;
                if(choice>=0){
                    auto& campaign=district_fury::Campaign::Get();
                    campaign.ApplyReward(rewardStage,choice);
                    const int next=rewardStage+1; rewardStage=0; activeStage=next;
                    if(next==2){stage2.PlayerRef().upgrades=campaign.upgrades;stage2.Init();}
                    else if(next==3){stage3.PlayerRef().upgrades=campaign.upgrades;stage3.Init();}
                    else if(next==4){stage4.PlayerRef().upgrades=campaign.upgrades;stage4.Init();}
                    else{stage5.PlayerRef().upgrades=campaign.upgrades;stage5.Init();}
                }
            }
            else if(!vsActive){
                if(IsKeyPressed(KEY_F1)){activeStage=1;stage1.Init();}
                if(IsKeyPressed(KEY_F2)){activeStage=2;stage2.Init();}
                if(IsKeyPressed(KEY_F3)){activeStage=3;stage3.Init();}
                if(IsKeyPressed(KEY_F4)){activeStage=4;stage4.Init();}
                if(IsKeyPressed(KEY_F5)){activeStage=5;stage5.Init();}
                if(activeStage==1)stage1.Update(GetFrameTime()); else if(activeStage==2)stage2.Update(GetFrameTime()); else if(activeStage==3)stage3.Update(GetFrameTime()); else if(activeStage==4)stage4.Update(GetFrameTime()); else stage5.Update(GetFrameTime());
                // DF-013.2 (19-09): CAMPANA ENCADENADA. Al vencer al boss de un
                // stage, su pantalla de victoria espera ENTER y aqui se carga el
                // stage siguiente. Antes cada stage volvia al menu o se
                // reiniciaba, y los stages 2-5 solo se abrian con F2-F5 (los
                // atajos siguen existiendo como debug).
                bool advance=false;
                if(activeStage==1)advance=stage1.ConsumeAdvance();
                else if(activeStage==2)advance=stage2.ConsumeAdvance();
                else if(activeStage==3)advance=stage3.ConsumeAdvance();
                else if(activeStage==4)advance=stage4.ConsumeAdvance();
                else advance=stage5.ConsumeAdvance();
                if(advance){
                    if(activeStage>=5){
                        // Campana terminada: se guarda y se vuelve al menu.
                        district_fury::Campaign::Get().stagesCompleted=5;
                        district_fury::Campaign::Get().Save();
                        activeStage=1;stage1.Init();stage1.ReturnToMenu();
                    }
                    else rewardStage=activeStage;   // abre la pantalla de suero/mejora
                }
                if(stage1.ExitRequested()) state=district_fury::core::ApplicationState::ExitRequested;
            }
        }
        BeginDrawing(); ClearBackground({8,11,11,255});
        if(vsActive)vsMode.Draw();
        // DF-013: Stage1StoryGame::Draw() ahora dibuja su propio menu con el
        // arte final (ui/MainMenu.h); ya no hace falta la vista alternativa
        // que vivia aqui (DrawMenuPrincipal queda sin usar, ver DF-013 D9).
        else if(activeStage==1)stage1.Draw();
        else if(activeStage==2)stage2.Draw();
        else if(activeStage==3)stage3.Draw();
        else if(activeStage==4)stage4.Draw();
        else stage5.Draw();
        if(!vsActive&&rewardStage>0){
            DrawRectangle(0,0,1280,720,{0,0,0,205});
            DrawRectangle(190,120,900,480,{8,12,16,240});
            DrawRectangleLines(190,120,900,480,{255,205,75,180});
            DrawText(TextFormat("STAGE %d SUPERADO",rewardStage),470,150,30,{255,205,75,255});
            DrawText("SUERO X // ELIGE UNA MEJORA PARA EL RESTO DE LA CAMPANA",265,195,17,{190,215,230,255});
            const auto options=district_fury::Campaign::Get().OptionsFor(rewardStage);
            for(int i=0;i<3;++i){
                const int y=250+i*105;
                DrawRectangle(230,y,820,90,{14,20,26,235});
                DrawRectangleLines(230,y,820,90,{80,120,140,150});
                DrawText(TextFormat("%d",i+1),258,y+28,34,{255,205,75,255});
                DrawText(options[(size_t)i].name,320,y+20,24,WHITE);
                DrawText(options[(size_t)i].description,320,y+55,16,{175,200,215,235});
            }
            DrawText("PULSA 1, 2 o 3",560,575,18,{200,220,230,240});
        }
        if(!vsActive&&rewardStage==0&&!stage1.IsMenu()){DrawRectangle(985,684,290,24,{5,8,10,185});DrawText("CAMPANA 1-5  |  F1..F5 DEBUG",993,689,11,{180,200,205,210});}
        EndDrawing();
    }
    district_fury::AudioSystem::Get().Shutdown(); district_fury::AssetManager::Get().UnloadAll(); CloseWindow(); return 0;
}
