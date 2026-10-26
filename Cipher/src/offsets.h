#pragma once
#include <cstdint>

namespace Off {
    // Globals (from your GWorld.hpp / Offsets.hpp)
    constexpr uint64_t GWorld            = 0x1B096DE8;
    constexpr uint64_t GObjectsChunks    = 0x1B028040;
    constexpr uint64_t GObjectsNum       = 0x1B028054;
    constexpr uint64_t GNames            = 0x1AF4CDC0;

    // World
    constexpr uint64_t World_GameState           = 0x1D0;
    constexpr uint64_t World_OwningGameInstance  = 0x248;
    constexpr uint64_t World_PersistentLevel     = 0x40;

    // GameInstance
    constexpr uint64_t GameInstance_LocalPlayers = 0x38;

    // LocalPlayer → PlayerController (standard)
    constexpr uint64_t LocalPlayer_PlayerController = 0x30;

    // PlayerController
    constexpr uint64_t PlayerController_AcknowledgedPawn   = 0x350; // verify if needed
    constexpr uint64_t PlayerController_PlayerCameraManager = 0x360;

    // Camera
    constexpr uint64_t CameraManager_CameraCachePrivate = 0x1510;
    constexpr uint64_t CameraCache_POV                  = 0x10;

    // GameState
    constexpr uint64_t GameState_PlayerArray = 0x2C0; // common Athena; adjust if your Offsets.hpp differs

    // Actor
    constexpr uint64_t Actor_RootComponent = 0x1B0; // from earlier SDK paste

    // SceneComponent
    constexpr uint64_t SceneComponent_RelativeLocation = 0x140;
    constexpr uint64_t SceneComponent_ComponentToWorld = 0x1E0;

    // PlayerState
    constexpr uint64_t PlayerState_PawnPrivate      = 0x320;
    constexpr uint64_t PlayerState_PlayerNamePrivate = 0xA00;
    constexpr uint64_t PlayerState_TeamIndex        = 0x10E8;
}
