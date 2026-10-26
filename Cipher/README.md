# Cipher — Loader + Fullscreen Overlay

## Flow
1. **Loader** opens (rich UI). Shows **Fortnite not open** or **Fortnite detected**.
2. Press **ATTACH** → process attach + **fullscreen topmost overlay**.
3. Loader **minimizes**. Menu draws on the overlay (no Alt+Tab).
4. **INSERT** toggles menu; when closed, overlay is click-through so you can play.

## Features
- Soft aim + hard/silent, FOV circle, **FOV radar triangles** (same color)
- ESP: box/corner/filled, skeleton, **chams**, snaplines, radar, colors, player list
- Triggerbot, exploits (no recoil/spread, FOV, speed, **spinbot** slider)
- Streamproof on overlay HWND
- SDK 20-10 offsets in `offsets.h`

## Build
```
git clone --depth 1 https://github.com/ocornut/imgui.git external/imgui
cmake -B build -A x64
cmake --build build --config Release
```

## Notes
Overlay uses color-key transparency (pure black). ESP/menu colors must not be pure black.
W2S still needs a valid camera/view path for on-screen ESP.
