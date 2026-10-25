#include "ui.h"
#include "theme.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <cstdio>
#include <cstring>
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

// Simple geometric tab glyphs (no external icon font required)
static void DrawTabGlyph(ImDrawList* dl, TabId id, ImVec2 c, float s, ImU32 col) {
    switch (id) {
    case TabId::Combat: {
        // crosshair
        dl->AddCircle(c, s * 0.45f, col, 0, 1.6f);
        dl->AddLine(ImVec2(c.x - s * 0.55f, c.y), ImVec2(c.x - s * 0.2f, c.y), col, 1.5f);
        dl->AddLine(ImVec2(c.x + s * 0.2f, c.y), ImVec2(c.x + s * 0.55f, c.y), col, 1.5f);
        dl->AddLine(ImVec2(c.x, c.y - s * 0.55f), ImVec2(c.x, c.y - s * 0.2f), col, 1.5f);
        dl->AddLine(ImVec2(c.x, c.y + s * 0.2f), ImVec2(c.x, c.y + s * 0.55f), col, 1.5f);
        break;
    }
    case TabId::Visuals: {
        // eye
        dl->AddEllipse(c, ImVec2(s * 0.55f, s * 0.32f), col, 0.f, 0, 1.6f);
        dl->AddCircleFilled(c, s * 0.14f, col, 12);
        break;
    }
    case TabId::Misc: {
        // sliders / knobs
        dl->AddCircle(c, s * 0.35f, col, 0, 1.6f);
        dl->AddLine(ImVec2(c.x, c.y - s * 0.35f), ImVec2(c.x + s * 0.22f, c.y - s * 0.12f), col, 1.5f);
        break;
    }
    case TabId::Config: {
        // gear-ish hex
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
        ImVec2(p.x, p.y),
        ImVec2(p.x + ImGui::GetContentRegionAvail().x, p.y + 1.f),
        ImGui::ColorConvertFloat4ToU32(Theme::BorderSoft));
    ImGui::Dummy(ImVec2(0, 8));
}

static bool ToggleRow(const char* label, bool* v, const char* hint = nullptr) {
    ImGui::PushID(label);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);
    if (hint && ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", hint);
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 52.f);
    // Custom soft toggle
    ImVec2 p = ImGui::GetCursorScreenPos();
    float h = 22.f, w = 42.f;
    ImGui::InvisibleButton("##tog", ImVec2(w, h));
    bool clicked = ImGui::IsItemClicked();
    if (clicked) *v = !*v;
    float t = *v ? 1.f : 0.f;
    // animate toward state with frame lerp
    static float anim[64];
    int idx = ImGui::GetID("anim") % 64;
    anim[idx] = Theme::Lerp(anim[idx], t, ImGui::GetIO().DeltaTime * 12.f);
    float a = anim[idx];
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImU32 track = ImGui::ColorConvertFloat4ToU32(
        ImVec4(0.15f + 0.55f * a, 0.15f + 0.55f * a, 0.16f + 0.55f * a, 1.f));
    dl->AddRectFilled(p, ImVec2(p.x + w, p.y + h), track, h * 0.5f);
    float knobX = p.x + 3.f + a * (w - h);
    // soft glow when on
    if (a > 0.05f) {
        ImU32 glow = ImGui::ColorConvertFloat4ToU32(ImVec4(1,1,1, 0.12f * a));
        dl->AddCircleFilled(ImVec2(knobX + (h - 6) * 0.5f, p.y + h * 0.5f), (h * 0.55f), glow, 24);
    }
    dl->AddCircleFilled(ImVec2(knobX + (h - 6) * 0.5f, p.y + h * 0.5f), (h - 6) * 0.5f,
                        IM_COL32(245, 245, 248, 255), 24);
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

static void DrawSidebar(AppState& st, ImVec2 origin, float height) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 a = origin;
    ImVec2 b = ImVec2(origin.x + kSidebarW, origin.y + height);

    // sidebar bg
    dl->AddRectFilled(a, b, ImGui::ColorConvertFloat4ToU32(Theme::BgSidebar), 10.f, ImDrawFlags_RoundCornersLeft);
    // right edge line
    dl->AddLine(ImVec2(b.x - 1, a.y + 12), ImVec2(b.x - 1, b.y - 12),
                ImGui::ColorConvertFloat4ToU32(Theme::BorderSoft), 1.f);

    // brand block
    float brandH = 88.f;
    ImVec2 logoC(a.x + kSidebarW * 0.5f, a.y + 42.f);
    if (st.logoSm.ok()) {
        float sz = 48.f;
        float pulse = Theme::Pulse(1.2f, 0.85f, 1.0f);
        ImVec2 tl(logoC.x - sz * 0.5f, logoC.y - sz * 0.5f);
        // soft white halo
        dl->AddCircleFilled(logoC, sz * 0.62f, IM_COL32(255, 255, 255, (int)(18 * pulse)), 48);
        dl->AddImage((ImTextureID)st.logoSm.srv, tl, ImVec2(tl.x + sz, tl.y + sz));
    }
    ImGui::SetCursorScreenPos(ImVec2(a.x, a.y + brandH - 8));
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::Text);
    float tw = ImGui::CalcTextSize("CIPHER").x;
    ImGui::SetCursorScreenPos(ImVec2(a.x + (kSidebarW - tw) * 0.5f, a.y + brandH - 4));
    ImGui::TextUnformatted("CIPHER");
    ImGui::PopStyleColor();
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::TextDim);
    const char* sub = "external";
    float sw = ImGui::CalcTextSize(sub).x;
    ImGui::SetCursorScreenPos(ImVec2(a.x + (kSidebarW - sw) * 0.5f, a.y + brandH + 14));
    ImGui::TextUnformatted(sub);
    ImGui::PopStyleColor();

    // tabs
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
        if (ImGui::IsItemClicked()) {
            st.tab = id;
            st.contentAlpha = 0.f; // retrigger fade-in
        }

        float blend = st.tabAnim[i];
        if (hovered && !active) blend = Theme::Lerp(blend, 0.35f, 0.5f);

        // pill background
        ImU32 bg = ImGui::ColorConvertFloat4ToU32(
            ImVec4(1.f, 1.f, 1.f, 0.04f + 0.10f * blend));
        dl->AddRectFilled(ta, tb, bg, 8.f);

        // active accent bar (left glow)
        if (blend > 0.01f) {
            float gh = tabH * 0.55f * blend;
            ImVec2 gc(ta.x + 3.f, (ta.y + tb.y) * 0.5f);
            dl->AddRectFilled(
                ImVec2(ta.x, gc.y - gh * 0.5f),
                ImVec2(ta.x + 3.f, gc.y + gh * 0.5f),
                IM_COL32(255, 255, 255, (int)(220 * blend)), 2.f);
            // soft outer glow
            dl->AddRectFilled(
                ImVec2(ta.x - 4.f, gc.y - gh * 0.7f),
                ImVec2(ta.x + 6.f, gc.y + gh * 0.7f),
                IM_COL32(255, 255, 255, (int)(40 * blend)), 6.f);
        }

        // glyph + label
        ImU32 col = ImGui::ColorConvertFloat4ToU32(
            ImVec4(0.55f + 0.45f * blend, 0.55f + 0.45f * blend, 0.58f + 0.42f * blend, 1.f));
        ImVec2 gc(ta.x + 28.f, (ta.y + tb.y) * 0.5f);
        DrawTabGlyph(dl, id, gc, 14.f, col);

        ImGui::SetCursorScreenPos(ImVec2(ta.x + 48.f, ta.y + (tabH - ImGui::GetFontSize()) * 0.5f - 1.f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(
            0.55f + 0.45f * blend, 0.55f + 0.45f * blend, 0.58f + 0.42f * blend, 1.f));
        ImGui::TextUnformatted(TabName(id));
        ImGui::PopStyleColor();
        ImGui::PopID();
    }

    // footer status
    ImGui::SetCursorScreenPos(ImVec2(a.x + 16.f, b.y - 36.f));
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::TextDim);
    ImGui::TextUnformatted("INSERT toggles menu");
    ImGui::PopStyleColor();
}

