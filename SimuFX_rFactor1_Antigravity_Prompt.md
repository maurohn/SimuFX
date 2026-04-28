# 🚀 PROMPT COMPLETO — SimuFX for rFactor 1

## CONTEXTO

Quiero desarrollar un software llamado **SimuFX for rFactor 1**.

El objetivo es crear un post-procesador gráfico global para **rFactor 1**, funcionando como un `d3d9.dll` proxy/injector de DirectX 9, similar conceptualmente a ReShade, pero pensado específicamente para mejorar visualmente rFactor 1.

No quiero modificar el juego, ni tocar el `.exe`, ni leer memoria, ni hacer hacks internos del motor. Quiero un sistema externo de post-procesado que se cargue junto al juego y mejore la imagen final.

---

# 🎯 OBJETIVO PRINCIPAL

Crear un `d3d9.dll` que se coloque en la carpeta raíz de rFactor 1 y que intercepte DirectX 9 para aplicar efectos visuales globales sobre el frame final del juego.

El objetivo visual es que rFactor 1 se vea mucho más moderno, con una estética cercana a simuladores actuales como RaceRoom:

- Colores más vivos
- Mejor contraste
- Más brillo visual
- Imagen más limpia
- Mejor nitidez
- Mejor antialiasing percibido
- Menos look viejo/lavado
- Más sensación premium
- Mejor profundidad visual

Debe ser una mejora fuerte, pero usable para manejar. No quiero un resultado exagerado, saturado o artificial.

---

# 🧠 ALCANCE DEL PROYECTO

El sistema debe aplicar a todo el juego de forma global.

Debe afectar:

- Menús
- Gameplay
- Cockpit
- Cámaras externas
- Replays
- Todos los autos
- Todas las pistas

No quiero presets por pista, por auto, por cámara, por clima ni por contexto. Quiero una configuración global que aplique siempre que SimuFX esté activo.

---

# 🚫 COSAS QUE NO QUIERO

No implementar:

- Presets por pista
- Presets por auto
- Detección de circuito
- Detección de vehículo
- Detección de cámara
- Integración con telemetría
- Lectura de memoria del juego
- Modificación del EXE
- Hacks internos del motor
- Cambios reales de materiales del juego
- Cambios reales de sombras internas
- Cheats
- Hooks de red
- Cualquier cosa que pueda afectar multiplayer

Este proyecto debe ser un post-procesador gráfico global y seguro.

---

# 🧩 ARQUITECTURA ESPERADA

El flujo debe ser:

```text
rFactor.exe
  -> d3d9.dll (SimuFX)
      -> d3d9.dll real de Windows
          -> GPU
```

El juego debe creer que está usando DirectX normalmente, pero SimuFX debe interceptar el frame final para aplicar efectos de post-procesado.

---

# 🛠️ STACK TÉCNICO

Usar:

- Lenguaje: C++17
- API gráfica: DirectX 9
- Shaders: HLSL
- UI: ImGui
- Build: Visual Studio
- Plataforma: Windows
- Output principal: `d3d9.dll`

---

# 🔧 HOOKS NECESARIOS

Implementar interceptación de:

- `Direct3DCreate9`
- `IDirect3DDevice9::Present`
- `IDirect3DDevice9::EndScene`
- `IDirect3DDevice9::Reset`

Debe manejar correctamente:

- device lost
- reset de device
- cambio de resolución
- fullscreen
- windowed mode
- recreación de recursos gráficos

---

# 🎨 OBJETIVO VISUAL DETALLADO

Quiero que rFactor 1 gane un look mucho más moderno.

Prioridades visuales:

## 1. Color fuerte y atractivo

Los colores deben verse más vivos, más definidos y más modernos.

Evitar:

- imagen apagada
- look gris
- look viejo
- saturación excesiva

## 2. Brillos más lindos

Los brillos deben mejorar la sensación visual general.

Mejorar:

- reflejos
- altas luces
- zonas iluminadas
- sensación de sol/luz

Evitar:

- bloom exagerado
- imagen quemada
- brillos que molesten al manejar

## 3. Antialiasing percibido

Reducir bordes dentados típicos de rFactor 1.

Debe verse más limpio en:

- autos
- guardrails
- cockpit
- líneas de pista
- carteles
- estructuras

## 4. Nitidez

Mejorar definición sin generar halos.

Debe verse más nítido:

- el asfalto
- los autos
- los detalles del cockpit
- los carteles
- los objetos lejanos

## 5. Contraste moderno

Quitar el look plano/lavado del juego.

Debe haber:

- negros más firmes
- luces mejor controladas
- imagen con más cuerpo
- mejor separación visual

---

# 🎛️ EFECTOS OBLIGATORIOS

## 1. Color Boost

Implementar shader de color con:

- Saturation
- Vibrance
- Temperature
- Tint
- Color balance simple

Objetivo:

- colores más vivos
- imagen más moderna
- no quemar colores
- mantener naturalidad

---

## 2. Tonemap / Contraste

Implementar:

- Exposure
- Gamma
- Contrast
- Highlights
- Shadows
- Filmic tonemap si es viable

