#include "win_pre.h"
#include <Windows.h>
#include "ui.h"
#include "streamproof.h"
#include "config.h"
#include "exploits.h"
#include "theme.h"
#include "imgui.h"
#include "license_crypto.h"
#include "license_store.h"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <vector>
#include <ctime>

static constexpr float kSidebarW = 196.f;
static constexpr float kHeaderH  = 64.f;

static const char* TabName(TabId id) {
    switch (id) {
    case TabId::Combat:   return "Combat";
    case TabId::Visuals:  return "Visuals";
    case TabId::Exploits: return "Exploits";
    case TabId::Misc:     return "Misc";
    case TabId::Config:   return "Config";
    default: return "?";
    }
}

static void DrawTabGlyph(ImDrawList* dl, TabId id, ImVec2 c, float s, ImU32 col) {
    switch (id) {
    case TabId::Combat:
        dl->AddCircle(c, s * 0.45f, col, 0, 1.6f);
        dl->AddLine(ImVec2(c.x - s * 0.55f, c.y), ImVec2(c.x - s * 0.2f, c.y), col, 1.5f);
        dl->AddLine(ImVec2(c.x + s * 0.2f, c.y), ImVec2(c.x + s * 0.55f, c.y), col, 1.5f);
        dl->AddLine(ImVec2(c.x, c.y - s * 0.55f), ImVec2(c.x, c.y - s * 0.2f), col, 1.5f);
        dl->AddLine(ImVec2(c.x, c.y + s * 0.2f), ImVec2(c.x, c.y + s * 0.55f), col, 1.5f);
        break;
    case TabId::Visuals:
        dl->AddEllipse(c, ImVec2(s * 0.55f, s * 0.32f), col, 0.f, 0, 1.6f);
        dl->AddCircleFilled(c, s * 0.14f, col, 12);
        break;
    case TabId::Exploits: {
        ImVec2 pts[6] = {
            ImVec2(c.x + s*0.05f, c.y - s*0.55f),
            ImVec2(c.x - s*0.15f, c.y - s*0.05f),
            ImVec2(c.x + s*0.05f, c.y - s*0.05f),
            ImVec2(c.x - s*0.05f, c.y + s*0.55f),
            ImVec2(c.x + s*0.15f, c.y + s*0.05f),
            ImVec2(c.x - s*0.05f, c.y + s*0.05f),
        };
        dl->AddLine(pts[0], pts[1], col, 1.6f);
        dl->AddLine(pts[1], pts[2], col, 1.6f);
        dl->AddLine(pts[2], pts[3], col, 1.6f);
        dl->AddLine(pts[3], pts[4], col, 1.6f);
        dl->AddLine(pts[4], pts[5], col, 1.6f);
        dl->AddLine(pts[5], pts[1], col, 1.6f);
        break;
    }
    case TabId::Misc:
        dl->AddCircle(c, s * 0.35f, col, 0, 1.6f);
        dl->AddLine(ImVec2(c.x, c.y - s * 0.35f), ImVec2(c.x + s * 0.22f, c.y - s * 0.12f), col, 1.5f);
        break;
    case TabId::Config: {
        const float r = s * 0.42f;
        ImVec2 pts[6];
        for (int i = 0; i < 6; i++) {
            float a = (3.14159265f / 3.f) * i - 3.14159265f / 6.f;
            pts[i] = ImVec2(c.x + cosf(a) * r, c.y + sinf(a) * r);
        }
        dl->AddPolyline(pts, 6, col, ImDrawFlags_Closed, 1.6f);
        dl->AddCircle(c, s * 0.14f, col, 0, 1.4f);
        break;
    }
    default: break;
    }
}

static void SectionHeader(const char* title) {
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::TextDim);
    ImGui::TextUnformatted(title);
    ImGui::PopStyleColor();
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddRectFilled(
        ImVec2(p.x, p.y), ImVec2(p.x + ImGui::GetContentRegionAvail().x, p.y + 1.f),
        ImGui::ColorConvertFloat4ToU32(Theme::BorderSoft));
    ImGui::Dummy(ImVec2(0, 8));
}

static bool ToggleRow(const char* label, bool* v, const char* hint = nullptr) {
    ImGui::PushID(label);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);
    if (hint && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", hint);
    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x - 52.f);
    ImVec2 p = ImGui::GetCursorScreenPos();
    float h = 22.f, w = 42.f;
    ImGui::InvisibleButton("##tog", ImVec2(w, h));
    bool clicked = ImGui::IsItemClicked();
    if (clicked) *v = !*v;
    float t = *v ? 1.f : 0.f;
    static float anim[128];
    int idx = static_cast<int>(ImGui::GetID("an")) & 127;
    anim[idx] = Theme::Lerp(anim[idx], t, ImGui::GetIO().DeltaTime * 12.f);
    float a = anim[idx];
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImU32 track = ImGui::ColorConvertFloat4ToU32(ImVec4(0.15f + 0.55f * a, 0.15f + 0.55f * a, 0.16f + 0.55f * a, 1.f));
    dl->AddRectFilled(p, ImVec2(p.x + w, p.y + h), track, h * 0.5f);
    float knobX = p.x + 3.f + a * (w - h);
    if (a > 0.05f)
        dl->AddCircleFilled(ImVec2(knobX + (h - 6) * 0.5f, p.y + h * 0.5f), h * 0.55f, IM_COL32(255,255,255,(int)(30*a)), 24);
    dl->AddCircleFilled(ImVec2(knobX + (h - 6) * 0.5f, p.y + h * 0.5f), (h - 6) * 0.5f, IM_COL32(245,245,248,255), 24);
    ImGui::PopID();
    return clicked;
}

