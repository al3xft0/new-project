#pragma once
#include <cstdint>

namespace Off {
        // Bone indices (common Fortnite / UE skeletal mesh)
    constexpr int Head       = 68;   // or 110 on some meshes — start with 68
    constexpr int Neck       = 67;
    constexpr int Chest      = 66;   // / spine_03
    constexpr int Pelvis     = 2;

    constexpr int LShoulder  = 9;
    constexpr int LElbow     = 10;
    constexpr int LHand      = 11;

    constexpr int RShoulder  = 38;
    constexpr int RElbow     = 39;
    constexpr int RHand      = 40;

    constexpr int LHip       = 71;
    constexpr int LKnee      = 72;
    constexpr int LFoot      = 73;

    constexpr int RHip       = 78;
    constexpr int RKnee      = 79;
    constexpr int RFoot      = 80;

    // Mesh / bone array
    constexpr uint64_t Mesh              = 0x328;   // adjust if your SDK differs
    constexpr uint64_t BoneArray         = 0x5E8;   // or 0x5F0 / 0x600 — common range
    constexpr uint64_t BoneArrayCached   = 0x5F8;
    constexpr uint64_t ComponentToWorld  = 0x1E0;
    constexpr uint64_t RootComponent     = 0x1B0;
    constexpr uint64_t RelativeLocation  = 0x140;
    constexpr uint64_t PlayerState       = 0x2B0;
    constexpr uint64_t TeamIndex         = 0x10E8;
    constexpr uint64_t bIsDBNO           = 0x841;   // example bitfield — verify
    constexpr uint64_t Actors            = 0x98;    // Level actors TArray
    constexpr uint64_t ActorsCount       = 0xA0;
    constexpr uint64_t PersistentLevel   = 0x40;
    constexpr uint64_t GameInstance      = 0x248;
    constexpr uint64_t LocalPlayers      = 0x38;
    constexpr uint64_t PlayerController  = 0x30;
    constexpr uint64_t AcknowledgedPawn  = 0x350;
    constexpr uint64_t PlayerCameraManager = 0x360;
    constexpr uint64_t CameraCache       = 0x1510;
    constexpr uint64_t CameraLocation    = 0x0;     // inside POV
    constexpr uint64_t CameraFOV         = 0x30;
    constexpr uint64_t ViewMatrix        = 0x0;     // fill if you have a direct RVA
    constexpr uint64_t GWorld            = 0x1B096DE8;
}
