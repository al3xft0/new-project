#include "win_pre.h"
#include "ui.h"
#include "theme.h"
#include "texture.h"
#include "embed_logo_sm.h"
#include "embed_logo_lg.h"
#include "overlay.h"
#include "streamproof.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include <cstdio>
#include <dwmapi.h>
#include <TlHelp32.h>

#pragma comment(lib, "dwmapi.lib")

static ID3D11Device*           g_dev = nullptr;
static ID3D11DeviceContext*    g_ctx = nullptr;
static IDXGISwapChain*         g_swap = nullptr;
static ID3D11RenderTargetView* g_rtv = nullptr;
static HWND                    g_loader = nullptr;
static HWND                    g_overlay = nullptr;
static bool                    g_running = true;
static bool                    g_attachedMode = false;
static const int kLoaderW = 520, kLoaderH = 640;
static const int kHotkeyId = 1;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

static void CreateRT() {
    ID3D11Texture2D* bb = nullptr;
    g_swap->GetBuffer(0, IID_PPV_ARGS(&bb));
    if (bb) { g_dev->CreateRenderTargetView(bb, nullptr, &g_rtv); bb->Release(); }
}
static void CleanupRT() { if (g_rtv) { g_rtv->Release(); g_rtv = nullptr; } }

static bool CreateDevice(HWND hwnd) {
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate = { 60, 1 };
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    D3D_FEATURE_LEVEL fl, levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        levels, 2, D3D11_SDK_VERSION, &sd, &g_swap, &g_dev, &fl, &g_ctx);
    if (FAILED(hr))
        hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0,
            levels, 2, D3D11_SDK_VERSION, &sd, &g_swap, &g_dev, &fl, &g_ctx);
    if (FAILED(hr)) return false;
    CreateRT();
    return true;
}
static void CleanupDevice() {
    CleanupRT();
    if (g_swap) { g_swap->Release(); g_swap = nullptr; }
    if (g_ctx) { g_ctx->Release(); g_ctx = nullptr; }
    if (g_dev) { g_dev->Release(); g_dev = nullptr; }
}

static bool RecreateSwapchainFor(HWND hwnd) {
    CleanupRT();
    if (g_swap) { g_swap->Release(); g_swap = nullptr; }
    if (g_ctx) { g_ctx->Release(); g_ctx = nullptr; }
    if (g_dev) { g_dev->Release(); g_dev = nullptr; }
    return CreateDevice(hwnd);
}

