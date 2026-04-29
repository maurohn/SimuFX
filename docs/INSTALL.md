# SimuFX for rFactor 1 — Installation Guide

## What you need to copy

After building, copy **two** things into the rFactor 1 root folder
(the folder that contains `rFactor.exe`):

```
rFactor/
  d3d9.dll          ← compiled output from build/d3d9.dll
  SimuFX/
    global.ini
    simufx.log      ← created automatically on first run
    shaders/
      fxaa.hlsl
      color_boost.hlsl
      tonemap.hlsl
      bloom_downsample.hlsl
      bloom_upsample.hlsl
      bloom_composite.hlsl
      sharpen_cas.hlsl
      luma_sharpen.hlsl
      vignette.hlsl
      final_composite.hlsl
    presets/
      Dynamic.ini
      Realistic.ini
      Cinematic.ini
      SharpClean.ini
      NightBoost.ini
```

## How to activate / deactivate

- **Active**: Just have `d3d9.dll` present in the rFactor folder.
- **Bypassed temporarily**: Open the overlay (F10) and uncheck **Enable SimuFX**.
- **Fully disabled**: Rename `d3d9.dll` to `d3d9.dll.bak`.

## Opening the overlay

Press **F10** while inside the game. The overlay works in menus, gameplay, cockpit, replay — everywhere.

## Hot reload

Press **F9** to reload `global.ini` and recompile shaders without restarting the game.

## Reverting if something breaks

1. Rename or delete `d3d9.dll` from the rFactor folder.
2. The game will run exactly as it did before.
3. Check `SimuFX/simufx.log` for error details.

## Build requirements

- Visual Studio 2022 (MSVC)
- CMake 3.20+
- **Build target: x86 (Win32)** — rFactor 1 is 32-bit
- DirectX SDK June 2010 (for d3dx9.lib) or Windows SDK 10+
- ImGui source files placed in `external/imgui/`

### Quick build

```bat
cmake -S . -B _build -A Win32
cmake --build _build --config Release
```

The compiled `d3d9.dll` will appear in `build/d3d9.dll`.

