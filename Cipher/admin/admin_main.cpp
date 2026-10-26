// Cipher Admin — polished panel (same monochrome theme as loader)
#include "win_pre.h"
#include "theme.h"
#include "license_crypto.h"
#include "license_store.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <dwmapi.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <ctime>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dwmapi.lib")

static ID3D11Device*            g_dev  = nullptr;
static ID3D11DeviceContext*     g_ctx  = nullptr;
static IDXGISwapChain*          g_swap = nullptr;
static ID3D11RenderTargetView*  g_rtv  = nullptr;
static HWND                     g_hwnd = nullptr;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

static LRESULT WINAPI WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (ImGui_ImplWin32_WndProcHandler(h, m, w, l)) return true;
    switch (m) {
    case WM_DESTROY: PostQuitMessage(0); return 0;
    case WM_NCHITTEST: {
        LRESULT hit = DefWindowProcW(h, m, w, l);
        if (hit != HTCLIENT) return hit;
        POINT pt{ (SHORT)LOWORD(l), (SHORT)HIWORD(l) };
        ScreenToClient(h, &pt);
        if (pt.y >= 0 && pt.y < 48) return HTCAPTION;
        return HTCLIENT;
    }
    }
    return DefWindowProcW(h, m, w, l);
}

static bool CreateDevice(HWND hwnd) {
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    D3D_FEATURE_LEVEL fl, levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    if (FAILED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        levels, 2, D3D11_SDK_VERSION, &sd, &g_swap, &g_dev, &fl, &g_ctx)))
        return false;
    ID3D11Texture2D* bb = nullptr;
    g_swap->GetBuffer(0, IID_PPV_ARGS(&bb));
    g_dev->CreateRenderTargetView(bb, nullptr, &g_rtv);
    bb->Release();
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

static void FormatExpiry(uint32_t expiresUnix, char* buf, size_t n) {
    time_t exp = (time_t)expiresUnix;
    struct tm tm{};
    localtime_s(&tm, &exp);
    strftime(buf, n, "%Y-%m-%d", &tm);
}

static void DaysLeft(uint32_t expiresUnix, char* buf, size_t n) {
    time_t now = time(nullptr);
    if ((time_t)expiresUnix <= now) {
        snprintf(buf, n, "expired");
        return;
    }
    int days = (int)(((time_t)expiresUnix - now) / 86400);
    snprintf(buf, n, "%d day%s left", days, days == 1 ? "" : "s");
}

// Rounded card background
static void DrawCard(ImDrawList* dl, ImVec2 a, ImVec2 b, float r = 12.f) {
    dl->AddRectFilled(a, b, IM_COL32(18, 18, 20, 255), r);
    dl->AddRect(a, b, IM_COL32(48, 48, 52, 90), r, 0, 1.0f);
}