static void DrawVisualsPage(AppState& st) {
    SectionHeader("ESP");
    ToggleRow("Enable ESP", &st.esp.enabled, "Master toggle for all visual overlays");
    ToggleRow("Box", &st.esp.box);
    ToggleRow("Skeleton", &st.esp.skeleton);
    ToggleRow("Name", &st.esp.name);
    ToggleRow("Distance", &st.esp.distance);
    ToggleRow("Team check", &st.esp.teamCheck, "Hide teammates");
    ToggleRow("Show downed", &st.esp.showDowned);

    SectionHeader("RANGE & STYLE");
    SliderRow("Max distance", &st.esp.maxDistance, 50.f, 500.f, "%.0f m");
    SliderRow("Box thickness", &st.esp.boxThickness, 0.5f, 4.f, "%.1f px");

    ImGui::Spacing();
    ImGui::TextUnformatted("Box style");
    ImGui::SetNextItemWidth(-1);
    ImGui::Combo("##boxstyle", &st.esp.boxStyle, "Full\0Corner\0");
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
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::TextDim);
    ImGui::TextWrapped("Combat hooks land after Visuals ESP is live.");
    ImGui::PopStyleColor();
}

static void DrawMiscPage(AppState& st) {
    SectionHeader("CLIENT");
    ToggleRow("Streamproof", &st.misc.streamproof, "Exclude menu from capture when supported");
    ToggleRow("VSync", &st.misc.vsync);
    ToggleRow("Watermark", &st.misc.watermark);
}

