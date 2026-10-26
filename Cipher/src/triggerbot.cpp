#include "win_pre.h"
#include "triggerbot.h"
#include <cmath>

void Triggerbot::Tick(EspSystem& esp, int screenW, int screenH) {
    if (!cfg.enabled || !esp.Attached()) return;
    if (cfg.requireKey && !(GetAsyncKeyState(cfg.key) & 0x8000)) return;

    const float cx = screenW * 0.5f, cy = screenH * 0.5f;
    const float hitR = 6.f; // crosshair radius px

    for (const auto& p : esp.frame.players) {
        if (cfg.visibleOnly && !p.visible) continue;
        if (p.downed) continue;
        if (p.distance > cfg.maxDist) continue;

        FVector2D sp = p.boneOk[0] ? p.bones[0] : p.headS;
        if (!p.validScreen && !p.boneOk[0]) continue;
        float dx = sp.X - cx, dy = sp.Y - cy;
        if (dx * dx + dy * dy > hitR * hitR) continue;

        ULONGLONG now = GetTickCount64();
        if (now - lastShot_ < (ULONGLONG)cfg.delayMs) return;
        lastShot_ = now;

        INPUT in[2]{};
        in[0].type = INPUT_MOUSE;
        in[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        in[1].type = INPUT_MOUSE;
        in[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
        SendInput(2, in, sizeof(INPUT));
        return;
    }
}
