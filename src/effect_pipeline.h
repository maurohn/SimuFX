#pragma once
#include <d3d9.h>
#include <d3dcompiler.h>
#include <string>

namespace SimuFX {

// Full-screen quad vertex (pre-transformed)
struct FSQVertex {
    float x, y, z, rhw, u, v;
    static const DWORD FVF = D3DFVF_XYZRHW | D3DFVF_TEX1;
};

// ============================================================================
// PostProcessor — applies the SimuFX effect chain each frame.
//
// Effects (in order): FXAA → Tonemap → Color → Bloom → Sharpen → Vignette
// Each effect only runs when Enabled=true in config AND its shader compiled OK.
// ============================================================================
class PostProcessor {
public:
    PostProcessor(IDirect3DDevice9* device, const D3DPRESENT_PARAMETERS& pp);
    ~PostProcessor();

    // Called from Present() every frame
    void Process();

    // Device-lost lifecycle (call before/after IDirect3DDevice9::Reset)
    void OnPreReset();
    void OnPostReset(IDirect3DDevice9* device, const D3DPRESENT_PARAMETERS& pp);

    // Hot-reload shaders from disk (F9)
    void Reload(const std::string& basePath);

    bool IsReady() const { return m_ready; }

private:
    // --- Internal helpers ---
    bool InitResources();
    void ReleaseResources();
    void CompileShaders(const std::string& shaderDir);
    IDirect3DPixelShader9* CompilePS(const std::string& path, const char* entry);
    void DrawFSQ();
    void SaveState();
    void RestoreState();

    // --- Shader constant setters ---
    void SetFXAAConstants();
    void SetColorConstants();
    void SetTonemapConstants();
    void SetBloomConstants();
    void SetSharpenConstants();
    void SetVignetteConstants();

    // --- Device & dimensions ---
    IDirect3DDevice9*     m_device;
    D3DPRESENT_PARAMETERS m_pp;
    UINT                  m_width;
    UINT                  m_height;
    bool                  m_ready;
    float                 m_timeAccum;

    // --- Render targets ---
    IDirect3DTexture9*    m_rtTexA;      // full-res source
    IDirect3DSurface9*    m_rtSurfA;
    IDirect3DTexture9*    m_rtTexB;      // full-res destination (ping-pong)
    IDirect3DSurface9*    m_rtSurfB;
    IDirect3DTexture9*    m_bloomTexA;   // half-res bloom
    IDirect3DSurface9*    m_bloomSurfA;
    IDirect3DTexture9*    m_bloomTexB;   // quarter-res bloom
    IDirect3DSurface9*    m_bloomSurfB;

    // --- Geometry ---
    IDirect3DVertexBuffer9* m_fsqVB;

    // --- Shaders ---
    IDirect3DVertexShader9* m_vsPassthrough;   // unused, kept for future use
    IDirect3DPixelShader9*  m_psFXAA;
    IDirect3DPixelShader9*  m_psColor;
    IDirect3DPixelShader9*  m_psTonemap;
    IDirect3DPixelShader9*  m_psBloomDown;
    IDirect3DPixelShader9*  m_psBloomUp;
    IDirect3DPixelShader9*  m_psBloomComposite;
    IDirect3DPixelShader9*  m_psSharpenCAS;
    IDirect3DPixelShader9*  m_psSharpenLuma;
    IDirect3DPixelShader9*  m_psVignette;

    // --- State save/restore ---
    IDirect3DStateBlock9*   m_savedState;

    // --- Shader directory ---
    std::string m_shaderDir;
};

} // namespace SimuFX