static void SliderRow(const char* label, float* v, float mn, float mx, const char* fmt = "%.1f") {
    ImGui::PushID(label);
    ImGui::TextUnformatted(label);
    ImGui::SetNextItemWidth(-1);
    ImGui::SliderFloat("##s", v, mn, mx, fmt);
    ImGui::PopID();
}

static void ColorRow(const char* label, float col[4]) {
    ImGui::PushID(label);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);
    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x - 40.f);
    ImGui::ColorEdit4("##c", col, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoLabel);
    ImGui::PopID();
}

static void DrawSidebar(AppState& st, ImVec2 origin, float height) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 a = origin;
    ImVec2 b = ImVec2(origin.x + kSidebarW, origin.y + height);
    dl->AddRectFilled(a, b, ImGui::ColorConvertFloat4ToU32(Theme::BgSidebar), 10.f, ImDrawFlags_RoundCornersLeft);
    dl->AddLine(ImVec2(b.x - 1, a.y + 12), ImVec2(b.x - 1, b.y - 12), ImGui::ColorConvertFloat4ToU32(Theme::BorderSoft), 1.f);

    float brandH = 88.f;
    ImVec2 logoC(a.x + kSidebarW * 0.5f, a.y + 40.f);
    if (st.logoSm.ok()) {
        float sz = 52.f;
        ImVec2 tl(logoC.x - sz * 0.5f, logoC.y - sz * 0.5f);
        dl->AddCircleFilled(logoC, sz * 0.70f, IM_COL32(255, 255, 255, 16), 48);
        dl->AddImage((ImTextureID)st.logoSm.srv, tl, ImVec2(tl.x + sz, tl.y + sz),
                     ImVec2(0,0), ImVec2(1,1), IM_COL32(255,255,255,255));
    }
    float tw = ImGui::CalcTextSize("CIPHER").x;
    ImGui::SetCursorScreenPos(ImVec2(a.x + (kSidebarW - tw) * 0.5f, a.y + brandH - 4));
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::Text);
    ImGui::TextUnformatted("CIPHER");
    ImGui::PopStyleColor();
    const char* sub = st.esp.Attached() ? "attached" : "external";
    float sw = ImGui::CalcTextSize(sub).x;
    ImGui::SetCursorScreenPos(ImVec2(a.x + (kSidebarW - sw) * 0.5f, a.y + brandH + 14));
    ImGui::PushStyleColor(ImGuiCol_Text, st.esp.Attached() ? ImVec4(0.85f,0.85f,0.88f,1.f) : Theme::TextDim);
    ImGui::TextUnformatted(sub);
    ImGui::PopStyleColor();

    float tabY = a.y + brandH + 40.f;
    float tabH = 44.f;
    float dt = ImGui::GetIO().DeltaTime;
    for (int i = 0; i < (int)TabId::COUNT; i++) {
        TabId id = (TabId)i;
        bool active = st.tab == id;
        st.tabAnim[i] = Theme::Lerp(st.tabAnim[i], active ? 1.f : 0.f, dt * 10.f);
        ImVec2 ta(a.x + 12.f, tabY + i * (tabH + 6.f));
        ImVec2 tb(a.x + kSidebarW - 12.f, ta.y + tabH);
        ImGui::SetCursorScreenPos(ta);
        ImGui::PushID(i);
        ImGui::InvisibleButton("tab", ImVec2(tb.x - ta.x, tabH));
        bool hovered = ImGui::IsItemHovered();
        if (ImGui::IsItemClicked()) { st.tab = id; st.contentAlpha = 0.f; }
        float blend = st.tabAnim[i];
        if (hovered && !active) blend = Theme::Lerp(blend, 0.35f, 0.5f);
        dl->AddRectFilled(ta, tb, ImGui::ColorConvertFloat4ToU32(ImVec4(1.f,1.f,1.f, 0.04f + 0.10f * blend)), 8.f);
        if (blend > 0.01f) {
            float gh = tabH * 0.55f * blend;
            ImVec2 gc(ta.x + 3.f, (ta.y + tb.y) * 0.5f);
            dl->AddRectFilled(ImVec2(ta.x, gc.y - gh * 0.5f), ImVec2(ta.x + 3.f, gc.y + gh * 0.5f),
                              IM_COL32(255,255,255,(int)(220*blend)), 2.f);
            dl->AddRectFilled(ImVec2(ta.x - 4.f, gc.y - gh * 0.7f), ImVec2(ta.x + 6.f, gc.y + gh * 0.7f),
                              IM_COL32(255,255,255,(int)(40*blend)), 6.f);
        }
        ImU32 col = ImGui::ColorConvertFloat4ToU32(ImVec4(0.55f+0.45f*blend, 0.55f+0.45f*blend, 0.58f+0.42f*blend, 1.f));
        DrawTabGlyph(dl, id, ImVec2(ta.x + 28.f, (ta.y + tb.y) * 0.5f), 14.f, col);
        ImGui::SetCursorScreenPos(ImVec2(ta.x + 48.f, ta.y + (tabH - ImGui::GetFontSize()) * 0.5f - 1.f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f+0.45f*blend, 0.55f+0.45f*blend, 0.58f+0.42f*blend, 1.f));
        ImGui::TextUnformatted(TabName(id));
        ImGui::PopStyleColor();
        ImGui::PopID();
    }
    ImGui::SetCursorScreenPos(ImVec2(a.x + 16.f, b.y - 36.f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.55f, 0.58f, 1.f));
    ImGui::TextUnformatted("INSERT toggles menu");
    ImGui::PopStyleColor();
}

