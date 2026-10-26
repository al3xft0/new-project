#pragma once
#include "esp.h"
#include <Windows.h>

struct TriggerConfig {
    bool  enabled     = false;
    bool  visibleOnly = true;
    float maxDist     = 80.f;
    int   delayMs     = 30;
    int   key         = VK_RBUTTON; // only fire while this held (optional)
    bool  requireKey  = false;
};

class Triggerbot {
public:
    TriggerConfig cfg;
    void Tick(EspSystem& esp, int screenW, int screenH);
private:
    ULONGLONG lastShot_ = 0;
};
