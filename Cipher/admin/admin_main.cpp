// Cipher Admin — Xeno soft-flow UI (Phase 1)
// Drop over existing admin_main.cpp. Same includes / LicenseStore API.
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
#include <cmath>
#include <algorithm>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dwmapi.lib")

static ID3D11Device*            g_dev  = nullptr;
static ID3D11DeviceContext*     g_ctx  = nullptr;
static IDXGISwapChain*          g_swap = nullptr;
static ID3D11RenderTargetView*  g_rtv  = nullptr;
static HWND                     g_hwnd = nullptr;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

// ─── math / easing ─────────────────────────────────────────────────────────
static float Clamp01(float v) { return v < 0.f ? 0.f : (v > 1.f ? 1.f : v); }
static float Lerp(float a, float b, float t) { return a + (b - a) * t; }
static float EaseOutCubic(float t) { float u = 1.f - t; return 1.f - u*u*u; }
static float EaseOutBack(float t) {
    const float c1 = 1.70158f, c3 = c1 + 1.f;
    return 1.f + c3 * powf(t - 1.f, 3.f) + c1 * powf(t - 1.f, 2.f);
}
static float EaseOutQuad(float t) { return 1.f - (1.f - t) * (1.f - t); }
static float SmoothStep(float t) { return t * t * (3.f - 2.f * t); }

