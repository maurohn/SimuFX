# SimuFX — CONFIG.md

All configuration lives in `SimuFX/global.ini`.
Presets live in `SimuFX/presets/*.ini` and override effect values when loaded.
Changes take effect immediately after pressing **F9** (hot reload) — no restart needed.

---

## [General]

| Key | Default | Description |
|---|---|---|
| `Enabled` | `true` | Master on/off switch. When false, the game renders unmodified. |
| `Preset` | `RaceRoomStyle` | Preset file to load from the presets/ folder. |
| `ShowOverlay` | `true` | Whether the overlay is shown by default on launch. |
| `ToggleKey` | `121` | VKey code for the overlay toggle (121 = F10). |
| `ReloadKey` | `120` | VKey code for hot reload (120 = F9). |

---

## [Color]

| Key | Default | Range | Description |
|---|---|---|---|
| `Enabled` | `true` | — | Enable the color grading pass. |
| `Saturation` | `1.25` | 0.5 – 2.0 | Uniform saturation multiplier. 1.0 = unchanged. |
| `Vibrance` | `0.35` | 0.0 – 1.0 | Boosts less-saturated colours more than already-vivid ones. Avoids over-saturation of reds. |
| `Temperature` | `0.03` | -0.5 – 0.5 | Colour temperature. Positive = warmer (more red/green). Negative = cooler (more blue). |
| `Tint` | `0.00` | -0.5 – 0.5 | Green/Magenta tint axis. Positive = more green. |

---

## [Tonemap]

| Key | Default | Range | Description |
|---|---|---|---|
| `Enabled` | `true` | — | Enable the tonemapping pass. |
| `Exposure` | `0.05` | -1.0 – 1.0 | EV exposure compensation. 0.05 = slightly brighter. |
| `Gamma` | `1.00` | 0.5 – 2.2 | Output gamma curve. Lower = brighter midtones. |
| `Contrast` | `1.18` | 0.5 – 2.0 | S-curve contrast. 1.0 = unchanged. Higher = more separation. |
| `Highlights` | `0.90` | 0.0 – 2.0 | Highlight rolloff. Lower = softer, more compressed highlights. |
| `Shadows` | `1.05` | 0.0 – 2.0 | Shadow lift multiplier. Above 1.0 lifts dark areas. |
| `Filmic` | `true` | true/false | Apply ACES filmic tonemapping curve before gamma. Prevents clipping. |

---

## [Bloom]

| Key | Default | Range | Description |
|---|---|---|---|
| `Enabled` | `true` | — | Enable the bloom pass. |
| `Threshold` | `0.78` | 0.0 – 2.0 | Brightness level above which bloom is applied. |
| `Intensity` | `0.28` | 0.0 – 1.0 | Bloom strength in the composite pass. |
| `Radius` | `0.65` | 0.0 – 1.0 | Bloom spread radius. |
| `SoftKnee` | `0.45` | 0.0 – 1.0 | Soft knee width around the threshold — prevents hard cut-off. |

---

## [Sharpen]

| Key | Default | Range | Description |
|---|---|---|---|
| `Enabled` | `true` | — | Enable the sharpening pass. |
| `Method` | `CAS` | `CAS` / `Luma` | CAS = AMD Contrast Adaptive Sharpening (recommended). Luma = unsharp mask fallback. |
| `Strength` | `0.55` | 0.0 – 1.5 | Sharpening intensity. Above 1.0 is very aggressive. |
| `Clamp` | `0.035` | 0.0 – 0.2 | Maximum per-channel sharpening delta. Prevents halos. |

---

## [AntiAliasing]

| Key | Default | Range | Description |
|---|---|---|---|
| `Enabled` | `true` | — | Enable the AA pass. |
| `Method` | `FXAA` | `FXAA` / `SMAA` | Algorithm. FXAA is faster. SMAA (when implemented) is higher quality. |
| `Strength` | `0.85` | 0.0 – 1.0 | Overall AA blend strength. Lower = less blurring. |
| `EdgeThreshold` | `0.125` | 0.0 – 0.5 | Luminance gradient to consider an edge. Lower = more edges processed. |
| `SubpixelQuality` | `0.75` | 0.0 – 1.0 | Sub-pixel aliasing correction. Higher = softer but cleaner. |

---

## [Vignette]

| Key | Default | Range | Description |
|---|---|---|---|
| `Enabled` | `false` | — | Enable the vignette pass. Off by default. |
| `Intensity` | `0.08` | 0.0 – 1.0 | How dark the vignette gets at the edges. |
| `Radius` | `0.85` | 0.0 – 1.0 | Where the vignette starts (from the centre). |
| `Softness` | `0.45` | 0.0 – 1.0 | Transition smoothness. |

---

## [SafeMode]

These are always `true` in this release and cannot be changed. They document what SimuFX explicitly does NOT do.

| Key | Value | Meaning |
|---|---|---|
| `DisableDepthAccess` | `true` | SimuFX never reads the depth buffer. |
| `DisableMemoryScan` | `true` | SimuFX never reads rFactor's process memory. |
| `DisableNetworkHooks` | `true` | SimuFX never intercepts network traffic. |
