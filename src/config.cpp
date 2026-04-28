#include "config.h"
#include "logger.h"
#include <fstream>
#include <sstream>
#include <windows.h>   // GetPrivateProfileString / WritePrivateProfileString

namespace SimuFX {

// --------------------------------------------------------------------------
// Small portable INI reader that doesn't need an absolute path trick
// --------------------------------------------------------------------------
static std::string ReadPrivate(const std::string& file,
                               const std::string& section,
                               const std::string& key,
                               const std::string& def)
{
    char buf[1024] = {};
    GetPrivateProfileStringA(section.c_str(), key.c_str(), def.c_str(),
                             buf, sizeof(buf), file.c_str());
    return buf;
}

static float ReadF(const std::string& f, const std::string& s,
                   const std::string& k, float d)
{
    std::string v = ReadPrivate(f, s, k, std::to_string(d));
    try { return std::stof(v); } catch (...) { return d; }
}

static bool ReadB(const std::string& f, const std::string& s,
                  const std::string& k, bool d)
{
    std::string v = ReadPrivate(f, s, k, d ? "true" : "false");
    if (v == "true"  || v == "1" || v == "yes") return true;
    if (v == "false" || v == "0" || v == "no")  return false;
    return d;
}

static void WriteP(const std::string& file, const char* sec,
                   const char* key, const std::string& val)
{
    WritePrivateProfileStringA(sec, key, val.c_str(), file.c_str());
}

// --------------------------------------------------------------------------
bool ConfigManager::Load(const std::string& basePath)
{
    std::string iniPath = basePath + "\\global.ini";
    Config& c = m_cfg;

    auto f  = iniPath;
    c.enabled     = ReadB(f, "General", "Enabled",     true);
    c.preset      = ReadPrivate(f, "General", "Preset", "RaceRoomStyle");
    c.showOverlay = ReadB(f, "General", "ShowOverlay", true);

    // Color
    c.colorEnabled = ReadB(f, "Color", "Enabled",     true);
    c.saturation   = ReadF(f, "Color", "Saturation",  1.25f);
    c.vibrance     = ReadF(f, "Color", "Vibrance",    0.35f);
    c.temperature  = ReadF(f, "Color", "Temperature", 0.03f);
    c.tint         = ReadF(f, "Color", "Tint",        0.00f);

    // Tonemap
    c.tonemapEnabled = ReadB(f, "Tonemap", "Enabled",    true);
    c.exposure       = ReadF(f, "Tonemap", "Exposure",   0.05f);
    c.gamma          = ReadF(f, "Tonemap", "Gamma",      1.00f);
    c.contrast       = ReadF(f, "Tonemap", "Contrast",   1.18f);
    c.highlights     = ReadF(f, "Tonemap", "Highlights", 0.90f);
    c.shadows        = ReadF(f, "Tonemap", "Shadows",    1.05f);
    c.filmic         = ReadB(f, "Tonemap", "Filmic",     true);

    // Bloom
    c.bloomEnabled   = ReadB(f, "Bloom", "Enabled",   true);
    c.bloomThreshold = ReadF(f, "Bloom", "Threshold", 0.78f);
    c.bloomIntensity = ReadF(f, "Bloom", "Intensity", 0.28f);
    c.bloomRadius    = ReadF(f, "Bloom", "Radius",    0.65f);
    c.bloomSoftKnee  = ReadF(f, "Bloom", "SoftKnee", 0.45f);

    // Sharpen
    c.sharpenEnabled  = ReadB(f, "Sharpen", "Enabled",  true);
    c.sharpenMethod   = ReadPrivate(f, "Sharpen", "Method", "CAS");
    c.sharpenStrength = ReadF(f, "Sharpen", "Strength", 0.55f);
    c.sharpenClamp    = ReadF(f, "Sharpen", "Clamp",    0.035f);

    // AntiAliasing
    c.aaEnabled         = ReadB(f, "AntiAliasing", "Enabled",         true);
    c.aaMethod          = ReadPrivate(f, "AntiAliasing", "Method",    "FXAA");
    c.aaStrength        = ReadF(f, "AntiAliasing", "Strength",        0.85f);
    c.aaEdgeThreshold   = ReadF(f, "AntiAliasing", "EdgeThreshold",   0.125f);
    c.aaSubpixelQuality = ReadF(f, "AntiAliasing", "SubpixelQuality", 0.75f);

    // Vignette
    c.vignetteEnabled   = ReadB(f, "Vignette", "Enabled",   false);
    c.vignetteIntensity = ReadF(f, "Vignette", "Intensity", 0.08f);
    c.vignetteRadius    = ReadF(f, "Vignette", "Radius",    0.85f);
    c.vignetteSoftness  = ReadF(f, "Vignette", "Softness",  0.45f);

    LOG_INFO("Config loaded from: " + iniPath);

    // Now overlay the preset on top
    if (!c.preset.empty()) {
        ApplyPreset(basePath, c.preset);
    }

    return true;
}

// --------------------------------------------------------------------------
bool ConfigManager::Save(const std::string& basePath) const
{
    std::string f = basePath + "\\global.ini";
    const Config& c = m_cfg;

    auto bs = [](bool b){ return b ? "true" : "false"; };
    auto fs = [](float v){
        std::ostringstream ss; ss << v; return ss.str();
    };

    WriteP(f, "General", "Enabled",      bs(c.enabled));
    WriteP(f, "General", "Preset",       c.preset);
    WriteP(f, "General", "ShowOverlay",  bs(c.showOverlay));

    WriteP(f, "Color", "Enabled",        bs(c.colorEnabled));
    WriteP(f, "Color", "Saturation",     fs(c.saturation));
    WriteP(f, "Color", "Vibrance",       fs(c.vibrance));
    WriteP(f, "Color", "Temperature",    fs(c.temperature));
    WriteP(f, "Color", "Tint",           fs(c.tint));

    WriteP(f, "Tonemap", "Enabled",      bs(c.tonemapEnabled));
    WriteP(f, "Tonemap", "Exposure",     fs(c.exposure));
    WriteP(f, "Tonemap", "Gamma",        fs(c.gamma));
    WriteP(f, "Tonemap", "Contrast",     fs(c.contrast));
    WriteP(f, "Tonemap", "Highlights",   fs(c.highlights));
    WriteP(f, "Tonemap", "Shadows",      fs(c.shadows));
    WriteP(f, "Tonemap", "Filmic",       bs(c.filmic));

    WriteP(f, "Bloom", "Enabled",        bs(c.bloomEnabled));
    WriteP(f, "Bloom", "Threshold",      fs(c.bloomThreshold));
    WriteP(f, "Bloom", "Intensity",      fs(c.bloomIntensity));
    WriteP(f, "Bloom", "Radius",         fs(c.bloomRadius));
    WriteP(f, "Bloom", "SoftKnee",       fs(c.bloomSoftKnee));

    WriteP(f, "Sharpen", "Enabled",      bs(c.sharpenEnabled));
    WriteP(f, "Sharpen", "Method",       c.sharpenMethod);
    WriteP(f, "Sharpen", "Strength",     fs(c.sharpenStrength));
    WriteP(f, "Sharpen", "Clamp",        fs(c.sharpenClamp));

    WriteP(f, "AntiAliasing", "Enabled",         bs(c.aaEnabled));
    WriteP(f, "AntiAliasing", "Method",           c.aaMethod);
    WriteP(f, "AntiAliasing", "Strength",         fs(c.aaStrength));
    WriteP(f, "AntiAliasing", "EdgeThreshold",    fs(c.aaEdgeThreshold));
    WriteP(f, "AntiAliasing", "SubpixelQuality",  fs(c.aaSubpixelQuality));

    WriteP(f, "Vignette", "Enabled",     bs(c.vignetteEnabled));
    WriteP(f, "Vignette", "Intensity",   fs(c.vignetteIntensity));
    WriteP(f, "Vignette", "Radius",      fs(c.vignetteRadius));
    WriteP(f, "Vignette", "Softness",    fs(c.vignetteSoftness));

    LOG_INFO("Config saved to: " + f);
    return true;
}

// --------------------------------------------------------------------------
bool ConfigManager::ApplyPreset(const std::string& basePath, const std::string& name)
{
    std::string presetPath = basePath + "\\presets\\" + name + ".ini";
    std::ifstream test(presetPath);
    if (!test.good()) {
        LOG_WARN("Preset file not found: " + presetPath);
        return false;
    }

    Config& c = m_cfg;
    auto f = presetPath;

    // Preset only overrides effect parameters, not General keys
    if (ReadB(f, "Color", "Enabled", c.colorEnabled)) {
        c.colorEnabled = ReadB(f, "Color", "Enabled", c.colorEnabled);
        c.saturation   = ReadF(f, "Color", "Saturation",  c.saturation);
        c.vibrance     = ReadF(f, "Color", "Vibrance",    c.vibrance);
        c.temperature  = ReadF(f, "Color", "Temperature", c.temperature);
        c.tint         = ReadF(f, "Color", "Tint",        c.tint);
    }

    c.tonemapEnabled = ReadB(f, "Tonemap", "Enabled",    c.tonemapEnabled);
    c.exposure       = ReadF(f, "Tonemap", "Exposure",   c.exposure);
    c.gamma          = ReadF(f, "Tonemap", "Gamma",      c.gamma);
    c.contrast       = ReadF(f, "Tonemap", "Contrast",   c.contrast);
    c.highlights     = ReadF(f, "Tonemap", "Highlights", c.highlights);
    c.shadows        = ReadF(f, "Tonemap", "Shadows",    c.shadows);
    c.filmic         = ReadB(f, "Tonemap", "Filmic",     c.filmic);

    c.bloomEnabled   = ReadB(f, "Bloom", "Enabled",   c.bloomEnabled);
    c.bloomThreshold = ReadF(f, "Bloom", "Threshold", c.bloomThreshold);
    c.bloomIntensity = ReadF(f, "Bloom", "Intensity", c.bloomIntensity);
    c.bloomRadius    = ReadF(f, "Bloom", "Radius",    c.bloomRadius);
    c.bloomSoftKnee  = ReadF(f, "Bloom", "SoftKnee", c.bloomSoftKnee);

    c.sharpenEnabled  = ReadB(f, "Sharpen", "Enabled",  c.sharpenEnabled);
    c.sharpenMethod   = ReadPrivate(f, "Sharpen", "Method", c.sharpenMethod);
    c.sharpenStrength = ReadF(f, "Sharpen", "Strength", c.sharpenStrength);
    c.sharpenClamp    = ReadF(f, "Sharpen", "Clamp",    c.sharpenClamp);

    c.aaEnabled         = ReadB(f, "AntiAliasing", "Enabled",         c.aaEnabled);
    c.aaMethod          = ReadPrivate(f, "AntiAliasing", "Method",    c.aaMethod);
    c.aaStrength        = ReadF(f, "AntiAliasing", "Strength",        c.aaStrength);
    c.aaEdgeThreshold   = ReadF(f, "AntiAliasing", "EdgeThreshold",   c.aaEdgeThreshold);
    c.aaSubpixelQuality = ReadF(f, "AntiAliasing", "SubpixelQuality", c.aaSubpixelQuality);

    c.vignetteEnabled   = ReadB(f, "Vignette", "Enabled",   c.vignetteEnabled);
    c.vignetteIntensity = ReadF(f, "Vignette", "Intensity", c.vignetteIntensity);
    c.vignetteRadius    = ReadF(f, "Vignette", "Radius",    c.vignetteRadius);
    c.vignetteSoftness  = ReadF(f, "Vignette", "Softness",  c.vignetteSoftness);

    c.preset = name;
    LOG_INFO("Preset applied: " + name);
    return true;
}

} // namespace SimuFX
