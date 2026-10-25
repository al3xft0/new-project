#pragma once
#include "texture.h"
#include <string>
#include <vector>
#include <cmath>

enum class TabId {
    Combat = 0,
    Visuals,   // ESP lives here first
    Misc,
    Config,
    COUNT
};

struct Toggle {
    const char* label;
    bool*       value;
    const char* hint = nullptr;
};

struct SliderF {
    const char* label;
    float*      value;
    float       vmin, vmax;
    const char* fmt = "%.1f";
};

// Feature state — only ESP wired as sections for now; rest placeholders
struct EspState {
    bool  enabled       = true;
    bool  box           = true;
    bool  skeleton      = false;
    bool  name          = true;
    bool  distance      = true;
    bool  teamCheck     = true;
    bool  showDowned    = true;
    float maxDistance   = 300.f;
    float boxThickness  = 1.5f;
    int   boxStyle      = 0; // 0 full, 1 corner
};

struct CombatState {
    bool  aimbot        = false;
    bool  silent        = false;
    bool  visibleCheck  = true;
    float fov           = 12.f;
    float smooth        = 6.f;
    int   bone          = 0; // 0 head, 1 neck, 2 chest
};

struct MiscState {
    bool  streamproof   = false;
    bool  vsync         = true;
    bool  watermark     = true;
    int   menuKey       = 0x2D; // VK_INSERT
};

struct AppState {
    TabId       tab           = TabId::Visuals;
    float       tabAnim[4]    = {0,1,0,0}; // hover/active blend per tab
    float       contentAlpha  = 0.f;
    float       openAnim      = 0.f;
    bool        menuOpen      = true;
    EspState    esp;
    CombatState combat;
    MiscState   misc;
    Texture     logoLg;
    Texture     logoSm;
    char        status[128]   = "UI ready — ESP next";
};

void UI_InitFonts();
void UI_Render(AppState& st, ID3D11Device* device);
