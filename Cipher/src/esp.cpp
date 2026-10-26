#include "esp.h"
#include "imgui.h"
#include <algorithm>
#include <cstdio>

// Skeleton pairs into bone array indices we fill
static const int kSkelPairs[][2] = {
    {0, 1}, {1, 2}, // head-neck-chest
    {2, 3}, {3, 4}, {4, 5}, // L arm
    {2, 6}, {6, 7}, {7, 8}, // R arm
    {2, 9}, // chest-pelvis
    {9, 10}, {10, 11}, {11, 12}, // L leg
    {9, 13}, {13, 14}, {14, 15}, // R leg
};

static ImU32 Col4(const float c[4]) {
    return ImGui::ColorConvertFloat4ToU32(ImVec4(c[0], c[1], c[2], c[3]));
}

static ImU32 Darken(ImU32 c) {
    return IM_COL32(0, 0, 0, (c >> 24) & 0xFF);
}

bool EspSystem::Attach() {
    auto p = Mem::Attach(L"FortniteClient-Win64-Shipping.exe");
    if (!p) {
        // also try shipping without path variants
        p = Mem::Attach(L"FortniteClient-Win64-Shipping_EAC_EOS.exe");
    }
    if (!p) return false;
    std::lock_guard lock(mu_);
    if (proc.handle) Mem::Close(proc);
    proc = *p;
    return true;
}

void EspSystem::Detach() {
    std::lock_guard lock(mu_);
    Mem::Close(proc);
    frame = {};
}

bool EspSystem::Attached() const {
    std::lock_guard lock(mu_);
    return Mem::IsValid(proc);
}

bool EspSystem::ReadViewMatrix() {
    // Prefer explicit ViewMatrix RVA if set; else build from camera cache
    if (Off::ViewMatrix && proc.base) {
        return Mem::ReadRaw(proc.handle, proc.base + Off::ViewMatrix, &viewProj_, sizeof(viewProj_));
    }
    // Camera chain: GWorld → GameInstance → LocalPlayers[0] → PlayerController → CameraManager → Cache
    if (!Off::GWorld || !proc.base) return false;
    uintptr_t world = Mem::ReadPtr(proc.handle, proc.base + Off::GWorld);
    if (!world) return false;
    uintptr_t gi = Mem::ReadPtr(proc.handle, world + Off::GameInstance);
    if (!gi) return false;
    uintptr_t lpArr = Mem::ReadPtr(proc.handle, gi + Off::LocalPlayers);
    if (!lpArr) return false;
    uintptr_t lp = Mem::ReadPtr(proc.handle, lpArr); // TArray data[0]
    if (!lp) return false;
    uintptr_t pc = Mem::ReadPtr(proc.handle, lp + Off::PlayerController);
    if (!pc) return false;
    localPawn_ = Mem::ReadPtr(proc.handle, pc + Off::AcknowledgedPawn);
    uintptr_t cam = Mem::ReadPtr(proc.handle, pc + Off::PlayerCameraManager);
    if (!cam) return false;

    // Read POV location / rotation / FOV — layout FILL
    FVector loc = Mem::Read<FVector>(proc.handle, cam + Off::CameraCache + Off::CameraLocation);
    // Minimal projection: if full matrix unavailable, identity-ish fails open
    frame.cameraLoc = loc;
    float fov = Mem::Read<float>(proc.handle, cam + Off::CameraCache + Off::CameraFOV);
    if (fov > 1.f && fov < 170.f) frame.cameraFov = fov;

    // Without a real view-projection matrix from the game, W2S will be wrong.
    // Callers must fill Off::ViewMatrix or extend this to build VP from rotation+FOV.
    (void)viewProj_;
    return Off::ViewMatrix != 0;
}

bool EspSystem::ReadBone(uintptr_t mesh, int index, FVector& out) {
    if (!mesh) return false;
    uintptr_t bones = Mem::ReadPtr(proc.handle, mesh + Off::BoneArray);
    if (!bones) bones = Mem::ReadPtr(proc.handle, mesh + Off::BoneArrayCached);
    if (!bones) return false;
    // FTransform array
    FTransform bt{};
    if (!Mem::ReadRaw(proc.handle, bones + static_cast<uintptr_t>(index) * sizeof(FTransform), &bt, sizeof(bt)))
        return false;
    FTransform c2w{};
    if (!Mem::ReadRaw(proc.handle, mesh + Off::ComponentToWorld, &c2w, sizeof(c2w)))
        return false;
    out = TransformPoint(c2w, bt.Translation);
    return true;
}

