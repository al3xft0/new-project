cmake_minimum_required(VERSION 3.20)
project(Cipher LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

add_library(imgui STATIC
    external/imgui/imgui.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/backends/imgui_impl_win32.cpp
    external/imgui/backends/imgui_impl_dx11.cpp
)
target_include_directories(imgui PUBLIC external/imgui external/imgui/backends)
target_link_libraries(imgui PUBLIC d3d11 dxgi d3dcompiler)
if(MSVC)
  target_compile_options(imgui PRIVATE /W3)
  set_property(TARGET imgui PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

add_executable(Cipher WIN32
    src/main.cpp
    src/ui.cpp
    src/theme.cpp
    src/texture.cpp
    src/memory.cpp
    src/esp.cpp
    src/aimbot.cpp
    src/triggerbot.cpp
    src/config.cpp
    src/exploits.cpp
    src/overlay.cpp
    src/license_crypto.cpp
    src/license_store.cpp
    src/license_gate_ui.cpp
)
target_include_directories(Cipher PRIVATE src)
target_link_libraries(Cipher PRIVATE imgui dwmapi)

target_compile_definitions(Cipher PRIVATE
    UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS
)
if(MSVC)
  target_compile_options(Cipher PRIVATE /W3)
  set_property(TARGET Cipher PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# --- Admin panel (separate exe) ---
add_executable(CipherAdmin WIN32
    admin/admin_main.cpp
    src/license_crypto.cpp
    src/license_store.cpp
    src/theme.cpp
)
target_include_directories(CipherAdmin PRIVATE
    src
    admin
)
target_link_libraries(CipherAdmin PRIVATE imgui dwmapi)
target_compile_definitions(CipherAdmin PRIVATE
    UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS
)
if(MSVC)
  target_compile_options(CipherAdmin PRIVATE /W3)
  set_property(TARGET CipherAdmin PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
