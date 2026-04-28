#pragma once
#include <d3d9.h>
#include <string>

namespace SimuFX {

// ImGui D3D9 overlay
class Overlay {
public:
    static Overlay& Instance() {
        static Overlay inst;
        return inst;
    }

    void Init(IDirect3DDevice9* device, const std::string& exeDir);
    void Shutdown();
    void Toggle() { m_visible = !m_visible; }
    bool IsVisible() const { return m_visible; }

    void Render(IDirect3DDevice9* device);

    // Device reset lifecycle
    void OnPreReset();
    void OnPostReset(IDirect3DDevice9* device);

private:
    Overlay() = default;
    ~Overlay() = default;
    Overlay(const Overlay&) = delete;
    Overlay& operator=(const Overlay&) = delete;

    bool        m_initialised = false;
    bool        m_visible     = false;
    HWND        m_hwnd        = nullptr;
    std::string m_exeDir;
    std::string m_cfgBase;

    // Performance counters
    float  m_fps       = 0.0f;
    float  m_frametime = 0.0f;
    LARGE_INTEGER m_lastTime = {};
    LARGE_INTEGER m_freq     = {};
};

} // namespace SimuFX
