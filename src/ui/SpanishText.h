#pragma once
#include "raylib.h"
#include <string>
#include <utility>

namespace district_fury {
inline std::string TranslateGameText(const char* text){
    if(!text)return{};std::string s(text);
    const std::pair<const char*,const char*> r[]={
        {"OLD STEEL YARD","VIEJO ASTILLERO"},{"DEEP LINE","LINEA PROFUNDA"},{"ASTRA TOWER","TORRE ASTRA"},{"PUBLIC ATRIUM","ATRIO PUBLICO"},{"RESEARCH FLOOR","PISO DE INVESTIGACION"},{"EXECUTIVE CORE","NUCLEO EJECUTIVO"},{"STEEL YARD","ASTILLERO DE ACERO"},{"STAGE CLEAR","ESCENARIO COMPLETADO"},{"STAGE 2 CLEAR","ESCENARIO 2 COMPLETADO"},{"GAME OVER","HAS CAIDO"},{"PAUSED","PAUSA"},{"RESUME","REANUDAR"},{"RESTART","REINICIAR"},{"CONTROLS","CONTROLES"},{"DIFFICULTY","DIFICULTAD"},{"SCORE","PUNTOS"},{"TIME","TIEMPO"},{"DAMAGE","DANO RECIBIDO"},{"DAMAGE TAKEN","DANO RECIBIDO"},{"MAX COMBO","COMBO MAXIMO"},{"WAVE","OLEADA"},{"PHASE","FASE"},{"HP","VIDA"},{"SP","ENERGIA"},{"RAGE","FURIA"},{"ENERGY","PODER"},{"GUARD","BLOQUEO"},{"BLOCK","BLOQUEO"},{"AREA LOCKED","AREA BLOQUEADA"},{"DEFEAT THE GATEKEEPER","DERROTA AL GUARDIAN"},{"NEXT","SIGUIENTE"},{"CLEAR","COMPLETADO"},{"THE CHAIN ROUTE","LA RUTA DE LAS CADENAS"},{"CHAIN ROUTE","RUTA DE LAS CADENAS"},{"TITAN-X has been shut down.","TITAN-X ha sido derrotado."},{"WASD MOVE","WASD MOVER"},{"J PUNCH","J GOLPE"},{"K KICK","K PATADA"},{"L ENERGY","L PODER"},{"SHIFT DASH","SHIFT DASH"},{"SPACE RAGE","SPACE FURIA"},{"ENTER / J  —  START","ENTER / J  —  COMENZAR"},{"ENTER / R — CONTINUE","ENTER / R — CONTINUAR"},{"R — RESET SAVE","R — BORRAR GUARDADO"},{"F1 — DIFFICULTY","F1 — DIFICULTAD"},{"ESC  RESUME","ESC  REANUDAR"},{"R  RESTART","R  REINICIAR"},{"Q  MENU","Q  MENU"}
    };for(const auto&x:r){std::string from(x.first),to(x.second);std::size_t p=0;while((p=s.find(from,p))!=std::string::npos){s.replace(p,from.size(),to);p+=to.size();}}return s;
}
inline void DrawTextTranslated(const char* text,int posX,int posY,int fontSize,Color color){const std::string translated=TranslateGameText(text);::DrawText(translated.c_str(),posX,posY,fontSize,color);}
}
#define DrawText district_fury::DrawTextTranslated