Objetivo:

- mejorar rango visual
- sacar look plano
- dar sensación moderna
- controlar altas luces

---

## 3. Bloom controlado

Implementar bloom con:

- Threshold
- Intensity
- Radius
- SoftKnee

Objetivo:

- mejorar luces y reflejos
- dar sensación premium
- simular brillo moderno

Importante:

- debe ser configurable
- no debe tapar la imagen
- no debe ser exagerado

---

## 4. Sharpen avanzado

Implementar:

- CAS como método preferido
- Luma sharpen como fallback
- evitar halos fuertes

Parámetros:

- Strength
- Clamp
- Radius si aplica

Objetivo:

- mejorar definición
- limpiar imagen
- dar sensación de resolución más alta

---

## 5. Antialiasing post-proceso

Implementar:

- FXAA como mínimo obligatorio
- SMAA como opcional si es viable

Parámetros:

- Strength
- EdgeThreshold
- SubpixelQuality

Objetivo:

- reducir bordes dentados
- mejorar limpieza visual
- suavizar imagen sin hacerla borrosa

---

## 6. Vignette opcional

Implementar viñeta muy sutil.

Debe venir apagada o casi apagada por defecto.

Parámetros:

- Intensity
- Radius
- Softness

---

# 🎬 PIPELINE GRÁFICO RECOMENDADO

Orden sugerido:

```text
Backbuffer original
  -> Copy to texture
  -> FXAA / SMAA
  -> Tonemap
  -> Color Boost
  -> Bloom
  -> Sharpen
  -> Final composite
  -> Present
```

El pipeline debe ser modular para poder activar/desactivar efectos.

---

# ⚙️ CONFIGURACIÓN GLOBAL

Crear archivo:

```text
/SimuFX/global.ini
```

Debe contener configuración única global.

Ejemplo inicial:

```ini
[General]
Enabled=true
Preset=RaceRoomStyle
ShowOverlay=true
ToggleKey=F10
ReloadKey=F9

[Color]
Enabled=true
Saturation=1.25
Vibrance=0.35
Temperature=0.03
Tint=0.00

[Tonemap]
Enabled=true
Exposure=0.05
Gamma=1.00
Contrast=1.18
Highlights=0.90
Shadows=1.05
Filmic=true

[Bloom]
Enabled=true
Threshold=0.78
Intensity=0.28
Radius=0.65
SoftKnee=0.45

[Sharpen]
Enabled=true
Method=CAS
Strength=0.55
Clamp=0.035

[AntiAliasing]
Enabled=true
Method=FXAA
Strength=0.85
EdgeThreshold=0.125
SubpixelQuality=0.75

[Vignette]
Enabled=false
Intensity=0.08
Radius=0.85
Softness=0.45

[SafeMode]
DisableDepthAccess=true
DisableMemoryScan=true
DisableNetworkHooks=true
```

---

# 🎚️ PRESETS GLOBALES

Crear presets globales, no por pista ni por auto.

Ubicación:

```text
/SimuFX/presets/
```

Presets:

```text
RaceRoomStyle.ini
Realistic.ini
Cinematic.ini
SharpClean.ini
NightBoost.ini
```

El preset principal por defecto debe ser:

```text
RaceRoomStyle.ini
```

## RaceRoomStyle.ini

Debe priorizar:

- colores vivos
- contraste moderno
- bloom moderado
- sharpen claro
- FXAA fuerte
- imagen limpia
- brillos atractivos
- sensación de simulador moderno

Debe ser el preset visualmente más impactante pero todavía manejable.

---

# 🎛️ OVERLAY IMGUI

Implementar overlay con ImGui.

Tecla para mostrar/ocultar:

```text
F10
```

El overlay debe incluir:

- Enable / Disable SimuFX
- Selector de preset global
- Botón Save
- Botón Reload
- FPS actual
- Frametime
- Estado de DirectX hook
- Estado de shaders
- Estado del archivo de configuración

## Sliders requeridos

### Color

- Saturation
- Vibrance
- Temperature
- Tint

### Tonemap

- Exposure
- Gamma
- Contrast
- Highlights
- Shadows

### Bloom

- Enabled
- Threshold
- Intensity
- Radius
- SoftKnee

### Sharpen

- Enabled
- Strength
- Clamp

### Antialiasing

- Enabled
- Method
- Strength
- EdgeThreshold
- SubpixelQuality

---

# 🔄 HOT RELOAD

Implementar hot reload con:

```text
F9
```

Cuando se presione F9:

- recargar `global.ini`
- recompilar shaders si cambiaron
- aplicar valores en runtime
- registrar evento en log

No debe requerir reiniciar el juego.

---

# 📁 ESTRUCTURA DEL PROYECTO

Crear esta estructura:

