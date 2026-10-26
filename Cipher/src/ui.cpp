#include "win_pre.h"
#include <Windows.h>
#include "ui.h"
#include "streamproof.h"
#include "config.h"
#include "exploits.h"
#include "theme.h"
#include "imgui.h"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <algorithm>

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
        // bolt
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
    ImVec2 logoC(a.x + kSidebarW * 0.5f, a.y + 42.f);
    if (st.logoSm.ok()) {
        float sz = 48.f;
        float pulse = Theme::Pulse(1.2f, 0.85f, 1.0f);
        ImVec2 tl(logoC.x - sz * 0.5f, logoC.y - sz * 0.5f);
        dl->AddCircleFilled(logoC, sz * 0.62f, IM_COL32(255, 255, 255, (int)(18 * pulse)), 48);
        dl->AddImage((ImTextureID)st.logoSm.srv, tl, ImVec2(tl.x + sz, tl.y + sz));
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
    // 0 = RButton, 1 = LButton, 2 = XButton1, 3 = LSHIFT
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

    // INSERT is handled by RegisterHotKey in main (works while hidden).
    // Also allow ImGui key when focused:
    if (ImGui::IsKeyPressed(ImGuiKey_Insert))
        st.menuOpen = !st.menuOpen;

    // Engines always tick
    if (st.autoAttach && !st.esp.Attached())
        st.esp.Attach();
    st.esp.Tick();
    st.esp.DrawOverlay();

    st.aim.Tick(st.esp, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    st.aim.DrawFovAndRadar(st.esp, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    st.trigger.Tick(st.esp, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    st.exploits.Tick(st.esp);
    {
        static bool lastSp = false, init = false;
        if (!init || st.misc.streamproof != lastSp) {
            StreamProof::Set(hwnd, st.misc.streamproof);
            lastSp = st.misc.streamproof;
            init = true;
        }
    }

    st.openAnim = Theme::Lerp(st.openAnim, st.menuOpen ? 1.f : 0.f, dt * 14.f);
    if (!st.menuOpen || st.openAnim < 0.01f) {
        // Do not draw menu; main.cpp hides the HWND so no black window
        (void)hwnd;
        return;
    }

    st.contentAlpha = Theme::Lerp(st.contentAlpha, 1.f, dt * 8.f);

    // Fill host client area (window is sized to menu in main.cpp)
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(io.DisplaySize, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.97f * st.openAnim);

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar;

    ImGui::Begin("##cipher", nullptr, flags);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 wp = ImGui::GetWindowPos();
    ImVec2 ws = ImGui::GetWindowSize();

    for (int i = 3; i >= 1; --i) {
        float exp = 10.f * i;
        dl->AddRect(ImVec2(wp.x - exp * 0.3f, wp.y - exp * 0.3f),
                    ImVec2(wp.x + ws.x + exp * 0.3f, wp.y + ws.y + exp * 0.3f),
                    IM_COL32(255, 255, 255, 6 * i), 12.f + i, 0, 1.5f);
    }
    dl->AddRect(wp, ImVec2(wp.x + ws.x, wp.y + ws.y),
                ImGui::ColorConvertFloat4ToU32(Theme::Border), 10.f, 0, 1.f);

    DrawSidebar(st, wp, ws.y);

    ImVec2 contentPos(wp.x + kSidebarW, wp.y);
    ImVec2 contentSize(ws.x - kSidebarW, ws.y);
    dl->AddRectFilled(contentPos, ImVec2(contentPos.x + contentSize.x, contentPos.y + contentSize.y),
                      ImGui::ColorConvertFloat4ToU32(Theme::BgPanel), 10.f, ImDrawFlags_RoundCornersRight);
    dl->AddRectFilled(contentPos, ImVec2(contentPos.x + contentSize.x, contentPos.y + kHeaderH),
                      ImGui::ColorConvertFloat4ToU32(Theme::BgCard), 0.f);
    dl->AddLine(ImVec2(contentPos.x, contentPos.y + kHeaderH),
                ImVec2(contentPos.x + contentSize.x, contentPos.y + kHeaderH),
                ImGui::ColorConvertFloat4ToU32(Theme::BorderSoft), 1.f);

    if (st.logoLg.ok()) {
        float sz = 160.f;
        ImVec2 lp(contentPos.x + contentSize.x - sz - 24.f, contentPos.y + contentSize.y - sz - 24.f);
        dl->AddImage((ImTextureID)st.logoLg.srv, lp, ImVec2(lp.x + sz, lp.y + sz),
                     ImVec2(0,0), ImVec2(1,1), IM_COL32(255,255,255,14));
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
    case TabId::Visuals: DrawVisualsPage(st); break;
    case TabId::Combat:  DrawCombatPage(st);  break;
    case TabId::Misc:    DrawMiscPage(st);    break;
    case TabId::Config:  DrawConfigPage(st);  break;
    default: break;
    }
    ImGui::PopStyleVar();
    ImGui::EndChild();
    ImGui::End();
}


void UI_RenderLoader(AppState& st, ID3D11Device*, HWND hwnd) {
    const ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(io.DisplaySize, ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.f);
    ImGui::Begin("##loader", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 wp = ImGui::GetWindowPos();
    ImVec2 ws = ImGui::GetWindowSize();

    // background card + glow
    dl->AddRectFilled(wp, ImVec2(wp.x+ws.x, wp.y+ws.y), IM_COL32(12,12,14,255), 12.f);
    for (int i = 3; i >= 1; --i)
        dl->AddRect(ImVec2(wp.x-i*2, wp.y-i*2), ImVec2(wp.x+ws.x+i*2, wp.y+ws.y+i*2),
                    IM_COL32(255,255,255, 4*i), 14.f);

    // brand
    ImVec2 lc(wp.x + ws.x*0.5f, wp.y + 90.f);
    if (st.logoSm.ok()) {
        float sz = 72.f;
        dl->AddCircleFilled(lc, sz*0.72f, IM_COL32(255,255,255,20), 48);
        dl->AddImage((ImTextureID)st.logoSm.srv, ImVec2(lc.x-sz*0.5f, lc.y-sz*0.5f),
                     ImVec2(lc.x+sz*0.5f, lc.y+sz*0.5f));
    }
    const char* title = "CIPHER";
    ImVec2 ts = ImGui::CalcTextSize(title);
    ImGui::SetCursorScreenPos(ImVec2(wp.x + (ws.x-ts.x)*0.5f, wp.y + 140.f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1,1,1,1));
    ImGui::TextUnformatted(title);
    ImGui::PopStyleColor();
    ImGui::SetCursorScreenPos(ImVec2(wp.x + 40.f, wp.y + 175.f));
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::TextDim);
    ImGui::TextWrapped("%s", st.status);
    ImGui::PopStyleColor();

    // status pill
    ImGui::SetCursorScreenPos(ImVec2(wp.x + 40.f, wp.y + 220.f));
    ImGui::BeginChild("##statuscard", ImVec2(ws.x - 80.f, 72.f), true);
    if (st.fortniteOpen) {
        ImGui::TextColored(ImVec4(0.55f, 1.f, 0.65f, 1.f), "Fortnite detected");
        ImGui::TextDisabled("Ready to attach");
    } else {
        ImGui::TextColored(ImVec4(1.f, 0.45f, 0.45f, 1.f), "Fortnite not open");
        ImGui::TextDisabled("Start the game, then attach");
    }
    ImGui::EndChild();

    ImGui::SetCursorScreenPos(ImVec2(wp.x + 40.f, wp.y + 310.f));
    ImGui::PushStyleColor(ImGuiCol_Button, st.fortniteOpen ? ImVec4(1,1,1,0.12f) : ImVec4(1,1,1,0.04f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1,1,1,0.20f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1,1,1,0.28f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.f);
    if (ImGui::Button("ATTACH", ImVec2(ws.x - 80.f, 52.f))) {
        st.requestAttach = true;
        if (!st.fortniteOpen)
            snprintf(st.status, sizeof(st.status), "Fortnite not open");
        else
            snprintf(st.status, sizeof(st.status), "Attaching...");
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);

    ImGui::SetCursorScreenPos(ImVec2(wp.x + 40.f, wp.y + 380.f));
    if (ImGui::Button("REFRESH STATUS", ImVec2(ws.x - 80.f, 40.f))) {
        snprintf(st.status, sizeof(st.status), st.fortniteOpen ? "Fortnite detected" : "Fortnite not open");
    }

    ImGui::SetCursorScreenPos(ImVec2(wp.x + 40.f, wp.y + 440.f));
    ImGui::TextDisabled("After attach: overlay covers the game.");
    ImGui::SetCursorScreenPos(ImVec2(wp.x + 40.f, wp.y + 460.f));
    ImGui::TextDisabled("INSERT toggles the in-game menu.");
    ImGui::SetCursorScreenPos(ImVec2(wp.x + 40.f, wp.y + 490.f));
    ImGui::TextDisabled("Loader minimizes on success.");

    // close
    ImGui::SetCursorScreenPos(ImVec2(wp.x + ws.x - 44.f, wp.y + 12.f));
    if (ImGui::Button("X", ImVec2(32, 32)))
        PostQuitMessage(0);

    if (st.logoLg.ok()) {
        float sz = 180.f;
        dl->AddImage((ImTextureID)st.logoLg.srv,
                     ImVec2(wp.x + ws.x - sz - 10, wp.y + ws.y - sz - 10),
                     ImVec2(wp.x + ws.x - 10, wp.y + ws.y - 10),
                     ImVec2(0,0), ImVec2(1,1), IM_COL32(255,255,255,18));
    }

    ImGui::End();
    ImGui::PopStyleVar();
    (void)hwnd;
}
