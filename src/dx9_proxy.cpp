#include "dx9_proxy.h"
#include "device_hook.h"
#include "logger.h"

namespace SimuFX {

ProxyDirect3D9::ProxyDirect3D9(IDirect3D9* real)
    : m_real(real), m_ref(1)
{
    LOG_INFO("ProxyDirect3D9 created");
}

ProxyDirect3D9::~ProxyDirect3D9()
{
    LOG_INFO("ProxyDirect3D9 destroyed");
}

// ---- IUnknown ---------------------------------------------------------------

HRESULT STDMETHODCALLTYPE ProxyDirect3D9::QueryInterface(REFIID riid, void** ppvObj)
{
    if (riid == IID_IDirect3D9 || riid == IID_IUnknown) {
        *ppvObj = this;
        AddRef();
        return S_OK;
    }
    return m_real->QueryInterface(riid, ppvObj);
}

ULONG STDMETHODCALLTYPE ProxyDirect3D9::AddRef()
{
    return ++m_ref;
}

ULONG STDMETHODCALLTYPE ProxyDirect3D9::Release()
{
    ULONG r = --m_ref;
    if (r == 0) {
        m_real->Release();
        delete this;
    }
    return r;
}

// ---- CreateDevice intercept -------------------------------------------------

HRESULT STDMETHODCALLTYPE ProxyDirect3D9::CreateDevice(
    UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow,
    DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPP,
    IDirect3DDevice9** ppReturnedDevice)
{
    LOG_INFO("CreateDevice called — intercepting");

    IDirect3DDevice9* realDevice = nullptr;
    HRESULT hr = m_real->CreateDevice(Adapter, DeviceType, hFocusWindow,
                                      BehaviorFlags, pPP, &realDevice);
    if (FAILED(hr)) {
        LOG_ERROR("Real CreateDevice failed — HRESULT: " + std::to_string(hr));
        return hr;
    }

    *ppReturnedDevice = new ProxyDevice(realDevice, this, *pPP);
    LOG_INFO("ProxyDevice created and returned to game");
    return S_OK;
}

// ---- All remaining forwards --------------------------------------------------

HRESULT STDMETHODCALLTYPE ProxyDirect3D9::RegisterSoftwareDevice(void* p)
    { return m_real->RegisterSoftwareDevice(p); }
UINT STDMETHODCALLTYPE ProxyDirect3D9::GetAdapterCount()
    { return m_real->GetAdapterCount(); }
HRESULT STDMETHODCALLTYPE ProxyDirect3D9::GetAdapterIdentifier(UINT A, DWORD F, D3DADAPTER_IDENTIFIER9* I)
    { return m_real->GetAdapterIdentifier(A, F, I); }
UINT STDMETHODCALLTYPE ProxyDirect3D9::GetAdapterModeCount(UINT A, D3DFORMAT F)
    { return m_real->GetAdapterModeCount(A, F); }
HRESULT STDMETHODCALLTYPE ProxyDirect3D9::EnumAdapterModes(UINT A, D3DFORMAT F, UINT M, D3DDISPLAYMODE* D)
    { return m_real->EnumAdapterModes(A, F, M, D); }
HRESULT STDMETHODCALLTYPE ProxyDirect3D9::GetAdapterDisplayMode(UINT A, D3DDISPLAYMODE* D)
    { return m_real->GetAdapterDisplayMode(A, D); }
HRESULT STDMETHODCALLTYPE ProxyDirect3D9::CheckDeviceType(UINT A, D3DDEVTYPE DT, D3DFORMAT AF, D3DFORMAT BF, BOOL W)
    { return m_real->CheckDeviceType(A, DT, AF, BF, W); }
HRESULT STDMETHODCALLTYPE ProxyDirect3D9::CheckDeviceFormat(UINT A, D3DDEVTYPE DT, D3DFORMAT AF, DWORD U, D3DRESOURCETYPE R, D3DFORMAT CF)
    { return m_real->CheckDeviceFormat(A, DT, AF, U, R, CF); }
HRESULT STDMETHODCALLTYPE ProxyDirect3D9::CheckDeviceMultiSampleType(UINT A, D3DDEVTYPE DT, D3DFORMAT SF, BOOL W, D3DMULTISAMPLE_TYPE MST, DWORD* QL)
    { return m_real->CheckDeviceMultiSampleType(A, DT, SF, W, MST, QL); }
HRESULT STDMETHODCALLTYPE ProxyDirect3D9::CheckDepthStencilMatch(UINT A, D3DDEVTYPE DT, D3DFORMAT AF, D3DFORMAT RF, D3DFORMAT DF)
    { return m_real->CheckDepthStencilMatch(A, DT, AF, RF, DF); }
HRESULT STDMETHODCALLTYPE ProxyDirect3D9::CheckDeviceFormatConversion(UINT A, D3DDEVTYPE DT, D3DFORMAT SF, D3DFORMAT TF)
    { return m_real->CheckDeviceFormatConversion(A, DT, SF, TF); }
HRESULT STDMETHODCALLTYPE ProxyDirect3D9::GetDeviceCaps(UINT A, D3DDEVTYPE DT, D3DCAPS9* C)
    { return m_real->GetDeviceCaps(A, DT, C); }
HMONITOR STDMETHODCALLTYPE ProxyDirect3D9::GetAdapterMonitor(UINT A)
    { return m_real->GetAdapterMonitor(A); }

} // namespace SimuFX