```text
SimuFX/
  src/
    main.cpp
    dx9_proxy.cpp
    dx9_proxy.h
    device_hook.cpp
    device_hook.h
    shader_manager.cpp
    shader_manager.h
    effect_pipeline.cpp
    effect_pipeline.h
    render_targets.cpp
    render_targets.h
    config.cpp
    config.h
    overlay.cpp
    overlay.h
    logger.cpp
    logger.h
    keys.cpp
    keys.h

  shaders/
    fxaa.hlsl
    smaa.hlsl
    tonemap.hlsl
    color_boost.hlsl
    bloom_downsample.hlsl
    bloom_upsample.hlsl
    bloom_composite.hlsl
    sharpen_cas.hlsl
    luma_sharpen.hlsl
    vignette.hlsl
    final_composite.hlsl

  presets/
    RaceRoomStyle.ini
    Realistic.ini
    Cinematic.ini
    SharpClean.ini
    NightBoost.ini

  external/
    imgui/

  docs/
    INSTALL.md
    CONFIG.md
    TROUBLESHOOTING.md

  build/
```

---

# 🧾 LOGGING

Crear log en:

```text
/SimuFX/simufx.log
```

Debe registrar:

- carga de DLL
- path de DirectX real
- creación de device
- hooks aplicados
- errores de shaders
- compilación de shaders
- reset de device
- device lost
- recreación de render targets
- cambios de preset
- hot reload
- errores críticos

---

# ⚡ PERFORMANCE

Objetivo:

- impacto ideal menor a 5 FPS
- debe funcionar bien en 1080p
- debe funcionar bien en 1440p
- no debe provocar stuttering fuerte
- no debe crashear al entrar/salir de pista
- no debe crashear al cambiar resolución

Optimizar:

- render targets
- cantidad de passes
- bloom
- sharpen
- FXAA

Permitir desactivar efectos pesados.

---

# 🧪 TESTING

Probar:

1. rFactor arranca sin crash.
2. rFactor entra al menú.
3. rFactor carga una pista.
4. rFactor entra a pista.
5. SimuFX apagado muestra imagen original.
6. SimuFX encendido mejora visualmente.
7. F10 abre overlay.
8. F9 recarga configuración.
9. Cambiar resolución no crashea.
10. Alt+Tab no crashea.
11. Entrar/salir de pista no crashea.
12. Replay funciona.
13. El rendimiento es estable.

---

# ✅ CRITERIOS DE ÉXITO

El proyecto se considera exitoso cuando:

1. Se genera una DLL `d3d9.dll` funcional.
2. rFactor 1 arranca con esa DLL en la carpeta raíz.
3. El juego funciona normalmente si SimuFX está apagado.
4. El juego mejora visualmente si SimuFX está activo.
5. Los colores se ven más vivos.
6. Los brillos se ven más atractivos.
7. El contraste mejora claramente.
8. La imagen se ve más nítida.
9. Los bordes dentados se reducen.
10. F10 abre/cierra overlay.
11. F9 recarga configuración.
12. No hay crashes importantes.
13. El impacto de FPS es razonable.
14. El código queda modular y documentado.

---

# 🚀 PRIMER ENTREGABLE

Primera versión funcional obligatoria:

- `d3d9.dll` proxy funcional
- hook de `Present`
- manejo básico de `Reset`
- overlay ImGui básico
- lectura de `global.ini`
- logging funcional
- shader de color boost
- shader de contraste/tonemap
- shader de sharpen
- FXAA básico
- preset `RaceRoomStyle.ini`

No avanzar a bloom avanzado hasta que esta base funcione estable.

---

# 🧱 PRIORIDAD DE DESARROLLO

Orden de trabajo:

1. Crear proyecto C++ DLL.
2. Implementar proxy `d3d9.dll`.
3. Cargar DirectX real de Windows.
4. Forward de funciones básicas.
5. Hook de device.
6. Hook de `Present`.
7. Renderizar frame sin modificar.
8. Agregar shader simple de color.
9. Agregar overlay ImGui.
10. Agregar config `global.ini`.
11. Agregar hot reload.
12. Agregar FXAA.
13. Agregar sharpen.
14. Agregar bloom.
15. Optimizar performance.
16. Documentar instalación.

---

# 📘 DOCUMENTACIÓN REQUERIDA

Crear:

## INSTALL.md

Debe explicar:

- dónde copiar `d3d9.dll`
- dónde copiar carpeta `SimuFX`
- cómo activar/desactivar
- cómo abrir overlay
- cómo volver atrás si falla

## CONFIG.md

Debe explicar cada parámetro del `global.ini`.

## TROUBLESHOOTING.md

Debe incluir:

- el juego no abre
- pantalla negra
- no aparece overlay
- baja de FPS
- crash al cambiar resolución
- shaders no compilan

---

# 🏁 RESULTADO FINAL ESPERADO

Quiero terminar con un sistema propio tipo ReShade, pero más simple, directo y enfocado en rFactor 1.

Debe hacer que rFactor 1 se vea mucho mejor globalmente, con una estética moderna tipo RaceRoom:

- colores vivos
- brillos lindos
- antialiasing mejorado
- imagen más nítida
- mejor contraste
- look menos viejo
- experiencia visual mucho más atractiva

El entregable final debe ser usable por una persona copiando `d3d9.dll` y la carpeta `SimuFX` dentro del directorio raíz de rFactor 1.