// ─── win32 ─────────────────────────────────────────────────────────────────
static LRESULT WINAPI WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (ImGui_ImplWin32_WndProcHandler(h, m, w, l)) return true;
    switch (m) {
    case WM_DESTROY: PostQuitMessage(0); return 0;
    case WM_NCHITTEST: {
        LRESULT hit = DefWindowProcW(h, m, w, l);
        if (hit != HTCLIENT) return hit;
        POINT pt{ (SHORT)LOWORD(l), (SHORT)HIWORD(l) };
        ScreenToClient(h, &pt);
        if (pt.y >= 0 && pt.y < 54) return HTCAPTION;
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
    int d = (int)((expiresUnix - (uint32_t)now) / 86400);
    if (d < 0)      snprintf(buf, n, "expired");
    else if (d == 0) snprintf(buf, n, "today");
    else             snprintf(buf, n, "%d days left", d);
}

// ─── draw helpers ──────────────────────────────────────────────────────────
static void DrawSoftShadow(ImDrawList* dl, ImVec2 a, ImVec2 b, float r, float spread = 10.f, float alphaMul = 1.f) {
    for (int i = 4; i >= 1; --i) {
        float o = spread * (float)i / 4.f;
        int alpha = (int)((10 * i) * alphaMul);
        dl->AddRectFilled(ImVec2(a.x - o * 0.25f, a.y + o * 0.55f),
                          ImVec2(b.x + o * 0.25f, b.y + o),
                          IM_COL32(0, 0, 0, alpha), r + o * 0.35f);
    }
}

static void DrawCard(ImDrawList* dl, ImVec2 a, ImVec2 b, float r, float alpha = 1.f, float glow = 0.f) {
    DrawSoftShadow(dl, a, b, r, 10.f, alpha);
    // body
    dl->AddRectFilled(a, b, IM_COL32(20, 20, 24, (int)(255 * alpha)), r);
    // border
    dl->AddRect(a, b, IM_COL32(48, 48, 56, (int)(160 * alpha)), r, 0, 1.f);
    // top inner highlight
    dl->AddLine(ImVec2(a.x + r, a.y + 1.f), ImVec2(b.x - r, a.y + 1.f),
                IM_COL32(255, 255, 255, (int)(16 * alpha)), 1.f);
    // optional soft accent glow on hover
    if (glow > 0.01f) {
        int ga = (int)(28 * glow * alpha);
        dl->AddRect(ImVec2(a.x - 1, a.y - 1), ImVec2(b.x + 1, b.y + 1),
                    IM_COL32(120, 160, 255, ga), r + 1.f, 0, 1.5f);
    }
}

// geometric icons (no extra fonts)
static void IconKey(ImDrawList* dl, ImVec2 c, float s, ImU32 col) {
    dl->AddCircle(ImVec2(c.x - s * 0.22f, c.y), s * 0.32f, col, 14, 1.55f);
    dl->AddLine(ImVec2(c.x + s * 0.05f, c.y), ImVec2(c.x + s * 0.52f, c.y), col, 1.55f);
    dl->AddLine(ImVec2(c.x + s * 0.32f, c.y), ImVec2(c.x + s * 0.32f, c.y + s * 0.22f), col, 1.4f);
    dl->AddLine(ImVec2(c.x + s * 0.44f, c.y), ImVec2(c.x + s * 0.44f, c.y + s * 0.16f), col, 1.4f);
}
static void IconLock(ImDrawList* dl, ImVec2 c, float s, ImU32 col) {
    dl->AddRect(ImVec2(c.x - s * 0.38f, c.y - s * 0.02f),
                ImVec2(c.x + s * 0.38f, c.y + s * 0.42f), col, 3.f, 0, 1.5f);
    dl->AddBezierCubic(ImVec2(c.x - s * 0.26f, c.y - s * 0.02f),
                       ImVec2(c.x - s * 0.26f, c.y - s * 0.52f),
                       ImVec2(c.x + s * 0.26f, c.y - s * 0.52f),
                       ImVec2(c.x + s * 0.26f, c.y - s * 0.02f), col, 1.5f, 14);
}
static void IconPlus(ImDrawList* dl, ImVec2 c, float s, ImU32 col) {
    dl->AddLine(ImVec2(c.x - s * 0.32f, c.y), ImVec2(c.x + s * 0.32f, c.y), col, 1.7f);
    dl->AddLine(ImVec2(c.x, c.y - s * 0.32f), ImVec2(c.x, c.y + s * 0.32f), col, 1.7f);
}
static void IconCopy(ImDrawList* dl, ImVec2 c, float s, ImU32 col) {
    dl->AddRect(ImVec2(c.x - s * 0.18f, c.y - s * 0.32f),
                ImVec2(c.x + s * 0.32f, c.y + s * 0.18f), col, 2.f, 0, 1.35f);
    dl->AddRect(ImVec2(c.x - s * 0.36f, c.y - s * 0.14f),
                ImVec2(c.x + s * 0.14f, c.y + s * 0.36f), col, 2.f, 0, 1.35f);
}

// thin accent flow line under a section header
static void DrawAccentFlow(ImDrawList* dl, ImVec2 left, float width, float t, float alpha) {
    float pulse = 0.55f + 0.45f * sinf(t * 2.1f);
    int a = (int)(90 * pulse * alpha);
    dl->AddRectFilled(ImVec2(left.x, left.y), ImVec2(left.x + width, left.y + 1.5f),
                      IM_COL32(110, 150, 255, a), 1.f);
}

// ─── main ──────────────────────────────────────────────────────────────────
int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {
    WNDCLASSEXW wc{ sizeof(wc), CS_CLASSDC, WndProc, 0, 0, GetModuleHandle(nullptr),
                    nullptr, nullptr, nullptr, nullptr, L"CipherAdmin", nullptr };
    RegisterClassExW(&wc);

    g_hwnd = CreateWindowExW(WS_EX_APPWINDOW, wc.lpszClassName, L"CIPHER  Admin",
        WS_POPUP | WS_VISIBLE, 120, 80, 740, 580, nullptr, nullptr, wc.hInstance, nullptr);

    BOOL dark = TRUE;
    DwmSetWindowAttribute(g_hwnd, 20, &dark, sizeof(dark));
    MARGINS m{ -1 }; DwmExtendFrameIntoClientArea(g_hwnd, &m);

    if (!CreateDevice(g_hwnd)) return 1;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;

    ImGui::StyleColorsDark();
    ImGuiStyle& st = ImGui::GetStyle();
    st.WindowRounding    = 0.f;
    st.ChildRounding     = 12.f;
    st.FrameRounding     = 9.f;
    st.PopupRounding     = 9.f;
    st.ScrollbarRounding = 7.f;
    st.GrabRounding      = 7.f;
    st.WindowBorderSize  = 0.f;
    st.FrameBorderSize   = 0.f;
    st.ItemSpacing       = ImVec2(10, 8);
    st.WindowPadding     = ImVec2(0, 0);
    st.Colors[ImGuiCol_WindowBg]       = ImVec4(0.035f, 0.035f, 0.042f, 1.f);
    st.Colors[ImGuiCol_ChildBg]        = ImVec4(0, 0, 0, 0);
    st.Colors[ImGuiCol_FrameBg]        = ImVec4(0.065f, 0.065f, 0.075f, 1.f);
    st.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.095f, 0.095f, 0.110f, 1.f);
    st.Colors[ImGuiCol_Button]         = ImVec4(1, 1, 1, 0.07f);
    st.Colors[ImGuiCol_ButtonHovered]  = ImVec4(1, 1, 1, 0.15f);
    st.Colors[ImGuiCol_ButtonActive]   = ImVec4(1, 1, 1, 0.22f);
    st.Colors[ImGuiCol_Header]         = ImVec4(1, 1, 1, 0.05f);
    st.Colors[ImGuiCol_HeaderHovered]  = ImVec4(1, 1, 1, 0.09f);
    st.Colors[ImGuiCol_Text]           = ImVec4(0.93f, 0.93f, 0.95f, 1.f);
    st.Colors[ImGuiCol_TextDisabled]   = ImVec4(0.42f, 0.42f, 0.46f, 1.f);
    st.Colors[ImGuiCol_Border]         = ImVec4(0.16f, 0.16f, 0.19f, 1.f);

    ImGui_ImplWin32_Init(g_hwnd);
    ImGui_ImplDX11_Init(g_dev, g_ctx);

    std::vector<LicenseStore::Record> recs;
    LicenseStore::Load(StorePath(), recs);

    bool  unlocked      = false;
    char  adminPass[64]{};
    char  genNote[128]{};
    char  filterBuf[64]{};
    char  statusMsg[128] = "Locked";
    int   genDays       = 30;

    // animation state
    float unlockShake   = 0.f;
    float unlockAnim    = 0.f;   // 0→1 card entrance
    float contentAnim   = 0.f;   // 0→1 content fade after unlock
    float statusPulse   = 0.f;
    float timeAccum     = 0.f;
    float genHover      = 0.f;
    float listHover     = 0.f;
    std::vector<float> rowHover; // per-row hover anim

    MSG msg{};
    while (msg.message != WM_QUIT) {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) break;
        }
        if (msg.message == WM_QUIT) break;

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        float dt = io.DeltaTime;
        timeAccum += dt;
        if (unlockShake > 0.f) unlockShake = (std::max)(0.f, unlockShake - dt * 3.4f);

        if (!unlocked) {
            unlockAnim  = (std::min)(1.f, unlockAnim + dt * 2.6f);
            contentAnim = 0.f;
        } else {
            contentAnim = (std::min)(1.f, contentAnim + dt * 3.2f);
        }
        statusPulse = fmodf(statusPulse + dt * 1.7f, 6.283185f);

        // keep rowHover sized
        if (rowHover.size() != recs.size())
            rowHover.assign(recs.size(), 0.f);

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("##root", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoBackground);

        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 wp = ImGui::GetWindowPos();
        ImVec2 ws = ImGui::GetWindowSize();

        // ── title bar ──────────────────────────────────────────────────────
        dl->AddRectFilled(wp, ImVec2(wp.x + ws.x, wp.y + 54), IM_COL32(13, 13, 16, 255));
        dl->AddLine(ImVec2(wp.x, wp.y + 54), ImVec2(wp.x + ws.x, wp.y + 54),
                    IM_COL32(38, 38, 44, 255), 1.f);

        IconKey(dl, ImVec2(wp.x + 30, wp.y + 27), 11.5f, IM_COL32(220, 220, 230, 255));
        dl->AddText(ImVec2(wp.x + 50, wp.y + 18), IM_COL32(238, 238, 242, 255), "CIPHER");
        dl->AddText(ImVec2(wp.x + 112, wp.y + 18), IM_COL32(105, 105, 115, 255), "Admin");

        // close button
        ImGui::SetCursorScreenPos(ImVec2(wp.x + ws.x - 46, wp.y + 13));
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1,1,1,0.08f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(1,1,1,0.14f));
        if (ImGui::Button("##x", ImVec2(30, 28))) PostQuitMessage(0);
        ImGui::PopStyleColor(3);
        {
            ImVec2 xc(wp.x + ws.x - 31, wp.y + 27);
            dl->AddLine(ImVec2(xc.x - 6, xc.y - 6), ImVec2(xc.x + 6, xc.y + 6), IM_COL32(175,175,185,255), 1.55f);
            dl->AddLine(ImVec2(xc.x + 6, xc.y - 6), ImVec2(xc.x - 6, xc.y + 6), IM_COL32(175,175,185,255), 1.55f);
        }

        const float pad = 22.f;
        float contentTop = wp.y + 70.f;
        float contentW   = ws.x - pad * 2.f;

        if (!unlocked) {
            // ── unlock card ────────────────────────────────────────────────
            float t  = EaseOutBack(Clamp01(unlockAnim));
            float a  = EaseOutCubic(Clamp01(unlockAnim));
            float cardW = 390.f, cardH = 248.f;
            float cx = wp.x + (ws.x - cardW) * 0.5f;
            float cy = wp.y + (ws.y - cardH) * 0.40f + (1.f - t) * 48.f;
            if (unlockShake > 0.f)
                cx += sinf(unlockShake * 40.f) * 8.f * unlockShake;

            DrawCard(dl, ImVec2(cx, cy), ImVec2(cx + cardW, cy + cardH), 16.f, a);

            IconLock(dl, ImVec2(cx + cardW * 0.5f, cy + 42), 17.f,
                     IM_COL32(205, 205, 215, (int)(230 * a)));

            dl->AddText(ImVec2(cx + 30, cy + 78),
                        IM_COL32(238, 238, 242, (int)(255 * a)), "Admin access");
            dl->AddText(ImVec2(cx + 30, cy + 102),
                        IM_COL32(108, 108, 118, (int)(255 * a)),
                        "Enter the admin password to manage keys.");

            ImGui::SetCursorScreenPos(ImVec2(cx + 30, cy + 136));
            ImGui::PushItemWidth(cardW - 60.f);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.f);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.055f, 0.055f, 0.065f, a));
            bool enter = ImGui::InputTextWithHint("##pw", "Password", adminPass, sizeof(adminPass),
                ImGuiInputTextFlags_Password | ImGuiInputTextFlags_EnterReturnsTrue);
            ImGui::PopStyleColor();
            ImGui::PopStyleVar();
            ImGui::PopItemWidth();

            ImGui::SetCursorScreenPos(ImVec2(cx + 30, cy + 182));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.f);
            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(1,1,1, 0.09f * a));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1,1,1, 0.17f * a));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(1,1,1, 0.25f * a));
            if ((ImGui::Button("Unlock", ImVec2(cardW - 60.f, 40.f)) || enter) && a > 0.92f) {
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

            float col[4] = { 0.52f, 0.52f, 0.56f, a };
            if (strstr(statusMsg, "Wrong")) { col[0]=0.95f; col[1]=0.32f; col[2]=0.34f; }
            ImVec2 ss = ImGui::CalcTextSize(statusMsg);
            dl->AddText(ImVec2(cx + (cardW - ss.x) * 0.5f, cy + cardH + 18.f),
                        IM_COL32((int)(col[0]*255),(int)(col[1]*255),(int)(col[2]*255),(int)(255*a)), statusMsg);
        } else {
            float ca = EaseOutCubic(Clamp01(contentAnim));

            // ── generate card ──────────────────────────────────────────────
            float genH = 116.f;
            ImVec2 ga(wp.x + pad, contentTop);
            ImVec2 gb(wp.x + pad + contentW, contentTop + genH);

            bool genHovered = ImGui::IsMouseHoveringRect(ga, gb);
            genHover = Lerp(genHover, genHovered ? 1.f : 0.f, 1.f - expf(-dt * 12.f));
            DrawCard(dl, ga, gb, 14.f, ca, genHover * 0.7f);

            IconPlus(dl, ImVec2(ga.x + 28, ga.y + 26), 9.5f, IM_COL32(200, 200, 212, (int)(235 * ca)));
            dl->AddText(ImVec2(ga.x + 46, ga.y + 17),
                        IM_COL32(238, 238, 242, (int)(255 * ca)), "Generate key");
            DrawAccentFlow(dl, ImVec2(ga.x + 20, ga.y + 42), 110.f, timeAccum, ca);

            ImGui::SetCursorScreenPos(ImVec2(ga.x + 20, ga.y + 52));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 9.f);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.055f, 0.055f, 0.065f, ca));

            ImGui::TextDisabled("Days");
            ImGui::SameLine(0, 8);
            ImGui::SetNextItemWidth(70);
            ImGui::InputInt("##days", &genDays, 0, 0);
            if (genDays < 1) genDays = 1;
            if (genDays > 3650) genDays = 3650;

            ImGui::SameLine(0, 14);
            ImGui::SetNextItemWidth(contentW - 310.f);
            ImGui::InputTextWithHint("##note", "Note (optional)", genNote, sizeof(genNote));

            ImGui::SameLine(0, 12);
            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(1,1,1, 0.11f * ca));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1,1,1, 0.20f * ca));
            if (ImGui::Button("Generate", ImVec2(112, 0))) {
                uint32_t id = LicenseStore::NextId(recs);
                std::string key = License::GenerateKey(id, (uint16_t)genDays);
                License::KeyInfo info;
                License::ValidateKey(key, info, LicenseStore::RevokedIds(recs));
                LicenseStore::Record r;
                r.info = info;
                r.info.note = genNote;
                r.info.status = License::Status::Valid;
                recs.push_back(r);
                rowHover.push_back(0.f);
                LicenseStore::Save(StorePath(), recs);
                snprintf(statusMsg, sizeof(statusMsg), "Generated id %u · %d days", id, genDays);
                genNote[0] = 0;
                ImGui::SetClipboardText(key.c_str());
            }
            ImGui::PopStyleColor(2);
            ImGui::PopStyleColor();
            ImGui::PopStyleVar();

            // ── key list card ──────────────────────────────────────────────
            float listTop = contentTop + genH + 14.f;
            float listH   = ws.y - (listTop - wp.y) - 58.f;
            if (listH < 150.f) listH = 150.f;
            ImVec2 la(wp.x + pad, listTop);
            ImVec2 lb(wp.x + pad + contentW, listTop + listH);

            bool listHovered = ImGui::IsMouseHoveringRect(la, lb);
            listHover = Lerp(listHover, listHovered ? 1.f : 0.f, 1.f - expf(-dt * 10.f));
            DrawCard(dl, la, lb, 14.f, ca, listHover * 0.45f);

            IconKey(dl, ImVec2(la.x + 26, la.y + 24), 9.5f, IM_COL32(200, 200, 212, (int)(235 * ca)));
            dl->AddText(ImVec2(la.x + 44, la.y + 15),
                        IM_COL32(238, 238, 242, (int)(255 * ca)), "License keys");
            DrawAccentFlow(dl, ImVec2(la.x + 20, la.y + 40), 120.f, timeAccum + 1.2f, ca);

            ImGui::SetCursorScreenPos(ImVec2(la.x + contentW - 198, la.y + 13));
            ImGui::PushItemWidth(170);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 9.f);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.055f, 0.055f, 0.065f, ca));
            ImGui::InputTextWithHint("##filter", "Filter…", filterBuf, sizeof(filterBuf));
            ImGui::PopStyleColor();
            ImGui::PopStyleVar();
            ImGui::PopItemWidth();

            // child list
            ImGui::SetCursorScreenPos(ImVec2(la.x + 14, la.y + 50));
            float childW = contentW - 28.f;
            float childH = listH - 64.f;
            ImGui::BeginChild("##keylist", ImVec2(childW, childH), false,
                              ImGuiWindowFlags_NoBackground);

            ImDrawList* cdl = ImGui::GetWindowDrawList();
            ImVec2 childPos = ImGui::GetCursorScreenPos();
            float  rowH     = 78.f;
            float  yOff     = 0.f;
            int    visibleIdx = 0;

            if (recs.empty()) {
                ImGui::SetCursorScreenPos(ImVec2(childPos.x + 8, childPos.y + 8));
                ImGui::TextDisabled("No keys yet — generate one above.");
            }

            for (size_t i = 0; i < recs.size(); ++i) {
                auto& r = recs[i];
                if (filterBuf[0] &&
                    r.key.find(filterBuf) == std::string::npos &&
                    r.info.note.find(filterBuf) == std::string::npos)
                    continue;

                // staggered entrance
                float rowDelay = visibleIdx * 0.045f;
                float rowT = Clamp01((contentAnim - 0.22f - rowDelay) * 2.8f);
                float rowA = EaseOutCubic(rowT);
                float rowSlide = (1.f - rowA) * 16.f;

                float rowY = childPos.y + yOff + rowSlide;
                // full width of the child, small side pad only
                ImVec2 rowApos(childPos.x + 2.f, rowY);
                ImVec2 rowBpos(childPos.x + childW - 6.f, rowY + rowH - 8.f);
                float rowW = rowBpos.x - rowApos.x;

                bool hovered = ImGui::IsMouseHoveringRect(rowApos, rowBpos) && rowA > 0.85f;
                if (i < rowHover.size())
                    rowHover[i] = Lerp(rowHover[i], hovered ? 1.f : 0.f, 1.f - expf(-dt * 14.f));
                float hov = (i < rowHover.size()) ? rowHover[i] : 0.f;

                // single solid row background (no seam)
                int bgA = (int)((175 + 45 * hov) * rowA);
                cdl->AddRectFilled(rowApos, rowBpos, IM_COL32(26, 26, 31, bgA), 10.f);
                cdl->AddRect(rowApos, rowBpos, IM_COL32(50, 50, 58, (int)(130 * rowA)), 10.f, 0, 1.f);
                if (hov > 0.05f) {
                    cdl->AddRect(ImVec2(rowApos.x - 0.5f, rowApos.y - 0.5f),
                                 ImVec2(rowBpos.x + 0.5f, rowBpos.y + 0.5f),
                                 IM_COL32(110, 150, 255, (int)(32 * hov * rowA)), 11.f, 0, 1.3f);
                }

                // status pill
                License::Status st = r.info.status;
                const char* lab = "Valid";
                ImU32 pillCol = IM_COL32(70, 195, 115, 255);
                if (st == License::Status::Revoked) {
                    lab = "Revoked";
                    pillCol = IM_COL32(220, 75, 85, 255);
                } else if (r.info.expiresUnix < (uint32_t)time(nullptr)) {
                    lab = "Expired";
                    pillCol = IM_COL32(200, 155, 55, 255);
                }
                ImVec2 pillA(rowApos.x + 12, rowApos.y + 10);
                ImVec2 pillSz = ImGui::CalcTextSize(lab);
                ImVec2 pillB(pillA.x + pillSz.x + 14, pillA.y + 17);
                cdl->AddRectFilled(pillA, pillB, (pillCol & 0x00FFFFFF) | 0x28000000, 5.f);
                cdl->AddText(ImVec2(pillA.x + 7, pillA.y + 1),
                             (pillCol & 0x00FFFFFF) | ((int)(255 * rowA) << 24), lab);

                // meta (id / days / left / date)
                char meta[128], expBuf[32], leftBuf[32];
                FormatExpiry(r.info.expiresUnix, expBuf, sizeof(expBuf));
                DaysLeft(r.info.expiresUnix, leftBuf, sizeof(leftBuf));
                snprintf(meta, sizeof(meta), "id %u  ·  %u days  ·  %s  ·  %s",
                         r.info.id, r.info.durationDays, leftBuf, expBuf);
                cdl->AddText(ImVec2(pillB.x + 10, rowApos.y + 11),
                             IM_COL32(125, 125, 135, (int)(255 * rowA)), meta);

                // key string — full width under meta
                ImGui::SetCursorScreenPos(ImVec2(rowApos.x + 12, rowApos.y + 34));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.88f, 0.88f, 0.91f, rowA));
                ImGui::TextUnformatted(r.key.c_str());
                ImGui::PopStyleColor();

                // actions top-right of the row
                float btnX = rowBpos.x - 140.f;
                ImGui::SetCursorScreenPos(ImVec2(btnX, rowApos.y + 9));
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, rowA);
                if (ImGui::SmallButton("Copy"))
                    ImGui::SetClipboardText(r.key.c_str());
                ImGui::SameLine(0, 6);
                if (st != License::Status::Revoked) {
                    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.85f, 0.25f, 0.28f, 0.20f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.25f, 0.28f, 0.38f));
                    if (ImGui::SmallButton("Revoke")) {
                        recs[i].info.status = License::Status::Revoked;
                        LicenseStore::Save(StorePath(), recs);
                        snprintf(statusMsg, sizeof(statusMsg), "Revoked id %u", r.info.id);
                    }
                    ImGui::PopStyleColor(2);
                }
                ImGui::PopStyleVar();

                // optional note under key
                if (!r.info.note.empty()) {
                    ImGui::SetCursorScreenPos(ImVec2(rowApos.x + 12, rowApos.y + 52));
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, rowA * 0.65f);
                    ImGui::TextDisabled("%s", r.info.note.c_str());
                    ImGui::PopStyleVar();
                }

                // grow content region (keeps ImGui assert dead)
                ImGui::SetCursorScreenPos(ImVec2(rowApos.x, rowApos.y + rowH));
                ImGui::Dummy(ImVec2(1.f, 0.1f));

                yOff += rowH;
                visibleIdx++;
            }

            ImGui::Dummy(ImVec2(1.f, 8.f));
            ImGui::EndChild();

            // ── footer ─────────────────────────────────────────────────────
            float fy = wp.y + ws.y - 48.f;
            dl->AddRectFilled(ImVec2(wp.x, fy), ImVec2(wp.x + ws.x, wp.y + ws.y),
                              IM_COL32(12, 12, 15, 255));
            dl->AddLine(ImVec2(wp.x + 18, fy), ImVec2(wp.x + ws.x - 18, fy),
                        IM_COL32(36, 36, 42, 255), 1.f);

            int pulseA = 155 + (int)(45.f * sinf(statusPulse));
            dl->AddText(ImVec2(wp.x + 24, fy + 15),
                        IM_COL32(135, 135, 145, pulseA), statusMsg);

            ImGui::SetCursorScreenPos(ImVec2(wp.x + ws.x - 100, fy + 11));
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1,1,1,0.06f));
            if (ImGui::SmallButton("Lock")) {
                unlocked = false;
                unlockAnim = 0.f;
                snprintf(statusMsg, sizeof(statusMsg), "Locked");
            }
            ImGui::PopStyleColor();
        }

        ImGui::End();
        ImGui::Render();
        const float clear[4] = { 0.035f, 0.035f, 0.042f, 1.f };
        g_ctx->OMSetRenderTargets(1, &g_rtv, nullptr);
        g_ctx->ClearRenderTargetView(g_rtv, clear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_swap->Present(1, 0);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    if (g_rtv)  { g_rtv->Release();  g_rtv  = nullptr; }
    if (g_swap) { g_swap->Release(); g_swap = nullptr; }
    if (g_ctx)  { g_ctx->Release();  g_ctx  = nullptr; }
    if (g_dev)  { g_dev->Release();  g_dev  = nullptr; }
    DestroyWindow(g_hwnd);
    UnregisterClassW(L"CipherAdmin", GetModuleHandle(nullptr));
    return 0;
}