bool EspSystem::EnumPlayers() {
    frame.players.clear();
    if (!Off::GWorld || !proc.base) return false;

    uintptr_t world = Mem::ReadPtr(proc.handle, proc.base + Off::GWorld);
    if (!world) return false;
    uintptr_t level = Mem::ReadPtr(proc.handle, world + Off::PersistentLevel);
    if (!level) return false;
    uintptr_t actors = Mem::ReadPtr(proc.handle, level + Off::Actors);
    int count = Mem::Read<int>(proc.handle, level + Off::ActorsCount);
    if (!actors || count <= 0 || count > 20000) return false;

    const float sw = ImGui::GetIO().DisplaySize.x;
    const float sh = ImGui::GetIO().DisplaySize.y;

    // local team from local pawn playerstate
    localTeam_ = -1;
    if (localPawn_) {
        uintptr_t ps = Mem::ReadPtr(proc.handle, localPawn_ + Off::PlayerState);
        if (ps) localTeam_ = Mem::Read<int>(proc.handle, ps + Off::TeamIndex);
    }

    int cap = (std::min)(count, 4096);
    for (int i = 0; i < cap; i++) {
        uintptr_t actor = Mem::ReadPtr(proc.handle, actors + static_cast<uintptr_t>(i) * 8);
        if (!actor || actor == localPawn_) {
            if (!cfg.showSelf && actor == localPawn_) continue;
            if (!actor) continue;
        }

        uintptr_t mesh = Mem::ReadPtr(proc.handle, actor + Off::Mesh);
        uintptr_t root = Mem::ReadPtr(proc.handle, actor + Off::RootComponent);
        if (!mesh || !root) continue;

        FVector loc = Mem::Read<FVector>(proc.handle, root + Off::RelativeLocation);
        float dist = frame.cameraLoc.DistTo(loc) / 100.f; // cm → m approx
        if (dist <= 0.1f || dist > cfg.maxDistance) continue;

        uintptr_t ps = Mem::ReadPtr(proc.handle, actor + Off::PlayerState);
        int team = ps ? Mem::Read<int>(proc.handle, ps + Off::TeamIndex) : -1;
        if (cfg.teamCheck && localTeam_ >= 0 && team == localTeam_ && actor != localPawn_)
            continue;

        bool downed = false;
        if (Off::bIsDBNO)
            downed = Mem::Read<uint8_t>(proc.handle, actor + Off::bIsDBNO) != 0;
        if (downed && !cfg.showDowned) continue;

        EspPlayer pl;
        pl.pawn = actor;
        pl.root = loc;
        pl.distance = dist;
        pl.team = team;
        pl.downed = downed;
        pl.name = "Player";

        // head bone
        if (!ReadBone(mesh, Off::Head, pl.head))
            pl.head = loc + FVector{ 0, 0, 90.f };

        pl.validScreen = WorldToScreen(pl.root, viewProj_, sw, sh, pl.rootS)
                      && WorldToScreen(pl.head, viewProj_, sw, sh, pl.headS);

        // skeleton bones
        const int boneIdx[EspPlayer::kBones] = {
            Off::Head, Off::Neck, Off::Chest,
            Off::LShoulder, Off::LElbow, Off::LHand,
            Off::RShoulder, Off::RElbow, Off::RHand,
            Off::Pelvis,
            Off::LHip, Off::LKnee, Off::LFoot,
            Off::RHip, Off::RKnee, Off::RFoot
        };
        for (int b = 0; b < EspPlayer::kBones; b++) {
            FVector wb;
            if (ReadBone(mesh, boneIdx[b], wb)) {
                pl.boneOk[b] = WorldToScreen(wb, viewProj_, sw, sh, pl.bones[b]);
            }
        }

        frame.players.push_back(std::move(pl));
    }
    frame.playerCount = static_cast<int>(frame.players.size());
    return true;
}

