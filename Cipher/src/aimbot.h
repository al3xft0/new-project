// src/aimbot.h — replace whole file
#pragma once
#include "esp.h"
#include <Windows.h>

struct AimAssistConfig {
    bool enabled = false;
    bool soft = false;           // true = mouse move, false = silent write
    bool silent = true;          // always use silent angles for best cheat feel
    bool visibleCheck = true;
    bool teamCheck = true;
    bool drawFov = true;
    bool drawTracers = true;
    bool showRadar = true;
    bool aimKeyToggle = false;
    float fovDeg = 12.f;
    float smooth = 6.f;          // soft aim smooth (1 = snappy)
    int bone = 0;                // 0 head, 1 neck, 2 chest
    int aimKey = VK_RBUTTON;
    float fovColor[4] = {1.f, 1.f, 1.f, 0.6f};
    float tracerColor[4] = {1.f, 0.f, 0.f, 1.f};
    float radarColor[4] = {0.f, 1.f, 0.f, 0.8f};
};

class AimAssist {
public:
    AimAssistConfig cfg;
    void Tick(EspSystem& esp, int screenW, int screenH);
    void DrawFov(int screenW, int screenH);
    void DrawTracers(int screenW, int screenH);
    void DrawRadar(int screenW, int screenH);

private:
    bool keyDown_ = false;
    bool toggled_ = false;
    bool KeyActive();
    int BoneIndex() const;
};
