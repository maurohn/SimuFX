// ============================================================================
// effect_pipeline.cpp — SimuFX Post-Processing Pipeline
//
// Clean, simple implementation. Only the effects defined in global.ini:
//   FXAA → Tonemap → Color → Bloom → Sharpen → Vignette
//
// Each effect only runs when Enabled=true in config AND shader compiled OK.
// Passthrough (all disabled) = identical copy of the backbuffer.
// ============================================================================

#include "effect_pipeline.h"
#include "config.h"
#include "logger.h"
#include <d3d9.h>
#include <d3dcompiler.h>
#include <algorithm>
#include <string>

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dcompiler.lib")

namespace SimuFX {

// ============================================================================
// Construction / Destruction
// ============================================================================

PostProcessor::PostProcessor(IDirect3DDevice9* device,
                             const D3DPRESENT_PARAMETERS& pp)
    : m_device(device)
    , m_pp(pp)
    , m_width(pp.BackBufferWidth)
    , m_height(pp.BackBufferHeight)
    , m_ready(false)
    , m_timeAccum(0.0f)
    // Render targets
    , m_rtTexA(nullptr),  m_rtSurfA(nullptr)
    , m_rtTexB(nullptr),  m_rtSurfB(nullptr)
    , m_bloomTexA(nullptr), m_bloomSurfA(nullptr)
    , m_bloomTexB(nullptr), m_bloomSurfB(nullptr)
    // Geometry
    , m_fsqVB(nullptr)
    // Shaders
    , m_vsPassthrough(nullptr)
    , m_psFXAA(nullptr)
    , m_psColor(nullptr)
    , m_psTonemap(nullptr)
    , m_psBloomDown(nullptr)
    , m_psBloomUp(nullptr)
    , m_psBloomComposite(nullptr)
    , m_psSharpenCAS(nullptr)
    , m_psSharpenLuma(nullptr)
    , m_psVignette(nullptr)
    , m_psAO(nullptr)
    , m_psShadowDepth(nullptr)
    // State
    , m_savedState(nullptr)
{
    // Derive shader dir from exe location
    char modPath[MAX_PATH];
    GetModuleFileNameA(nullptr, modPath, MAX_PATH);
    std::string exeDir(modPath);
    auto slash = exeDir.find_last_of("\\/");
    if (slash != std::string::npos) exeDir = exeDir.substr(0, slash);
    m_shaderDir = exeDir + "\\SimuFX\\shaders\\";

    if (!InitResources())
        LOG_ERROR("PostProcessor: InitResources failed");
}

PostProcessor::~PostProcessor()
{
    ReleaseResources();
}

// ============================================================================
// Resource Management
// ============================================================================

bool PostProcessor::InitResources()
{
    UINT w = m_width;
    UINT h = m_height;

    // Query the actual backbuffer format — use it for all our RTs
    // so StretchRect is always a 1:1 format copy (no conversion, no artifacts)
    D3DFORMAT fmt = D3DFMT_X8R8G8B8;
    {
        IDirect3DSurface9* bb = nullptr;
        if (SUCCEEDED(m_device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &bb))) {
            D3DSURFACE_DESC desc = {};
            if (SUCCEEDED(bb->GetDesc(&desc))) fmt = desc.Format;
            bb->Release();
        }
        LOG_INFO("Backbuffer format: " + std::to_string((int)fmt) +
                 " (" + std::to_string(w) + "x" + std::to_string(h) + ")");
    }

    // --- Full-res ping-pong RTs (A = source, B = destination each pass) ---
    auto makeRT = [&](IDirect3DTexture9*& tex, IDirect3DSurface9*& surf,
                      UINT rw, UINT rh, const char* name) -> bool {
        if (FAILED(m_device->CreateTexture(rw, rh, 1, D3DUSAGE_RENDERTARGET,
                                           fmt, D3DPOOL_DEFAULT, &tex, nullptr))) {
            LOG_ERROR(std::string("Failed RT: ") + name); return false;
        }
        if (FAILED(tex->GetSurfaceLevel(0, &surf))) {
            LOG_ERROR(std::string("Failed surface: ") + name); return false;
        }
        return true;
    };

