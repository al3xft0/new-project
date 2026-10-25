#include "win_pre.h"
#include "ui.h"
#include "theme.h"
#include "texture.h"
#include "embed_logo_sm.h"
#include "embed_logo_lg.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include <string>
#include <cstdio>
#include <dwmapi.h>

#pragma comment(lib, "dwmapi.lib")

static ID3D11Device*           g_device = nullptr;
static ID3D11DeviceContext*    g_ctx = nullptr;
static IDXGISwapChain*         g_swap = nullptr;
static ID3D11RenderTargetView* g_rtv = nullptr;
static bool                    g_running = true;
static HWND                    g_hwnd = nullptr;

// Menu size = window size (no OS chrome)
static const int kMenuW = 760;
static const int kMenuH = 520;
static const int kDragH = 64;      // top strip height for dragging
static const int kSidebarW = 196;  // match UI sidebar — whole sidebar is drag/hit zone helper

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

static void CreateRT() {
    ID3D11Texture2D* bb = nullptr;
    g_swap->GetBuffer(0, IID_PPV_ARGS(&bb));
    if (bb) {
        g_device->CreateRenderTargetView(bb, nullptr, &g_rtv);
        bb->Release();
    }
}
static void CleanupRT() {
    if (g_rtv) { g_rtv->Release(); g_rtv = nullptr; }
}

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

    D3D_FEATURE_LEVEL fl;
    const D3D_FEATURE_LEVEL levels[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        levels, 2, D3D11_SDK_VERSION, &sd,
        &g_swap, &g_device, &fl, &g_ctx);
    if (hr == DXGI_ERROR_UNSUPPORTED)
        hr = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0,
            levels, 2, D3D11_SDK_VERSION, &sd,
            &g_swap, &g_device, &fl, &g_ctx);
    if (FAILED(hr)) return false;
    CreateRT();
    return true;
}

static void CleanupDevice() {
    CleanupRT();
    if (g_swap) { g_swap->Release(); g_swap = nullptr; }
    if (g_ctx) { g_ctx->Release(); g_ctx = nullptr; }
    if (g_device) { g_device->Release(); g_device = nullptr; }
}

static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
    case WM_NCHITTEST: {
        // Let ImGui controls keep clicks; empty chrome regions drag the window
        LRESULT hit = DefWindowProcW(hWnd, msg, wParam, lParam);
        if (hit != HTCLIENT)
            return hit;

        POINT pt{ (SHORT)LOWORD(lParam), (SHORT)HIWORD(lParam) };
        ScreenToClient(hWnd, &pt);

        // Top-right reserve for close / min buttons drawn by ImGui (~80px)
        if (pt.y < kDragH && pt.x > kMenuW - 90)
            return HTCLIENT;

        // Top header strip + left sidebar brand area → draggable
        if (pt.y < kDragH || pt.x < kSidebarW)
            return HTCAPTION;

        return HTCLIENT;
    }
    case WM_SIZE:
        if (g_device && wParam != SIZE_MINIMIZED) {
            CleanupRT();
            g_swap->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam),
                                  DXGI_FORMAT_UNKNOWN, 0);
            CreateRT();
        }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hi, HINSTANCE, LPWSTR, int) {
    WNDCLASSEXW wc{ sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, hi,
                    nullptr, nullptr, nullptr, nullptr, L"CipherUI", nullptr };
    RegisterClassExW(&wc);

    // Borderless layered-looking popup; size == menu
    DWORD style = WS_POPUP;
    DWORD exStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int posX = (screenW - kMenuW) / 2;
    int posY = (screenH - kMenuH) / 2;

    g_hwnd = CreateWindowExW(
        exStyle, wc.lpszClassName, L"Cipher",
        style,
        posX, posY, kMenuW, kMenuH,
        nullptr, nullptr, wc.hInstance, nullptr);

    // Rounded corners on Win11 (ignored on older)
    const DWORD DWMWCP_ROUND = 2;
    DwmSetWindowAttribute(g_hwnd, 33 /* DWMWA_WINDOW_CORNER_PREFERENCE */,
                          (void*)&DWMWCP_ROUND, sizeof(DWMWCP_ROUND));
    BOOL dark = TRUE;
    DwmSetWindowAttribute(g_hwnd, 20 /* DWMWA_USE_IMMERSIVE_DARK_MODE */,
                          &dark, sizeof(dark));

    // Subtle window shadow
    MARGINS shadow{ 1, 1, 1, 1 };
    DwmExtendFrameIntoClientArea(g_hwnd, &shadow);

    if (!CreateDevice(g_hwnd)) {
        CleanupDevice();
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ShowWindow(g_hwnd, SW_SHOWDEFAULT);
    UpdateWindow(g_hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;
    Theme::Apply();
    UI_InitFonts();

    ImGui_ImplWin32_Init(g_hwnd);
    ImGui_ImplDX11_Init(g_device, g_ctx);

    AppState st{};
    std::snprintf(st.status, sizeof(st.status), "Feature 2 - ESP");

    Tex::LoadPNGMemory(g_device, kLogoSmPng, kLogoSmPng_len, st.logoSm);
    Tex::LoadPNGMemory(g_device, kLogoLgPng, kLogoLgPng_len, st.logoLg);
    if (!st.logoSm.ok() && st.logoLg.ok()) st.logoSm = st.logoLg;
    if (!st.logoLg.ok() && st.logoSm.ok()) st.logoLg = st.logoSm;

    g_running = true;
    while (g_running) {
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (msg.message == WM_QUIT) g_running = false;
        }
        if (!g_running) break;

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        UI_Render(st, g_device);

        ImGui::Render();
        const float clear[4] = { 0.04f, 0.04f, 0.05f, 1.f };
        g_ctx->OMSetRenderTargets(1, &g_rtv, nullptr);
        g_ctx->ClearRenderTargetView(g_rtv, clear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_swap->Present(st.misc.vsync ? 1 : 0, 0);
    }

    Tex::Release(st.logoLg);
    Tex::Release(st.logoSm);
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDevice();
    DestroyWindow(g_hwnd);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 0;
}
