#pragma once
#include "texture.h"
#include "esp.h"
#include <string>

enum class TabId {
    Combat = 0,
    Visuals,
    Misc,
    Config,
    COUNT
};

struct CombatState {
    bool  aimbot        = false;
    bool  silent        = false;
    bool  visibleCheck  = true;
    float fov           = 12.f;
    float smooth        = 6.f;
    int   bone          = 0;
};

struct MiscState {
    bool  streamproof   = false;
    bool  vsync         = true;
    bool  watermark     = true;
    int   menuKey       = 0x2D;
};

struct AppState {
    TabId       tab           = TabId::Visuals;
    float       tabAnim[4]    = {0,1,0,0};
    float       contentAlpha  = 0.f;
    float       openAnim      = 0.f;
    bool        menuOpen      = true;
    CombatState combat;
    MiscState   misc;
    Texture     logoLg;
    Texture     logoSm;
    char status[168] = "Feature 2 - ESP";
    EspSystem   esp;
    bool        autoAttach    = true;
};

void UI_InitFonts();
void UI_Render(AppState& st, ID3D11Device* device);