void EspSystem::Tick() {
    if (!cfg.enabled) {
        frame.players.clear();
        return;
    }
    std::lock_guard lock(mu_);
    if (!Mem::IsValid(proc)) {
        frame.attached = false;
        frame.players.clear();
        return;
    }
    frame.attached = true;

    if (proc.base) {
        uintptr_t uworld = Decrypt::GWorld(proc.base, proc.handle);
        if (uworld) {
            uintptr_t gi = Mem::ReadPtr(proc.handle, uworld + Off::GameInstance);
            uintptr_t lpArr = Mem::ReadPtr(proc.handle, gi + Off::LocalPlayers);
            uintptr_t lp = Mem::ReadPtr(proc.handle, lpArr);
            uintptr_t pc = Mem::ReadPtr(proc.handle, lp + Off::PlayerController);
            localPawn_ = Mem::ReadPtr(proc.handle, pc + Off::AcknowledgedPawn);
        }
    }

    ReadViewMatrix();
    EnumPlayers();
}

void EspSystem::DrawBox(const EspPlayer& p, ImU32 col) {
    if (!p.validScreen) return;
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    float top = p.headS.Y;
    float bot = p.rootS.Y;
    if (bot < top) std::swap(bot, top);
    float h = bot - top;
    if (h < 4.f) return;
    float w = h * 0.45f;
    float cx = (p.headS.X + p.rootS.X) * 0.5f;
    float l = cx - w * 0.5f;
    float r = cx + w * 0.5f;
    float th = cfg.boxThickness;

    auto line = [&](float x1, float y1, float x2, float y2, ImU32 c, float t) {
        if (cfg.outline)
            dl->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), IM_COL32(0, 0, 0, 200), t + 2.f);
        dl->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), c, t);
    };

    if (cfg.boxStyle == BoxStyle::Filled) {
        dl->AddRectFilled(ImVec2(l, top), ImVec2(r, bot), Col4(cfg.colors.filledBox));
        dl->AddRect(ImVec2(l, top), ImVec2(r, bot), col, 0.f, 0, th);
    } else if (cfg.boxStyle == BoxStyle::Corner) {
        float cl = w * cfg.cornerLen;
        float ch = h * cfg.cornerLen;
        // TL
        line(l, top, l + cl, top, col, th); line(l, top, l, top + ch, col, th);
        // TR
        line(r, top, r - cl, top, col, th); line(r, top, r, top + ch, col, th);
        // BL
        line(l, bot, l + cl, bot, col, th); line(l, bot, l, bot - ch, col, th);
        // BR
        line(r, bot, r - cl, bot, col, th); line(r, bot, r, bot - ch, col, th);
    } else {
        if (cfg.outline)
            dl->AddRect(ImVec2(l, top), ImVec2(r, bot), IM_COL32(0, 0, 0, 200), 0.f, 0, th + 2.f);
        dl->AddRect(ImVec2(l, top), ImVec2(r, bot), col, 0.f, 0, th);
    }

    if (cfg.healthBar) {
        float bx = l - 5.f;
        dl->AddRectFilled(ImVec2(bx - 2, top), ImVec2(bx + 2, bot), IM_COL32(0, 0, 0, 160));
        dl->AddRectFilled(ImVec2(bx - 2, top), ImVec2(bx + 2, bot), IM_COL32(255, 255, 255, 200));
    }
}

void EspSystem::DrawSkeleton(const EspPlayer& p) {
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    ImU32 col = Col4(cfg.colors.skeleton);
    float th = cfg.skelThickness;
    for (auto& pr : kSkelPairs) {
        int a = pr[0], b = pr[1];
        if (!p.boneOk[a] || !p.boneOk[b]) continue;
        if (cfg.outline)
            dl->AddLine(ImVec2(p.bones[a].X, p.bones[a].Y), ImVec2(p.bones[b].X, p.bones[b].Y),
                        IM_COL32(0, 0, 0, 180), th + 2.f);
        dl->AddLine(ImVec2(p.bones[a].X, p.bones[a].Y), ImVec2(p.bones[b].X, p.bones[b].Y), col, th);
    }
}

