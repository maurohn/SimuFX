# SimuFX — TROUBLESHOOTING.md

**First step always:** Check `SimuFX/simufx.log` — it logs every event with timestamps.

---

## The game doesn't open at all

**Cause**: Wrong DLL architecture or missing dependency.

- Ensure `d3d9.dll` was compiled for **x86 (Win32)**, not x64.
- Run `dumpbin /HEADERS build\d3d9.dll` and confirm `machine (x86)`.
- Make sure Visual C++ Redistributable for x86 is installed.
- Check that `DirectX End-User Runtime` is installed (for d3dx9.dll).
- Look for the line `"FATAL: could not load real d3d9.dll"` in the log.

---

## Black screen on launch

**Cause**: Shader compilation failure or render target creation failure.

- Check the log for `"Shader FAILED:"` or `"Failed to create RT"` entries.
- Ensure the `SimuFX/shaders/` folder is present and contains all `.hlsl` files.
- Try renaming `d3d9.dll` to `d3d9.dll.bak` — if the game runs fine without it, the issue is in SimuFX.
- Set `Enabled=false` in `global.ini` to bypass effects and test if the proxy itself works.

---

## Overlay (F10) doesn't appear

**Cause**: ImGui DX9 backend not initialised, or key not registered.

- Check the log for `"Overlay initialised"`. If missing, ImGui init failed.
- Ensure `ShowOverlay=true` in `global.ini`.
- Make sure `ToggleKey=121` (that's F10). If you changed it, use the correct VKey code.
- The overlay only appears when the game has focus.

---

## FPS drop is too high

**Suggestions**:

1. Disable bloom first (heaviest pass): `[Bloom] Enabled=false`
2. Switch AA to FXAA (faster than SMAA): `[AntiAliasing] Method=FXAA`
3. Reduce sharpen strength: `[Sharpen] Strength=0.3`
4. On weak GPUs: disable AA entirely, keep only Color + Tonemap + Sharpen.

Typical overhead at 1080p on a modern GPU: **1–3 FPS**.
At 1440p: **2–5 FPS**.

---

## Crash when changing resolution

**Cause**: Device Reset handler has an error — this should not happen in stable builds.

- Check the log for `"Reset called"` followed by `"Reset failed"`.
- If it crashes mid-reset, ensure no custom shaders hold D3DPOOL_DEFAULT resources outside of SimuFX.
- Try `[Bloom] Enabled=false` — bloom uses extra render targets that are most sensitive to reset.

---

## Crash on Alt+Tab

**Cause**: Device Lost not handled (should not happen in stable builds).

- Check the log for `"device lost"` or `"TestCooperativeLevel"` errors.
- The proxy correctly calls `OnPreReset` / `OnPostReset` during all `Reset()` calls.
- If the crash persists, check if rFactor uses `D3DSWAPEFFECT_DISCARD` in windowed mode — some older titles don't handle this correctly.

---

## Shaders don't compile

**Cause**: Missing HLSL file, syntax error, or D3DX9 not available.

- Look for `"Shader FAILED:"` in the log with the exact HLSL error message.
- Ensure `d3dx9_43.dll` is present (part of the DirectX End-User Runtime June 2010).
- If you modified an `.hlsl` file, press **F9** to hot-reload and check the log.

---

## Crash entering / exiting track

**Cause**: Render target recreation issue (device reset between sessions).

- Check the log for `"PostProcessor: OnPreReset"` and `"PostProcessor: OnPostReset"`.
- Both must appear paired for every track load/unload.
- If `OnPostReset` is missing, the issue is in the Reset call chain — open a bug report with the full log.

---

## Log not being created

- Ensure the `SimuFX/` folder exists in the rFactor root.
- If the folder doesn't exist, create it manually. SimuFX will create `simufx.log` inside it.
- On some systems, the UAC may prevent writing to `Program Files`. Move rFactor to a non-system drive.
