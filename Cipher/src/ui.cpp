#include "win_pre.h"
#include "ui.h"
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
    case TabId::Combat:  return "Combat";
    case TabId::Visuals: return "Visuals";
    case TabId::Misc:    return "Misc";
    case TabId::Config:  return "Config";
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

    SectionHeader("OFFSETS");
    ImGui::TextDisabled("GWorld / bones / camera are FILL in offsets.h");
    ImGui::TextDisabled("Re-dump after each Fortnite patch.");
    if (ImGui::Button("Copy offsets path", ImVec2(-1, 28)))
        snprintf(st.status, sizeof(st.status), "Edit src/offsets.h - all RVA in one block");
}

static void DrawCombatPage(AppState& st) {
    SectionHeader("AIM");
    ToggleRow("Aimbot", &st.combat.aimbot);
    ToggleRow("Silent", &st.combat.silent);
    ToggleRow("Visible check", &st.combat.visibleCheck);
    SliderRow("FOV", &st.combat.fov, 1.f, 45.f, "%.0f°");
    SliderRow("Smooth", &st.combat.smooth, 1.f, 20.f, "%.1f");
    ImGui::TextUnformatted("Bone");
    ImGui::SetNextItemWidth(-1);
    ImGui::Combo("##bone", &st.combat.bone, "Head\0Neck\0Chest\0");
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::TextDim);
    ImGui::TextWrapped("Combat is Feature 3 - ESP target list is ready.");
    ImGui::PopStyleColor();
}

static void DrawMiscPage(AppState& st) {
    SectionHeader("CLIENT");
    ToggleRow("Streamproof", &st.misc.streamproof);
    ToggleRow("VSync", &st.misc.vsync);
    ToggleRow("Watermark", &st.misc.watermark);
}

static void DrawConfigPage(AppState& st) {
    SectionHeader("CONFIG");
    if (ImGui::Button("Save config", ImVec2(-1, 34)))
        snprintf(st.status, sizeof(st.status), "Config IO - Feature 3");
    if (ImGui::Button("Load config", ImVec2(-1, 34)))
        snprintf(st.status, sizeof(st.status), "Config IO - Feature 3");
    SectionHeader("ABOUT");
    ImGui::TextUnformatted("Cipher  ·  ESP Feature 2");
    ImGui::TextDisabled("External read-only. Offsets must be current.");
}

void UI_InitFonts() {
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontDefault();
}

void UI_Render(AppState& st, ID3D11Device*) {
    const ImGuiIO& io = ImGui::GetIO();
    const float dt = io.DeltaTime;

    if (ImGui::IsKeyPressed(ImGuiKey_Insert))
        st.menuOpen = !st.menuOpen;

    // ESP tick every frame
    if (st.autoAttach && !st.esp.Attached())
        st.esp.Attach();
    st.esp.Tick();
    st.esp.DrawOverlay();

    st.openAnim = Theme::Lerp(st.openAnim, st.menuOpen ? 1.f : 0.f, dt * 10.f);
    if (st.openAnim < 0.01f) {
        if (st.misc.watermark) {
            ImDrawList* dl = ImGui::GetForegroundDrawList();
            const char* wm = "CIPHER";
            ImVec2 ts = ImGui::CalcTextSize(wm);
            float pulse = Theme::Pulse(1.4f, 0.35f, 0.75f);
            dl->AddText(ImVec2(io.DisplaySize.x - ts.x - 16.f, 12.f),
                        IM_COL32(255, 255, 255, (int)(255 * pulse)), wm);
        }
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
        // Custom window controls (borderless)
    {
        const float btn = 28.f;
        const float pad = 10.f;
        ImVec2 cMin(wp.x + ws.x - pad - btn * 2.f - 8.f, wp.y + pad);
        ImGui::SetCursorScreenPos(cMin);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.08f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 1, 1, 0.14f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.f);

        if (ImGui::Button("##min", ImVec2(btn, btn))) {
            ShowWindow(GetActiveWindow(), SW_MINIMIZE);
        }
        // minimize glyph
        {
            ImVec2 a = ImGui::GetItemRectMin();
            ImVec2 b = ImGui::GetItemRectMax();
            ImVec2 c((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f);
            dl->AddLine(ImVec2(c.x - 5.f, c.y), ImVec2(c.x + 5.f, c.y),
                        IM_COL32(220, 220, 225, 220), 1.5f);
        }

        ImGui::SameLine(0, 8);

        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.25f, 0.28f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.95f, 0.30f, 0.32f, 1.f));
        if (ImGui::Button("##cls", ImVec2(btn, btn))) {
            PostQuitMessage(0);
        }
        {
            ImVec2 a = ImGui::GetItemRectMin();
            ImVec2 b = ImGui::GetItemRectMax();
            ImVec2 c((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f);
            ImU32 col = IM_COL32(230, 230, 235, 230);
            dl->AddLine(ImVec2(c.x - 5.f, c.y - 5.f), ImVec2(c.x + 5.f, c.y + 5.f), col, 1.5f);
            dl->AddLine(ImVec2(c.x + 5.f, c.y - 5.f), ImVec2(c.x - 5.f, c.y + 5.f), col, 1.5f);
        }
        ImGui::PopStyleColor(2);

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);
    }
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
