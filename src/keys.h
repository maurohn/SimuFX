#pragma once
#include <windows.h>

namespace SimuFX {

// Simple per-frame key edge-detector (no dependencies on ImGui or DirectInput)
class KeyManager {
public:
    static KeyManager& Instance() {
        static KeyManager inst;
        return inst;
    }

    // Call once per EndScene to snapshot key states
    void Update() {
        for (int i = 0; i < 256; ++i) {
            bool cur = (GetAsyncKeyState(i) & 0x8000) != 0;
            m_pressed[i] = cur && !m_prev[i];
            m_prev[i]    = cur;
        }
    }

    // Returns true on the frame the key transitions down
    bool Pressed(int vkey) const {
        if (vkey < 0 || vkey > 255) return false;
        return m_pressed[vkey];
    }

private:
    KeyManager() = default;
    bool m_prev[256]    = {};
    bool m_pressed[256] = {};
};

} // namespace SimuFX