static LRESULT WINAPI LoaderProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (ImGui_ImplWin32_WndProcHandler(h, m, w, l)) return true;
    switch (m) {
    case WM_CLOSE:
        DestroyWindow(h);
        return 0;
    case WM_DESTROY:
        if (!g_attachedMode) PostQuitMessage(0);
        else if (!g_overlay || !IsWindow(g_overlay)) PostQuitMessage(0);
        return 0;
    case WM_NCHITTEST: {
        LRESULT hit = DefWindowProcW(h, m, w, l);
        if (hit != HTCLIENT) return hit;
        POINT pt{ (SHORT)LOWORD(l), (SHORT)HIWORD(l) };
        ScreenToClient(h, &pt);
        RECT rc; GetClientRect(h, &rc);
        // close button zone
        if (pt.y < 40 && pt.x > rc.right - 56) return HTCLIENT;
        if (pt.y < 40) return HTCAPTION; // drag bar
        return HTCLIENT;
    }
    case WM_SIZE:
        if (g_dev && w != SIZE_MINIMIZED && !g_attachedMode) {
            CleanupRT();
            g_swap->ResizeBuffers(0, LOWORD(l), HIWORD(l), DXGI_FORMAT_UNKNOWN, 0);
            CreateRT();
        }
        return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

static LRESULT WINAPI OverlayProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (ImGui_ImplWin32_WndProcHandler(h, m, w, l)) return true;
    switch (m) {
    case WM_HOTKEY:
        if ((int)w == kHotkeyId)
            PostMessageW(h, WM_APP + 7, 0, 0);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_SIZE:
        if (g_dev && w != SIZE_MINIMIZED) {
            CleanupRT();
            g_swap->ResizeBuffers(0, LOWORD(l), HIWORD(l), DXGI_FORMAT_UNKNOWN, 0);
            CreateRT();
        }
        return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

static bool IsFortniteRunning() {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return false;
    PROCESSENTRY32W pe{ sizeof(pe) };
    bool found = false;
    if (Process32FirstW(snap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, L"FortniteClient-Win64-Shipping.exe") == 0 ||
                _wcsicmp(pe.szExeFile, L"FortniteClient-Win64-Shipping_EAC_EOS.exe") == 0) {
                found = true; break;
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return found;
}

static bool EnterOverlayMode(HINSTANCE hi, AppState& st) {
    int w = GetSystemMetrics(SM_CXSCREEN);
    int h = GetSystemMetrics(SM_CYSCREEN);

    WNDCLASSEXW wc{ sizeof(wc) };
    wc.lpfnWndProc = OverlayProc;
    wc.hInstance = hi;
    wc.lpszClassName = L"CipherOverlay";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassExW(&wc);

    g_overlay = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
        L"CipherOverlay", L"Cipher",
        WS_POPUP, 0, 0, w, h,
        nullptr, nullptr, hi, nullptr);
    if (!g_overlay) return false;

    // Color-key pure black → transparent where we clear black; draw UI/ESP in non-black
    SetLayeredWindowAttributes(g_overlay, RGB(0, 0, 0), 0, LWA_COLORKEY);
    MARGINS margins{ -1 };
    DwmExtendFrameIntoClientArea(g_overlay, &margins);

    // Hide loader (not minimize) so it does not leave a dead zombie window
    ShowWindow(g_loader, SW_HIDE);

    // ImGui DX11 still holds the OLD device — must tear it down BEFORE we destroy the device
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();

    if (!RecreateSwapchainFor(g_overlay)) {
        DestroyWindow(g_overlay);
        g_overlay = nullptr;
        if (RecreateSwapchainFor(g_loader)) {
            ImGui_ImplWin32_Init(g_loader);
            ImGui_ImplDX11_Init(g_dev, g_ctx);
        }
        ShowWindow(g_loader, SW_RESTORE);
        SetForegroundWindow(g_loader);
        return false;
    }

    ImGui_ImplWin32_Init(g_overlay);
    ImGui_ImplDX11_Init(g_dev, g_ctx);
    RegisterHotKey(g_overlay, kHotkeyId, 0, VK_INSERT);

    g_attachedMode = true;
    st.mode = AppMode::Overlay;
    st.menuOpen = true;
    st.attaching = false;
    Overlay::SetClickThrough(g_overlay, false);
    StreamProof::Set(g_overlay, st.misc.streamproof);

    ShowWindow(g_overlay, SW_SHOW);
    SetForegroundWindow(g_overlay);
    snprintf(st.status, sizeof(st.status), "Overlay live — INSERT toggles menu");
    return true;
}

int WINAPI wWinMain(HINSTANCE hi, HINSTANCE, LPWSTR, int) {
    WNDCLASSEXW wc{ sizeof(wc), CS_CLASSDC, LoaderProc, 0, 0, hi,
                    nullptr, nullptr, nullptr, nullptr, L"CipherLoader", nullptr };
    RegisterClassExW(&wc);

    int sx = (GetSystemMetrics(SM_CXSCREEN) - kLoaderW) / 2;
    int sy = (GetSystemMetrics(SM_CYSCREEN) - kLoaderH) / 2;
    g_loader = CreateWindowExW(WS_EX_APPWINDOW, L"CipherLoader", L"Cipher",
        WS_POPUP | WS_MINIMIZEBOX,
        sx, sy, kLoaderW, kLoaderH, nullptr, nullptr, hi, nullptr);

    BOOL dark = TRUE;
    DwmSetWindowAttribute(g_loader, 20, &dark, sizeof(dark));
    DWORD round = 2;
    DwmSetWindowAttribute(g_loader, 33, &round, sizeof(round));
    MARGINS shadow{ 1,1,1,1 };
    DwmExtendFrameIntoClientArea(g_loader, &shadow);

    if (!CreateDevice(g_loader)) return 1;
    ShowWindow(g_loader, SW_SHOWDEFAULT);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    Theme::Apply();
    UI_InitFonts();
    ImGui_ImplWin32_Init(g_loader);
    ImGui_ImplDX11_Init(g_dev, g_ctx);

    AppState st{};
    st.mode = AppMode::Loader;
    Tex::LoadPNGMemory(g_dev, kLogoSmPng, kLogoSmPng_len, st.logoSm);
    Tex::LoadPNGMemory(g_dev, kLogoLgPng, kLogoLgPng_len, st.logoLg);

    g_running = true;
    while (g_running) {
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_APP + 7) {
                st.menuOpen = !st.menuOpen;
                if (g_overlay)
                    Overlay::SetClickThrough(g_overlay, !st.menuOpen);
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (msg.message == WM_QUIT) g_running = false;
        }
        if (!g_running) break;

        // Loader: poll Fortnite status
        if (st.mode == AppMode::Loader) {
            st.fortniteOpen = IsFortniteRunning();
            if (st.requestAttach) {
                st.requestAttach = false;
                if (!st.fortniteOpen) {
                    st.attaching = false;
                    snprintf(st.status, sizeof(st.status), "Fortnite not open");
                } else {
                    st.attaching = true;
                    snprintf(st.status, sizeof(st.status), "Attaching...");
                    if (!st.esp.Attach()) {
                        st.attaching = false;
                        DWORD err = GetLastError();
                        snprintf(st.status, sizeof(st.status),
                                 "Attach failed (err %lu) — run as Admin", (unsigned long)err);
                    } else if (!EnterOverlayMode(hi, st)) {
                        st.attaching = false;
                        st.mode = AppMode::Loader;
                        g_attachedMode = false;
                        snprintf(st.status, sizeof(st.status), "Overlay failed — try again as Admin");
                    }
                }
            }
        } else {
            // Overlay engines — throttle auto-attach
            static float attachCooldown = 0.f;
            attachCooldown -= io.DeltaTime;
            if (st.autoAttach && !st.esp.Attached() && attachCooldown <= 0.f) {
                st.esp.Attach();
                attachCooldown = 1.5f;
            }
            st.esp.Tick();
            st.aim.Tick(st.esp, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
            st.trigger.Tick(st.esp, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
            st.exploits.Tick(st.esp);
            if (st.misc.streamproof)
                StreamProof::Set(g_overlay, true);

            // INSERT backup if RegisterHotKey is blocked
            static bool insertWasDown = false;
            bool insertDown = (GetAsyncKeyState(VK_INSERT) & 0x8000) != 0;
            if (insertDown && !insertWasDown) {
                st.menuOpen = !st.menuOpen;
                if (g_overlay)
                    Overlay::SetClickThrough(g_overlay, !st.menuOpen);
            }
            insertWasDown = insertDown;
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        if (st.mode == AppMode::Loader)
            UI_RenderLoader(st, g_dev, g_loader);
        else {
            // Always draw world overlays
            st.esp.DrawOverlay();
            st.aim.DrawFovAndRadar(st.esp, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
            if (st.menuOpen)
                UI_RenderOverlay(st, g_dev, g_overlay);
        }

        ImGui::Render();
        // Clear: loader uses dark bg; overlay uses pure black for color-key transparency
        float clear[4];
        if (st.mode == AppMode::Loader) {
            clear[0] = 0.04f; clear[1] = 0.04f; clear[2] = 0.05f; clear[3] = 1.f;
        } else {
            clear[0] = 0.f; clear[1] = 0.f; clear[2] = 0.f; clear[3] = 0.f;
        }
        g_ctx->OMSetRenderTargets(1, &g_rtv, nullptr);
        g_ctx->ClearRenderTargetView(g_rtv, clear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_swap->Present(st.misc.vsync ? 1 : 0, 0);
    }

    UnregisterHotKey(g_overlay, kHotkeyId);
    Tex::Release(st.logoLg); Tex::Release(st.logoSm);
    ImGui_ImplDX11_Shutdown(); ImGui_ImplWin32_Shutdown(); ImGui::DestroyContext();
    CleanupDevice();
    if (g_overlay) DestroyWindow(g_overlay);
    if (g_loader) DestroyWindow(g_loader);
    return 0;
}
