#pragma once
#include <cstdint>

// Merged from user dump (annotated 42.40-style) + Cipher off:: layout.
// Prefer GEngine -> GameViewport -> World over encrypted GWorld when possible.

namespace off {
    // ---------- Globals (RVA from image base) ----------
    inline uintptr_t GEngine             = 0x1B098778;
    inline uintptr_t GWorld              = 0x1B096DE8; // may be encrypted; fallback
    inline uintptr_t GNames              = 0x1AF4A680;
    inline uintptr_t GObjects            = 0x1B028040;
    inline uintptr_t GObjectsCount       = 0x1B028054;
    inline uintptr_t ViewMatrix          = 0; // use camera POV path instead

    // UEngine / viewport chain (no decrypt)
    inline uintptr_t GameViewport        = 0xB70;  // UEngine::GameViewport
    inline uintptr_t ViewportWorld       = 0x78;   // UGameViewportClient::World

    // UWorld
    inline uintptr_t PersistentLevel     = 0x40;
    inline uintptr_t GameInstance        = 0x248;  // OwningGameInstance
    inline uintptr_t GameState           = 0x1D0;
    inline uintptr_t Levels              = 0x1E8;
    inline uintptr_t Seconds             = 0x198;

    // ULevel / actors
    inline uintptr_t Actors              = 0x98;   // classic TArray; verify vs 0x50 LEVEL_ACTORS
    inline uintptr_t ActorsCount         = 0xA0;
    inline uintptr_t LevelActors         = 0x50;   // alt from dump
    inline uintptr_t ActorCluster        = 0x110;
    inline uintptr_t ActorContainerActors = 0x28;

    // GameState
    inline uintptr_t PlayerArray         = 0x288;  // AGameStateBase::PlayerArray — preferred enum path

    // GameInstance / LocalPlayer
    inline uintptr_t LocalPlayers        = 0x38;
    inline uintptr_t PlayerController    = 0x30;

    // APlayerController
    inline uintptr_t AcknowledgedPawn    = 0x318;
    inline uintptr_t PlayerCameraManager = 0x328;
    inline uintptr_t ControlRotation     = 0x2E8;
    inline uintptr_t FOV                 = 0x374;

    // PlayerCameraManager / POV
    inline uintptr_t CameraCache         = 0x1590; // CameraCachePrivate
    inline uintptr_t POV_Location        = 0x10;
    inline uintptr_t POV_Rotation        = 0x28;
    inline uintptr_t POV_FOV             = 0x40;
    // Some builds expose pointer fields:
    inline uintptr_t CameraLocationPtr   = 0x178;
    inline uintptr_t CameraRotationPtr   = 0x188;

    // AActor / components
    inline uintptr_t RootComponent       = 0x1B0;
    inline uintptr_t RelativeLocation    = 0x140;
    inline uintptr_t RelativeRotation    = 0x158;
    inline uintptr_t ComponentToWorld    = 0x1E0;
    inline uintptr_t ComponentVelocity   = 0x188;
    inline uintptr_t LastRenderTime      = 0x548;

    // Character / pawn
    inline uintptr_t Mesh                = 0x2F0;
    inline uintptr_t PlayerState         = 0x290;
    inline uintptr_t CharacterMovement   = 0x2F8;
    inline uintptr_t CurrentWeapon       = 0x9D0;
    inline uintptr_t bIsDying            = 0x728;
    inline uintptr_t bIsDBNO             = 0x881;
    inline uintptr_t bIsCrouched         = 0x430;
    inline uintptr_t HealthSet           = 0x13D8;

    // PlayerState / Athena
    inline uintptr_t PawnPrivate         = 0x2E8;
    inline uintptr_t TeamIndex           = 0xF69;
    inline uintptr_t KillScore           = 0xF80;
    inline uintptr_t PlayerName          = 0x308;
    inline uintptr_t Platform            = 0x400;
    inline uintptr_t CurrentHealth       = 0xD0C;
    inline uintptr_t MaxHealth           = 0xD10;
    inline uintptr_t bIsABot             = 0x27A;

    // FortHealthSet (via HealthSet)
    inline uintptr_t HS_Health           = 0x48;
    inline uintptr_t HS_MaxHealth        = 0x70;
    inline uintptr_t HS_Shield           = 0x98;

    // Skeletal mesh / bones (updated from dump — was 0x5E8)
    inline uintptr_t BoneArray           = 0x660;
    inline uintptr_t BoneArrayCached     = 0x670;
    inline uintptr_t CurrentBoneArray    = 0x6C8;
    inline uintptr_t BoneStride          = 0x60;
    inline uintptr_t CachedBoneSpaceTransforms = 0x9F0;
    inline uintptr_t CachedComponentSpaceTransforms = 0xA00;

    // Weapon
    inline uintptr_t WeaponData          = 0x648;
    inline uintptr_t AmmoCount           = 0x11B0;
    inline uintptr_t WeaponRecoilOffset  = 0x2358;
    inline uintptr_t PlayerAimOffset     = 0x2340;
    inline uintptr_t ProjectileSpeed     = 0x214C;
    inline uintptr_t ProjectileGravity   = 0x20D8;

    // Aim helpers
    inline uintptr_t TargetedFortPawn    = 0x16D0;
    inline uintptr_t LocationUnderReticle = 0x21B0;
    inline uintptr_t RotationInput       = 0x4B0;

    inline uintptr_t MaxWalkSpeed        = 0x218;  // CharacterMovementComponent — verify
    inline uintptr_t DefaultFOV          = 0x284;  // PlayerCameraManager::DefaultFOV
    inline uintptr_t RecoilProcessingShotCounter = 0x25F4;
    inline uintptr_t BulletCountForPerBulletRecoil = 0x25F0;
    inline uintptr_t TimeRemainingForBulletSpread = 0x2600;
    inline uintptr_t BulletCountForPerBulletSpread = 0x25FC;
    namespace bone {
        inline int Head = 68, Neck = 67, Chest = 66, Pelvis = 2;
        inline int LShoulder = 9, LElbow = 10, LHand = 11;
        inline int RShoulder = 38, RElbow = 39, RHand = 40;
        inline int LHip = 71, LKnee = 72, LFoot = 73;
        inline int RHip = 78, RKnee = 79, RFoot = 80;
    }
}

namespace Off = off;
