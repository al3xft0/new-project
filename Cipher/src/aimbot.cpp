#include "win_pre.h"
#include "aimbot.h"
#include "imgui.h"
#include <cmath>
#include <algorithm>

int Aimbot::BoneIndex() const {
    switch (cfg.bone) { case 1: return 1; case 2: return 2; default: return 0; }
}

bool Aimbot::KeyActive() {
    const bool down = (GetAsyncKeyState(cfg.aimKey) & 0x8000) != 0;
    if (cfg.aimKeyToggle) {
        if (down && !keyDown_) toggled_ = !toggled_;
        keyDown_ = down;
        return toggled_;
    }
    keyDown_ = down;
    return down;
}

void Aimbot::DrawFovAndRadar(EspSystem& esp, int screenW, int screenH) {
    if (!esp.Attached()) return; // no FOV floating when not on the game process
    if (!cfg.drawFov && !cfg.fovRadar) return;
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    const float cx = screenW * 0.5f, cy = screenH * 0.5f;
    const float radius = (cfg.fovDeg / 90.f) * (screenH * 0.5f);
    ImU32 col = ImGui::ColorConvertFloat4ToU32(ImVec4(cfg.fovColor[0], cfg.fovColor[1], cfg.fovColor[2], cfg.fovColor[3]));

    if (cfg.drawFov)
        dl->AddCircle(ImVec2(cx, cy), radius, col, 64, 1.6f);

    if (!cfg.fovRadar || !esp.Attached()) return;

    // Triangles on the FOV ring pointing toward each enemy (closest emphasis)
    struct Hit { float ang; float dist; };
    Hit hits[64];
    int n = 0;
    for (const auto& p : esp.frame.players) {
        if (n >= 64) break;
        if (!p.validScreen && !p.boneOk[0]) continue;
        FVector2D sp = p.boneOk[0] ? p.bones[0] : p.headS;
        float dx = sp.X - cx, dy = sp.Y - cy;
        float ang = atan2f(dy, dx);
        hits[n++] = { ang, p.distance };
    }
    // sort by distance for priority alpha
    for (int i = 0; i < n; i++) {
        float ang = hits[i].ang;
        float px = cx + cosf(ang) * radius;
        float py = cy + sinf(ang) * radius;
        // triangle pointing outward along angle
        float ox = cosf(ang), oy = sinf(ang);
        float tx = -oy, ty = ox; // tangent
        float size = 7.f + (hits[i].dist < 50.f ? 3.f : 0.f);
        ImVec2 tip(px + ox * size, py + oy * size);
        ImVec2 a(px - ox * size * 0.4f + tx * size * 0.55f, py - oy * size * 0.4f + ty * size * 0.55f);
        ImVec2 b(px - ox * size * 0.4f - tx * size * 0.55f, py - oy * size * 0.4f - ty * size * 0.55f);
        int alpha = hits[i].dist < 80.f ? 230 : 140;
        ImU32 tcol = IM_COL32((int)(cfg.fovColor[0]*255), (int)(cfg.fovColor[1]*255),
                              (int)(cfg.fovColor[2]*255), alpha);
        dl->AddTriangleFilled(tip, a, b, tcol);
    }
}

void Aimbot::Tick(EspSystem& esp, int screenW, int screenH) {
    if (!cfg.enabled) return;
    if (!KeyActive()) return;
    if (!esp.Attached()) return;

    const float cx = screenW * 0.5f, cy = screenH * 0.5f;
    const float maxR = (cfg.fovDeg / 90.f) * (screenH * 0.5f);
    const int bi = BoneIndex();

    const EspPlayer* best = nullptr;
    float bestDist = maxR;
    for (const auto& p : esp.frame.players) {
        if (cfg.visibleCheck && !p.visible) continue;
        if (p.downed) continue;
        FVector2D sp;
        if (p.boneOk[bi]) sp = p.bones[bi];
        else if (p.validScreen) sp = p.headS;
        else continue;
        float dx = sp.X - cx, dy = sp.Y - cy;
        float d = sqrtf(dx * dx + dy * dy);
        if (d < bestDist) { bestDist = d; best = &p; }
    }
    if (!best) return;

    FVector2D target = best->boneOk[bi] ? best->bones[bi] : best->headS;
    float dx = target.X - cx, dy = target.Y - cy;
    float smooth = cfg.silent ? 1.f : (cfg.softAim ? (std::max)(1.f, cfg.softSmooth) : (std::max)(1.f, cfg.hardSmooth));
    float mx = dx / smooth, my = dy / smooth;
    const float cap = cfg.softAim ? 40.f : 80.f;
    mx = std::clamp(mx, -cap, cap);
    my = std::clamp(my, -cap, cap);
    if (fabsf(mx) < 0.05f && fabsf(my) < 0.05f) return;

    INPUT in{};
    in.type = INPUT_MOUSE;
    in.mi.dwFlags = MOUSEEVENTF_MOVE;
    in.mi.dx = (LONG)mx;
    in.mi.dy = (LONG)my;
    SendInput(1, &in, sizeof(INPUT));
}
