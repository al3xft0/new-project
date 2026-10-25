# Cipher — Feature 1: UI shell

Black / white external menu for the Fortnite cheat project. **UI only** — no process attach, no ESP draw yet.

## Layout
- Left sidebar tabs: **Combat · Visuals · Misc · Config** (glyph icons + active glow bar)
- Brand logo centered in sidebar (`logo_sm.png`)
- Large faded logo watermark in content panel (`logo_lg.png`)
- Soft open/close animation, tab cross-fade, pulsing watermark when menu hidden
- Visuals tab hosts ESP controls (ready for Feature 2 wiring)

## Build (Windows)
```bat
cd Cipher
git clone --depth 1 https://github.com/ocornut/imgui.git external\imgui
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
copy assets\*.png build\Release\
```

Run `build\Release\Cipher.exe`. **INSERT** toggles the menu.

## Assets
| File | Use |
|------|-----|
| `assets/logo_sm.png` | Sidebar brand (48px draw) |
| `assets/logo_lg.png` | Content watermark |

Both sources were the same resolution; UI scales them differently.

## Next feature
ESP: attach to game, actor loop, world-to-screen, draw boxes using these toggles.
