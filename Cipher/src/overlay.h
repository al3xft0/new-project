#pragma once
#include <Windows.h>
#include <d3d11.h>

struct OverlayState {
    HWND  hwnd = nullptr;
    bool  menuOpen = true;
    bool  clickThrough = false;
    int   width = 0;
    int   height = 0;
};

namespace Overlay {
    bool Create(HINSTANCE hi, int w, int h, HWND* outHwnd);
    void Destroy(HWND hwnd);
    void SetClickThrough(HWND hwnd, bool enabled);
    void ResizeToMonitor(HWND hwnd, int* outW, int* outH);
}
