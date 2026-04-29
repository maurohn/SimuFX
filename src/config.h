#pragma once
#include <string>
#include <windows.h>

namespace SimuFX {

// Global configuration loaded from SimuFX/global.ini (+ active preset)
struct Config {
    // General
    bool        enabled         = true;
    std::string preset          = "Dynamic";
    bool        showOverlay     = true;
    int         toggleKey       = VK_F10;
    int         reloadKey       = VK_F9;

    // Color
    bool  colorEnabled   = true;
    float saturation     = 1.25f;
    float vibrance       = 0.35f;
    float temperature    = 0.03f;
    float tint           = 0.00f;

    // Tonemap
    bool  tonemapEnabled = true;
    float exposure       = 0.05f;
    float gamma          = 1.00f;
    float contrast       = 1.18f;
    float highlights     = 0.90f;
    float shadows        = 1.05f;
    bool  filmic         = true;

    // Bloom
    bool  bloomEnabled   = true;
    float bloomThreshold = 0.78f;
    float bloomIntensity = 0.28f;
    float bloomRadius    = 0.65f;
    float bloomSoftKnee  = 0.45f;

    // Sharpen
    bool        sharpenEnabled = true;
    std::string sharpenMethod  = "CAS";  // "CAS" | "Luma"
    float       sharpenStrength= 0.55f;
    float       sharpenClamp   = 0.035f;

    // AntiAliasing
    bool        aaEnabled        = true;
    std::string aaMethod         = "FXAA"; // "FXAA" | "SMAA"
    float       aaStrength       = 0.85f;
    float       aaEdgeThreshold  = 0.125f;
    float       aaSubpixelQuality= 0.75f;

    // Vignette
    bool  vignetteEnabled  = false;
    float vignetteIntensity= 0.08f;
    float vignetteRadius   = 0.85f;
    float vignetteSoftness = 0.45f;

    // Ambient Occlusion (color-based SSAO approximation)
    bool  aoEnabled  = false;
    float aoStrength = 0.65f;   // 0=none, 1=max
    float aoRadius   = 3.0f;    // sample radius in pixels
    float aoBias     = 0.02f;   // min luma diff to count as occlusion

    // Shadow Depth Enhancement
    bool  shadowDepthEnabled  = false;
    float shadowDepth         = 0.55f;   // 0=none, 1=max
    float shadowThreshold     = 0.45f;   // luma cutoff
    float shadowFeather       = 0.20f;   // transition softness
};

class ConfigManager {
public:
    static ConfigManager& Instance() {
        static ConfigManager inst;
        return inst;
    }

    // Load global.ini, then overlay the active preset on top
    bool Load(const std::string& basePath);

    // Save current config back to global.ini
    bool Save(const std::string& basePath) const;

    // Save current effect settings as a named preset file
    bool SavePreset(const std::string& basePath, const std::string& name) const;

    Config&       Get()       { return m_cfg; }
    const Config& Get() const { return m_cfg; }

    // Apply a named preset from basePath/presets/<name>.ini
    bool ApplyPreset(const std::string& basePath, const std::string& name);

private:
    ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    bool ParseFile(const std::string& filePath, Config& cfg);
    void WriteKey(std::ofstream& f, const char* section,
                  const char* key, const std::string& val) const;

    static std::string ReadIni(const std::string& file,
                               const std::string& section,
                               const std::string& key,
                               const std::string& def);

    Config m_cfg;
};

} // namespace SimuFX
