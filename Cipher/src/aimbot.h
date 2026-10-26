#pragma once
#include "esp.h"
#include <Windows.h>

struct AimConfig {
    bool  enabled       = false;
    bool  softAim       = true;    // smooth tracking
    bool  silent        = false;
    bool  visibleCheck  = true;
    bool  drawFov       = true;
    bool  fovRadar      = true;    // triangles on FOV ring toward enemies
    float fovDeg        = 12.f;
    float softSmooth    = 8.f;     // soft aim smoothness
    float hardSmooth    = 2.f;     // when soft off / silent
    float fovColor[4]   = { 1.f, 1.f, 1.f, 0.55f };
    int   bone          = 0;
    int   aimKey        = VK_RBUTTON;
    bool  aimKeyToggle  = false;
};

class Aimbot {
public:
    AimConfig cfg;
    void Tick(EspSystem& esp, int screenW, int screenH);
    void DrawFovAndRadar(EspSystem& esp, int screenW, int screenH);
private:
    bool keyDown_ = false;
    bool toggled_ = false;
    bool KeyActive();
    int  BoneIndex() const;
};
