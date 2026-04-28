cmake_minimum_required(VERSION 3.20)
project(SimuFX VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- Windows SDK paths (10.0.26100.0) ----
set(WINSDK_VERSION "10.0.26100.0")
set(WINSDK_INC "C:/Program Files (x86)/Windows Kits/10/Include/${WINSDK_VERSION}")
set(WINSDK_LIB "C:/Program Files (x86)/Windows Kits/10/Lib/${WINSDK_VERSION}")

# ---- DLL output named d3d9.dll ----
add_library(SimuFX SHARED
    src/main.cpp
    src/dx9_proxy.cpp
    src/device_hook.cpp
    src/effect_pipeline.cpp
    src/config.cpp
    src/overlay.cpp
    src/d3d9_exports.def
    # ImGui core
    external/imgui/imgui.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp
    # ImGui backends
    external/imgui/backends/imgui_impl_dx9.cpp
    external/imgui/backends/imgui_impl_win32.cpp
)

set_target_properties(SimuFX PROPERTIES OUTPUT_NAME "d3d9")

# ---- Include directories ----
target_include_directories(SimuFX PRIVATE
    src
    external/imgui
    external/imgui/backends
    "${WINSDK_INC}/um"
    "${WINSDK_INC}/shared"
)

# ---- Compiler options ----
target_compile_options(SimuFX PRIVATE
    /W3
    /MP
    /MD            # Dynamic CRT — safe for DLLs; we bundle the redist alongside
    /DWIN32
    /D_WINDOWS
    /DNOMINMAX
    /D_CRT_SECURE_NO_WARNINGS
    /DUNICODE
    /D_UNICODE
)

# ---- Linker: use Windows SDK x86 libs ----
target_link_directories(SimuFX PRIVATE
    "${WINSDK_LIB}/um/x86"
)

target_link_libraries(SimuFX PRIVATE
    d3d9.lib
    d3dcompiler.lib
    dxguid.lib
    user32.lib
    gdi32.lib
    shell32.lib
    advapi32.lib
)

# ---- Redist paths ----
set(VC_REDIST "C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Redist/MSVC/14.44.35112/x86/Microsoft.VC143.CRT")

# ---- Post-build: copy all required DLLs to build/ ----
add_custom_command(TARGET SimuFX POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/build

    # Our proxy DLL
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:SimuFX> ${CMAKE_SOURCE_DIR}/build/d3d9.dll

    # D3D shader compiler (x86 from SysWOW64)
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "C:/Windows/SysWOW64/D3DCOMPILER_47.dll"
        ${CMAKE_SOURCE_DIR}/build/D3DCOMPILER_47.dll

    # VC Runtime (x86 from VS redist — safe to redistribute)
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${VC_REDIST}/msvcp140.dll"
        ${CMAKE_SOURCE_DIR}/build/msvcp140.dll
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${VC_REDIST}/vcruntime140.dll"
        ${CMAKE_SOURCE_DIR}/build/vcruntime140.dll
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${VC_REDIST}/msvcp140_1.dll"
        ${CMAKE_SOURCE_DIR}/build/msvcp140_1.dll

    COMMENT "Bundling d3d9.dll + all runtime dependencies to build/"
)
