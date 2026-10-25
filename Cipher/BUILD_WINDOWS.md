# Cipher builds on **Windows**, not Codespaces/Linux

This app uses **Win32 + D3D11 + dwmapi**. MinGW cross-compile from Ubuntu will fight you
(no easy d3d11/dwmapi runtime, wrong subsystem).

## Supported path

1. Windows 10/11 PC (or a Windows VM / GitHub `windows-latest` runner)
2. Visual Studio 2022 — workload **Desktop development with C++**
3. CMake 3.20+

```bat
cd Cipher
git clone --depth 1 https://github.com/ocornut/imgui.git external\imgui
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
copy assets\*.png build\Release\
build\Release\Cipher.exe
```

## GitHub Actions (optional)

Use `runs-on: windows-latest` — same commands. Do **not** use `ubuntu-latest` + mingw for this repo.

## Codespaces

Codespaces is Linux. Either:
- open the repo in VS on a Windows machine, or
- add a `windows-latest` workflow and download the artifact.
