// Cipher Admin — license generate / revoke / list
// Same monochrome theme as loader. Separate exe.
// Build: link imgui + dx11 + license_crypto.cpp + license_store.cpp

#include "win_pre.h"
#include "theme.h"
#include "license_crypto.h"
#include "license_store.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include <cstdio>
#include <string>
#include <vector>
#include <ctime>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dwmapi.lib")
#include <dwmapi.h>

static ID3D11Device* g_dev; static ID3D11DeviceContext* g_ctx;
static IDXGISwapChain* g_swap; static ID3D11RenderTargetView* g_rtv;
static HWND g_hwnd;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

static LRESULT WINAPI WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (ImGui_ImplWin32_WndProcHandler(h, m, w, l)) return true;
    if (m == WM_DESTROY) { PostQuitMessage(0); return 0; }
    if (m == WM_NCHITTEST) {
        LRESULT hit = DefWindowProcW(h, m, w, l);
        if (hit != HTCLIENT) return hit;
        POINT pt{ (SHORT)LOWORD(l), (SHORT)HIWORD(l) };
        ScreenToClient(h, &pt);
        if (pt.y < 40) return HTCAPTION;
        return HTCLIENT;
    }
    return DefWindowProcW(h, m, w, l);
}

static bool CreateDevice(HWND hwnd) {
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2; sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1; sd.Windowed = TRUE; sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    D3D_FEATURE_LEVEL fl, levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    if (FAILED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        levels, 2, D3D11_SDK_VERSION, &sd, &g_swap, &g_dev, &fl, &g_ctx))) return false;
    ID3D11Texture2D* bb{};
    g_swap->GetBuffer(0, IID_PPV_ARGS(&bb));
    g_dev->CreateRenderTargetView(bb, nullptr, &g_rtv); bb->Release();
    return true;
}

static std::wstring StorePath() {
    wchar_t path[MAX_PATH]{};
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    std::wstring p(path);
    size_t s = p.find_last_of(L"\\/");
    if (s != std::wstring::npos) p = p.substr(0, s + 1);
    return p + L"keys.db";
}