    if (!makeRT(m_rtTexA, m_rtSurfA, w,   h,   "RT_A"))    return false;
    if (!makeRT(m_rtTexB, m_rtSurfB, w,   h,   "RT_B"))    return false;
    if (!makeRT(m_bloomTexA, m_bloomSurfA, w/2, h/2, "Bloom_A")) return false;
    if (!makeRT(m_bloomTexB, m_bloomSurfB, w/4, h/4, "Bloom_B")) return false;

    // --- Full-screen quad (D3D9 XYZRHW — pre-transformed, no VS needed) ---
    // The -0.5 half-pixel offset is mandatory for correct texel alignment in D3D9.
    struct FSQVert { float x, y, z, rhw, u, v; };
    FSQVert verts[4] = {
        { -0.5f,        -0.5f,        0, 1, 0, 0 },
        { (float)w-0.5f,-0.5f,        0, 1, 1, 0 },
        { -0.5f,        (float)h-0.5f,0, 1, 0, 1 },
        { (float)w-0.5f,(float)h-0.5f,0, 1, 1, 1 },
    };
    if (FAILED(m_device->CreateVertexBuffer(sizeof(verts),
               D3DUSAGE_WRITEONLY, FSQVertex::FVF,
               D3DPOOL_DEFAULT, &m_fsqVB, nullptr))) {
        LOG_ERROR("Failed to create FSQ VB"); return false;
    }
    void* pData = nullptr;
    m_fsqVB->Lock(0, 0, &pData, 0);
    memcpy(pData, verts, sizeof(verts));
    m_fsqVB->Unlock();

    // --- Compile shaders ---
    CompileShaders(m_shaderDir);

    m_ready = true;
    LOG_INFO("PostProcessor ready");
    return true;
}

void PostProcessor::ReleaseResources()
{
    auto rel = [](auto*& p) { if (p) { p->Release(); p = nullptr; } };

    rel(m_savedState);
    rel(m_fsqVB);

    rel(m_vsPassthrough);
    rel(m_psFXAA);
    rel(m_psColor);
    rel(m_psTonemap);
    rel(m_psBloomDown);
    rel(m_psBloomUp);
    rel(m_psBloomComposite);
    rel(m_psSharpenCAS);
    rel(m_psSharpenLuma);
    rel(m_psVignette);

    rel(m_bloomSurfB); rel(m_bloomTexB);
    rel(m_bloomSurfA); rel(m_bloomTexA);
    rel(m_rtSurfB);    rel(m_rtTexB);
    rel(m_rtSurfA);    rel(m_rtTexA);

    m_ready = false;
}

// ============================================================================
// Shader Compilation
// ============================================================================

IDirect3DPixelShader9* PostProcessor::CompilePS(const std::string& path,
                                                 const char* entry)
{
    std::wstring wp(path.begin(), path.end());
    ID3DBlob* code = nullptr;
    ID3DBlob* errs = nullptr;
    HRESULT hr = D3DCompileFromFile(wp.c_str(), nullptr,
                                    D3D_COMPILE_STANDARD_FILE_INCLUDE,
                                    entry, "ps_3_0",
                                    D3DCOMPILE_OPTIMIZATION_LEVEL1,
                                    0, &code, &errs);
    if (FAILED(hr)) {
        std::string msg = "PS compile FAILED: " + path;
        if (errs) {
            msg += "\n";
            msg += std::string((char*)errs->GetBufferPointer(), errs->GetBufferSize());
            errs->Release();
        }
        LOG_ERROR(msg);
        return nullptr;
    }
    IDirect3DPixelShader9* ps = nullptr;
    m_device->CreatePixelShader(static_cast<DWORD*>(code->GetBufferPointer()), &ps);
    code->Release();
    if (errs) errs->Release();
    return ps;
}

void PostProcessor::CompileShaders(const std::string& dir)
{
    auto load = [&](IDirect3DPixelShader9*& ps, const char* file, const char* entry) {
        ps = CompilePS(dir + file, entry);
        if (ps) LOG_INFO(std::string("Shader OK: ") + file);
        else    LOG_WARN(std::string("Shader FAILED: ") + file);
    };

    load(m_psFXAA,          "fxaa.hlsl",            "PS_FXAA");
    load(m_psColor,         "color_boost.hlsl",      "PS_Color");
    load(m_psTonemap,       "tonemap.hlsl",          "PS_Tonemap");
    load(m_psBloomDown,     "bloom_downsample.hlsl", "PS_BloomDown");
    load(m_psBloomUp,       "bloom_upsample.hlsl",   "PS_BloomUp");
    load(m_psBloomComposite,"bloom_composite.hlsl",  "PS_BloomComposite");
    load(m_psSharpenCAS,    "sharpen_cas.hlsl",        "PS_SharpenCAS");
    load(m_psSharpenLuma,   "luma_sharpen.hlsl",       "PS_LumaSharpen");
    load(m_psVignette,      "vignette.hlsl",           "PS_Vignette");
    load(m_psAO,            "ambient_occlusion.hlsl",  "PS_AmbientOcclusion");
    load(m_psShadowDepth,   "shadow_depth.hlsl",       "PS_ShadowDepth");
}

