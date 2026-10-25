cmake_minimum_required(VERSION 3.20)
project(Cipher LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Dear ImGui — isolated static lib, ZERO project defines (keeps MSVC cmath sane)
add_library(imgui STATIC
    external/imgui/imgui.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/backends/imgui_impl_win32.cpp
    external/imgui/backends/imgui_impl_dx11.cpp
)
target_include_directories(imgui PUBLIC
    external/imgui
    external/imgui/backends
)
target_link_libraries(imgui PUBLIC d3d11 dxgi d3dcompiler)
if(MSVC)
  target_compile_options(imgui PRIVATE /W3)
endif()

# Cipher app
add_executable(Cipher WIN32
    src/main.cpp
    src/ui.cpp
    src/theme.cpp
    src/texture.cpp
    src/memory.cpp
    src/esp.cpp
)
target_include_directories(Cipher PRIVATE src)
target_link_libraries(Cipher PRIVATE imgui dwmapi)

target_compile_definitions(Cipher PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _CRT_SECURE_NO_WARNINGS
)

if(MSVC)
  target_compile_options(Cipher PRIVATE /W3)
endif()
