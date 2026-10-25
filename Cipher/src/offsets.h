#pragma once
#include <cstdint>

// Cipher — Unreal / Fortnite-style external offsets
// All values are FILL placeholders. Re-dump after every client patch.
// Layout matches common GWorld → Level → Actors / camera chains.

namespace off {
    // Globals (image base + RVA)
    inline uintptr_t GWorld              = 0; // FILL
    inline uintptr_t GNames              = 0; // FILL
    inline uintptr_t GObjects            = 0; // FILL
    inline uintptr_t ViewMatrix          = 0; // FILL optional direct matrix

    // UWorld
    inline uintptr_t PersistentLevel     = 0x30;  // FILL
    inline uintptr_t GameInstance        = 0x1B8; // FILL
    inline uintptr_t GameState           = 0x158; // FILL

    // ULevel
    inline uintptr_t Actors              = 0x98;  // FILL TArray
    inline uintptr_t ActorsCount         = 0xA0;  // FILL

    // GameInstance → LocalPlayers
    inline uintptr_t LocalPlayers        = 0x38;  // FILL
    inline uintptr_t PlayerController    = 0x30;  // FILL ULocalPlayer

    // APlayerController
    inline uintptr_t AcknowledgedPawn    = 0x330; // FILL
    inline uintptr_t PlayerCameraManager = 0x340; // FILL

    // Camera
    inline uintptr_t CameraCache         = 0x1AF0; // FILL POV
    inline uintptr_t CameraLocation      = 0x0;    // relative inside POV FILL
    inline uintptr_t CameraRotation      = 0xC;    // FILL
    inline uintptr_t CameraFOV           = 0x18;   // FILL

    // AActor / Pawn
    inline uintptr_t RootComponent       = 0x190; // FILL
    inline uintptr_t RelativeLocation    = 0x128; // FILL USceneComponent
    inline uintptr_t Mesh                = 0x310; // FILL
    inline uintptr_t PlayerState         = 0x2A8; // FILL
    inline uintptr_t TeamIndex           = 0x10E8; // FILL on PlayerState
    inline uintptr_t PawnPrivate         = 0x300; // FILL PlayerState→Pawn
    inline uintptr_t bIsDying            = 0x728; // FILL
    inline uintptr_t bIsDBNO             = 0x802; // FILL downed

    // Skeletal mesh
    inline uintptr_t BoneArray           = 0x5E8; // FILL
    inline uintptr_t BoneArrayCached     = 0x5F8; // FILL alternate
    inline uintptr_t ComponentToWorld    = 0x1C0; // FILL

    // Bone indices (Fortnite-ish defaults — verify)
    namespace bone {
        inline int Head     = 110;
        inline int Neck     = 67;
        inline int Chest    = 66;
        inline int Pelvis   = 2;
        inline int LShoulder= 9;
        inline int LElbow   = 10;
        inline int LHand    = 11;
        inline int RShoulder= 38;
        inline int RElbow   = 39;
        inline int RHand    = 40;
        inline int LHip     = 71;
        inline int LKnee    = 72;
        inline int LFoot    = 73;
        inline int RHip     = 78;
        inline int RKnee    = 79;
        inline int RFoot    = 80;
    }
}