// ============================================================================
// Device Reset
// ============================================================================

void PostProcessor::OnPreReset()
{
    LOG_INFO("PostProcessor: OnPreReset");
    ReleaseResources();
}

void PostProcessor::OnPostReset(IDirect3DDevice9* device,
                                const D3DPRESENT_PARAMETERS& pp)
{
    m_device = device;
    m_pp     = pp;
    m_width  = pp.BackBufferWidth;
    m_height = pp.BackBufferHeight;
    LOG_INFO("PostProcessor: OnPostReset " +
             std::to_string(m_width) + "x" + std::to_string(m_height));
    InitResources();
}

// ============================================================================
// Hot Reload
// ============================================================================

void PostProcessor::Reload(const std::string& basePath)
{
    LOG_INFO("PostProcessor: hot reload");
    auto rel = [](auto*& p) { if (p) { p->Release(); p = nullptr; } };
    rel(m_psFXAA); rel(m_psColor); rel(m_psTonemap);
    rel(m_psBloomDown); rel(m_psBloomUp); rel(m_psBloomComposite);
    rel(m_psSharpenCAS); rel(m_psSharpenLuma); rel(m_psVignette);
    rel(m_psAO); rel(m_psShadowDepth);

    m_shaderDir = basePath + "\\shaders\\";
    CompileShaders(m_shaderDir);
    LOG_INFO("Hot reload complete");
}

// ============================================================================
// State Save / Restore
// ============================================================================

void PostProcessor::SaveState()
{
    if (m_savedState) { m_savedState->Release(); m_savedState = nullptr; }
    m_device->CreateStateBlock(D3DSBT_ALL, &m_savedState);
    if (m_savedState) m_savedState->Capture();
}

void PostProcessor::RestoreState()
{
    if (m_savedState) {
        m_savedState->Apply();
        m_savedState->Release();
        m_savedState = nullptr;
    }
}

// ============================================================================
// Full-Screen Quad
// ============================================================================

void PostProcessor::DrawFSQ()
{
    m_device->SetStreamSource(0, m_fsqVB, 0, sizeof(FSQVertex));
    m_device->SetFVF(FSQVertex::FVF);
    m_device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
}

// ============================================================================
// Common Post-Processing State
// ============================================================================

static void SetPostState(IDirect3DDevice9* dev)
{
    dev->SetRenderState(D3DRS_ZENABLE,           FALSE);
    dev->SetRenderState(D3DRS_ZWRITEENABLE,      FALSE);
    dev->SetRenderState(D3DRS_ALPHABLENDENABLE,  FALSE);
    dev->SetRenderState(D3DRS_ALPHATESTENABLE,   FALSE);
    dev->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
    dev->SetRenderState(D3DRS_CULLMODE,          D3DCULL_NONE);
    dev->SetRenderState(D3DRS_COLORWRITEENABLE,  0xF);
    dev->SetRenderState(D3DRS_STENCILENABLE,     FALSE);
    dev->SetVertexShader(nullptr);  // use FVF pipeline, no VS
    dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    dev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
    dev->SetSamplerState(0, D3DSAMP_ADDRESSU,  D3DTADDRESS_CLAMP);
    dev->SetSamplerState(0, D3DSAMP_ADDRESSV,  D3DTADDRESS_CLAMP);
}

// Helper: set float4 PS constant
static void SetC(IDirect3DDevice9* dev, UINT reg,
                 float x, float y, float z, float w)
{
    float v[4] = { x, y, z, w };
    dev->SetPixelShaderConstantF(reg, v, 1);
}

// ============================================================================
// Shader Constant Setters
// ============================================================================

