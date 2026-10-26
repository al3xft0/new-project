#pragma once
#include "esp.h"
#include "aimbot.h"
#include "triggerbot.h"
#include <string>

namespace Config {
    bool Save(const EspConfig& esp, const AimConfig& aim, const TriggerConfig& trig, const std::wstring& path);
    bool Load(EspConfig& esp, AimConfig& aim, TriggerConfig& trig, const std::wstring& path);
    std::wstring DefaultPath();
}