static void DrawVisualsPage(AppState& st) {
    auto& e = st.esp.cfg;

    SectionHeader("CONNECTION");
    ToggleRow("Auto-attach", &st.autoAttach, "Find Fortnite process each tick");
    if (ImGui::Button(st.esp.Attached() ? "Re-attach" : "Attach", ImVec2(-1, 32))) {
        if (st.esp.Attach())
            snprintf(st.status, sizeof(st.status), "Attached pid %lu", (unsigned long)st.esp.proc.pid);
        else
            snprintf(st.status, sizeof(st.status), "Attach failed - start Fortnite");
    }
    ImGui::TextDisabled("Players: %d  |  %s", st.esp.frame.playerCount,
                        st.esp.Attached() ? "live" : "idle");

    SectionHeader("MASTER");
    ToggleRow("Enable ESP", &e.enabled);
    ToggleRow("Team check", &e.teamCheck);
    ToggleRow("Show downed", &e.showDowned);
    ToggleRow("Show self", &e.showSelf);
    ToggleRow("Visible only", &e.visibleOnly, "Needs visibility bit when offset known");
    ToggleRow("Black outline", &e.outline);

    SectionHeader("ELEMENTS");
    ToggleRow("Box", &e.box);
    ToggleRow("Skeleton", &e.skeleton);
    ToggleRow("Name", &e.name);
    ToggleRow("Distance", &e.distance);
    ToggleRow("Head dot", &e.headDot);
    ToggleRow("Health bar", &e.healthBar);
    ToggleRow("Chams", &e.chams, "Filled body silhouette");
    ToggleRow("Snaplines", &e.snaplines);
    ToggleRow("Radar", &e.radar);

    SectionHeader("BOX STYLE");
    ImGui::TextUnformatted("Style");
    ImGui::SetNextItemWidth(-1);
    int bs = (int)e.boxStyle;
    if (ImGui::Combo("##boxstyle", &bs, "Full\0Corner\0Filled\0"))
        e.boxStyle = (BoxStyle)bs;
    SliderRow("Thickness", &e.boxThickness, 0.5f, 4.f, "%.1f px");
    SliderRow("Corner length", &e.cornerLen, 0.1f, 0.5f, "%.2f");

    SectionHeader("SKELETON");
    SliderRow("Bone thickness", &e.skelThickness, 0.5f, 4.f, "%.1f px");
    SliderRow("Head dot size", &e.headDotSize, 1.f, 10.f, "%.1f");

    SectionHeader("SNAPLINES");
    ImGui::TextUnformatted("Origin");
    ImGui::SetNextItemWidth(-1);
    int sp = (int)e.snapPos;
    if (ImGui::Combo("##snapos", &sp, "Off\0Top\0Center\0Bottom\0"))
        e.snapPos = (SnapPos)sp;

    SectionHeader("RADAR");
    ImGui::TextUnformatted("Position");
    ImGui::SetNextItemWidth(-1);
    int rp = (int)e.radarPos;
    if (ImGui::Combo("##radpos", &rp, "Top Left\0Top Right\0Bottom Left\0Bottom Right\0"))
        e.radarPos = (RadarPos)rp;
    SliderRow("Radar size", &e.radarSize, 80.f, 240.f, "%.0f");
    SliderRow("Radar range", &e.radarRange, 40.f, 300.f, "%.0f m");

    SectionHeader("RANGE");
    SliderRow("Max distance", &e.maxDistance, 25.f, 600.f, "%.0f m");
    SliderRow("Text size", &e.textSize, 10.f, 22.f, "%.0f");

    SectionHeader("COLORS");
    ColorRow("Box visible", e.colors.boxVisible);
    ColorRow("Box invisible", e.colors.boxInvisible);
    ColorRow("Box downed", e.colors.boxDowned);
    ColorRow("Filled box", e.colors.filledBox);
    ColorRow("Skeleton", e.colors.skeleton);
    ColorRow("Name", e.colors.name);
    ColorRow("Distance", e.colors.distance);
    ColorRow("Snapline", e.colors.snapline);
    ColorRow("Head dot", e.colors.headDot);
    ColorRow("Radar blip", e.colors.radar);
    ColorRow("Chams", e.colors.chams);

    SectionHeader("PLAYER LIST");
    ImGui::BeginChild("##plist", ImVec2(0, 120), true);
    ImGui::TextDisabled("count %d", st.esp.frame.playerCount);
    for (int i = 0; i < (int)st.esp.frame.players.size() && i < 32; i++) {
        auto& pl = st.esp.frame.players[i];
        ImGui::Text("%02d  %5.0fm  team %d  %s", i, pl.distance, pl.team, pl.downed ? "DBNO" : "up");
    }
    ImGui::EndChild();

    SectionHeader("OFFSETS");
    ImGui::TextDisabled("GWorld / bones / camera are FILL in offsets.h");
    ImGui::TextDisabled("Re-dump after each Fortnite patch.");
    if (ImGui::Button("Copy offsets path", ImVec2(-1, 28)))
        snprintf(st.status, sizeof(st.status), "Edit src/offsets.h - all RVA in one block");
}

