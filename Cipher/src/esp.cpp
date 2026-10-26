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
    // Real game process only — not the EAC launcher
    auto p = Mem::Attach(L"FortniteClient-Win64-Shipping.exe");
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

uintptr_t EspSystem::ResolveWorld() const {
    if (!proc.base || !proc.handle) return 0;

    // Preferred: GEngine -> GameViewport -> World (no decrypt)
    if (off::GEngine) {
        uintptr_t engine = Mem::ReadPtr(proc.handle, proc.base + off::GEngine);
        if (engine) {
            uintptr_t viewport = Mem::ReadPtr(proc.handle, engine + off::GameViewport);
            if (viewport) {
                uintptr_t world = Mem::ReadPtr(proc.handle, viewport + off::ViewportWorld);
                if (world) return world;
            }
        }
    }

    // Fallback: direct GWorld RVA (may be encrypted on some builds)
    if (off::GWorld) {
        uintptr_t world = Mem::ReadPtr(proc.handle, proc.base + off::GWorld);
        if (world) return world;
    }
    return 0;
}

bool EspSystem::ReadViewMatrix() {
    world_ = ResolveWorld();
    if (!world_) return false;

    uintptr_t gi = Mem::ReadPtr(proc.handle, world_ + off::GameInstance);
    if (!gi) return false;

    // LocalPlayers is a TArray<ULocalPlayer*>
    uintptr_t lpData = Mem::ReadPtr(proc.handle, gi + off::LocalPlayers);
    if (!lpData) return false;
    uintptr_t lp = Mem::ReadPtr(proc.handle, lpData);
    if (!lp) return false;

    uintptr_t pc = Mem::ReadPtr(proc.handle, lp + off::PlayerController);
    if (!pc) return false;
    localPC_ = pc;
    localPawn_ = Mem::ReadPtr(proc.handle, pc + off::AcknowledgedPawn);

    uintptr_t cam = Mem::ReadPtr(proc.handle, pc + off::PlayerCameraManager);
    if (!cam) return false;

    FVector loc{};
    FRotator rot{};
    float fov = 80.f;

    // POV inside CameraCachePrivate
    const uintptr_t cache = cam + off::CameraCache;
    loc = Mem::Read<FVector>(proc.handle, cache + off::POV_Location);
    rot = Mem::Read<FRotator>(proc.handle, cache + off::POV_Rotation);
    fov = Mem::Read<float>(proc.handle, cache + off::POV_FOV);

    // Fallback pointer fields if cache looks empty
    if (loc.X == 0 && loc.Y == 0 && loc.Z == 0 && off::CameraLocationPtr) {
        loc = Mem::Read<FVector>(proc.handle, cam + off::CameraLocationPtr);
        rot = Mem::Read<FRotator>(proc.handle, cam + off::CameraRotationPtr);
    }
    if (fov < 1.f || fov > 170.f) fov = 80.f;

    frame.cameraLoc = loc;
    frame.cameraFov = fov;

    // Optional direct ViewMatrix RVA
    if (off::ViewMatrix && proc.base) {
        if (Mem::ReadRaw(proc.handle, proc.base + off::ViewMatrix, &viewProj_, sizeof(viewProj_)))
            return true;
    }

    float sw = (float)GetSystemMetrics(SM_CXSCREEN);
    float sh = (float)GetSystemMetrics(SM_CYSCREEN);
    float aspect = (sh > 1.f) ? (sw / sh) : (16.f / 9.f);
    viewProj_ = BuildViewProjection(loc, rot, fov, aspect);
    return true;
}

bool EspSystem::ReadBone(uintptr_t mesh, int index, FVector& out) {
    if (!mesh) return false;
    uintptr_t bones = Mem::ReadPtr(proc.handle, mesh + off::BoneArray);
    if (!bones) bones = Mem::ReadPtr(proc.handle, mesh + off::BoneArrayCached);
    if (!bones && off::CurrentBoneArray)
        bones = Mem::ReadPtr(proc.handle, mesh + off::CurrentBoneArray);
    if (!bones) return false;

    const uintptr_t stride = off::BoneStride ? off::BoneStride : 0x60;
    FTransform bt{};
    if (!Mem::ReadRaw(proc.handle, bones + static_cast<uintptr_t>(index) * stride, &bt, sizeof(bt)))
        return false;

    FTransform c2w{};
    if (!Mem::ReadRaw(proc.handle, mesh + off::ComponentToWorld, &c2w, sizeof(c2w)))
        return false;

    out = TransformPoint(c2w, bt.Translation);
    return true;
}

