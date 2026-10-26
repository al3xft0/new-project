#pragma once
#include <cstdint>

// Generated / merged from FortniteClient-Win64-Shipping_20-10 SDK dump (beardumper)
// Re-verify after every client patch.

namespace off {
    // Globals (RVA)
    inline uintptr_t GNames              = 0x1AF4CDC0;
    inline uintptr_t GObjectsChunks      = 0x1B028040;
    inline uintptr_t GObjectsNum         = 0x1B028054;
    inline uintptr_t GWorld              = 0x1B096DE8;
    inline uintptr_t ViewMatrix          = 0; // optional direct; else camera chain

    // UWorld (Offsets::World)
    inline uintptr_t PersistentLevel     = 0x40;
    inline uintptr_t GameInstance        = 0x248; // OwningGameInstance
    inline uintptr_t GameState           = 0x1D0;

    // ULevel
    inline uintptr_t Actors              = 0x98;
    inline uintptr_t ActorsCount         = 0xA0;

    // GameInstance / LocalPlayer
    inline uintptr_t LocalPlayers        = 0x38;
    inline uintptr_t PlayerController    = 0x30;

    // APlayerController
    inline uintptr_t AcknowledgedPawn    = 0x318;
    inline uintptr_t PlayerCameraManager = 0x328;

    // PlayerCameraManager
    inline uintptr_t CameraCache         = 0x1590; // CameraCachePrivate
    inline uintptr_t CameraLocation      = 0x0;
    inline uintptr_t CameraRotation      = 0xC;
    inline uintptr_t CameraFOV           = 0x18;
    inline uintptr_t DefaultFOV          = 0x284;

    // AActor / Character
    inline uintptr_t RootComponent       = 0x1B0;
    inline uintptr_t RelativeLocation    = 0x140;
    inline uintptr_t Mesh                = 0x2F0;  // Character Mesh
    inline uintptr_t PlayerState         = 0x2B0;
    inline uintptr_t CustomTimeDilation  = 0x68;   // AActor — verify if used

    // Fort / Athena
    inline uintptr_t TeamIndex           = 0x10E8; // verify on FortPlayerStateAthena
    inline uintptr_t bIsDBNO             = 0x841;
    inline uintptr_t CurrentWeapon       = 0x9D0;  // FortPlayerPawn / Athena
    inline uintptr_t CurrentWeaponList   = 0xA08;

    // Skeletal mesh / bones
    inline uintptr_t BoneArray           = 0x5E8;
    inline uintptr_t BoneArrayCached     = 0x5F8;
    inline uintptr_t ComponentToWorld    = 0x1E0;

    // FortWeapon / ranged
    inline uintptr_t WeaponData          = 0x648;
    inline uintptr_t WeaponRecoilOffset  = 0x2358; // FVector or similar on weapon
    inline uintptr_t RecoilProcessingShotCounter = 0x25F4;
    inline uintptr_t BulletCountForPerBulletRecoil = 0x25F0;
    inline uintptr_t TimeRemainingForBulletSpread = 0x2600;
    inline uintptr_t BulletCountForPerBulletSpread = 0x25FC;

    // FortHealthSet (if ability system path resolved)
    inline uintptr_t Health              = 0x48;
    inline uintptr_t MaxHealth           = 0x70;
    inline uintptr_t Shield              = 0x98;
    inline uintptr_t CurrentShield       = 0xC0;

    // CharacterMovement
    inline uintptr_t CharacterMovement   = 0x318; // FILL on Character if needed
    inline uintptr_t MaxWalkSpeed        = 0x218; // typical CMC — verify

    namespace bone {
        inline int Head = 68, Neck = 67, Chest = 66, Pelvis = 2;
        inline int LShoulder = 9, LElbow = 10, LHand = 11;
        inline int RShoulder = 38, RElbow = 39, RHand = 40;
        inline int LHip = 71, LKnee = 72, LFoot = 73;
        inline int RHip = 78, RKnee = 79, RFoot = 80;
    }
}
namespace Off = off;
