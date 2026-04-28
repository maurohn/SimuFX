#include "overlay.h"
#include "config.h"
#include "logger.h"
#include <string>

// ImGui headers
#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace SimuFX {

// Our subclassed WndProc — intercepts messages for ImGui before forwarding
static WNDPROC  g_origWndProc = nullptr;

static LRESULT CALLBACK HookWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // Feed every message to ImGui first
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return TRUE;

    // If ImGui wants to capture mouse/keyboard, eat the input so game doesn't also react
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse &&
        (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP ||
         msg == WM_RBUTTONDOWN || msg == WM_RBUTTONUP ||
         msg == WM_MBUTTONDOWN || msg == WM_MBUTTONUP ||
         msg == WM_MOUSEWHEEL  || msg == WM_MOUSEMOVE))
        return 0;
    if (io.WantCaptureKeyboard &&
        (msg == WM_KEYDOWN || msg == WM_KEYUP || msg == WM_CHAR))
        return 0;

    return CallWindowProcA(g_origWndProc, hWnd, msg, wParam, lParam);
}

static HWND g_foundHwnd = nullptr;
static DWORD g_targetPid = 0;

static BOOL CALLBACK FindWindowCallback(HWND hwnd, LPARAM)
{
    DWORD wPid = 0;
    GetWindowThreadProcessId(hwnd, &wPid);
    if (wPid == g_targetPid && IsWindowVisible(hwnd) && GetWindow(hwnd, GW_OWNER) == nullptr) {
        g_foundHwnd = hwnd;
        return FALSE; // stop
    }
    return TRUE;
}

static HWND FindGameWindow()
{
    g_targetPid  = GetCurrentProcessId();
    g_foundHwnd  = nullptr;
    EnumWindows(FindWindowCallback, 0);
    if (!g_foundHwnd) g_foundHwnd = GetForegroundWindow();
    return g_foundHwnd;
}

// ============================================================================

void Overlay::Init(IDirect3DDevice9* device, const std::string& exeDir)
{
    if (m_initialised) return;

    m_exeDir  = exeDir;
    m_cfgBase = exeDir + "\\SimuFX";
    m_hwnd    = FindGameWindow();

    QueryPerformanceFrequency(&m_freq);
    QueryPerformanceCounter(&m_lastTime);

    // Setup ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr; // Don't save imgui.ini alongside the game

    // Dark, premium style
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding    = 8.0f;
    style.FrameRounding     = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding      = 4.0f;
    style.WindowBorderSize  = 1.0f;
    style.FramePadding      = ImVec2(8, 4);
    style.ItemSpacing       = ImVec2(8, 6);
    style.WindowPadding     = ImVec2(12, 12);
    style.Alpha             = 0.95f;

    // Custom palette — deep charcoal + orange accent
    ImVec4* col = style.Colors;
    col[ImGuiCol_WindowBg]         = ImVec4(0.10f, 0.10f, 0.12f, 0.95f);
    col[ImGuiCol_ChildBg]          = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
    col[ImGuiCol_PopupBg]          = ImVec4(0.10f, 0.10f, 0.12f, 0.98f);
    col[ImGuiCol_Border]           = ImVec4(0.30f, 0.30f, 0.35f, 0.60f);
    col[ImGuiCol_FrameBg]          = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
    col[ImGuiCol_FrameBgHovered]   = ImVec4(0.25f, 0.25f, 0.28f, 1.00f);
    col[ImGuiCol_FrameBgActive]    = ImVec4(0.30f, 0.30f, 0.34f, 1.00f);
    col[ImGuiCol_TitleBg]          = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    col[ImGuiCol_TitleBgActive]    = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    col[ImGuiCol_Header]           = ImVec4(0.22f, 0.22f, 0.26f, 1.00f);
    col[ImGuiCol_HeaderHovered]    = ImVec4(0.85f, 0.45f, 0.10f, 0.80f);
    col[ImGuiCol_HeaderActive]     = ImVec4(0.90f, 0.50f, 0.10f, 1.00f);
    col[ImGuiCol_SliderGrab]       = ImVec4(0.90f, 0.50f, 0.10f, 1.00f);
    col[ImGuiCol_SliderGrabActive] = ImVec4(1.00f, 0.60f, 0.15f, 1.00f);
    col[ImGuiCol_CheckMark]        = ImVec4(0.90f, 0.50f, 0.10f, 1.00f);
    col[ImGuiCol_Button]           = ImVec4(0.22f, 0.22f, 0.26f, 1.00f);
    col[ImGuiCol_ButtonHovered]    = ImVec4(0.85f, 0.45f, 0.10f, 0.90f);
    col[ImGuiCol_ButtonActive]     = ImVec4(0.90f, 0.50f, 0.10f, 1.00f);
    col[ImGuiCol_Tab]              = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);
    col[ImGuiCol_TabHovered]       = ImVec4(0.85f, 0.45f, 0.10f, 0.80f);
    col[ImGuiCol_TabActive]        = ImVec4(0.90f, 0.50f, 0.10f, 1.00f);
    col[ImGuiCol_Separator]        = ImVec4(0.30f, 0.30f, 0.35f, 0.60f);

    ImGui_ImplWin32_Init(m_hwnd);
    ImGui_ImplDX9_Init(device);

    // Subclass the game window so keyboard/WM messages reach ImGui
    g_origWndProc = (WNDPROC)SetWindowLongA(m_hwnd, GWL_WNDPROC,
                                             (LONG)HookWndProc);

    // Fix window title
    SetWindowTextA(m_hwnd, "SimuV3");

    m_initialised = true;
    LOG_INFO("Overlay initialised (ImGui " IMGUI_VERSION ")");
}

