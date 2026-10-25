cmake_minimum_required(VERSION 3.20)
project(Cipher LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(Cipher WIN32
    src/main.cpp
    src/ui.cpp
    src/theme.cpp
    src/texture.cpp
    src/memory.cpp
    src/esp.cpp
    external/imgui/imgui.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/backends/imgui_impl_win32.cpp
    external/imgui/backends/imgui_impl_dx11.cpp
)

target_include_directories(Cipher PRIVATE src external/imgui external/imgui/backends)
target_link_libraries(Cipher PRIVATE d3d11 dxgi d3dcompiler dwmapi)
target_compile_definitions(Cipher PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)

# MSVC: keep cmath healthy (Windows.h must not precede <cmath>)
if(MSVC)
  target_compile_options(Cipher PRIVATE /W3 /permissive- /Zc:__cplusplus)
  target_compile_definitions(Cipher PRIVATE _USE_MATH_DEFINES)
  # Force include order for every TU
  target_compile_options(Cipher PRIVATE "/FI${CMAKE_CURRENT_SOURCE_DIR}/src/win_pre.h")
endif()

