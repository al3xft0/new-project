// Cipher Admin — Xeno soft flow (same monochrome base, real motion)
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

// ─── easing ────────────────────────────────────────────────────────────────
static float EaseOutCubic(float t) {
    float u = 1.f - t;
    return 1.f - u * u * u;
}
static float EaseOutBack(float t) {
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.f;
    return 1.f + c3 * powf(t - 1.f, 3.f) + c1 * powf(t - 1.f, 2.f);
}
static float Lerp(float a, float b, float t) { return a + (b - a) * t; }

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
        if (pt.y >= 0 && pt.y < 52) return HTCAPTION;
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
    if (d < 0) snprintf(buf, n, "expired");
    else if (d == 0) snprintf(buf, n, "today");
    else snprintf(buf, n, "%d days left", d);
}

// ─── soft draw helpers ─────────────────────────────────────────────────────
static void DrawSoftShadow(ImDrawList* dl, ImVec2 a, ImVec2 b, float r, float spread = 8.f) {
    for (int i = 3; i >= 1; --i) {
        float o = spread * (float)i / 3.f;
        int alpha = 12 * i;
        dl->AddRectFilled(ImVec2(a.x - o * 0.3f, a.y + o * 0.5f),
                          ImVec2(b.x + o * 0.3f, b.y + o),
                          IM_COL32(0, 0, 0, alpha), r + o * 0.4f);
    }
}

static void DrawCard(ImDrawList* dl, ImVec2 a, ImVec2 b, float r, float alpha = 1.f) {
    DrawSoftShadow(dl, a, b, r);
    // subtle top highlight
    dl->AddRectFilled(a, b, IM_COL32(22, 22, 26, (int)(255 * alpha)), r);
    dl->AddRect(a, b, IM_COL32(48, 48, 56, (int)(180 * alpha)), r, 0, 1.f);
    // inner top glow line
    dl->AddLine(ImVec2(a.x + r, a.y + 1), ImVec2(b.x - r, a.y + 1),
                IM_COL32(255, 255, 255, (int)(18 * alpha)), 1.f);
}

static void DrawAccentPill(ImDrawList* dl, ImVec2 a, ImVec2 b, ImU32 col, float r = 6.f) {
    dl->AddRectFilled(a, b, (col & 0x00FFFFFF) | 0x28000000, r);
    dl->AddText(ImVec2(a.x + 8, a.y + 3), col, ""); // placeholder, text drawn by caller
}

// simple geometric icons (no font atlas needed)
static void IconKey(ImDrawList* dl, ImVec2 c, float s, ImU32 col) {
    dl->AddCircle(ImVec2(c.x - s * 0.25f, c.y), s * 0.35f, col, 12, 1.6f);
    dl->AddLine(ImVec2(c.x + s * 0.05f, c.y), ImVec2(c.x + s * 0.55f, c.y), col, 1.6f);
    dl->AddLine(ImVec2(c.x + s * 0.35f, c.y), ImVec2(c.x + s * 0.35f, c.y + s * 0.25f), col, 1.4f);
    dl->AddLine(ImVec2(c.x + s * 0.48f, c.y), ImVec2(c.x + s * 0.48f, c.y + s * 0.18f), col, 1.4f);
}

static void IconLock(ImDrawList* dl, ImVec2 c, float s, ImU32 col) {
    dl->AddRect(ImVec2(c.x - s * 0.4f, c.y - s * 0.05f),
                ImVec2(c.x + s * 0.4f, c.y + s * 0.45f), col, 3.f, 0, 1.5f);
    dl->AddBezierCubic(ImVec2(c.x - s * 0.28f, c.y - s * 0.05f),
                       ImVec2(c.x - s * 0.28f, c.y - s * 0.55f),
                       ImVec2(c.x + s * 0.28f, c.y - s * 0.55f),
                       ImVec2(c.x + s * 0.28f, c.y - s * 0.05f), col, 1.5f, 12);
}

static void IconPlus(ImDrawList* dl, ImVec2 c, float s, ImU32 col) {
    dl->AddLine(ImVec2(c.x - s * 0.35f, c.y), ImVec2(c.x + s * 0.35f, c.y), col, 1.8f);
    dl->AddLine(ImVec2(c.x, c.y - s * 0.35f), ImVec2(c.x, c.y + s * 0.35f), col, 1.8f);
}