bool EspSystem::EnumPlayers() {
    frame.players.clear();
    if (!world_ && !(world_ = ResolveWorld())) return false;

    const float sw = (float)GetSystemMetrics(SM_CXSCREEN);
    const float sh = (float)GetSystemMetrics(SM_CYSCREEN);
    localTeam_ = -1;

    // Local team from local pawn's player state
    if (localPawn_) {
        uintptr_t lps = Mem::ReadPtr(proc.handle, localPawn_ + off::PlayerState);
        if (lps) localTeam_ = Mem::Read<int>(proc.handle, lps + off::TeamIndex);
    }

    // Preferred: GameState -> PlayerArray (TArray of APlayerState*)
    uintptr_t gs = Mem::ReadPtr(proc.handle, world_ + off::GameState);
    uintptr_t data = 0;
    int count = 0;
    if (gs && off::PlayerArray) {
        data = Mem::ReadPtr(proc.handle, gs + off::PlayerArray);
        count = Mem::Read<int>(proc.handle, gs + off::PlayerArray + 0x8);
    }

    // Fallback: PersistentLevel actors TArray
    if (!data || count <= 0 || count > 200) {
        uintptr_t level = Mem::ReadPtr(proc.handle, world_ + off::PersistentLevel);
        if (!level) return false;
        uintptr_t actorsOff = off::Actors ? off::Actors : off::LevelActors;
        data = Mem::ReadPtr(proc.handle, level + actorsOff);
        count = Mem::Read<int>(proc.handle, level + actorsOff + 0x8);
        if (!data || count <= 0 || count > 5000) return false;

        for (int i = 0; i < count; i++) {
            uintptr_t actor = Mem::ReadPtr(proc.handle, data + static_cast<uintptr_t>(i) * 8);
            if (!actor || actor == localPawn_) continue;
            uintptr_t mesh = Mem::ReadPtr(proc.handle, actor + off::Mesh);
            if (!mesh) continue;
            uintptr_t root = Mem::ReadPtr(proc.handle, actor + off::RootComponent);
            if (!root) continue;
            uintptr_t ps = Mem::ReadPtr(proc.handle, actor + off::PlayerState);
            if (!ps) continue; // filter non-pawns roughly

            FVector loc = Mem::Read<FVector>(proc.handle, root + off::RelativeLocation);
            float dist = frame.cameraLoc.DistTo(loc) / 100.f;
            if (dist <= 0.05f || dist > cfg.maxDistance) continue;

            int team = Mem::Read<int>(proc.handle, ps + off::TeamIndex);
            if (cfg.teamCheck && localTeam_ >= 0 && team == localTeam_) continue;

            bool downed = off::bIsDBNO && Mem::Read<uint8_t>(proc.handle, actor + off::bIsDBNO) != 0;
            if (downed && !cfg.showDowned) continue;

            EspPlayer pl;
            pl.pawn = actor;
            pl.root = loc;
            pl.distance = dist;
            pl.team = team;
            pl.downed = downed;
            pl.name = "Player";
            if (!ReadBone(mesh, off::bone::Head, pl.head))
                pl.head = loc + FVector{ 0, 0, 90.f };

            pl.validScreen = WorldToScreen(pl.root, viewProj_, sw, sh, pl.rootS)
                          && WorldToScreen(pl.head, viewProj_, sw, sh, pl.headS);

            const int boneIdx[EspPlayer::kBones] = {
                off::bone::Head, off::bone::Neck, off::bone::Chest,
                off::bone::LShoulder, off::bone::LElbow, off::bone::LHand,
                off::bone::RShoulder, off::bone::RElbow, off::bone::RHand,
                off::bone::Pelvis,
                off::bone::LHip, off::bone::LKnee, off::bone::LFoot,
                off::bone::RHip, off::bone::RKnee, off::bone::RFoot
            };
            for (int b = 0; b < EspPlayer::kBones; b++) {
                FVector wb;
                if (ReadBone(mesh, boneIdx[b], wb))
                    pl.boneOk[b] = WorldToScreen(wb, viewProj_, sw, sh, pl.bones[b]);
            }
            frame.players.push_back(std::move(pl));
        }
        frame.playerCount = (int)frame.players.size();
        return true;
    }

    // PlayerArray path
    if (count > 128) count = 128;
    for (int i = 0; i < count; i++) {
        uintptr_t ps = Mem::ReadPtr(proc.handle, data + static_cast<uintptr_t>(i) * 8);
        if (!ps) continue;
        uintptr_t pawn = Mem::ReadPtr(proc.handle, ps + off::PawnPrivate);
        if (!pawn || pawn == localPawn_) continue;

        uintptr_t mesh = Mem::ReadPtr(proc.handle, pawn + off::Mesh);
        uintptr_t root = Mem::ReadPtr(proc.handle, pawn + off::RootComponent);
        if (!mesh || !root) continue;

        FVector loc = Mem::Read<FVector>(proc.handle, root + off::RelativeLocation);
        float dist = frame.cameraLoc.DistTo(loc) / 100.f;
        if (dist <= 0.05f || dist > cfg.maxDistance) continue;

        int team = Mem::Read<int>(proc.handle, ps + off::TeamIndex);
        if (cfg.teamCheck && localTeam_ >= 0 && team == localTeam_) continue;

        bool downed = off::bIsDBNO && Mem::Read<uint8_t>(proc.handle, pawn + off::bIsDBNO) != 0;
        if (downed && !cfg.showDowned) continue;

        EspPlayer pl;
        pl.pawn = pawn;
        pl.root = loc;
        pl.distance = dist;
        pl.team = team;
        pl.downed = downed;
        pl.name = "Player";
        if (!ReadBone(mesh, off::bone::Head, pl.head))
            pl.head = loc + FVector{ 0, 0, 90.f };

        pl.validScreen = WorldToScreen(pl.root, viewProj_, sw, sh, pl.rootS)
                      && WorldToScreen(pl.head, viewProj_, sw, sh, pl.headS);

        const int boneIdx[EspPlayer::kBones] = {
            off::bone::Head, off::bone::Neck, off::bone::Chest,
            off::bone::LShoulder, off::bone::LElbow, off::bone::LHand,
            off::bone::RShoulder, off::bone::RElbow, off::bone::RHand,
            off::bone::Pelvis,
            off::bone::LHip, off::bone::LKnee, off::bone::LFoot,
            off::bone::RHip, off::bone::RKnee, off::bone::RFoot
        };
        for (int b = 0; b < EspPlayer::kBones; b++) {
            FVector wb;
            if (ReadBone(mesh, boneIdx[b], wb))
                pl.boneOk[b] = WorldToScreen(wb, viewProj_, sw, sh, pl.bones[b]);
        }
        frame.players.push_back(std::move(pl));
    }
    frame.playerCount = (int)frame.players.size();
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
        // placeholder full bar — wire real HP when offset known
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
        // top-down X/Y (ignore Z); scale to radarRange meters
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
        if (cfg.chams) {
            // External "chams": filled body capsule between head and feet
            ImDrawList* dl = ImGui::GetBackgroundDrawList();
            float top = p.headS.Y, bot = p.rootS.Y;
            if (bot < top) { float t = top; top = bot; bot = t; }
            float h = bot - top; if (h < 4.f) h = 4.f;
            float w = h * 0.42f;
            float cx = (p.headS.X + p.rootS.X) * 0.5f;
            ImU32 c = ImGui::ColorConvertFloat4ToU32(ImVec4(cfg.colors.chams[0], cfg.colors.chams[1], cfg.colors.chams[2], cfg.colors.chams[3]));
            dl->AddRectFilled(ImVec2(cx - w*0.5f, top), ImVec2(cx + w*0.5f, bot), c, 4.f);
        }

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

        // text stack above head
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