static void DrawCombatPage(AppState& st) {
    auto& a = st.aim.cfg;
    SectionHeader("AIM");
    ToggleRow("Aimbot", &a.enabled, "Hold aim key to track targets from ESP list");
    ToggleRow("Soft aim", &a.softAim, "Higher smooth tracking");
    ToggleRow("FOV radar", &a.fovRadar, "Triangles on FOV ring toward enemies");
    ToggleRow("Silent", &a.silent, "Hard lock / angle write when offsets exist");
    ToggleRow("Visible check", &a.visibleCheck);
    ToggleRow("Draw FOV", &a.drawFov);
    ImGui::TextUnformatted("FOV / radar color");
    ImGui::ColorEdit4("##fovcol", a.fovColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
    SliderRow("FOV", &a.fovDeg, 1.f, 45.f, "%.0f deg");
    SliderRow("Soft smooth", &a.softSmooth, 2.f, 25.f, "%.1f");
    SliderRow("Hard smooth", &a.hardSmooth, 1.f, 10.f, "%.1f");
    ImGui::TextUnformatted("Bone");
    ImGui::SetNextItemWidth(-1);
    ImGui::Combo("##bone", &a.bone, "Head\0Neck\0Chest\0");
    ImGui::TextUnformatted("Aim key");
    ImGui::SetNextItemWidth(-1);
    static int keySel = 0;
    if (a.aimKey == VK_LBUTTON) keySel = 1;
    else if (a.aimKey == VK_XBUTTON1) keySel = 2;
    else if (a.aimKey == VK_LSHIFT) keySel = 3;
    else keySel = 0;
    if (ImGui::Combo("##aimkey", &keySel, "Mouse Right\0Mouse Left\0Mouse X1\0Left Shift\0")) {
        const int keys[] = { VK_RBUTTON, VK_LBUTTON, VK_XBUTTON1, VK_LSHIFT };
        a.aimKey = keys[keySel];
    }
    ToggleRow("Toggle key", &a.aimKeyToggle, "On = press to toggle aim, Off = hold");
    ImGui::Spacing();
    ImGui::TextDisabled("Targets come from the ESP actor list.");
    ImGui::TextDisabled("Silent needs live camera write offsets for true no-move.");
    SectionHeader("TRIGGERBOT");
    ToggleRow("Enable trigger", &st.trigger.cfg.enabled);
    ToggleRow("Visible only", &st.trigger.cfg.visibleOnly);
    ToggleRow("Require aim key", &st.trigger.cfg.requireKey);
    SliderRow("Max distance", &st.trigger.cfg.maxDist, 10.f, 200.f, "%.0f m");
    ImGui::TextUnformatted("Delay ms");
    ImGui::SetNextItemWidth(-1);
    ImGui::SliderInt("##tdelay", &st.trigger.cfg.delayMs, 0, 200);
}

static void DrawExploitsPage(AppState& st) {
    auto& x = st.exploits.cfg;
    SectionHeader("WEAPON");
    ToggleRow("No recoil", &x.noRecoil, "Zero WeaponRecoilOffset + recoil counters");
    ToggleRow("No spread", &x.noSpread, "Clear bullet spread timers/counters");
    ToggleRow("Rapid fire", &x.rapidFire, "Clear local reload/equip gate bits");
    ToggleRow("No reload", &x.noReload, "Client-side only — often ignored by server");
    ToggleRow("Infinite ammo", &x.infiniteAmmo, "Usually server-authoritative");

    SectionHeader("CAMERA");
    ToggleRow("FOV changer", &x.fovChanger);
    SliderRow("FOV value", &x.fovValue, 60.f, 140.f, "%.0f");

    SectionHeader("MOVEMENT");
    ToggleRow("Speed multiplier", &x.speedHack, "Writes MaxWalkSpeed when CMC offset is valid");
    SliderRow("Speed mult", &x.speedMult, 1.f, 3.f, "%.2fx");
    ToggleRow("Spinbot", &x.spinbot, "Local experiment — not wired until rotation write is set");
    SliderRow("Spin speed", &x.spinSpeed, 1.f, 40.f, "%.0f");

    SectionHeader("NOTES");
    ImGui::TextWrapped("Offsets from SDK dump 20-10. Weapon writes target CurrentWeapon on local pawn.");
    ImGui::TextDisabled("Server may reject or rubberband movement/ammo changes.");
}

static void DrawMiscPage(AppState& st) {
    SectionHeader("CLIENT");
    ToggleRow("Streamproof", &st.misc.streamproof);
    ToggleRow("VSync", &st.misc.vsync);
    ToggleRow("Watermark", &st.misc.watermark);
    ToggleRow("Show debug", &st.misc.showDebug, "PID / player count in status");
}

static void DrawConfigPage(AppState& st) {
    SectionHeader("CONFIG");
    if (ImGui::Button("Save config", ImVec2(-1, 34))) {
        if (Config::Save(st.esp.cfg, st.aim.cfg, st.trigger.cfg, Config::DefaultPath()))
            snprintf(st.status, sizeof(st.status), "Config saved");
        else
            snprintf(st.status, sizeof(st.status), "Config save failed");
    }
    if (ImGui::Button("Load config", ImVec2(-1, 34))) {
        if (Config::Load(st.esp.cfg, st.aim.cfg, st.trigger.cfg, Config::DefaultPath()))
            snprintf(st.status, sizeof(st.status), "Config loaded");
        else
            snprintf(st.status, sizeof(st.status), "Config load failed");
    }
    SectionHeader("ABOUT");
    ImGui::TextUnformatted("Cipher");
    ImGui::TextDisabled("External stack. Offsets in src/offsets.h must match live client.");
}

void UI_InitFonts() {
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontDefault();
}

void UI_RenderOverlay(AppState& st, ID3D11Device*, HWND hwnd) {
    const ImGuiIO& io = ImGui::GetIO();
    const float dt = io.DeltaTime;

    st.openAnim = Theme::Lerp(st.openAnim, st.menuOpen ? 1.f : 0.f, dt * 18.f);
    if (!st.menuOpen || st.openAnim < 0.01f) {
        (void)hwnd;
        return;
    }

    st.contentAlpha = Theme::Lerp(st.contentAlpha, 1.f, dt * 8.f);

    const float menuW = 780.f;
    const float menuH = 520.f;
    ImVec2 menuPos((io.DisplaySize.x - menuW) * 0.5f, (io.DisplaySize.y - menuH) * 0.5f);
    ImGui::SetNextWindowPos(menuPos, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(menuW, menuH), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.0f);

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar;

    ImGui::Begin("##cipher", nullptr, flags);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 wp = ImGui::GetWindowPos();
    ImVec2 ws = ImGui::GetWindowSize();

    dl->AddRectFilled(wp, ImVec2(wp.x + ws.x, wp.y + ws.y),
                      IM_COL32(14, 14, 18, 255), 12.f);
    dl->AddRect(wp, ImVec2(wp.x + ws.x, wp.y + ws.y),
                ImGui::ColorConvertFloat4ToU32(Theme::Border), 12.f, 0, 1.2f);

    DrawSidebar(st, wp, ws.y);

    ImVec2 contentPos(wp.x + kSidebarW, wp.y);
    ImVec2 contentSize(ws.x - kSidebarW, ws.y);
    dl->AddRectFilled(contentPos, ImVec2(contentPos.x + contentSize.x, contentPos.y + contentSize.y),
                      ImGui::ColorConvertFloat4ToU32(Theme::BgPanel), 12.f, ImDrawFlags_RoundCornersRight);
    dl->AddRectFilled(contentPos, ImVec2(contentPos.x + contentSize.x, contentPos.y + kHeaderH),
                      ImGui::ColorConvertFloat4ToU32(Theme::BgCard), 0.f);
    dl->AddLine(ImVec2(contentPos.x, contentPos.y + kHeaderH),
                ImVec2(contentPos.x + contentSize.x, contentPos.y + kHeaderH),
                ImGui::ColorConvertFloat4ToU32(Theme::BorderSoft), 1.f);

    if (st.logoSm.ok()) {
        float sz = 72.f;
        ImVec2 lp(contentPos.x + contentSize.x - sz - 16.f,
                  contentPos.y + contentSize.y - sz - 16.f);
        dl->AddImage((ImTextureID)st.logoSm.srv, lp, ImVec2(lp.x + sz, lp.y + sz),
                     ImVec2(0,0), ImVec2(1,1), IM_COL32(255,255,255,28));
    }

    ImGui::SetCursorScreenPos(ImVec2(contentPos.x + 22.f, contentPos.y + 20.f));
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::Text);
    ImGui::TextUnformatted(TabName(st.tab));
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::TextDim);
    ImGui::Text("  /  %s", st.status);
    ImGui::PopStyleColor();

    ImGui::SetCursorScreenPos(ImVec2(contentPos.x + 20.f, contentPos.y + kHeaderH + 12.f));
    ImGui::BeginChild("##body", ImVec2(contentSize.x - 28.f, contentSize.y - kHeaderH - 24.f), false,
                      ImGuiWindowFlags_NoBackground);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, st.contentAlpha * st.openAnim);
    switch (st.tab) {
    case TabId::Visuals:  DrawVisualsPage(st);  break;
    case TabId::Combat:   DrawCombatPage(st);   break;
    case TabId::Exploits: DrawExploitsPage(st); break;
    case TabId::Misc:     DrawMiscPage(st);     break;
    case TabId::Config:   DrawConfigPage(st);   break;
    default: break;
    }
    ImGui::PopStyleVar();
    ImGui::EndChild();
    ImGui::End();
}

