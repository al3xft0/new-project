#pragma once
#include "esp.h"
#include <Windows.h>

struct AimConfig {
    bool  enabled       = false;
    bool  silent        = false;   // memory angle write when offsets live; else hard lock
    bool  visibleCheck  = true;
    bool  teamCheck     = true;
    bool  drawFov       = true;
    float fovDeg        = 12.f;
    float smooth        = 6.f;     // 1 = snappy, higher = slower
    int   bone          = 0;       // 0 head, 1 neck, 2 chest
    int   aimKey        = VK_RBUTTON; // hold to aim
    bool  aimKeyToggle  = false;   // if true, aimKey is toggle not hold
};

class Aimbot {
public:
    AimConfig cfg;
    void Tick(EspSystem& esp, int screenW, int screenH);
    void DrawFov(int screenW, int screenH);

private:
    bool keyDown_ = false;
    bool toggled_ = false;
    bool KeyActive();
    int  BoneIndex() const;
};
