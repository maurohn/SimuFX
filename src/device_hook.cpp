#include "device_hook.h"
#include "effect_pipeline.h"
#include "config.h"
#include "logger.h"
#include "overlay.h"
#include "keys.h"

namespace SimuFX {

// ============================================================================
// Construction / Destruction
// ============================================================================

ProxyDevice::ProxyDevice(IDirect3DDevice9* real, IDirect3D9* creator,
                         D3DPRESENT_PARAMETERS pp)
    : m_real(real)
    , m_d3d(creator)
    , m_pp(pp)
    , m_ref(1)
    , m_postProc(nullptr)
    , m_inReset(false)
{
    LOG_INFO("ProxyDevice created — " +
             std::to_string(pp.BackBufferWidth) + "x" +
             std::to_string(pp.BackBufferHeight));

    // Derive base path (folder where rFactor.exe lives)
    char modPath[MAX_PATH];
    GetModuleFileNameA(nullptr, modPath, MAX_PATH);
    std::string exeDir(modPath);
    auto slash = exeDir.find_last_of("\\/");
    if (slash != std::string::npos) exeDir = exeDir.substr(0, slash);

    // Load config
    std::string cfgBase = exeDir + "\\SimuFX";
    ConfigManager::Instance().Load(cfgBase);

    // Create post-processor
    m_postProc = new PostProcessor(real, pp);

    // Init overlay (ImGui)
    Overlay::Instance().Init(real, exeDir);

    LOG_INFO("ProxyDevice ready");
}

ProxyDevice::~ProxyDevice()
{
    LOG_INFO("ProxyDevice destroyed");
    Overlay::Instance().Shutdown();
    delete m_postProc;
    m_postProc = nullptr;
    m_real->Release();
}

// ============================================================================
// IUnknown
// ============================================================================

HRESULT STDMETHODCALLTYPE ProxyDevice::QueryInterface(REFIID riid, void** ppvObj)
{
    if (riid == IID_IDirect3DDevice9 || riid == IID_IUnknown) {
        *ppvObj = this;
        AddRef();
        return S_OK;
    }
    return m_real->QueryInterface(riid, ppvObj);
}

ULONG STDMETHODCALLTYPE ProxyDevice::AddRef()  { return ++m_ref; }
ULONG STDMETHODCALLTYPE ProxyDevice::Release()
{
    ULONG r = --m_ref;
    if (r == 0) delete this;
    return r;
}

// ============================================================================
// EndScene — ImGui rendering happens here (before the game's own UI clears)
// ============================================================================

HRESULT STDMETHODCALLTYPE ProxyDevice::EndScene()
{
    // Handle F10 / F9
    KeyManager::Instance().Update();

    const Config& cfg = ConfigManager::Instance().Get();

    if (KeyManager::Instance().Pressed(cfg.toggleKey)) {
        Overlay::Instance().Toggle();
    }

    if (KeyManager::Instance().Pressed(cfg.reloadKey)) {
        char modPath[MAX_PATH];
        GetModuleFileNameA(nullptr, modPath, MAX_PATH);
        std::string exeDir(modPath);
        auto sl = exeDir.find_last_of("\\/");
        if (sl != std::string::npos) exeDir = exeDir.substr(0, sl);
        std::string cfgBase = exeDir + "\\SimuFX";
        ConfigManager::Instance().Load(cfgBase);
        if (m_postProc) m_postProc->Reload(cfgBase);
        LOG_INFO("Hot reload complete");
    }

    // Draw ImGui overlay if visible
    if (Overlay::Instance().IsVisible()) {
        Overlay::Instance().Render(m_real);
    }

    return m_real->EndScene();
}

// ============================================================================
// Present — post-processing applied here
// ============================================================================

HRESULT STDMETHODCALLTYPE ProxyDevice::Present(const RECT* pSrc, const RECT* pDst,
                                                HWND hWnd, const RGNDATA* pDirty)
{
    if (m_postProc && !m_inReset) {
        m_postProc->Process();
    }
    return m_real->Present(pSrc, pDst, hWnd, pDirty);
}

// ============================================================================
// Reset — critical: release and recreate all default-pool resources
// ============================================================================

HRESULT STDMETHODCALLTYPE ProxyDevice::Reset(D3DPRESENT_PARAMETERS* pPP)
{
    LOG_INFO("Reset called");
    m_inReset = true;

    if (m_postProc) m_postProc->OnPreReset();
    Overlay::Instance().OnPreReset();

    HRESULT hr = m_real->Reset(pPP);

    if (SUCCEEDED(hr)) {
        m_pp = *pPP;
        if (m_postProc) m_postProc->OnPostReset(m_real, *pPP);
        Overlay::Instance().OnPostReset(m_real);
        LOG_INFO("Reset succeeded — " +
                 std::to_string(pPP->BackBufferWidth) + "x" +
                 std::to_string(pPP->BackBufferHeight));
    } else {
        LOG_ERROR("Reset failed — HRESULT: " + std::to_string(hr));
    }

    m_inReset = false;
    return hr;
}

// ============================================================================
// All remaining IDirect3DDevice9 forwards
// ============================================================================

HRESULT STDMETHODCALLTYPE ProxyDevice::TestCooperativeLevel()                       { return m_real->TestCooperativeLevel(); }
UINT    STDMETHODCALLTYPE ProxyDevice::GetAvailableTextureMem()                     { return m_real->GetAvailableTextureMem(); }
HRESULT STDMETHODCALLTYPE ProxyDevice::EvictManagedResources()                      { return m_real->EvictManagedResources(); }
HRESULT STDMETHODCALLTYPE ProxyDevice::GetDirect3D(IDirect3D9** p)                  { return m_real->GetDirect3D(p); }
HRESULT STDMETHODCALLTYPE ProxyDevice::GetDeviceCaps(D3DCAPS9* p)                   { return m_real->GetDeviceCaps(p); }
HRESULT STDMETHODCALLTYPE ProxyDevice::GetDisplayMode(UINT i, D3DDISPLAYMODE* p)    { return m_real->GetDisplayMode(i, p); }
HRESULT STDMETHODCALLTYPE ProxyDevice::GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS* p) { return m_real->GetCreationParameters(p); }
HRESULT STDMETHODCALLTYPE ProxyDevice::SetCursorProperties(UINT x, UINT y, IDirect3DSurface9* s) { return m_real->SetCursorProperties(x, y, s); }
void    STDMETHODCALLTYPE ProxyDevice::SetCursorPosition(int x, int y, DWORD f)     { m_real->SetCursorPosition(x, y, f); }
BOOL    STDMETHODCALLTYPE ProxyDevice::ShowCursor(BOOL b)                           { return m_real->ShowCursor(b); }
HRESULT STDMETHODCALLTYPE ProxyDevice::CreateAdditionalSwapChain(D3DPRESENT_PARAMETERS* p, IDirect3DSwapChain9** s) { return m_real->CreateAdditionalSwapChain(p, s); }
HRESULT STDMETHODCALLTYPE ProxyDevice::GetSwapChain(UINT i, IDirect3DSwapChain9** s){ return m_real->GetSwapChain(i, s); }
UINT    STDMETHODCALLTYPE ProxyDevice::GetNumberOfSwapChains()                      { return m_real->GetNumberOfSwapChains(); }
HRESULT STDMETHODCALLTYPE ProxyDevice::SetDialogBoxMode(BOOL b)                     { return m_real->SetDialogBoxMode(b); }
void    STDMETHODCALLTYPE ProxyDevice::SetGammaRamp(UINT i, DWORD f, const D3DGAMMARAMP* r) { m_real->SetGammaRamp(i, f, r); }
void    STDMETHODCALLTYPE ProxyDevice::GetGammaRamp(UINT i, D3DGAMMARAMP* r)        { m_real->GetGammaRamp(i, r); }
HRESULT STDMETHODCALLTYPE ProxyDevice::CreateTexture(UINT w, UINT h, UINT l, DWORD u, D3DFORMAT f, D3DPOOL p, IDirect3DTexture9** t, HANDLE* sh) { return m_real->CreateTexture(w,h,l,u,f,p,t,sh); }
HRESULT STDMETHODCALLTYPE ProxyDevice::CreateVolumeTexture(UINT w, UINT h, UINT d, UINT l, DWORD u, D3DFORMAT f, D3DPOOL p, IDirect3DVolumeTexture9** t, HANDLE* sh) { return m_real->CreateVolumeTexture(w,h,d,l,u,f,p,t,sh); }
HRESULT STDMETHODCALLTYPE ProxyDevice::CreateCubeTexture(UINT e, UINT l, DWORD u, D3DFORMAT f, D3DPOOL p, IDirect3DCubeTexture9** t, HANDLE* sh) { return m_real->CreateCubeTexture(e,l,u,f,p,t,sh); }
HRESULT STDMETHODCALLTYPE ProxyDevice::CreateVertexBuffer(UINT l, DWORD u, DWORD fvf, D3DPOOL p, IDirect3DVertexBuffer9** b, HANDLE* sh) { return m_real->CreateVertexBuffer(l,u,fvf,p,b,sh); }
HRESULT STDMETHODCALLTYPE ProxyDevice::CreateIndexBuffer(UINT l, DWORD u, D3DFORMAT f, D3DPOOL p, IDirect3DIndexBuffer9** b, HANDLE* sh) { return m_real->CreateIndexBuffer(l,u,f,p,b,sh); }
HRESULT STDMETHODCALLTYPE ProxyDevice::CreateRenderTarget(UINT w, UINT h, D3DFORMAT f, D3DMULTISAMPLE_TYPE ms, DWORD mq, BOOL lk, IDirect3DSurface9** s, HANDLE* sh) { return m_real->CreateRenderTarget(w,h,f,ms,mq,lk,s,sh); }
HRESULT STDMETHODCALLTYPE ProxyDevice::CreateDepthStencilSurface(UINT w, UINT h, D3DFORMAT f, D3DMULTISAMPLE_TYPE ms, DWORD mq, BOOL d, IDirect3DSurface9** s, HANDLE* sh) { return m_real->CreateDepthStencilSurface(w,h,f,ms,mq,d,s,sh); }
HRESULT STDMETHODCALLTYPE ProxyDevice::UpdateSurface(IDirect3DSurface9* s, const RECT* sr, IDirect3DSurface9* d, const POINT* dp) { return m_real->UpdateSurface(s,sr,d,dp); }
HRESULT STDMETHODCALLTYPE ProxyDevice::UpdateTexture(IDirect3DBaseTexture9* s, IDirect3DBaseTexture9* d) { return m_real->UpdateTexture(s,d); }
HRESULT STDMETHODCALLTYPE ProxyDevice::GetRenderTargetData(IDirect3DSurface9* r, IDirect3DSurface9* d) { return m_real->GetRenderTargetData(r,d); }
HRESULT STDMETHODCALLTYPE ProxyDevice::GetFrontBufferData(UINT i, IDirect3DSurface9* d) { return m_real->GetFrontBufferData(i,d); }
HRESULT STDMETHODCALLTYPE ProxyDevice::StretchRect(IDirect3DSurface9* s, const RECT* sr, IDirect3DSurface9* d, const RECT* dr, D3DTEXTUREFILTERTYPE f) { return m_real->StretchRect(s,sr,d,dr,f); }
HRESULT STDMETHODCALLTYPE ProxyDevice::ColorFill(IDirect3DSurface9* s, const RECT* r, D3DCOLOR c) { return m_real->ColorFill(s,r,c); }
HRESULT STDMETHODCALLTYPE ProxyDevice::CreateOffscreenPlainSurface(UINT w, UINT h, D3DFORMAT f, D3DPOOL p, IDirect3DSurface9** s, HANDLE* sh) { return m_real->CreateOffscreenPlainSurface(w,h,f,p,s,sh); }
HRESULT STDMETHODCALLTYPE ProxyDevice::SetRenderTarget(DWORD i, IDirect3DSurface9* s) { return m_real->SetRenderTarget(i,s); }
HRESULT STDMETHODCALLTYPE ProxyDevice::GetRenderTarget(DWORD i, IDirect3DSurface9** s) { return m_real->GetRenderTarget(i,s); }
HRESULT STDMETHODCALLTYPE ProxyDevice::SetDepthStencilSurface(IDirect3DSurface9* s) { return m_real->SetDepthStencilSurface(s); }
HRESULT STDMETHODCALLTYPE ProxyDevice::GetDepthStencilSurface(IDirect3DSurface9** s) { return m_real->GetDepthStencilSurface(s); }
HRESULT STDMETHODCALLTYPE ProxyDevice::BeginScene()                                 { return m_real->BeginScene(); }
HRESULT STDMETHODCALLTYPE ProxyDevice::Clear(DWORD c, const D3DRECT* r, DWORD f, D3DCOLOR col, float z, DWORD s) { return m_real->Clear(c,r,f,col,z,s); }
HRESULT STDMETHODCALLTYPE ProxyDevice::SetTransform(D3DTRANSFORMSTATETYPE s, const D3DMATRIX* m) { return m_real->SetTransform(s,m); }
HRESULT STDMETHODCALLTYPE ProxyDevice::GetTransform(D3DTRANSFORMSTATETYPE s, D3DMATRIX* m) { return m_real->GetTransform(s,m); }
HRESULT STDMETHODCALLTYPE ProxyDevice::MultiplyTransform(D3DTRANSFORMSTATETYPE s, const D3DMATRIX* m) { return m_real->MultiplyTransform(s,m); }
HRESULT STDMETHODCALLTYPE ProxyDevice::SetViewport(const D3DVIEWPORT9* v)           { return m_real->SetViewport(v); }
HRESULT STDMETHODCALLTYPE ProxyDevice::GetViewport(D3DVIEWPORT9* v)                 { return m_real->GetViewport(v); }
HRESULT STDMETHODCALLTYPE ProxyDevice::SetMaterial(const D3DMATERIAL9* m)           { return m_real->SetMaterial(m); }
HRESULT STDMETHODCALLTYPE ProxyDevice::GetMaterial(D3DMATERIAL9* m)                 { return m_real->GetMaterial(m); }
HRESULT STDMETHODCALLTYPE ProxyDevice::SetLight(DWORD i, const D3DLIGHT9* l)        { return m_real->SetLight(i,l); }
HRESULT STDMETHODCALLTYPE ProxyDevice::GetLight(DWORD i, D3DLIGHT9* l)              { return m_real->GetLight(i,l); }
HRESULT STDMETHODCALLTYPE ProxyDevice::LightEnable(DWORD i, BOOL e)                 { return m_real->LightEnable(i,e); }
HRESULT STDMETHODCALLTYPE ProxyDevice::GetLightEnable(DWORD i, BOOL* e)             { return m_real->GetLightEnable(i,e); }
HRESULT STDMETHODCALLTYPE ProxyDevice::SetClipPlane(DWORD i, const float* p)        { return m_real->SetClipPlane(i,p); }
HRESULT STDMETHODCALLTYPE ProxyDevice::GetClipPlane(DWORD i, float* p)              { return m_real->GetClipPlane(i,p); }
HRESULT STDMETHODCALLTYPE ProxyDevice::SetRenderState(D3DRENDERSTATETYPE s, DWORD v){ return m_real->SetRenderState(s,v); }
HRESULT STDMETHODCALLTYPE ProxyDevice::GetRenderState(D3DRENDERSTATETYPE s, DWORD* v){ return m_real->GetRenderState(s,v); }
HRESULT STDMETHODCALLTYPE ProxyDevice::CreateStateBlock(D3DSTATEBLOCKTYPE t, IDirect3DStateBlock9** sb) { return m_real->CreateStateBlock(t,sb); }
HRESULT STDMETHODCALLTYPE ProxyDevice::BeginStateBlock()                            { return m_real->BeginStateBlock(); }
HRESULT STDMETHODCALLTYPE ProxyDevice::EndStateBlock(IDirect3DStateBlock9** sb)     { return m_real->EndStateBlock(sb); }
HRESULT STDMETHODCALLTYPE ProxyDevice::SetClipStatus(const D3DCLIPSTATUS9* c)       { return m_real->SetClipStatus(c); }
HRESULT STDMETHODCALLTYPE ProxyDevice::GetClipStatus(D3DCLIPSTATUS9* c)             { return m_real->GetClipStatus(c); }
HRESULT STDMETHODCALLTYPE ProxyDevice::GetTexture(DWORD s, IDirect3DBaseTexture9** t) { return m_real->GetTexture(s,t); }
HRESULT STDMETHODCALLTYPE ProxyDevice::SetTexture(DWORD s, IDirect3DBaseTexture9* t) { return m_real->SetTexture(s,t); }
HRESULT STDMETHODCALLTYPE ProxyDevice::GetTextureStageState(DWORD s, D3DTEXTURESTAGESTATETYPE t, DWORD* v) { return m_real->GetTextureStageState(s,t,v); }
HRESULT STDMETHODCALLTYPE ProxyDevice::SetTextureStageState(DWORD s, D3DTEXTURESTAGESTATETYPE t, DWORD v) { return m_real->SetTextureStageState(s,t,v); }
HRESULT STDMETHODCALLTYPE ProxyDevice::GetSamplerState(DWORD s, D3DSAMPLERSTATETYPE t, DWORD* v) { return m_real->GetSamplerState(s,t,v); }
HRESULT STDMETHODCALLTYPE ProxyDevice::SetSamplerState(DWORD s, D3DSAMPLERSTATETYPE t, DWORD v)
{
    // Upgrade game texture filtering to Anisotropic 16x for smoother textures.
    // Only apply to game-owned samplers (not our post-processing ones).
    // Our pipeline calls SetSamplerState directly on m_real, so no double-upgrade.
    if (t == D3DSAMP_MINFILTER && v == D3DTEXF_LINEAR) v = D3DTEXF_ANISOTROPIC;
    if (t == D3DSAMP_MAGFILTER && v == D3DTEXF_LINEAR) v = D3DTEXF_ANISOTROPIC;
    if (t == D3DSAMP_MIPFILTER && v == D3DTEXF_LINEAR) v = D3DTEXF_LINEAR; // keep mip linear
    if (t == D3DSAMP_MAXANISOTROPY) v = 16; // force 16x anisotropy
    return m_real->SetSamplerState(s, t, v);
}

