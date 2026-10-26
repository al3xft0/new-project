#pragma once
#include "texture.h"
#include "esp.h"
#include "aimbot.h"
#include "triggerbot.h"
#include "exploits.h"
#include <d3d11.h>
#include <Windows.h>

enum class AppMode { Loader, Overlay };

enum class TabId {
    Combat = 0,
    Visuals,
    Exploits,
    Misc,
    Config,
    COUNT
};

struct MiscState {
    bool streamproof = false;
    bool vsync       = true;
    bool watermark   = true;
    bool showDebug   = true;
};

struct AppState {
    AppMode     mode         = AppMode::Loader;
    TabId       tab          = TabId::Visuals;
    float       tabAnim[5]   = {0,1,0,0,0};
    float       contentAlpha = 0.f;
    float       openAnim     = 1.f;
    bool        menuOpen     = true;
    bool        autoAttach   = true;
    bool        fortniteOpen = false;
    bool        requestAttach = false;
    MiscState   misc;
    Texture     logoLg, logoSm;
    char        status[160]  = "Waiting for Fortnite";
    EspSystem   esp;
    Aimbot      aim;
    Triggerbot  trigger;
    Exploits    exploits;
};

void UI_InitFonts();
void UI_RenderLoader(AppState& st, ID3D11Device* device, HWND hwnd);
void UI_RenderOverlay(AppState& st, ID3D11Device* device, HWND hwnd);