void PostProcessor::SetFXAAConstants()
{
    const Config& c = ConfigManager::Instance().Get();
    float rW = 1.0f / (float)m_width;
    float rH = 1.0f / (float)m_height;
    SetC(m_device, 0, rW, rH, c.aaEdgeThreshold, c.aaSubpixelQuality);
    SetC(m_device, 1, c.aaStrength, 0, 0, 0);
}

void PostProcessor::SetColorConstants()
{
    const Config& c = ConfigManager::Instance().Get();
    SetC(m_device, 2, c.saturation, c.vibrance, c.temperature, c.tint);
}

void PostProcessor::SetTonemapConstants()
{
    const Config& c = ConfigManager::Instance().Get();
    SetC(m_device, 3, c.exposure, c.gamma, c.contrast, c.highlights);
    SetC(m_device, 4, c.shadows, c.filmic ? 1.0f : 0.0f, 0, 0);
}

void PostProcessor::SetBloomConstants()
{
    const Config& c = ConfigManager::Instance().Get();
    float rW = 1.0f / (float)m_width;
    float rH = 1.0f / (float)m_height;
    SetC(m_device, 5, c.bloomThreshold, c.bloomIntensity, c.bloomRadius, c.bloomSoftKnee);
    SetC(m_device, 6, rW, rH, 0, 0);
}

void PostProcessor::SetSharpenConstants()
{
    const Config& c = ConfigManager::Instance().Get();
    float rW = 1.0f / (float)m_width;
    float rH = 1.0f / (float)m_height;
    SetC(m_device, 7, c.sharpenStrength, c.sharpenClamp, rW, rH);
}

void PostProcessor::SetVignetteConstants()
{
    const Config& c = ConfigManager::Instance().Get();
    SetC(m_device, 8, c.vignetteIntensity, c.vignetteRadius, c.vignetteSoftness, 0);
}

void PostProcessor::SetAOConstants()
{
    const Config& c = ConfigManager::Instance().Get();
    float rW = 1.0f / (float)m_width;
    float rH = 1.0f / (float)m_height;
    SetC(m_device, 0, rW, rH, c.aoRadius, c.aoStrength);
    SetC(m_device, 1, c.aoBias, 0, 0, 0);
}

void PostProcessor::SetShadowDepthConstants()
{
    const Config& c = ConfigManager::Instance().Get();
    SetC(m_device, 0, c.shadowDepth, c.shadowThreshold, c.shadowFeather, 0);
}

// ============================================================================
// Main Process — called from Present(), after game has finished rendering
// ============================================================================

