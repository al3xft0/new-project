#include "theme.h"
#include <cmath>
#include <chrono>

void Theme::Apply() {
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding    = 10.f;
    s.ChildRounding     = 8.f;
    s.FrameRounding     = 6.f;
    s.PopupRounding     = 8.f;
    s.ScrollbarRounding = 8.f;
    s.GrabRounding      = 4.f;
    s.TabRounding       = 6.f;
    s.WindowBorderSize  = 1.f;
    s.FrameBorderSize   = 0.f;
    s.PopupBorderSize   = 1.f;
    s.ScrollbarSize     = 10.f;
    s.WindowPadding     = ImVec2(0, 0);
    s.FramePadding      = ImVec2(12, 8);
    s.ItemSpacing       = ImVec2(10, 8);
    s.ItemInnerSpacing  = ImVec2(8, 6);
    s.IndentSpacing     = 16.f;

    ImVec4* c = s.Colors;
    c[ImGuiCol_WindowBg]             = BgDeep;
    c[ImGuiCol_ChildBg]              = BgPanel;
    c[ImGuiCol_PopupBg]              = BgCard;
    c[ImGuiCol_Border]               = Border;
    c[ImGuiCol_BorderShadow]         = ImVec4(0,0,0,0);
    c[ImGuiCol_FrameBg]              = ImVec4(0.12f, 0.12f, 0.13f, 1.f);
    c[ImGuiCol_FrameBgHovered]       = ImVec4(0.16f, 0.16f, 0.18f, 1.f);
    c[ImGuiCol_FrameBgActive]        = ImVec4(0.20f, 0.20f, 0.22f, 1.f);
    c[ImGuiCol_TitleBg]              = BgSidebar;
    c[ImGuiCol_TitleBgActive]        = BgSidebar;
    c[ImGuiCol_TitleBgCollapsed]     = BgSidebar;
    c[ImGuiCol_MenuBarBg]            = BgSidebar;
    c[ImGuiCol_ScrollbarBg]          = ImVec4(0.05f, 0.05f, 0.06f, 0.6f);
    c[ImGuiCol_ScrollbarGrab]        = ImVec4(0.30f, 0.30f, 0.32f, 0.8f);
    c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.45f, 0.45f, 0.48f, 0.9f);
    c[ImGuiCol_ScrollbarGrabActive]  = Accent;
    c[ImGuiCol_CheckMark]            = Accent;
    c[ImGuiCol_SliderGrab]           = ImVec4(0.85f, 0.85f, 0.88f, 1.f);
    c[ImGuiCol_SliderGrabActive]     = Accent;
    c[ImGuiCol_Button]               = ImVec4(0.14f, 0.14f, 0.15f, 1.f);
    c[ImGuiCol_ButtonHovered]        = ImVec4(0.22f, 0.22f, 0.24f, 1.f);
    c[ImGuiCol_ButtonActive]         = ImVec4(0.30f, 0.30f, 0.32f, 1.f);
    c[ImGuiCol_Header]               = AccentSoft;
    c[ImGuiCol_HeaderHovered]        = AccentGlow;
    c[ImGuiCol_HeaderActive]         = ImVec4(1.f, 1.f, 1.f, 0.35f);
    c[ImGuiCol_Separator]            = BorderSoft;
    c[ImGuiCol_SeparatorHovered]     = AccentGlow;
    c[ImGuiCol_SeparatorActive]      = Accent;
    c[ImGuiCol_ResizeGrip]           = ImVec4(1,1,1,0.08f);
    c[ImGuiCol_ResizeGripHovered]    = ImVec4(1,1,1,0.25f);
    c[ImGuiCol_ResizeGripActive]     = ImVec4(1,1,1,0.45f);
    // Custom left sidebar does not use ImGui tabs — skip fragile Tab* enums
    // (names differ across imgui master vs docking).
    c[ImGuiCol_Text]                 = Text;
    c[ImGuiCol_TextDisabled]         = TextDim;
    c[ImGuiCol_TextSelectedBg]       = AccentSoft;
    c[ImGuiCol_PlotLines]            = Accent;
    c[ImGuiCol_PlotHistogram]        = Accent;
    c[ImGuiCol_TableHeaderBg]        = BgCard;
    c[ImGuiCol_TableBorderStrong]    = Border;
    c[ImGuiCol_TableBorderLight]     = BorderSoft;
    c[ImGuiCol_TableRowBg]           = ImVec4(0,0,0,0);
    c[ImGuiCol_TableRowBgAlt]        = ImVec4(1,1,1,0.02f);
}

float Theme::Pulse(float speed, float minA, float maxA) {
    using clock = std::chrono::steady_clock;
    static auto t0 = clock::now();
    float t = std::chrono::duration<float>(clock::now() - t0).count();
    float s = 0.5f + 0.5f * std::sin(t * speed);
    return Lerp(minA, maxA, s);
}

float Theme::SmoothStep(float t) {
    t = t < 0.f ? 0.f : (t > 1.f ? 1.f : t);
    return t * t * (3.f - 2.f * t);
}

float Theme::Lerp(float a, float b, float t) {
    return a + (b - a) * t;
}
