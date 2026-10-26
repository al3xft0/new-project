#include "win_pre.h"
#include "aimbot.h"
#include "imgui.h"
#include <cmath>
#include <algorithm>

int Aimbot::BoneIndex() const {
    switch (cfg.bone) {
    case 1: return 1; // neck slot in EspPlayer::bones
    case 2: return 2; // chest
    default: return 0; // head
    }
}

bool Aimbot::KeyActive() {
    const bool down = (GetAsyncKeyState(cfg.aimKey) & 0x8000) != 0;
    if (cfg.aimKeyToggle) {
        if (down && !keyDown_)
            toggled_ = !toggled_;
        keyDown_ = down;
        return toggled_;
    }
    keyDown_ = down;
    return down;
}

void Aimbot::DrawFov(int screenW, int screenH) {
    if (!cfg.enabled || !cfg.drawFov) return;
    // Approximate FOV circle in pixels (rough, not true angular FOV)
    float radius = (cfg.fovDeg / 90.f) * (screenH * 0.5f);
    ImVec2 c(screenW * 0.5f, screenH * 0.5f);
    ImGui::GetBackgroundDrawList()->AddCircle(c, radius, IM_COL32(255, 255, 255, 50), 64, 1.5f);
}

void Aimbot::Tick(EspSystem& esp, int screenW, int screenH) {
    if (!cfg.enabled) return;
    if (!KeyActive()) return;
    if (!esp.Attached()) return;

    const float cx = screenW * 0.5f;
    const float cy = screenH * 0.5f;
    const float maxR = (cfg.fovDeg / 90.f) * (screenH * 0.5f);

    const EspPlayer* best = nullptr;
    float bestDist = maxR;

    const int bi = BoneIndex();
    for (const auto& p : esp.frame.players) {
        if (cfg.visibleCheck && !p.visible) continue;
        if (p.downed) continue;

        FVector2D sp;
        if (p.boneOk[bi])
            sp = p.bones[bi];
        else if (p.validScreen)
            sp = p.headS;
        else
            continue;

        float dx = sp.X - cx;
        float dy = sp.Y - cy;
        float d = std::sqrt(dx * dx + dy * dy);
        if (d < bestDist) {
            bestDist = d;
            best = &p;
        }
    }

    if (!best) return;

    FVector2D target;
    if (best->boneOk[bi])
        target = best->bones[bi];
    else
        target = best->headS;

    float dx = target.X - cx;
    float dy = target.Y - cy;

    // Silent path: attempt camera/control rotation write when you wire offsets.
    // External silent without a live write target degrades to an unsmoothed lock.
    float smooth = cfg.silent ? 1.f : (std::max)(1.f, cfg.smooth);
    float mx = dx / smooth;
    float my = dy / smooth;

    // Clamp single-frame move
    const float cap = 80.f;
    mx = std::clamp(mx, -cap, cap);
    my = std::clamp(my, -cap, cap);

    if (std::fabs(mx) < 0.1f && std::fabs(my) < 0.1f)
        return;

    INPUT in{};
    in.type = INPUT_MOUSE;
    in.mi.dwFlags = MOUSEEVENTF_MOVE;
    in.mi.dx = (LONG)mx;
    in.mi.dy = (LONG)my;
    SendInput(1, &in, sizeof(INPUT));
}