HRESULT STDMETHODCALLTYPE ProxyDevice::ValidateDevice(DWORD* p)                     { return m_real->ValidateDevice(p); }
HRESULT STDMETHODCALLTYPE ProxyDevice::SetPaletteEntries(UINT n, const PALETTEENTRY* e) { return m_real->SetPaletteEntries(n,e); }
HRESULT STDMETHODCALLTYPE ProxyDevice::GetPaletteEntries(UINT n, PALETTEENTRY* e)   { return m_real->GetPaletteEntries(n,e); }
HRESULT STDMETHODCALLTYPE ProxyDevice::SetCurrentTexturePalette(UINT n)             { return m_real->SetCurrentTexturePalette(n); }
HRESULT STDMETHODCALLTYPE ProxyDevice::GetCurrentTexturePalette(UINT* n)            { return m_real->GetCurrentTexturePalette(n); }
HRESULT STDMETHODCALLTYPE ProxyDevice::SetScissorRect(const RECT* r)                { return m_real->SetScissorRect(r); }
HRESULT STDMETHODCALLTYPE ProxyDevice::GetScissorRect(RECT* r)                      { return m_real->GetScissorRect(r); }
HRESULT STDMETHODCALLTYPE ProxyDevice::SetSoftwareVertexProcessing(BOOL b)          { return m_real->SetSoftwareVertexProcessing(b); }
BOOL    STDMETHODCALLTYPE ProxyDevice::GetSoftwareVertexProcessing()                { return m_real->GetSoftwareVertexProcessing(); }
HRESULT STDMETHODCALLTYPE ProxyDevice::SetNPatchMode(float n)                       { return m_real->SetNPatchMode(n); }
float   STDMETHODCALLTYPE ProxyDevice::GetNPatchMode()                              { return m_real->GetNPatchMode(); }
HRESULT STDMETHODCALLTYPE ProxyDevice::DrawPrimitive(D3DPRIMITIVETYPE t, UINT s, UINT p) { return m_real->DrawPrimitive(t,s,p); }
HRESULT STDMETHODCALLTYPE ProxyDevice::DrawIndexedPrimitive(D3DPRIMITIVETYPE t, INT b, UINT mn, UINT nv, UINT si, UINT pc) { return m_real->DrawIndexedPrimitive(t,b,mn,nv,si,pc); }
HRESULT STDMETHODCALLTYPE ProxyDevice::DrawPrimitiveUP(D3DPRIMITIVETYPE t, UINT pc, const void* d, UINT s) { return m_real->DrawPrimitiveUP(t,pc,d,s); }
HRESULT STDMETHODCALLTYPE ProxyDevice::DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE t, UINT mn, UINT nv, UINT pc, const void* id, D3DFORMAT idf, const void* vd, UINT vs) { return m_real->DrawIndexedPrimitiveUP(t,mn,nv,pc,id,idf,vd,vs); }
HRESULT STDMETHODCALLTYPE ProxyDevice::ProcessVertices(UINT si, UINT di, UINT vc, IDirect3DVertexBuffer9* db, IDirect3DVertexDeclaration9* vd, DWORD f) { return m_real->ProcessVertices(si,di,vc,db,vd,f); }
HRESULT STDMETHODCALLTYPE ProxyDevice::CreateVertexDeclaration(const D3DVERTEXELEMENT9* e, IDirect3DVertexDeclaration9** d) { return m_real->CreateVertexDeclaration(e,d); }
HRESULT STDMETHODCALLTYPE ProxyDevice::SetVertexDeclaration(IDirect3DVertexDeclaration9* d) { return m_real->SetVertexDeclaration(d); }
HRESULT STDMETHODCALLTYPE ProxyDevice::GetVertexDeclaration(IDirect3DVertexDeclaration9** d) { return m_real->GetVertexDeclaration(d); }
HRESULT STDMETHODCALLTYPE ProxyDevice::SetFVF(DWORD f)                              { return m_real->SetFVF(f); }
HRESULT STDMETHODCALLTYPE ProxyDevice::GetFVF(DWORD* f)                             { return m_real->GetFVF(f); }
HRESULT STDMETHODCALLTYPE ProxyDevice::CreateVertexShader(const DWORD* fn, IDirect3DVertexShader9** s) { return m_real->CreateVertexShader(fn,s); }
HRESULT STDMETHODCALLTYPE ProxyDevice::SetVertexShader(IDirect3DVertexShader9* s)   { return m_real->SetVertexShader(s); }
HRESULT STDMETHODCALLTYPE ProxyDevice::GetVertexShader(IDirect3DVertexShader9** s)  { return m_real->GetVertexShader(s); }
HRESULT STDMETHODCALLTYPE ProxyDevice::SetVertexShaderConstantF(UINT r, const float* d, UINT c) { return m_real->SetVertexShaderConstantF(r,d,c); }
HRESULT STDMETHODCALLTYPE ProxyDevice::GetVertexShaderConstantF(UINT r, float* d, UINT c) { return m_real->GetVertexShaderConstantF(r,d,c); }
HRESULT STDMETHODCALLTYPE ProxyDevice::SetVertexShaderConstantI(UINT r, const int* d, UINT c) { return m_real->SetVertexShaderConstantI(r,d,c); }
HRESULT STDMETHODCALLTYPE ProxyDevice::GetVertexShaderConstantI(UINT r, int* d, UINT c) { return m_real->GetVertexShaderConstantI(r,d,c); }
HRESULT STDMETHODCALLTYPE ProxyDevice::SetVertexShaderConstantB(UINT r, const BOOL* d, UINT c) { return m_real->SetVertexShaderConstantB(r,d,c); }
HRESULT STDMETHODCALLTYPE ProxyDevice::GetVertexShaderConstantB(UINT r, BOOL* d, UINT c) { return m_real->GetVertexShaderConstantB(r,d,c); }
HRESULT STDMETHODCALLTYPE ProxyDevice::SetStreamSource(UINT sn, IDirect3DVertexBuffer9* sd, UINT ob, UINT s) { return m_real->SetStreamSource(sn,sd,ob,s); }
HRESULT STDMETHODCALLTYPE ProxyDevice::GetStreamSource(UINT sn, IDirect3DVertexBuffer9** sd, UINT* ob, UINT* s) { return m_real->GetStreamSource(sn,sd,ob,s); }
HRESULT STDMETHODCALLTYPE ProxyDevice::SetStreamSourceFreq(UINT sn, UINT s)         { return m_real->SetStreamSourceFreq(sn,s); }
HRESULT STDMETHODCALLTYPE ProxyDevice::GetStreamSourceFreq(UINT sn, UINT* s)        { return m_real->GetStreamSourceFreq(sn,s); }
HRESULT STDMETHODCALLTYPE ProxyDevice::SetIndices(IDirect3DIndexBuffer9* ib)        { return m_real->SetIndices(ib); }
HRESULT STDMETHODCALLTYPE ProxyDevice::GetIndices(IDirect3DIndexBuffer9** ib)       { return m_real->GetIndices(ib); }
HRESULT STDMETHODCALLTYPE ProxyDevice::CreatePixelShader(const DWORD* fn, IDirect3DPixelShader9** s) { return m_real->CreatePixelShader(fn,s); }
HRESULT STDMETHODCALLTYPE ProxyDevice::SetPixelShader(IDirect3DPixelShader9* s)     { return m_real->SetPixelShader(s); }
HRESULT STDMETHODCALLTYPE ProxyDevice::GetPixelShader(IDirect3DPixelShader9** s)    { return m_real->GetPixelShader(s); }
HRESULT STDMETHODCALLTYPE ProxyDevice::SetPixelShaderConstantF(UINT r, const float* d, UINT c) { return m_real->SetPixelShaderConstantF(r,d,c); }
HRESULT STDMETHODCALLTYPE ProxyDevice::GetPixelShaderConstantF(UINT r, float* d, UINT c) { return m_real->GetPixelShaderConstantF(r,d,c); }
HRESULT STDMETHODCALLTYPE ProxyDevice::SetPixelShaderConstantI(UINT r, const int* d, UINT c) { return m_real->SetPixelShaderConstantI(r,d,c); }
HRESULT STDMETHODCALLTYPE ProxyDevice::GetPixelShaderConstantI(UINT r, int* d, UINT c) { return m_real->GetPixelShaderConstantI(r,d,c); }
HRESULT STDMETHODCALLTYPE ProxyDevice::SetPixelShaderConstantB(UINT r, const BOOL* d, UINT c) { return m_real->SetPixelShaderConstantB(r,d,c); }
HRESULT STDMETHODCALLTYPE ProxyDevice::GetPixelShaderConstantB(UINT r, BOOL* d, UINT c) { return m_real->GetPixelShaderConstantB(r,d,c); }
HRESULT STDMETHODCALLTYPE ProxyDevice::DrawRectPatch(UINT h, const float* ns, const D3DRECTPATCH_INFO* ri) { return m_real->DrawRectPatch(h,ns,ri); }
HRESULT STDMETHODCALLTYPE ProxyDevice::DrawTriPatch(UINT h, const float* ns, const D3DTRIPATCH_INFO* ti) { return m_real->DrawTriPatch(h,ns,ti); }
HRESULT STDMETHODCALLTYPE ProxyDevice::DeletePatch(UINT h)                          { return m_real->DeletePatch(h); }
HRESULT STDMETHODCALLTYPE ProxyDevice::GetBackBuffer(UINT i, UINT j, D3DBACKBUFFER_TYPE t, IDirect3DSurface9** s) { return m_real->GetBackBuffer(i,j,t,s); }
HRESULT STDMETHODCALLTYPE ProxyDevice::GetRasterStatus(UINT i, D3DRASTER_STATUS* r) { return m_real->GetRasterStatus(i,r); }
HRESULT STDMETHODCALLTYPE ProxyDevice::CreateQuery(D3DQUERYTYPE t, IDirect3DQuery9** q) { return m_real->CreateQuery(t,q); }

} // namespace SimuFX