static float EaseOutCubic(float t) {
    float u = 1.f - t;
    return 1.f - u * u * u;
}

static bool UI_RenderLicenseGate(AppState& st, ImDrawList* dl, ImVec2 wp, ImVec2 ws, float mf) {
    const float dt = ImGui::GetIO().DeltaTime;

    if (st.introDone && st.logoAnim < 1.f) {
        st.logoAnim = Theme::Lerp(st.logoAnim, 1.f, dt * 2.2f);
        if (st.logoAnim > 0.995f) st.logoAnim = 1.f;
    }
    float a = EaseOutCubic(st.logoAnim);

    ImVec2 center(wp.x + ws.x * 0.5f, wp.y + ws.y * 0.42f);
    ImVec2 top(wp.x + ws.x * 0.5f, wp.y + 100.f);
    ImVec2 logoPos(
        center.x + (top.x - center.x) * a,
        center.y + (top.y - center.y) * a);
    float sz = 120.f + (64.f - 120.f) * a;

    for (int i = 4; i >= 1; --i) {
        float g = sz * (0.55f + i * 0.12f);
        int alpha = (int)((14.f * mf) / (float)i);
        dl->AddCircleFilled(logoPos, g, IM_COL32(230, 235, 255, alpha), 48);
    }
    if (st.logoSm.ok()) {
        ImVec2 tl(logoPos.x - sz * 0.5f, logoPos.y - sz * 0.5f);
        dl->AddImage((ImTextureID)st.logoSm.srv, tl, ImVec2(tl.x + sz, tl.y + sz),
                     ImVec2(0,0), ImVec2(1,1), IM_COL32(255,255,255,(int)(255*mf)));
    }

    if (a > 0.6f) {
        float ta = (a - 0.6f) / 0.4f;
        const char* title = "CIPHER";
        ImVec2 ts = ImGui::CalcTextSize(title);
        dl->AddText(ImVec2(logoPos.x - ts.x * 0.5f, logoPos.y + sz * 0.55f),
                    IM_COL32(255,255,255,(int)(220 * ta * mf)), title);
    }

    if (a < 0.85f) return st.licenseOk;

    float fieldW = 320.f;
    float btnW = 48.f;
    float gap = 10.f;
    float total = fieldW + gap + btnW;
    float rowX = wp.x + (ws.x - total) * 0.5f;
    float rowY = wp.y + ws.y * 0.62f;

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.f);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, mf);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.09f, 0.09f, 0.10f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.12f, 0.12f, 0.13f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.f, 1.f, 1.f, 0.12f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.f, 1.f, 1.f, 0.22f));

    ImGui::SetCursorScreenPos(ImVec2(rowX, rowY));
    ImGui::PushItemWidth(fieldW);
    bool submit = ImGui::InputTextWithHint("##lic", "Enter license key", st.licenseBuf, sizeof(st.licenseBuf),
                                           ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::PopItemWidth();
    ImGui::SameLine(0, gap);
    if (ImGui::Button(">", ImVec2(btnW, 0))) submit = true;

    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(2);

    if (submit && st.licenseBuf[0]) {
        st.licenseTried = true;
        std::vector<uint32_t> revoked;
        st.licenseStatus = License::ValidateKey(st.licenseBuf, st.licenseInfo, revoked);
        st.licenseOk = (st.licenseStatus == License::Status::Valid);
        if (st.licenseOk) {
            snprintf(st.status, sizeof(st.status), "License valid · %u days left",
                     st.licenseInfo.durationDays);
        }
    }

    if (st.licenseTried) {
        float col[4];
        License::StatusColor(st.licenseStatus, col);
        const char* label = License::StatusString(st.licenseStatus);
        ImVec2 ls = ImGui::CalcTextSize(label);
        dl->AddText(ImVec2(wp.x + (ws.x - ls.x) * 0.5f, rowY + 42.f),
                    IM_COL32((int)(col[0]*255), (int)(col[1]*255), (int)(col[2]*255), (int)(255*mf)),
                    label);
        if (st.licenseStatus == License::Status::Valid) {
            char line[96];
            time_t exp = (time_t)st.licenseInfo.expiresUnix;
            struct tm tm {};
            localtime_s(&tm, &exp);
            char dbuf[32];
            strftime(dbuf, sizeof(dbuf), "%Y-%m-%d", &tm);
            snprintf(line, sizeof(line), "Active %u days · expires %s",
                     st.licenseInfo.durationDays, dbuf);
            ImVec2 ts = ImGui::CalcTextSize(line);
            dl->AddText(ImVec2(wp.x + (ws.x - ts.x) * 0.5f, rowY + 64.f),
                        IM_COL32(140, 140, 145, (int)(255*mf)), line);
        }
    }

    return st.licenseOk;
}

