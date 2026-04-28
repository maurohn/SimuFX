#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d9.h>
#include "logger.h"
#include "dx9_proxy.h"

// ---------------------------------------------------------------------------
// Real d3d9 — loaded lazily via Win32 Interlocked, NO std::once_flag
// ---------------------------------------------------------------------------
static HMODULE  g_realD3D9  = nullptr;
static volatile LONG g_initDone = 0;     // 0 = not done, 1 = done
static CRITICAL_SECTION g_initCS;
static LONG     g_csReady   = 0;

using PFN_Create9   = IDirect3D9*  (WINAPI*)(UINT);
using PFN_Create9Ex = HRESULT      (WINAPI*)(UINT, IDirect3D9Ex**);

static void EnsureRealD3D9()
{
    // Fast path
    if (InterlockedCompareExchange(&g_initDone, 0, 0) == 1) return;

    // Lazy init of our CRITICAL_SECTION (once)
    if (InterlockedCompareExchange(&g_csReady, 1, 0) == 0)
        InitializeCriticalSection(&g_initCS);

    EnterCriticalSection(&g_initCS);

    if (g_initDone == 0) {
        // Derive exe directory
        char modPath[MAX_PATH] = {};
        GetModuleFileNameA(nullptr, modPath, MAX_PATH);
        char exeDir[MAX_PATH] = {};
        lstrcpynA(exeDir, modPath, MAX_PATH);
        // trim to directory
        char* last = exeDir;
        for (char* p = exeDir; *p; ++p) if (*p == '\\' || *p == '/') last = p;
        *last = '\0';

        // Init logger (pure Win32, no CRT needed)
        char logPath[MAX_PATH];
        wsprintfA(logPath, "%s\\SimuFX\\simufx.log", exeDir);
        SimuFX::Logger::Instance().Init(logPath);
        LOG_INFO("SimuFX " __DATE__ " " __TIME__ " loaded");
        LOG_INFO(modPath);

        // Load real d3d9 from System32
        char sysDir[MAX_PATH] = {};
        GetSystemDirectoryA(sysDir, MAX_PATH);
        char realPath[MAX_PATH];
        wsprintfA(realPath, "%s\\d3d9.dll", sysDir);

        g_realD3D9 = LoadLibraryA(realPath);
        if (g_realD3D9) {
            LOG_INFO("Real d3d9.dll loaded OK");
        } else {
            char err[64];
            wsprintfA(err, "FATAL: LoadLibrary failed, GLE=%u", GetLastError());
            LOG_ERROR(err);
        }

        InterlockedExchange(&g_initDone, 1);
    }

    LeaveCriticalSection(&g_initCS);
}

// ---------------------------------------------------------------------------
// DllMain — ABSOLUTELY MINIMAL. Only DisableThreadLibraryCalls.
// ---------------------------------------------------------------------------
BOOL WINAPI DllMain(HINSTANCE h, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
        DisableThreadLibraryCalls(h);
    return TRUE;
}

// ---------------------------------------------------------------------------
// Direct3DCreate9
// ---------------------------------------------------------------------------
IDirect3D9* WINAPI Direct3DCreate9(UINT SDKVersion)
{
    EnsureRealD3D9();
    if (!g_realD3D9) return nullptr;

    auto fn = (PFN_Create9)GetProcAddress(g_realD3D9, "Direct3DCreate9");
    if (!fn) { LOG_ERROR("GetProcAddress Direct3DCreate9 failed"); return nullptr; }

    IDirect3D9* real = fn(SDKVersion);
    if (!real)  { LOG_ERROR("Real Direct3DCreate9 returned null"); return nullptr; }

    LOG_INFO("Wrapping IDirect3D9");
    return new SimuFX::ProxyDirect3D9(real);
}

// ---------------------------------------------------------------------------
// Direct3DCreate9Ex — forward only, no wrapping needed for rFactor
// ---------------------------------------------------------------------------
HRESULT WINAPI Direct3DCreate9Ex(UINT SDKVersion, IDirect3D9Ex** ppD3D)
{
    EnsureRealD3D9();
    if (!g_realD3D9) return E_FAIL;

    auto fn = (PFN_Create9Ex)GetProcAddress(g_realD3D9, "Direct3DCreate9Ex");
    if (!fn) return E_NOTIMPL;
    return fn(SDKVersion, ppD3D);
}