int WINAPI wWinMain(HINSTANCE hi, HINSTANCE, LPWSTR, int) {
    WNDCLASSEXW wc{ sizeof(wc), CS_CLASSDC, WndProc, 0, 0, hi,
                    nullptr, nullptr, nullptr, nullptr, L"CipherAdmin", nullptr };
    RegisterClassExW(&wc);

    const int W = 780, H = 620;
    int sx = (GetSystemMetrics(SM_CXSCREEN) - W) / 2;
    int sy = (GetSystemMetrics(SM_CYSCREEN) - H) / 2;
    g_hwnd = CreateWindowExW(WS_EX_APPWINDOW, L"CipherAdmin", L"Cipher Admin",
        WS_POPUP | WS_MINIMIZEBOX, sx, sy, W, H, nullptr, nullptr, hi, nullptr);

    BOOL dark = TRUE;
    DwmSetWindowAttribute(g_hwnd, 20, &dark, sizeof(dark));
    DWORD round = 2;
    DwmSetWindowAttribute(g_hwnd, 33, &round, sizeof(round));

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
    int  genDays = 30;
    char statusMsg[192] = "Locked";
    char filterBuf[64]{};
    float unlockShake = 0.f;

    bool running = true;
    while (running) {
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (msg.message == WM_QUIT) running = false;
        }
        if (!running) break;

        if (unlockShake > 0.f) unlockShake -= io.DeltaTime * 4.f;

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("##adminRoot", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);

        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 wp = ImGui::GetWindowPos();
        ImVec2 ws = ImGui::GetWindowSize();

        // Full black body + header (no gray OS chrome look)
        dl->AddRectFilled(wp, ImVec2(wp.x + ws.x, wp.y + ws.y), IM_COL32(10, 10, 12, 255));
        dl->AddRectFilled(wp, ImVec2(wp.x + ws.x, wp.y + 48), IM_COL32(10, 10, 12, 255));
        dl->AddLine(ImVec2(wp.x + 16, wp.y + 47), ImVec2(wp.x + ws.x - 16, wp.y + 47),
                    IM_COL32(40, 40, 44, 255), 1.f);

        // Header title
        dl->AddText(ImVec2(wp.x + 20, wp.y + 15), IM_COL32(240, 240, 245, 255), "CIPHER");
        ImVec2 csz = ImGui::CalcTextSize("CIPHER");
        dl->AddText(ImVec2(wp.x + 20 + csz.x + 10, wp.y + 16), IM_COL32(110, 110, 118, 255), "Admin");

        // Close
        ImGui::SetCursorScreenPos(ImVec2(wp.x + ws.x - 40, wp.y + 10));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1,1,1,0.08f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1,1,1,0.14f));
        if (ImGui::Button("X", ImVec2(28, 28))) PostQuitMessage(0);
        ImGui::PopStyleColor(3);

        const float pad = 24.f;
        float contentTop = wp.y + 64.f;
        float contentW = ws.x - pad * 2.f;

        if (!unlocked) {
            // Centered unlock card
            float cardW = 400.f, cardH = 220.f;
            float cx = wp.x + (ws.x - cardW) * 0.5f;
            float cy = wp.y + (ws.y - cardH) * 0.45f;
            if (unlockShake > 0.f)
                cx += sinf(unlockShake * 40.f) * 6.f * unlockShake;

            DrawCard(dl, ImVec2(cx, cy), ImVec2(cx + cardW, cy + cardH), 14.f);

            dl->AddText(ImVec2(cx + 24, cy + 22), IM_COL32(235, 235, 240, 255), "Admin access");
            dl->AddText(ImVec2(cx + 24, cy + 48), IM_COL32(110, 110, 118, 255),
                        "Enter the admin password to manage keys.");

            ImGui::SetCursorScreenPos(ImVec2(cx + 24, cy + 88));
            ImGui::PushItemWidth(cardW - 48.f);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.f);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.07f, 0.07f, 0.08f, 1.f));
            bool enter = ImGui::InputTextWithHint("##pw", "Password", adminPass, sizeof(adminPass),
                ImGuiInputTextFlags_Password | ImGuiInputTextFlags_EnterReturnsTrue);
            ImGui::PopStyleColor();
            ImGui::PopStyleVar();
            ImGui::PopItemWidth();

            ImGui::SetCursorScreenPos(ImVec2(cx + 24, cy + 136));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.f);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1,1,1,0.12f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1,1,1,0.20f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1,1,1,0.28f));
            if (ImGui::Button("Unlock", ImVec2(cardW - 48.f, 40.f)) || enter) {
                if (LicenseStore::CheckAdminPassword(adminPass)) {
                    unlocked = true;
                    snprintf(statusMsg, sizeof(statusMsg), "Unlocked · %zu keys loaded", recs.size());
                    memset(adminPass, 0, sizeof(adminPass));
                } else {
                    snprintf(statusMsg, sizeof(statusMsg), "Wrong password");
                    unlockShake = 1.f;
                }
            }
            ImGui::PopStyleColor(3);
            ImGui::PopStyleVar();

            float col[4] = { 0.55f, 0.55f, 0.58f, 1.f };
            if (strstr(statusMsg, "Wrong")) { col[0]=0.95f; col[1]=0.30f; col[2]=0.32f; }
            ImVec2 ss = ImGui::CalcTextSize(statusMsg);
            dl->AddText(ImVec2(cx + (cardW - ss.x) * 0.5f, cy + cardH + 16.f),
                        IM_COL32((int)(col[0]*255),(int)(col[1]*255),(int)(col[2]*255),255), statusMsg);
        } else {
            // ===== GENERATE CARD =====
            float genH = 118.f;
            DrawCard(dl, ImVec2(wp.x + pad, contentTop),
                     ImVec2(wp.x + pad + contentW, contentTop + genH), 12.f);

            dl->AddText(ImVec2(wp.x + pad + 18, contentTop + 14),
                        IM_COL32(235, 235, 240, 255), "Generate key");

            ImGui::SetCursorScreenPos(ImVec2(wp.x + pad + 18, contentTop + 44));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.f);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.07f, 0.07f, 0.08f, 1.f));

            ImGui::TextDisabled("Days");
            ImGui::SameLine(0, 8);
            ImGui::SetNextItemWidth(72);
            ImGui::InputInt("##days", &genDays, 0, 0);
            if (genDays < 1) genDays = 1;
            if (genDays > 3650) genDays = 3650;

            ImGui::SameLine(0, 16);
            ImGui::SetNextItemWidth(contentW - 320.f);
            ImGui::InputTextWithHint("##note", "Note (optional)", genNote, sizeof(genNote));

            ImGui::SameLine(0, 12);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1,1,1,0.14f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1,1,1,0.24f));
            if (ImGui::Button("Generate", ImVec2(110, 0))) {
                uint32_t id = LicenseStore::NextId(recs);
                std::string key = License::GenerateKey(id, (uint16_t)genDays);
                License::KeyInfo info;
                License::ValidateKey(key, info, LicenseStore::RevokedIds(recs));
                LicenseStore::Record r;
                r.info = info;
                r.info.note = genNote;
                r.info.status = License::Status::Valid;
                recs.push_back(r);
                LicenseStore::Save(StorePath(), recs);
                snprintf(statusMsg, sizeof(statusMsg), "Generated id %u · %d days", id, genDays);
                genNote[0] = 0;
                ImGui::SetClipboardText(key.c_str());
            }
            ImGui::PopStyleColor(2);
            ImGui::PopStyleColor();
            ImGui::PopStyleVar();

            // ===== KEY LIST CARD =====
            float listTop = contentTop + genH + 16.f;
            float listH = ws.y - (listTop - wp.y) - 72.f;
            if (listH < 160.f) listH = 160.f;
            DrawCard(dl, ImVec2(wp.x + pad, listTop),
                     ImVec2(wp.x + pad + contentW, listTop + listH), 12.f);

            dl->AddText(ImVec2(wp.x + pad + 18, listTop + 12),
                        IM_COL32(235, 235, 240, 255), "License keys");

            ImGui::SetCursorScreenPos(ImVec2(wp.x + pad + contentW - 210, listTop + 10));
            ImGui::PushItemWidth(180);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.f);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.07f, 0.07f, 0.08f, 1.f));
            ImGui::InputTextWithHint("##filter", "Filter…", filterBuf, sizeof(filterBuf));
            ImGui::PopStyleColor();
            ImGui::PopStyleVar();
            ImGui::PopItemWidth();

            ImGui::SetCursorScreenPos(ImVec2(wp.x + pad + 12, listTop + 42));
            ImGui::BeginChild("##keylist", ImVec2(contentW - 24.f, listH - 54.f), false);

            if (recs.empty()) {
                ImGui::TextDisabled("No keys yet — generate one above.");
            }

            for (int i = 0; i < (int)recs.size(); ++i) {
                auto& r = recs[i].info;
                if (filterBuf[0]) {
                    if (r.key.find(filterBuf) == std::string::npos &&
                        r.note.find(filterBuf) == std::string::npos)
                        continue;
                }

                License::Status st = r.status;
                if (st != License::Status::Revoked) {
                    License::KeyInfo tmp;
                    st = License::ValidateKey(r.key, tmp, LicenseStore::RevokedIds(recs));
                }

                float col[4];
                License::StatusColor(st, col);

                ImGui::PushID(i);
                ImVec2 rowA = ImGui::GetCursorScreenPos();
                float rowW = contentW - 40.f;
                ImDrawList* cdl = ImGui::GetWindowDrawList();
                cdl->AddRectFilled(rowA, ImVec2(rowA.x + rowW, rowA.y + 72.f),
                                   IM_COL32(14, 14, 16, 255), 10.f);

                // status pill
                const char* lab = License::StatusString(st);
                ImVec2 ls = ImGui::CalcTextSize(lab);
                ImVec2 pillA(rowA.x + 12, rowA.y + 12);
                ImVec2 pillB(pillA.x + ls.x + 16, pillA.y + 20);
                cdl->AddRectFilled(pillA, pillB,
                    IM_COL32((int)(col[0]*255),(int)(col[1]*255),(int)(col[2]*255),40), 6.f);
                cdl->AddText(ImVec2(pillA.x + 8, pillA.y + 2),
                    IM_COL32((int)(col[0]*255),(int)(col[1]*255),(int)(col[2]*255),255), lab);

                char meta[128];
                char expBuf[32], leftBuf[32];
                FormatExpiry(r.expiresUnix, expBuf, sizeof(expBuf));
                DaysLeft(r.expiresUnix, leftBuf, sizeof(leftBuf));
                snprintf(meta, sizeof(meta), "id %u  ·  %u days  ·  %s  ·  %s",
                         r.id, r.durationDays, leftBuf, expBuf);
                cdl->AddText(ImVec2(pillB.x + 12, rowA.y + 14),
                             IM_COL32(130, 130, 138, 255), meta);

                // key string
                ImGui::SetCursorScreenPos(ImVec2(rowA.x + 12, rowA.y + 40));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.85f, 0.88f, 1.f));
                ImGui::TextUnformatted(r.key.c_str());
                ImGui::PopStyleColor();

                // actions on right
                ImGui::SetCursorScreenPos(ImVec2(rowA.x + rowW - 150, rowA.y + 12));
                if (ImGui::SmallButton("Copy"))
                    ImGui::SetClipboardText(r.key.c_str());
                ImGui::SameLine();
                if (st != License::Status::Revoked) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.25f, 0.28f, 0.25f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.25f, 0.28f, 0.45f));
                    if (ImGui::SmallButton("Revoke")) {
                        recs[i].info.status = License::Status::Revoked;
                        LicenseStore::Save(StorePath(), recs);
                        snprintf(statusMsg, sizeof(statusMsg), "Revoked id %u", r.id);
                    }
                    ImGui::PopStyleColor(2);
                }

                if (!r.note.empty()) {
                    ImGui::SetCursorScreenPos(ImVec2(rowA.x + 12, rowA.y + 56));
                    ImGui::TextDisabled("%s", r.note.c_str());
                }

                ImGui::SetCursorScreenPos(ImVec2(rowA.x, rowA.y + 80.f));
                ImGui::PopID();
            }
            ImGui::EndChild();

            // footer status bar
            float fy = wp.y + ws.y - 44.f;
            dl->AddRectFilled(ImVec2(wp.x, fy), ImVec2(wp.x + ws.x, wp.y + ws.y),
                              IM_COL32(12, 12, 14, 255));
            dl->AddLine(ImVec2(wp.x + 16, fy), ImVec2(wp.x + ws.x - 16, fy),
                        IM_COL32(40, 40, 44, 255), 1.f);
            dl->AddText(ImVec2(wp.x + 20, fy + 12), IM_COL32(130, 130, 138, 255), statusMsg);

            ImGui::SetCursorScreenPos(ImVec2(wp.x + ws.x - 100, fy + 8));
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1,1,1,0.06f));
            if (ImGui::SmallButton("Lock")) {
                unlocked = false;
                snprintf(statusMsg, sizeof(statusMsg), "Locked");
            }
            ImGui::PopStyleColor();
        }

        ImGui::End();
        ImGui::Render();
        const float clear[4] = { 0.039f, 0.039f, 0.047f, 1.f };
        g_ctx->OMSetRenderTargets(1, &g_rtv, nullptr);
        g_ctx->ClearRenderTargetView(g_rtv, clear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_swap->Present(1, 0);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    return 0;
}
