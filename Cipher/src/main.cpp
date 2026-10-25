#include "win_pre.h"
#include "ui.h"
#include "theme.h"
#include "texture.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include <string>
#include <dwmapi.h>

#pragma comment(lib, "dwmapi.lib")

static ID3D11Device*           g_device = nullptr;
static ID3D11DeviceContext*    g_ctx = nullptr;
static IDXGISwapChain*         g_swap = nullptr;
static ID3D11RenderTargetView* g_rtv = nullptr;
static bool                    g_running = true;

// Menu pixel size — window client area matches this (no black sea)
static const int kMenuW = 760;
static const int kMenuH = 520;

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
    case WM_SIZE:
        if (g_device && wParam != SIZE_MINIMIZED) {
            CleanupRT();
            g_swap->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRT();
        }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

static std::wstring ExeDir() {
    wchar_t buf[MAX_PATH]{};
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    std::wstring p = buf;
    size_t slash = p.find_last_of(L"\\/");
    return slash == std::wstring::npos ? L"." : p.substr(0, slash);
}

static bool TryLoadLogo(ID3D11Device* dev, const std::wstring& dir, const wchar_t* name, Texture& out) {
    const std::wstring candidates[] = {
        dir + L"\\" + name,
        dir + L"\\assets\\" + name,
        dir + L"\\..\\assets\\" + name,
        std::wstring(L"assets\\") + name,
        name,
    };
    for (auto& p : candidates) {
        if (Tex::LoadPNG(dev, p, out))
            return true;
    }
    return false;
}

int WINAPI wWinMain(HINSTANCE hi, HINSTANCE, LPWSTR, int) {
    WNDCLASSEXW wc{ sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, hi,
                    nullptr, nullptr, nullptr, nullptr, L"CipherUI", nullptr };
    RegisterClassExW(&wc);

    // Size window so *client area* == menu size (account for non-client chrome)
    RECT rc{ 0, 0, kMenuW, kMenuH };
    AdjustWindowRect(&rc, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, FALSE);
    int winW = rc.right - rc.left;
    int winH = rc.bottom - rc.top;

    HWND hwnd = CreateWindowW(
        wc.lpszClassName, L"Cipher",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, winW, winH,
        nullptr, nullptr, wc.hInstance, nullptr);

    BOOL dark = TRUE;
    DwmSetWindowAttribute(hwnd, 20, &dark, sizeof(dark));

    if (!CreateDevice(hwnd)) {
        CleanupDevice();
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;
    Theme::Apply();
    UI_InitFonts();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_device, g_ctx);

    AppState st{};
    std::snprintf(st.status, sizeof(st.status), "Feature 2 — ESP");

    std::wstring dir = ExeDir();
    TryLoadLogo(g_device, dir, L"logo_sm.png", st.logoSm);
    TryLoadLogo(g_device, dir, L"logo_lg.png", st.logoLg);
    if (!st.logoSm.ok() && st.logoLg.ok()) st.logoSm = st.logoLg;
    if (!st.logoLg.ok() && st.logoSm.ok()) st.logoLg = st.logoSm;

    if (!st.logoSm.ok())
        std::snprintf(st.status, sizeof(st.status), "ESP — put logo_sm.png next to exe");

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
        // Match panel deep bg — no pure black “void”
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
    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 0;
}
