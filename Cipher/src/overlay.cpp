#include "win_pre.h"
#include "overlay.h"
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

static LRESULT CALLBACK OverlayWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    // Forwarded from main after ImGui — basic default here
    switch (m) {
    case WM_DESTROY: return 0;
    case WM_NCHITTEST: {
        // Main loop / ImGui owns fine-grained hit testing when menu open
        return HTCLIENT;
    }
    }
    return DefWindowProcW(h, m, w, l);
}

bool Overlay::Create(HINSTANCE hi, int w, int h, HWND* outHwnd) {
    WNDCLASSEXW wc{ sizeof(wc) };
    wc.lpfnWndProc = OverlayWndProc;
    wc.hInstance = hi;
    wc.lpszClassName = L"CipherOverlay";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassExW(&wc);

    HWND hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        L"CipherOverlay", L"Cipher Overlay",
        WS_POPUP,
        0, 0, w, h,
        nullptr, nullptr, hi, nullptr);
    if (!hwnd) return false;

    // Fully opaque for D3D clear; transparency via color key optional —
    // we use layered + per-pixel via D3D clear alpha path: color-key black
    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);

    BOOL dark = TRUE;
    DwmSetWindowAttribute(hwnd, 20, &dark, sizeof(dark));
    MARGINS m{ -1 };
    DwmExtendFrameIntoClientArea(hwnd, &m);

    ShowWindow(hwnd, SW_SHOWNA);
    *outHwnd = hwnd;
    return true;
}

void Overlay::Destroy(HWND hwnd) {
    if (hwnd) DestroyWindow(hwnd);
}

void Overlay::SetClickThrough(HWND hwnd, bool enabled) {
    if (!hwnd) return;
    LONG_PTR ex = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    if (enabled)
        ex |= WS_EX_TRANSPARENT | WS_EX_NOACTIVATE;
    else
        ex &= ~(WS_EX_TRANSPARENT);
    SetWindowLongPtrW(hwnd, GWL_EXSTYLE, ex);
}

void Overlay::ResizeToMonitor(HWND hwnd, int* outW, int* outH) {
    HMONITOR mon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi{ sizeof(mi) };
    GetMonitorInfoW(mon, &mi);
    int w = mi.rcMonitor.right - mi.rcMonitor.left;
    int h = mi.rcMonitor.bottom - mi.rcMonitor.top;
    SetWindowPos(hwnd, HWND_TOPMOST, mi.rcMonitor.left, mi.rcMonitor.top, w, h,
                 SWP_SHOWWINDOW | SWP_NOACTIVATE);
    if (outW) *outW = w;
    if (outH) *outH = h;
}
