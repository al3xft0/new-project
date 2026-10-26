#pragma once
#include "texture.h"
#include "esp.h"
#include "aimbot.h"
#include "triggerbot.h"
#include "exploits.h"
#include "license_crypto.h"
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
    bool        vsync       = false;
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
    bool        attaching    = false;
    // intro splash
    bool        introDone    = false;
    float       introT       = 0.f;   // seconds elapsed
    float       menuFade     = 0.f;   // 0..1 after intro

    // license gate
    bool                  licenseOk     = false;
    bool                  licenseTried  = false;
    float                 logoAnim      = 0.f;
    char                  licenseBuf[96] = {};
    License::Status       licenseStatus = License::Status::BadFormat;
    License::KeyInfo      licenseInfo{};

    MiscState   misc;
    Texture     logoLg, logoSm;
    char        status[160]  = "Initializing";
    EspSystem   esp;
    Aimbot      aim;
    Triggerbot  trigger;
    Exploits    exploits;
};

void UI_InitFonts();
void UI_RenderLoader(AppState& st, ID3D11Device* device, HWND hwnd);
void UI_RenderOverlay(AppState& st, ID3D11Device* device, HWND hwnd);