void PostProcessor::Process()
{
    if (!m_ready) return;

    const Config& cfg = ConfigManager::Instance().Get();
    if (!cfg.enabled) return;

    m_timeAccum = fmodf(m_timeAccum + 0.016f, 100.0f);

    // -------------------------------------------------------------------------
    // 1. Capture the game's current render target (the completed frame).
    //    We use GetRenderTarget(0) — this is whatever the game last rendered to.
    //    We copy it to RT A and at the end write RT A back to the same surface.
    // -------------------------------------------------------------------------
    IDirect3DSurface9* gameSurf = nullptr;
    if (FAILED(m_device->GetRenderTarget(0, &gameSurf))) return;

    if (FAILED(m_device->StretchRect(gameSurf, nullptr,
                                     m_rtSurfA, nullptr, D3DTEXF_NONE))) {
        gameSurf->Release();
        return;
    }

    // -------------------------------------------------------------------------
    // 2. Save state, enter post-process render state
    // -------------------------------------------------------------------------
    SaveState();
    SetPostState(m_device);

    // -------------------------------------------------------------------------
    // 3. Effect passes — each one reads m_rtTexA and writes to m_rtSurfB,
    //    then copies B → A so the next pass always reads from A.
    // -------------------------------------------------------------------------

    // Simple single-pass helper
    auto pass = [&](IDirect3DPixelShader9* ps, auto setConst) {
        if (!ps) return;
        m_device->SetRenderTarget(0, m_rtSurfB);
        m_device->SetTexture(0, m_rtTexA);
        m_device->SetPixelShader(ps);
        setConst();
        DrawFSQ();
        m_device->StretchRect(m_rtSurfB, nullptr, m_rtSurfA, nullptr, D3DTEXF_NONE);
    };

    // 3a. FXAA
    if (cfg.aaEnabled && cfg.aaMethod == "FXAA" && m_psFXAA)
        pass(m_psFXAA, [&]{ SetFXAAConstants(); });

    // 3b. Tonemap
    if (cfg.tonemapEnabled && m_psTonemap)
        pass(m_psTonemap, [&]{ SetTonemapConstants(); });

    // 3c. Color boost
    if (cfg.colorEnabled && m_psColor)
        pass(m_psColor, [&]{ SetColorConstants(); });

    // 3d. Ambient Occlusion — after color, before bloom
    //     Darkens crevices, tire contact, panel gaps to simulate occlusion.
    if (cfg.aoEnabled && m_psAO)
        pass(m_psAO, [&]{ SetAOConstants(); });

    // 3e. Shadow Depth — deepens existing shadows for richer dark areas
    if (cfg.shadowDepthEnabled && m_psShadowDepth)
        pass(m_psShadowDepth, [&]{ SetShadowDepthConstants(); });

    // 3f. Bloom — multi-pass (down → down → up → composite)
    if (cfg.bloomEnabled && m_psBloomDown && m_psBloomUp && m_psBloomComposite) {
        D3DVIEWPORT9 vpFull, vpHalf, vpQuarter;
        m_device->GetViewport(&vpFull);
        vpHalf    = { 0, 0, m_width / 2, m_height / 2, 0.0f, 1.0f };
        vpQuarter = { 0, 0, m_width / 4, m_height / 4, 0.0f, 1.0f };

        // Downsample to half-res
        m_device->SetRenderTarget(0, m_bloomSurfA);
        m_device->SetTexture(0, m_rtTexA);
        m_device->SetPixelShader(m_psBloomDown);
        m_device->SetViewport(&vpHalf);
        SetBloomConstants();
        DrawFSQ();

        // Downsample to quarter-res
        m_device->SetRenderTarget(0, m_bloomSurfB);
        m_device->SetTexture(0, m_bloomTexA);
        m_device->SetViewport(&vpQuarter);
        DrawFSQ();

        // Upsample back to half-res
        m_device->SetRenderTarget(0, m_bloomSurfA);
        m_device->SetTexture(0, m_bloomTexB);
        m_device->SetPixelShader(m_psBloomUp);
        m_device->SetViewport(&vpHalf);
        DrawFSQ();

        // Composite bloom onto scene
        m_device->SetViewport(&vpFull);
        m_device->SetRenderTarget(0, m_rtSurfB);
        m_device->SetTexture(0, m_rtTexA);
        m_device->SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
        m_device->SetSamplerState(1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
        m_device->SetSamplerState(1, D3DSAMP_ADDRESSU,  D3DTADDRESS_CLAMP);
        m_device->SetSamplerState(1, D3DSAMP_ADDRESSV,  D3DTADDRESS_CLAMP);
        m_device->SetTexture(1, m_bloomTexA);
        m_device->SetPixelShader(m_psBloomComposite);
        SetBloomConstants();
        DrawFSQ();
        m_device->SetTexture(1, nullptr);
        m_device->StretchRect(m_rtSurfB, nullptr, m_rtSurfA, nullptr, D3DTEXF_NONE);
    }

    // 3g. Sharpen (CAS or Luma)
    if (cfg.sharpenEnabled) {
        IDirect3DPixelShader9* ps =
            (cfg.sharpenMethod == "CAS") ? m_psSharpenCAS : m_psSharpenLuma;
        pass(ps, [&]{ SetSharpenConstants(); });
    }

    // 3h. Vignette
    if (cfg.vignetteEnabled && m_psVignette)
        pass(m_psVignette, [&]{ SetVignetteConstants(); });

    // -------------------------------------------------------------------------
    // 4. Write processed frame back to the game's render target surface
    //    (the same surface we read from in step 1)
    // -------------------------------------------------------------------------
    m_device->SetTexture(0, nullptr);
    m_device->SetPixelShader(nullptr);
    m_device->SetRenderTarget(0, gameSurf);  // restore RT before StretchRect
    m_device->StretchRect(m_rtSurfA, nullptr, gameSurf, nullptr, D3DTEXF_NONE);

    gameSurf->Release();

    // -------------------------------------------------------------------------
    // 5. Restore full D3D9 state for the game
    // -------------------------------------------------------------------------
    RestoreState();
}

} // namespace SimuFX
