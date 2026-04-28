// SimuFX minimal proxy — pure Win32, zero STL statics
// Tests that the proxy mechanism works before adding any features.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d9.h>

static HMODULE g_real = nullptr;
using PFN_Create9   = IDirect3D9*  (WINAPI*)(UINT);
using PFN_Create9Ex = HRESULT      (WINAPI*)(UINT, IDirect3D9Ex**);

static void LoadReal()
{
    if (g_real) return;
    char sys[MAX_PATH];
    GetSystemDirectoryA(sys, MAX_PATH);
    lstrcat(sys, "\\system32_d3d9_real.dll"); // won't exist — fallback below
    g_real = LoadLibraryA(sys);
    if (!g_real) {
        // Standard path: load from the real system32
        GetSystemDirectoryA(sys, MAX_PATH);
        char path[MAX_PATH];
        wsprintfA(path, "%s\\d3d9.dll", sys);
        g_real = LoadLibraryA(path);
    }
    // Write a simple marker to log
    char exePath[MAX_PATH];
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    // Find last slash
    char* last = exePath;
    for (char* p = exePath; *p; ++p) if (*p == '\\' || *p == '/') last = p;
    *last = 0; // truncate to dir
    char logPath[MAX_PATH];
    wsprintfA(logPath, "%s\\SimuFX\\simufx.log", exePath);
    HANDLE hLog = CreateFileA(logPath, GENERIC_WRITE, FILE_SHARE_READ,
                              nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hLog != INVALID_HANDLE_VALUE) {
        const char* msg = g_real
            ? "[SimuFX MINIMAL] DLL loaded. Real d3d9 OK.\r\n"
            : "[SimuFX MINIMAL] DLL loaded. FAILED to load real d3d9!\r\n";
        DWORD written;
        WriteFile(hLog, msg, lstrlenA(msg), &written, nullptr);
        CloseHandle(hLog);
    }
}

BOOL WINAPI DllMain(HINSTANCE h, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH) DisableThreadLibraryCalls(h);
    return TRUE;
}

IDirect3D9* WINAPI Direct3DCreate9(UINT sdk)
{
    LoadReal();
    if (!g_real) return nullptr;
    auto fn = (PFN_Create9)GetProcAddress(g_real, "Direct3DCreate9");
    return fn ? fn(sdk) : nullptr;
}

HRESULT WINAPI Direct3DCreate9Ex(UINT sdk, IDirect3D9Ex** pp)
{
    LoadReal();
    if (!g_real) return E_FAIL;
    auto fn = (PFN_Create9Ex)GetProcAddress(g_real, "Direct3DCreate9Ex");
    return fn ? fn(sdk, pp) : E_NOTIMPL;
}