// ============================================================================

void Overlay::Shutdown()
{
    if (!m_initialised) return;

    // Restore original WndProc before destroying ImGui
    if (g_origWndProc && m_hwnd)
        SetWindowLongPtrA(m_hwnd, GWLP_WNDPROC, (LONG_PTR)g_origWndProc);
    g_origWndProc = nullptr;

    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    m_initialised = false;
    LOG_INFO("Overlay shutdown");
}

// ============================================================================

void Overlay::OnPreReset()
{
    if (m_initialised) ImGui_ImplDX9_InvalidateDeviceObjects();
}

void Overlay::OnPostReset(IDirect3DDevice9*)
{
    if (m_initialised) ImGui_ImplDX9_CreateDeviceObjects();
}

// ============================================================================

void Overlay::Render(IDirect3DDevice9*)
{
    // Keep window title in sync — game resets it every frame
    if (m_hwnd) SetWindowTextA(m_hwnd, "SimuV3");

    // Update FPS counter
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    double dt = (double)(now.QuadPart - m_lastTime.QuadPart) / (double)m_freq.QuadPart;
    m_lastTime = now;
    if (dt > 0.0) {
        float fps = (float)(1.0 / dt);
        m_fps       = m_fps * 0.9f + fps * 0.1f;   // EMA smoothing
        m_frametime = m_frametime * 0.9f + (float)(dt * 1000.0) * 0.1f;
    }

    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();

    // ---- Manual mouse input for DirectInput games ----
    // rFactor uses DirectInput — WM_LBUTTONDOWN etc. are never generated,
    // so we must feed button state directly to ImGui via GetAsyncKeyState.
    {
        ImGuiIO& io = ImGui::GetIO();
        POINT pt = {};
        GetCursorPos(&pt);
        ScreenToClient(m_hwnd, &pt);
        io.AddMousePosEvent((float)pt.x, (float)pt.y);
        io.AddMouseButtonEvent(0, (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0);
        io.AddMouseButtonEvent(1, (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0);
        io.AddMouseButtonEvent(2, (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0);
    }

    ImGui::NewFrame();

    Config& cfg = ConfigManager::Instance().Get();

    // ---- Main overlay window ----
    ImGui::SetNextWindowSize(ImVec2(480, 680), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);

    ImGui::Begin("SimuFX  |  SimuV3 Post-Processor", nullptr,
                 ImGuiWindowFlags_NoCollapse);

    // Header info bar
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.06f, 0.06f, 0.08f, 1.0f));
    ImGui::BeginChild("##header", ImVec2(0, 58), true);
    ImGui::Columns(3, "hcols", false);
    ImGui::SetColumnWidth(0, 160);
    ImGui::SetColumnWidth(1, 160);

    ImGui::TextColored(ImVec4(0.9f, 0.5f, 0.1f, 1), "FPS");
    ImGui::Text("%.1f", m_fps);
    ImGui::NextColumn();

    ImGui::TextColored(ImVec4(0.9f, 0.5f, 0.1f, 1), "Frame time");
    ImGui::Text("%.2f ms", m_frametime);
    ImGui::NextColumn();

    ImGui::TextColored(ImVec4(0.9f, 0.5f, 0.1f, 1), "Status");
    ImGui::TextColored(cfg.enabled
        ? ImVec4(0.4f, 1.0f, 0.4f, 1) : ImVec4(1, 0.4f, 0.4f, 1),
        cfg.enabled ? "ACTIVE" : "BYPASSED");
    ImGui::Columns(1);
    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::Spacing();

    // Enable / Preset row
    ImGui::Checkbox("Enable SimuFX", &cfg.enabled);
    ImGui::SameLine(200);
    ImGui::SetNextItemWidth(200);
    const char* presets[] = { "Dynamic", "Realistic", "Cinematic",
                               "SharpClean", "NightBoost" };
    int curPreset = 0;
    for (int i = 0; i < 5; ++i)
        if (cfg.preset == presets[i]) { curPreset = i; break; }

    if (ImGui::Combo("Preset", &curPreset, presets, 5)) {
        cfg.preset = presets[curPreset];
        ConfigManager::Instance().ApplyPreset(m_cfgBase, cfg.preset);
        LOG_INFO("Preset changed via overlay: " + cfg.preset);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ---- Tabs ----
    if (ImGui::BeginTabBar("##tabs")) {

        // ---- COLOR ----
        if (ImGui::BeginTabItem("Color")) {
            ImGui::Checkbox("Enabled##color", &cfg.colorEnabled);
            ImGui::Spacing();
            ImGui::SetNextItemWidth(-1);
            ImGui::SliderFloat("Saturation",  &cfg.saturation,  0.5f, 2.0f);
            ImGui::SetNextItemWidth(-1);
            ImGui::SliderFloat("Vibrance",    &cfg.vibrance,    0.0f, 1.0f);
            ImGui::SetNextItemWidth(-1);
            ImGui::SliderFloat("Temperature", &cfg.temperature, -0.5f, 0.5f);
            ImGui::SetNextItemWidth(-1);
            ImGui::SliderFloat("Tint",        &cfg.tint,        -0.5f, 0.5f);
            ImGui::EndTabItem();
        }

        // ---- TONEMAP ----
        if (ImGui::BeginTabItem("Tonemap")) {
            ImGui::Checkbox("Enabled##tm", &cfg.tonemapEnabled);
            ImGui::Checkbox("Filmic Tonemap", &cfg.filmic);
            ImGui::Spacing();
            ImGui::SetNextItemWidth(-1);
            ImGui::SliderFloat("Exposure",   &cfg.exposure,   -1.0f, 1.0f);
            ImGui::SetNextItemWidth(-1);
            ImGui::SliderFloat("Gamma",      &cfg.gamma,       0.5f, 2.2f);
            ImGui::SetNextItemWidth(-1);
            ImGui::SliderFloat("Contrast",   &cfg.contrast,    0.5f, 2.0f);
            ImGui::SetNextItemWidth(-1);
            ImGui::SliderFloat("Highlights", &cfg.highlights,  0.0f, 2.0f);
            ImGui::SetNextItemWidth(-1);
            ImGui::SliderFloat("Shadows",    &cfg.shadows,     0.0f, 2.0f);
            ImGui::EndTabItem();
        }

        // ---- BLOOM ----
        if (ImGui::BeginTabItem("Bloom")) {
            ImGui::Checkbox("Enabled##bloom", &cfg.bloomEnabled);
            ImGui::Spacing();
            ImGui::SetNextItemWidth(-1);
            ImGui::SliderFloat("Threshold",  &cfg.bloomThreshold, 0.0f, 2.0f);
            ImGui::SetNextItemWidth(-1);
            ImGui::SliderFloat("Intensity",  &cfg.bloomIntensity, 0.0f, 1.0f);
            ImGui::SetNextItemWidth(-1);
            ImGui::SliderFloat("Radius",     &cfg.bloomRadius,    0.0f, 1.0f);
            ImGui::SetNextItemWidth(-1);
            ImGui::SliderFloat("Soft Knee",  &cfg.bloomSoftKnee,  0.0f, 1.0f);
            ImGui::EndTabItem();
        }

        // ---- SHARPEN ----
        if (ImGui::BeginTabItem("Sharpen")) {
            ImGui::Checkbox("Enabled##sh", &cfg.sharpenEnabled);
            bool isCAS = (cfg.sharpenMethod == "CAS");
            if (ImGui::RadioButton("CAS",  isCAS))  cfg.sharpenMethod = "CAS";
            ImGui::SameLine();
            if (ImGui::RadioButton("Luma", !isCAS)) cfg.sharpenMethod = "Luma";
            ImGui::Spacing();
            ImGui::SetNextItemWidth(-1);
            ImGui::SliderFloat("Strength", &cfg.sharpenStrength, 0.0f, 1.5f);
            ImGui::SetNextItemWidth(-1);
            ImGui::SliderFloat("Clamp",    &cfg.sharpenClamp,    0.0f, 0.2f);
            ImGui::EndTabItem();
        }

        // ---- AA ----
        if (ImGui::BeginTabItem("AA")) {
            ImGui::Checkbox("Enabled##aa", &cfg.aaEnabled);
            bool isFXAA = (cfg.aaMethod == "FXAA");
            if (ImGui::RadioButton("FXAA",  isFXAA))  cfg.aaMethod = "FXAA";
            ImGui::SameLine();
            if (ImGui::RadioButton("SMAA", !isFXAA)) cfg.aaMethod = "SMAA";
            ImGui::Spacing();
            ImGui::SetNextItemWidth(-1);
            ImGui::SliderFloat("Strength",        &cfg.aaStrength,        0.0f, 1.0f);
            ImGui::SetNextItemWidth(-1);
            ImGui::SliderFloat("Edge Threshold",  &cfg.aaEdgeThreshold,   0.0f, 0.5f);
            ImGui::SetNextItemWidth(-1);
            ImGui::SliderFloat("Subpixel Quality",&cfg.aaSubpixelQuality,  0.0f, 1.0f);
            ImGui::EndTabItem();
        }

        // ---- VIGNETTE ----
        if (ImGui::BeginTabItem("Vignette")) {
            ImGui::Checkbox("Enabled##vig", &cfg.vignetteEnabled);
            ImGui::Spacing();
            ImGui::SetNextItemWidth(-1);
            ImGui::SliderFloat("Intensity", &cfg.vignetteIntensity, 0.0f, 1.0f);
            ImGui::SetNextItemWidth(-1);
            ImGui::SliderFloat("Radius",    &cfg.vignetteRadius,    0.0f, 1.0f);
            ImGui::SetNextItemWidth(-1);
            ImGui::SliderFloat("Softness",  &cfg.vignetteSoftness,  0.0f, 1.0f);
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Action buttons
    float bW = (ImGui::GetContentRegionAvail().x - 8) / 2.0f;
    if (ImGui::Button("Save Config", ImVec2(bW, 32))) {
        ConfigManager::Instance().Save(m_cfgBase);
    }
    ImGui::SameLine(0, 8);
    if (ImGui::Button("Reload (F9)", ImVec2(bW, 32))) {
        ConfigManager::Instance().Load(m_cfgBase);
        LOG_INFO("Config reloaded from overlay");
    }

    ImGui::Spacing();
    ImGui::TextDisabled("F10  Toggle overlay   |   F9  Hot reload");

    ImGui::End();

    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
}

} // namespace SimuFX