void UI_RenderLoader(AppState& st, ID3D11Device*, HWND hwnd) {
    const ImGuiIO& io = ImGui::GetIO();
    const float dt = io.DeltaTime > 0.f ? io.DeltaTime : 0.016f;
    st.introT += dt;

    if (ImGui::IsKeyPressed(ImGuiKey_Escape))
        PostQuitMessage(0);

    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(io.DisplaySize, ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 16.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("##loader", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 wp = ImGui::GetWindowPos();
    ImVec2 ws = ImGui::GetWindowSize();

    // Paint window bg IM_COL32(10,10,12) full including top (no gray bar)
    dl->AddRectFilled(wp, ImVec2(wp.x + ws.x, wp.y + ws.y), IM_COL32(10, 10, 12, 255), 16.f);

    for (int i = 5; i >= 1; --i) {
        float e = (float)i * 18.f;
        dl->AddRect(ImVec2(wp.x - e * 0.15f, wp.y - e * 0.15f),
                    ImVec2(wp.x + ws.x + e * 0.15f, wp.y + ws.y + e * 0.15f),
                    IM_COL32(255, 255, 255, 3 * i), 18.f + i, 0, 1.2f);
    }

    const float introHold = 0.85f;
    const float introFade = 0.65f;
    const float introEnd  = introHold + introFade;

    if (!st.introDone) {
        float t = st.introT;
        float alpha = 1.f;
        float scale = 1.f;
        if (t < 0.25f) {
            float u = t / 0.25f;
            scale = 0.65f + 0.45f * u;
            alpha = u;
        } else if (t < introHold) {
            scale = 1.1f + 0.04f * sinf(t * 6.f);
            alpha = 1.f;
        } else if (t < introEnd) {
            float u = (t - introHold) / introFade;
            alpha = 1.f - u;
            scale = 1.1f + 0.25f * u;
        } else {
            st.introDone = true;
            st.menuFade = 0.f;
            alpha = 0.f;
        }

        ImVec2 c(wp.x + ws.x * 0.5f, wp.y + ws.y * 0.5f);
        float sz = 120.f * scale;
        for (int i = 5; i >= 1; --i) {
            float g = sz * (0.50f + i * 0.14f);
            int a = (int)((16.f * alpha) / (float)i);
            dl->AddCircleFilled(c, g, IM_COL32(230, 235, 255, a), 64);
        }
        if (st.logoLg.ok()) {
            int a = (int)(255 * alpha);
            dl->AddImage((ImTextureID)st.logoLg.srv,
                         ImVec2(c.x - sz * 0.5f, c.y - sz * 0.5f),
                         ImVec2(c.x + sz * 0.5f, c.y + sz * 0.5f),
                         ImVec2(0,0), ImVec2(1,1), IM_COL32(255,255,255,a));
        } else if (st.logoSm.ok()) {
            int a = (int)(255 * alpha);
            dl->AddImage((ImTextureID)st.logoSm.srv,
                         ImVec2(c.x - sz * 0.5f, c.y - sz * 0.5f),
                         ImVec2(c.x + sz * 0.5f, c.y + sz * 0.5f),
                         ImVec2(0,0), ImVec2(1,1), IM_COL32(255,255,255,a));
        }
        if (t > 0.3f && t < introEnd) {
            const char* title = "CIPHER";
            ImVec2 ts = ImGui::CalcTextSize(title);
            int a = (int)(220 * alpha);
            dl->AddText(ImVec2(c.x - ts.x * 0.5f, c.y + sz * 0.55f), IM_COL32(255,255,255,a), title);
        }

        ImGui::End();
        ImGui::PopStyleVar(2);
        (void)hwnd;
        return;
    }

    st.menuFade = Theme::Lerp(st.menuFade, 1.f, dt * 4.5f);
    float mf = st.menuFade;

    {
        ImVec2 cmin(wp.x + ws.x - 48.f, wp.y + 8.f);
        ImGui::SetCursorScreenPos(cmin);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f,0.2f,0.25f,0.55f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.f,0.25f,0.3f,0.85f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.f);
        if (ImGui::Button("##cls", ImVec2(36, 28)))
            PostQuitMessage(0);
        ImVec2 a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
        ImVec2 c((a.x+b.x)*0.5f, (a.y+b.y)*0.5f);
        dl->AddLine(ImVec2(c.x-5, c.y-5), ImVec2(c.x+5, c.y+5), IM_COL32(230,230,235,220), 1.6f);
        dl->AddLine(ImVec2(c.x+5, c.y-5), ImVec2(c.x-5, c.y+5), IM_COL32(230,230,235,220), 1.6f);
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);
    }

    // License gate check: if not verified, render gate and return
    if (!st.licenseOk) {
        UI_RenderLicenseGate(st, dl, wp, ws, mf);
        ImGui::End();
        ImGui::PopStyleVar(2);
        (void)hwnd;
        return;
    }

    // ---------- MAIN ATTACH UI ----------
    ImVec2 lc(wp.x + ws.x * 0.5f, wp.y + 88.f);
    float pulse = 0.5f + 0.5f * sinf(st.introT * 2.2f);
    dl->AddCircleFilled(lc, 44.f + pulse * 3.f, IM_COL32(230, 235, 255, (int)(12 * mf)), 48);
    dl->AddCircle(lc, 50.f + pulse * 2.f, IM_COL32(255, 255, 255, (int)(22 * mf)), 48, 1.2f);
    if (st.logoSm.ok()) {
        float sz = 64.f;
        dl->AddImage((ImTextureID)st.logoSm.srv,
                     ImVec2(lc.x - sz*0.5f, lc.y - sz*0.5f),
                     ImVec2(lc.x + sz*0.5f, lc.y + sz*0.5f),
                     ImVec2(0,0), ImVec2(1,1), IM_COL32(255,255,255,(int)(255*mf)));
    }
    {
        const char* title = "CIPHER";
        ImVec2 ts = ImGui::CalcTextSize(title);
        dl->AddText(ImVec2(lc.x - ts.x*0.5f, lc.y + 42.f), IM_COL32(255,255,255,(int)(245*mf)), title);
        const char* sub = "external";
        ImVec2 ss = ImGui::CalcTextSize(sub);
        dl->AddText(ImVec2(lc.x - ss.x*0.5f, lc.y + 60.f), IM_COL32(160,160,170,(int)(200*mf)), sub);
    }

    float cardX = wp.x + 36.f;
    float cardW = ws.x - 72.f;
    float cardY = wp.y + 175.f;
    ImVec2 c0(cardX, cardY), c1(cardX + cardW, cardY + 78.f);
    dl->AddRectFilled(c0, c1, IM_COL32(255,255,255, (int)(12 * mf)), 12.f);
    dl->AddRect(c0, c1, IM_COL32(255,255,255, (int)(22 * mf)), 12.f, 0, 1.f);

    if (!st.attaching) {
        bool isIdle =
            std::strncmp(st.status, "Fortnite", 8) == 0 ||
            std::strncmp(st.status, "Initializing", 12) == 0 ||
            st.status[0] == 0;
        if (isIdle) {
            if (st.fortniteOpen)
                snprintf(st.status, sizeof(st.status), "Fortnite detected — ready");
            else
                snprintf(st.status, sizeof(st.status), "Fortnite not open — launch game first");
        }
    }

    ImGui::SetCursorScreenPos(ImVec2(cardX + 18.f, cardY + 16.f));
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, mf);
    if (st.attaching) {
        ImGui::TextColored(ImVec4(1.f, 0.85f, 0.4f, 1.f), "Attaching...");
        ImGui::TextDisabled("Opening process + overlay");
    } else if (st.fortniteOpen) {
        ImVec2 d(cardX + 18.f, cardY + 22.f);
        dl->AddCircleFilled(ImVec2(d.x + 4, d.y + 4), 5.f + pulse * 2.f, IM_COL32(80, 255, 140, (int)(40 + 30*pulse)), 16);
        dl->AddCircleFilled(ImVec2(d.x + 4, d.y + 4), 4.f, IM_COL32(90, 255, 150, 255), 16);
        ImGui::SetCursorScreenPos(ImVec2(cardX + 36.f, cardY + 14.f));
        ImGui::TextColored(ImVec4(0.55f, 1.f, 0.65f, 1.f), "Fortnite detected");
        ImGui::SetCursorScreenPos(ImVec2(cardX + 36.f, cardY + 36.f));
        ImGui::TextDisabled("Ready to attach");
    } else {
        ImVec2 d(cardX + 18.f, cardY + 22.f);
        dl->AddCircleFilled(ImVec2(d.x + 4, d.y + 4), 4.f, IM_COL32(255, 90, 90, 255), 16);
        ImGui::SetCursorScreenPos(ImVec2(cardX + 36.f, cardY + 14.f));
        ImGui::TextColored(ImVec4(1.f, 0.45f, 0.45f, 1.f), "Fortnite not open");
        ImGui::SetCursorScreenPos(ImVec2(cardX + 36.f, cardY + 36.f));
        ImGui::TextDisabled("Launch the game, then attach");
    }
    ImGui::PopStyleVar();

    ImGui::SetCursorScreenPos(ImVec2(cardX, cardY + 80.f));
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, mf);
    ImGui::PushTextWrapPos(cardX + cardW);
    ImGui::TextColored(ImVec4(0.75f, 0.75f, 0.80f, 1.f), "%s", st.status);
    ImGui::PopTextWrapPos();
    ImGui::PopStyleVar();

    float btnY = cardY + 100.f;
    ImGui::SetCursorScreenPos(ImVec2(cardX, btnY));
    bool canAttach = !st.attaching;
    ImVec4 btnCol = canAttach ? ImVec4(1,1,1,0.14f) : ImVec4(1,1,1,0.05f);
    ImVec4 btnHov = canAttach ? ImVec4(1,1,1,0.24f) : btnCol;
    ImGui::PushStyleColor(ImGuiCol_Button, btnCol);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, btnHov);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1,1,1,0.32f));
    ImGui::PushStyleColor(ImGuiCol_Text, canAttach ? ImVec4(1,1,1,1) : ImVec4(1,1,1,0.35f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.f);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, mf);
    if (ImGui::Button(st.attaching ? "ATTACHING..." : "ATTACH", ImVec2(cardW, 54.f))) {
        if (canAttach) {
            st.attaching = true;
            st.requestAttach = true;
            snprintf(st.status, sizeof(st.status), "Scanning for Shipping.exe...");
        }
    }
    if (canAttach && ImGui::IsItemHovered()) {
        ImVec2 a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
        dl->AddRect(ImVec2(a.x-2, a.y-2), ImVec2(b.x+2, b.y+2), IM_COL32(255,255,255,50), 14.f, 0, 1.5f);
    }
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(4);

    ImGui::SetCursorScreenPos(ImVec2(cardX, btnY + 66.f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1,1,1,0.06f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1,1,1,0.12f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.f);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, mf);
    if (ImGui::Button("REFRESH", ImVec2(cardW * 0.48f, 42.f))) {
        st.attaching = false;
        snprintf(st.status, sizeof(st.status), st.fortniteOpen ? "Fortnite detected — ready" : "Fortnite not open");
    }
    ImGui::SameLine(0, cardW * 0.04f);
    if (ImGui::Button("EXIT", ImVec2(cardW * 0.48f, 42.f)))
        PostQuitMessage(0);
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);

    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, mf * 0.85f);
    ImGui::SetCursorScreenPos(ImVec2(cardX, btnY + 130.f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f,0.55f,0.58f,1.f));
    ImGui::TextWrapped("Overlay covers the game after attach. INSERT toggles the menu.");
    ImGui::SetCursorScreenPos(ImVec2(cardX, btnY + 168.f));
    ImGui::TextDisabled("Run as Administrator if attach fails.");
    ImGui::SetCursorScreenPos(ImVec2(cardX, btnY + 190.f));
    ImGui::TextDisabled("ESC or EXIT to quit.");
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();

    if (st.logoLg.ok()) {
        float sz = 160.f;
        dl->AddImage((ImTextureID)st.logoLg.srv,
                     ImVec2(wp.x + ws.x - sz - 8, wp.y + ws.y - sz - 8),
                     ImVec2(wp.x + ws.x - 8, wp.y + ws.y - 8),
                     ImVec2(0,0), ImVec2(1,1), IM_COL32(255,255,255, (int)(16 * mf)));
    }

    ImGui::End();
    ImGui::PopStyleVar(2);
    (void)hwnd;
}