void EspSystem::DrawRadar() {
    if (!cfg.radar) return;
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    const float sw = ImGui::GetIO().DisplaySize.x;
    const float sh = ImGui::GetIO().DisplaySize.y;
    float sz = cfg.radarSize;
    float pad = 16.f;
    ImVec2 origin;
    switch (cfg.radarPos) {
    case RadarPos::TopLeft:     origin = { pad, pad }; break;
    case RadarPos::TopRight:    origin = { sw - sz - pad, pad }; break;
    case RadarPos::BottomLeft:  origin = { pad, sh - sz - pad }; break;
    default:                    origin = { sw - sz - pad, sh - sz - pad }; break;
    }
    ImVec2 center{ origin.x + sz * 0.5f, origin.y + sz * 0.5f };
    dl->AddRectFilled(origin, ImVec2(origin.x + sz, origin.y + sz), IM_COL32(0, 0, 0, 140), 8.f);
    dl->AddRect(origin, ImVec2(origin.x + sz, origin.y + sz), IM_COL32(255, 255, 255, 40), 8.f);
    dl->AddCircleFilled(center, 3.f, IM_COL32(255, 255, 255, 220));
    dl->AddLine(ImVec2(center.x, origin.y + 8), ImVec2(center.x, origin.y + sz - 8), IM_COL32(255, 255, 255, 25));
    dl->AddLine(ImVec2(origin.x + 8, center.y), ImVec2(origin.x + sz - 8, center.y), IM_COL32(255, 255, 255, 25));

    for (auto& p : frame.players) {
        FVector d = p.root - frame.cameraLoc;
        float rx = d.X / 100.f / cfg.radarRange;
        float ry = d.Y / 100.f / cfg.radarRange;
        if (rx * rx + ry * ry > 1.f) continue;
        ImVec2 pt{ center.x + rx * (sz * 0.45f), center.y + ry * (sz * 0.45f) };
        dl->AddCircleFilled(pt, 3.f, Col4(cfg.colors.radar));
    }
}

void EspSystem::DrawOverlay() {
    if (!cfg.enabled) return;
    std::lock_guard lock(mu_);
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    const float sw = ImGui::GetIO().DisplaySize.x;
    const float sh = ImGui::GetIO().DisplaySize.y;

    for (auto& p : frame.players) {
        if (!p.validScreen) continue;

        ImU32 boxCol = p.downed ? Col4(cfg.colors.boxDowned)
                     : (p.visible ? Col4(cfg.colors.boxVisible) : Col4(cfg.colors.boxInvisible));

        if (cfg.box) DrawBox(p, boxCol);
        if (cfg.skeleton) DrawSkeleton(p);

        if (cfg.headDot && p.boneOk[0]) {
            float r = cfg.headDotSize;
            if (cfg.outline)
                dl->AddCircleFilled(ImVec2(p.bones[0].X, p.bones[0].Y), r + 1.2f, IM_COL32(0, 0, 0, 200));
            dl->AddCircleFilled(ImVec2(p.bones[0].X, p.bones[0].Y), r, Col4(cfg.colors.headDot));
        }

        if (cfg.snaplines && cfg.snapPos != SnapPos::Off) {
            ImVec2 from;
            switch (cfg.snapPos) {
            case SnapPos::Top:    from = { sw * 0.5f, 0.f }; break;
            case SnapPos::Center: from = { sw * 0.5f, sh * 0.5f }; break;
            default:              from = { sw * 0.5f, sh }; break;
            }
            ImVec2 to{ (p.headS.X + p.rootS.X) * 0.5f, p.rootS.Y };
            if (cfg.outline)
                dl->AddLine(from, to, IM_COL32(0, 0, 0, 120), 2.5f);
            dl->AddLine(from, to, Col4(cfg.colors.snapline), 1.2f);
        }

        float ty = p.headS.Y - 16.f;
        float cx = (p.headS.X + p.rootS.X) * 0.5f;
        if (cfg.name) {
            const char* n = p.name.c_str();
            ImVec2 ts = ImGui::CalcTextSize(n);
            ImVec2 pos{ cx - ts.x * 0.5f, ty - ts.y };
            if (cfg.outline) {
                for (int ox = -1; ox <= 1; ox++)
                    for (int oy = -1; oy <= 1; oy++)
                        if (ox || oy)
                            dl->AddText(ImVec2(pos.x + ox, pos.y + oy), IM_COL32(0, 0, 0, 200), n);
            }
            dl->AddText(pos, Col4(cfg.colors.name), n);
            ty = pos.y;
        }
        if (cfg.distance) {
            char buf[32];
            snprintf(buf, sizeof(buf), "%.0fm", p.distance);
            ImVec2 ts = ImGui::CalcTextSize(buf);
            ImVec2 pos{ cx - ts.x * 0.5f, p.rootS.Y + 4.f };
            if (cfg.outline) {
                for (int ox = -1; ox <= 1; ox++)
                    for (int oy = -1; oy <= 1; oy++)
                        if (ox || oy)
                            dl->AddText(ImVec2(pos.x + ox, pos.y + oy), IM_COL32(0, 0, 0, 180), buf);
            }
            dl->AddText(pos, Col4(cfg.colors.distance), buf);
        }
    }

    DrawRadar();
}