static void IconCopy(ImDrawList* dl, ImVec2 c, float s, ImU32 col) {
    dl->AddRect(ImVec2(c.x - s * 0.2f, c.y - s * 0.35f),
                ImVec2(c.x + s * 0.35f, c.y + s * 0.2f), col, 2.f, 0, 1.4f);
    dl->AddRect(ImVec2(c.x - s * 0.4f, c.y - s * 0.15f),
                ImVec2(c.x + s * 0.15f, c.y + s * 0.4f), col, 2.f, 0, 1.4f);
}

// ─── main ──────────────────────────────────────────────────────────────────
int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {
    WNDCLASSEXW wc{ sizeof(wc), CS_CLASSDC, WndProc, 0, 0, GetModuleHandle(nullptr),
                    nullptr, nullptr, nullptr, nullptr, L"CipherAdmin", nullptr };
    RegisterClassExW(&wc);

    g_hwnd = CreateWindowExW(WS_EX_APPWINDOW, wc.lpszClassName, L"CIPHER  Admin",
        WS_POPUP | WS_VISIBLE, 100, 100, 720, 560, nullptr, nullptr, wc.hInstance, nullptr);

    BOOL dark = TRUE;
    DwmSetWindowAttribute(g_hwnd, 20, &dark, sizeof(dark)); // DWMWA_USE_IMMERSIVE_DARK_MODE
    MARGINS m{ -1 }; DwmExtendFrameIntoClientArea(g_hwnd, &m);

    if (!CreateDevice(g_hwnd)) return 1;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;

    ImGui::StyleColorsDark();
    ImGuiStyle& st = ImGui::GetStyle();
    st.WindowRounding = 0.f;
    st.ChildRounding  = 10.f;
    st.FrameRounding  = 8.f;
    st.PopupRounding  = 8.f;
    st.ScrollbarRounding = 6.f;
    st.GrabRounding   = 6.f;
    st.WindowBorderSize = 0.f;
    st.FrameBorderSize  = 0.f;
    st.ItemSpacing    = ImVec2(10, 8);
    st.WindowPadding  = ImVec2(0, 0);
    st.Colors[ImGuiCol_WindowBg]      = ImVec4(0.039f, 0.039f, 0.047f, 1.f);
    st.Colors[ImGuiCol_ChildBg]       = ImVec4(0, 0, 0, 0);
    st.Colors[ImGuiCol_FrameBg]       = ImVec4(0.07f, 0.07f, 0.08f, 1.f);
    st.Colors[ImGuiCol_FrameBgHovered]= ImVec4(0.10f, 0.10f, 0.12f, 1.f);
    st.Colors[ImGuiCol_Button]        = ImVec4(1, 1, 1, 0.08f);
    st.Colors[ImGuiCol_ButtonHovered] = ImVec4(1, 1, 1, 0.16f);
    st.Colors[ImGuiCol_ButtonActive]  = ImVec4(1, 1, 1, 0.24f);
    st.Colors[ImGuiCol_Header]        = ImVec4(1, 1, 1, 0.06f);
    st.Colors[ImGuiCol_HeaderHovered] = ImVec4(1, 1, 1, 0.10f);
    st.Colors[ImGuiCol_Text]          = ImVec4(0.92f, 0.92f, 0.94f, 1.f);
    st.Colors[ImGuiCol_TextDisabled]  = ImVec4(0.45f, 0.45f, 0.48f, 1.f);
    st.Colors[ImGuiCol_Border]        = ImVec4(0.18f, 0.18f, 0.20f, 1.f);

    ImGui_ImplWin32_Init(g_hwnd);
    ImGui_ImplDX11_Init(g_dev, g_ctx);

    std::vector<LicenseStore::Record> recs;
    LicenseStore::Load(StorePath(), recs);

    bool  unlocked     = false;
    char  adminPass[64]{};
    char  genNote[128]{};
    char  filterBuf[64]{};
    char  statusMsg[128] = "Locked";
    int   genDays      = 30;
    float unlockShake  = 0.f;
    float unlockAnim   = 0.f;   // 0→1 entrance
    float contentAnim  = 0.f;   // content fade after unlock
    float statusPulse  = 0.f;

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
        if (unlockShake > 0.f) unlockShake = (std::max)(0.f, unlockShake - dt * 3.2f);
        if (!unlocked) {
            unlockAnim = (std::min)(1.f, unlockAnim + dt * 2.4f);
            contentAnim = 0.f;
        } else {
            contentAnim = (std::min)(1.f, contentAnim + dt * 3.0f);
        }
        statusPulse = fmodf(statusPulse + dt * 1.8f, 6.28318f);

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
        dl->AddRectFilled(wp, ImVec2(wp.x + ws.x, wp.y + 52), IM_COL32(14, 14, 17, 255));
        dl->AddLine(ImVec2(wp.x, wp.y + 52), ImVec2(wp.x + ws.x, wp.y + 52),
                    IM_COL32(40, 40, 46, 255), 1.f);

        // logo mark
        IconKey(dl, ImVec2(wp.x + 28, wp.y + 26), 11.f, IM_COL32(220, 220, 230, 255));
        dl->AddText(ImVec2(wp.x + 46, wp.y + 17), IM_COL32(235, 235, 240, 255), "CIPHER");
        dl->AddText(ImVec2(wp.x + 108, wp.y + 17), IM_COL32(110, 110, 120, 255), "Admin");

        // close
        ImGui::SetCursorScreenPos(ImVec2(wp.x + ws.x - 44, wp.y + 12));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1,1,1,0.08f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1,1,1,0.14f));
        if (ImGui::Button("##x", ImVec2(28, 28))) PostQuitMessage(0);
        ImGui::PopStyleColor(3);
        // draw the X
        {
            ImVec2 xc = ImVec2(wp.x + ws.x - 30, wp.y + 26);
            dl->AddLine(ImVec2(xc.x - 6, xc.y - 6), ImVec2(xc.x + 6, xc.y + 6), IM_COL32(180,180,190,255), 1.6f);
            dl->AddLine(ImVec2(xc.x + 6, xc.y - 6), ImVec2(xc.x - 6, xc.y + 6), IM_COL32(180,180,190,255), 1.6f);
        }

        const float pad = 22.f;
        float contentTop = wp.y + 68.f;
        float contentW   = ws.x - pad * 2.f;

        if (!unlocked) {
            // ── unlock card (soft entrance) ────────────────────────────────
            float t = EaseOutBack((std::min)(1.f, unlockAnim));
            float cardW = 380.f, cardH = 240.f;
            float cx = wp.x + (ws.x - cardW) * 0.5f;
            float cy = wp.y + (ws.y - cardH) * 0.42f + (1.f - t) * 40.f;
            if (unlockShake > 0.f)
                cx += sinf(unlockShake * 38.f) * 7.f * unlockShake;

            float a = EaseOutCubic(unlockAnim);
            DrawCard(dl, ImVec2(cx, cy), ImVec2(cx + cardW, cy + cardH), 16.f, a);

            // lock icon
            IconLock(dl, ImVec2(cx + cardW * 0.5f, cy + 38), 16.f,
                     IM_COL32(200, 200, 210, (int)(220 * a)));

            dl->AddText(ImVec2(cx + 28, cy + 70),
                        IM_COL32(235, 235, 240, (int)(255 * a)), "Admin access");
            dl->AddText(ImVec2(cx + 28, cy + 94),
                        IM_COL32(110, 110, 120, (int)(255 * a)),
                        "Enter the admin password to manage keys.");

            ImGui::SetCursorScreenPos(ImVec2(cx + 28, cy + 128));
            ImGui::PushItemWidth(cardW - 56.f);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.f);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.06f, 0.06f, 0.07f, a));
            bool enter = ImGui::InputTextWithHint("##pw", "Password", adminPass, sizeof(adminPass),
                ImGuiInputTextFlags_Password | ImGuiInputTextFlags_EnterReturnsTrue);
            ImGui::PopStyleColor();
            ImGui::PopStyleVar();
            ImGui::PopItemWidth();

            ImGui::SetCursorScreenPos(ImVec2(cx + 28, cy + 174));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.f);
            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(1,1,1, 0.10f * a));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1,1,1, 0.18f * a));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(1,1,1, 0.26f * a));
            if ((ImGui::Button("Unlock", ImVec2(cardW - 56.f, 38.f)) || enter) && a > 0.9f) {
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

            // status under card
            float col[4] = { 0.55f, 0.55f, 0.58f, a };
            if (strstr(statusMsg, "Wrong")) { col[0]=0.95f; col[1]=0.32f; col[2]=0.34f; }
            ImVec2 ss = ImGui::CalcTextSize(statusMsg);
            dl->AddText(ImVec2(cx + (cardW - ss.x) * 0.5f, cy + cardH + 18.f),
                        IM_COL32((int)(col[0]*255),(int)(col[1]*255),(int)(col[2]*255),(int)(255*a)), statusMsg);
        } else {
            float ca = EaseOutCubic(contentAnim);

            // ── generate card ──────────────────────────────────────────────
            float genH = 112.f;
            ImVec2 ga(wp.x + pad, contentTop);
            ImVec2 gb(wp.x + pad + contentW, contentTop + genH);
            DrawCard(dl, ga, gb, 14.f, ca);

            IconPlus(dl, ImVec2(ga.x + 26, ga.y + 24), 9.f, IM_COL32(200, 200, 210, (int)(230 * ca)));
            dl->AddText(ImVec2(ga.x + 42, ga.y + 16),
                        IM_COL32(235, 235, 240, (int)(255 * ca)), "Generate key");

            ImGui::SetCursorScreenPos(ImVec2(ga.x + 20, ga.y + 48));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.f);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.06f, 0.06f, 0.07f, ca));

            ImGui::TextDisabled("Days");
            ImGui::SameLine(0, 8);
            ImGui::SetNextItemWidth(68);
            ImGui::InputInt("##days", &genDays, 0, 0);
            if (genDays < 1) genDays = 1;
            if (genDays > 3650) genDays = 3650;

            ImGui::SameLine(0, 14);
            ImGui::SetNextItemWidth(contentW - 300.f);
            ImGui::InputTextWithHint("##note", "Note (optional)", genNote, sizeof(genNote));

            ImGui::SameLine(0, 12);
            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(1,1,1, 0.12f * ca));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1,1,1, 0.22f * ca));
            if (ImGui::Button("Generate", ImVec2(108, 0))) {
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

            // ── key list card ──────────────────────────────────────────────
            float listTop = contentTop + genH + 14.f;
            float listH   = ws.y - (listTop - wp.y) - 58.f;
            if (listH < 140.f) listH = 140.f;
            ImVec2 la(wp.x + pad, listTop);
            ImVec2 lb(wp.x + pad + contentW, listTop + listH);
            DrawCard(dl, la, lb, 14.f, ca);

            IconKey(dl, ImVec2(la.x + 24, la.y + 22), 9.f, IM_COL32(200, 200, 210, (int)(230 * ca)));
            dl->AddText(ImVec2(la.x + 40, la.y + 14),
                        IM_COL32(235, 235, 240, (int)(255 * ca)), "License keys");

            ImGui::SetCursorScreenPos(ImVec2(la.x + contentW - 200, la.y + 12));
            ImGui::PushItemWidth(172);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.f);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.06f, 0.06f, 0.07f, ca));
            ImGui::InputTextWithHint("##filter", "Filter…", filterBuf, sizeof(filterBuf));
            ImGui::PopStyleColor();
            ImGui::PopStyleVar();
            ImGui::PopItemWidth();

            // child for the list — CRITICAL: Dummy grows content size so SetCursorScreenPos is legal
            ImGui::SetCursorScreenPos(ImVec2(la.x + 12, la.y + 44));
            ImGui::BeginChild("##keylist", ImVec2(contentW - 24.f, listH - 56.f), false,
                              ImGuiWindowFlags_NoBackground);

            ImDrawList* cdl = ImGui::GetWindowDrawList();
            ImVec2 childPos = ImGui::GetCursorScreenPos();
            float  rowH     = 78.f;
            float  yOff     = 0.f;
            int    shown    = 0;

            if (recs.empty()) {
                ImGui::TextDisabled("No keys yet — generate one above.");
            }

            for (size_t i = 0; i < recs.size(); ++i) {
                auto& r = recs[i];
                if (filterBuf[0] && r.key.find(filterBuf) == std::string::npos &&
                    r.info.note.find(filterBuf) == std::string::npos)
                    continue;

                float rowY = childPos.y + yOff;
                ImVec2 rowA(childPos.x, rowY);
                ImVec2 rowB(childPos.x + contentW - 40.f, rowY + rowH - 8.f);

                // hover lift
                bool hovered = ImGui::IsMouseHoveringRect(rowA, rowB);
                float lift = hovered ? 1.f : 0.f;
                ImU32 rowBg = hovered ? IM_COL32(32, 32, 38, 220) : IM_COL32(24, 24, 28, 180);
                cdl->AddRectFilled(rowA, rowB, rowBg, 10.f);
                cdl->AddRect(rowA, rowB, IM_COL32(48, 48, 54, 160), 10.f, 0, 1.f);

                // status pill
                License::Status st = r.info.status;
                const char* lab = "Valid";
                ImU32 pillCol = IM_COL32(80, 200, 120, 255);
                if (st == License::Status::Revoked) {
                    lab = "Revoked";
                    pillCol = IM_COL32(220, 80, 90, 255);
                } else if (r.info.expiresUnix < (uint32_t)time(nullptr)) {
                    lab = "Expired";
                    pillCol = IM_COL32(200, 160, 60, 255);
                }
                ImVec2 pillA(rowA.x + 12, rowA.y + 10);
                ImVec2 pillSz = ImGui::CalcTextSize(lab);
                ImVec2 pillB(pillA.x + pillSz.x + 16, pillA.y + 18);
                cdl->AddRectFilled(pillA, pillB, (pillCol & 0x00FFFFFF) | 0x30000000, 6.f);
                cdl->AddText(ImVec2(pillA.x + 8, pillA.y + 2), pillCol, lab);

                // meta
                char meta[128], expBuf[32], leftBuf[32];
                FormatExpiry(r.info.expiresUnix, expBuf, sizeof(expBuf));
                DaysLeft(r.info.expiresUnix, leftBuf, sizeof(leftBuf));
                snprintf(meta, sizeof(meta), "id %u  ·  %u days  ·  %s  ·  %s",
                         r.info.id, r.info.durationDays, leftBuf, expBuf);
                cdl->AddText(ImVec2(pillB.x + 12, rowA.y + 12),
                             IM_COL32(130, 130, 140, 255), meta);

                // key string
                ImGui::SetCursorScreenPos(ImVec2(rowA.x + 12, rowA.y + 36));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.88f, 0.88f, 0.90f, 1.f));
                ImGui::TextUnformatted(r.key.c_str());
                ImGui::PopStyleColor();

                // actions
                ImGui::SetCursorScreenPos(ImVec2(rowA.x + (contentW - 40.f) - 148, rowA.y + 10));
                if (ImGui::SmallButton("Copy"))
                    ImGui::SetClipboardText(r.key.c_str());
                ImGui::SameLine();
                if (st != License::Status::Revoked) {
                    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.85f, 0.25f, 0.28f, 0.22f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.25f, 0.28f, 0.40f));
                    if (ImGui::SmallButton("Revoke")) {
                        recs[i].info.status = License::Status::Revoked;
                        LicenseStore::Save(StorePath(), recs);
                        snprintf(statusMsg, sizeof(statusMsg), "Revoked id %u", r.info.id);
                    }
                    ImGui::PopStyleColor(2);
                }

                if (!r.info.note.empty()) {
                    ImGui::SetCursorScreenPos(ImVec2(rowA.x + 12, rowA.y + 54));
                    ImGui::TextDisabled("%s", r.info.note.c_str());
                }

                // *** THIS is the assert kill ***
                // Advance the content region so the next SetCursorScreenPos is legal
                ImGui::SetCursorScreenPos(ImVec2(rowA.x, rowA.y + rowH));
                ImGui::Dummy(ImVec2(1.f, 0.1f));   // grows the child content size

                yOff += rowH;
                shown++;
            }

            // final spacer so scroll feels natural
            ImGui::Dummy(ImVec2(1.f, 8.f));
            ImGui::EndChild();

            // ── footer ─────────────────────────────────────────────────────
            float fy = wp.y + ws.y - 46.f;
            dl->AddRectFilled(ImVec2(wp.x, fy), ImVec2(wp.x + ws.x, wp.y + ws.y),
                              IM_COL32(12, 12, 15, 255));
            dl->AddLine(ImVec2(wp.x + 16, fy), ImVec2(wp.x + ws.x - 16, fy),
                        IM_COL32(38, 38, 44, 255), 1.f);

            // soft pulse on status
            int pulseA = 160 + (int)(40.f * sinf(statusPulse));
            dl->AddText(ImVec2(wp.x + 22, fy + 14),
                        IM_COL32(140, 140, 150, pulseA), statusMsg);

            ImGui::SetCursorScreenPos(ImVec2(wp.x + ws.x - 96, fy + 10));
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
        const float clear[4] = { 0.039f, 0.039f, 0.047f, 1.f };
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
