#pragma once
#include "cipher_math.h"
#include "memory.h"
#include "offsets.h"
#include "imgui.h"
#include <string>
#include <vector>
#include <mutex>
#include <atomic>

struct EspColors {
    float boxVisible[4]     = { 1.f, 1.f, 1.f, 0.95f };
    float boxInvisible[4]   = { 0.55f, 0.55f, 0.55f, 0.85f };
    float boxDowned[4]      = { 0.85f, 0.35f, 0.35f, 0.95f };
    float skeleton[4]       = { 1.f, 1.f, 1.f, 0.90f };
    float name[4]           = { 1.f, 1.f, 1.f, 0.95f };
    float distance[4]       = { 0.75f, 0.75f, 0.78f, 0.90f };
    float snapline[4]       = { 1.f, 1.f, 1.f, 0.35f };
    float headDot[4]        = { 1.f, 1.f, 1.f, 1.00f };
    float filledBox[4]      = { 1.f, 1.f, 1.f, 0.08f };
    float radar[4]          = { 1.f, 1.f, 1.f, 0.80f };
};

enum class BoxStyle : int { Full = 0, Corner = 1, Filled = 2 };
enum class SnapPos  : int { Off = 0, Top = 1, Center = 2, Bottom = 3 };
enum class RadarPos : int { TopLeft = 0, TopRight = 1, BottomLeft = 2, BottomRight = 3 };

struct EspConfig {
    bool enabled = true, box = true, skeleton = true, name = true, distance = true;
    bool headDot = true, snaplines = false, teamCheck = true, showDowned = true;
    bool showSelf = false, visibleOnly = false, radar = false, outline = true, healthBar = true;
    BoxStyle boxStyle = BoxStyle::Corner;
    SnapPos  snapPos  = SnapPos::Bottom;
    RadarPos radarPos = RadarPos::TopRight;
    float maxDistance = 350.f, boxThickness = 1.4f, skelThickness = 1.2f;
    float cornerLen = 0.25f, headDotSize = 3.0f, radarSize = 140.f, radarRange = 120.f, textSize = 14.f;
    EspColors colors;
};

struct EspPlayer {
    uintptr_t pawn = 0;
    FVector root{}, head{};
    FVector2D rootS{}, headS{};
    float distance = 0;
    int team = -1;
    bool downed = false, visible = true, validScreen = false;
    std::string name;
    static constexpr int kBones = 16;
    FVector2D bones[kBones]{};
    bool boneOk[kBones]{};
};

struct EspFrame {
    std::vector<EspPlayer> players;
    FVector cameraLoc{};
    float cameraFov = 80.f;
    bool attached = false;
    int playerCount = 0;
};

class EspSystem {
public:
    EspConfig cfg;
    EspFrame  frame;
    GameProcess proc;
    bool Attach();
    void Detach();
    bool Attached() const;
    void Tick();
    void DrawOverlay();
private:
    mutable std::mutex mu_;
    FMatrix viewProj_{};
    int localTeam_ = -1;
    uintptr_t localPawn_ = 0;
    bool ReadViewMatrix();
    bool EnumPlayers();
    bool ReadBone(uintptr_t mesh, int index, FVector& out);
    void DrawBox(const EspPlayer& p, ImU32 col);
    void DrawSkeleton(const EspPlayer& p);
    void DrawRadar();
};
