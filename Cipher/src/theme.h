#pragma once
#include "imgui.h"

namespace Theme {
    // Cipher — monochrome with soft white glow accents
    inline constexpr ImVec4 BgDeep      { 0.04f, 0.04f, 0.05f, 1.00f };
    inline constexpr ImVec4 BgPanel     { 0.07f, 0.07f, 0.08f, 1.00f };
    inline constexpr ImVec4 BgSidebar   { 0.05f, 0.05f, 0.06f, 1.00f };
    inline constexpr ImVec4 BgCard      { 0.09f, 0.09f, 0.10f, 1.00f };
    inline constexpr ImVec4 Border      { 0.18f, 0.18f, 0.20f, 0.90f };
    inline constexpr ImVec4 BorderSoft  { 0.28f, 0.28f, 0.30f, 0.45f };
    inline constexpr ImVec4 Text        { 0.92f, 0.92f, 0.94f, 1.00f };
    inline constexpr ImVec4 TextDim     { 0.45f, 0.45f, 0.48f, 1.00f };
    inline constexpr ImVec4 Accent      { 1.00f, 1.00f, 1.00f, 1.00f };
    inline constexpr ImVec4 AccentSoft  { 1.00f, 1.00f, 1.00f, 0.12f };
    inline constexpr ImVec4 AccentGlow  { 1.00f, 1.00f, 1.00f, 0.25f };
    inline constexpr ImVec4 Danger      { 0.85f, 0.25f, 0.28f, 1.00f };

    void Apply();
    // Animated helpers
    float Pulse(float speed = 1.6f, float minA = 0.35f, float maxA = 1.0f);
    float SmoothStep(float t);
    float Lerp(float a, float b, float t);
}