static void DrawConfigPage(AppState& st) {
    SectionHeader("CONFIG");
    if (ImGui::Button("Save config", ImVec2(-1, 34))) {
        std::snprintf(st.status, sizeof(st.status), "Config save — next feature pass");
    }
    if (ImGui::Button("Load config", ImVec2(-1, 34))) {
        std::snprintf(st.status, sizeof(st.status), "Config load — next feature pass");
    }
    ImGui::Spacing();
    SectionHeader("ABOUT");
    ImGui::TextUnformatted("Cipher");
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::TextDim);
    ImGui::TextWrapped("Black / white external shell. Feature 1: UI only. ESP attach is next.");
    ImGui::PopStyleColor();
}

void UI_InitFonts() {
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontDefault();
    // Optional: load a clean TTF if present next to exe later
}

void UI_Render(AppState& st, ID3D11Device*) {
    const ImGuiIO& io = ImGui::GetIO();
    const float dt = io.DeltaTime;

    // INSERT toggle
    if (ImGui::IsKeyPressed(ImGuiKey_Insert))
        st.menuOpen = !st.menuOpen;

    st.openAnim = Theme::Lerp(st.openAnim, st.menuOpen ? 1.f : 0.f, dt * 10.f);
    if (st.openAnim < 0.01f) {
        // watermark only when closed
        if (st.misc.watermark) {
            ImDrawList* dl = ImGui::GetForegroundDrawList();
            const char* wm = "CIPHER";
            ImVec2 ts = ImGui::CalcTextSize(wm);
            ImVec2 wp(io.DisplaySize.x - ts.x - 16.f, 12.f);
            float pulse = Theme::Pulse(1.4f, 0.35f, 0.75f);
            dl->AddText(wp, IM_COL32(255, 255, 255, (int)(255 * pulse)), wm);
        }
        return;
    }

    st.contentAlpha = Theme::Lerp(st.contentAlpha, 1.f, dt * 8.f);

    const float winW = 720.f;
    const float winH = 480.f;
    ImGui::SetNextWindowSize(ImVec2(winW, winH), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
                            ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowBgAlpha(0.97f * st.openAnim);

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar;

    ImGui::Begin("##cipher", nullptr, flags);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 wp = ImGui::GetWindowPos();
    ImVec2 ws = ImGui::GetWindowSize();

    // outer soft glow frame
    float g = 10.f;
    for (int i = 3; i >= 1; --i) {
        float exp = g * i;
        dl->AddRect(
            ImVec2(wp.x - exp * 0.3f, wp.y - exp * 0.3f),
            ImVec2(wp.x + ws.x + exp * 0.3f, wp.y + ws.y + exp * 0.3f),
            IM_COL32(255, 255, 255, 6 * i), 12.f + i, 0, 1.5f);
    }
    // main border
    dl->AddRect(wp, ImVec2(wp.x + ws.x, wp.y + ws.y),
                ImGui::ColorConvertFloat4ToU32(Theme::Border), 10.f, 0, 1.f);

    // sidebar
    DrawSidebar(st, wp, ws.y);

    // content panel
    ImVec2 contentPos(wp.x + kSidebarW, wp.y);
    ImVec2 contentSize(ws.x - kSidebarW, ws.y);
    dl->AddRectFilled(contentPos, ImVec2(contentPos.x + contentSize.x, contentPos.y + contentSize.y),
                      ImGui::ColorConvertFloat4ToU32(Theme::BgPanel), 10.f, ImDrawFlags_RoundCornersRight);

    // content header
    dl->AddRectFilled(contentPos,
                      ImVec2(contentPos.x + contentSize.x, contentPos.y + kHeaderH),
                      ImGui::ColorConvertFloat4ToU32(Theme::BgCard), 0.f);
    dl->AddLine(ImVec2(contentPos.x, contentPos.y + kHeaderH),
                ImVec2(contentPos.x + contentSize.x, contentPos.y + kHeaderH),
                ImGui::ColorConvertFloat4ToU32(Theme::BorderSoft), 1.f);

    // large logo watermark faded in content corner
    if (st.logoLg.ok()) {
        float sz = 160.f;
        ImVec2 lp(contentPos.x + contentSize.x - sz - 24.f,
                  contentPos.y + contentSize.y - sz - 24.f);
        dl->AddImage((ImTextureID)st.logoLg.srv, lp, ImVec2(lp.x + sz, lp.y + sz),
                     ImVec2(0,0), ImVec2(1,1), IM_COL32(255, 255, 255, 14));
    }

    ImGui::SetCursorScreenPos(ImVec2(contentPos.x + 22.f, contentPos.y + 20.f));
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::Text);
    ImGui::TextUnformatted(TabName(st.tab));
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::TextDim);
    ImGui::Text("  /  %s", st.status);
    ImGui::PopStyleColor();

    // scrollable body
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