int WINAPI wWinMain(HINSTANCE hi, HINSTANCE, LPWSTR, int) {
    WNDCLASSEXW wc{ sizeof(wc), CS_CLASSDC, WndProc, 0, 0, hi, nullptr, nullptr, nullptr, nullptr, L"CipherAdmin", nullptr };
    RegisterClassExW(&wc);
    const int W = 720, H = 560;
    int sx = (GetSystemMetrics(SM_CXSCREEN) - W) / 2;
    int sy = (GetSystemMetrics(SM_CYSCREEN) - H) / 2;
    g_hwnd = CreateWindowExW(WS_EX_APPWINDOW, L"CipherAdmin", L"Cipher Admin",
        WS_POPUP | WS_MINIMIZEBOX, sx, sy, W, H, nullptr, nullptr, hi, nullptr);
    BOOL dark = TRUE;
    DwmSetWindowAttribute(g_hwnd, 20, &dark, sizeof(dark));
    if (!CreateDevice(g_hwnd)) return 1;
    ShowWindow(g_hwnd, SW_SHOWDEFAULT);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    Theme::Apply();
    ImGui_ImplWin32_Init(g_hwnd);
    ImGui_ImplDX11_Init(g_dev, g_ctx);

    std::vector<LicenseStore::Record> recs;
    LicenseStore::Load(StorePath(), recs);

    bool unlocked = false;
    char adminPass[128]{};
    char genNote[128]{};
    int genDays = 30;
    char statusMsg[160] = "Enter admin password";
    char filterBuf[64]{};
    int selected = -1;

    bool running = true;
    while (running) {
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg); DispatchMessageW(&msg);
            if (msg.message == WM_QUIT) running = false;
        }
        if (!running) break;

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("##admin", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 wp = ImGui::GetWindowPos(), ws = ImGui::GetWindowSize();
        // full black chrome (no gray title strip)
        dl->AddRectFilled(wp, ImVec2(wp.x + ws.x, wp.y + ws.y), IM_COL32(10, 10, 12, 255));
        dl->AddRectFilled(wp, ImVec2(wp.x + ws.x, wp.y + 40), IM_COL32(10, 10, 12, 255));
        dl->AddText(ImVec2(wp.x + 16, wp.y + 12), IM_COL32(235, 235, 240, 255), "CIPHER  ·  Admin");
        ImGui::SetCursorScreenPos(ImVec2(wp.x + ws.x - 36, wp.y + 8));
        if (ImGui::InvisibleButton("##x", ImVec2(24, 24))) PostQuitMessage(0);
        dl->AddText(ImVec2(wp.x + ws.x - 28, wp.y + 10), IM_COL32(180, 180, 185, 255), "X");

        ImGui::SetCursorScreenPos(ImVec2(wp.x + 20, wp.y + 52));

        if (!unlocked) {
            ImGui::TextUnformatted("Admin access");
            ImGui::PushItemWidth(280);
            ImGui::InputText("##pw", adminPass, sizeof(adminPass), ImGuiInputTextFlags_Password);
            ImGui::PopItemWidth();
            ImGui::SameLine();
            if (ImGui::Button("Unlock", ImVec2(100, 0))) {
                if (LicenseStore::CheckAdminPassword(adminPass)) {
                    unlocked = true;
                    snprintf(statusMsg, sizeof(statusMsg), "Unlocked");
                } else {
                    snprintf(statusMsg, sizeof(statusMsg), "Wrong password");
                }
            }
            ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.58f, 1), "%s", statusMsg);
            ImGui::TextDisabled("Default password is in license_crypto.h — change it.");
        } else {
            // Generate row
            ImGui::TextUnformatted("Generate key");
            ImGui::SetNextItemWidth(100);
            ImGui::InputInt("Days", &genDays);
            if (genDays < 1) genDays = 1;
            if (genDays > 3650) genDays = 3650;
            ImGui::SameLine();
            ImGui::SetNextItemWidth(200);
            ImGui::InputTextWithHint("##note", "Note (optional)", genNote, sizeof(genNote));
            ImGui::SameLine();
            if (ImGui::Button("Generate", ImVec2(110, 0))) {
                uint32_t id = LicenseStore::NextId(recs);
                std::string key = License::GenerateKey(id, (uint16_t)genDays);
                License::KeyInfo info;
                auto revoked = LicenseStore::RevokedIds(recs);
                License::ValidateKey(key, info, revoked);
                LicenseStore::Record r;
                r.info = info;
                r.info.note = genNote;
                r.info.status = License::Status::Valid;
                recs.push_back(r);
                LicenseStore::Save(StorePath(), recs);
                snprintf(statusMsg, sizeof(statusMsg), "Generated id %u", id);
                genNote[0] = 0;
            }

            ImGui::Separator();
            ImGui::TextUnformatted("Keys");
            ImGui::SetNextItemWidth(200);
            ImGui::InputTextWithHint("##filter", "Filter key / note", filterBuf, sizeof(filterBuf));

            ImGui::BeginChild("##list", ImVec2(-1, 280), true);
            for (int i = 0; i < (int)recs.size(); i++) {
                auto& r = recs[i].info;
                if (filterBuf[0]) {
                    std::string f = filterBuf;
                    if (r.key.find(f) == std::string::npos && r.note.find(f) == std::string::npos)
                        continue;
                }
                ImGui::PushID(i);
                float col[4];
                // live re-validate display status
                License::Status st = r.status;
                if (st != License::Status::Revoked) {
                    License::KeyInfo tmp;
                    st = License::ValidateKey(r.key, tmp, LicenseStore::RevokedIds(recs));
                    if (st == License::Status::Revoked) st = License::Status::Revoked;
                }
                License::StatusColor(st, col);
                ImGui::TextColored(ImVec4(col[0], col[1], col[2], col[3]), "%s", License::StatusString(st));
                ImGui::SameLine();
                ImGui::Text("id %u · %u days · exp %u", r.id, r.durationDays, r.expiresUnix);
                // human expiry
                time_t exp = (time_t)r.expiresUnix;
                char tbuf[64];
                struct tm tm {};
                localtime_s(&tm, &exp);
                strftime(tbuf, sizeof(tbuf), "%Y-%m-%d", &tm);
                ImGui::SameLine();
                ImGui::TextDisabled("(%s)", tbuf);
                ImGui::TextWrapped("%s", r.key.c_str());
                if (!r.note.empty()) ImGui::TextDisabled("%s", r.note.c_str());
                if (st != License::Status::Revoked) {
                    if (ImGui::SmallButton("Revoke")) {
                        recs[i].info.status = License::Status::Revoked;
                        LicenseStore::Save(StorePath(), recs);
                        snprintf(statusMsg, sizeof(statusMsg), "Revoked id %u", r.id);
                    }
                    ImGui::SameLine();
                }
                if (ImGui::SmallButton("Copy")) {
                    ImGui::SetClipboardText(r.key.c_str());
                }
                ImGui::Separator();
                ImGui::PopID();
            }
            ImGui::EndChild();
            ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.58f, 1), "%s", statusMsg);
        }

        ImGui::End();
        ImGui::Render();
        float clear[4] = { 0.04f, 0.04f, 0.05f, 1.f };
        g_ctx->OMSetRenderTargets(1, &g_rtv, nullptr);
        g_ctx->ClearRenderTargetView(g_rtv, clear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_swap->Present(1, 0);
    }
    ImGui_ImplDX11_Shutdown(); ImGui_ImplWin32_Shutdown(); ImGui::DestroyContext();
    return 0;
}
