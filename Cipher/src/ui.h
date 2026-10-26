#pragma once
#include "texture.h"
#include "esp.h"
#include "aimbot.h"
#include <string>
#include <d3d11.h>

enum class TabId {
    Combat = 0,
    Visuals,
    Misc,
    Config,
    COUNT
};

struct MiscState {
    bool streamproof = false;
    bool vsync       = true;
    bool watermark   = true;
};

struct AppState {
    TabId       tab          = TabId::Visuals;
    float       tabAnim[4]   = { 0, 1, 0, 0 };
    float       contentAlpha = 0.f;
    float       openAnim     = 0.f;
    bool        menuOpen     = true;
    bool        autoAttach   = true;
    MiscState   misc;
    Texture     logoLg;
    Texture     logoSm;
    char        status[160]  = "Feature 2 - ESP";
    EspSystem   esp;
    Aimbot      aim;
};

void UI_InitFonts();
void UI_Render(AppState& st, ID3D11Device* device, HWND hwnd);
