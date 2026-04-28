#pragma once
// Logger puro Win32 — sin std::mutex, sin std::ofstream, sin STL pesado.
// Compatible con cualquier CRT del proceso host.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>   // Only for the Write(std::string) overload

namespace SimuFX {

class Logger {
public:
    static Logger& Instance() {
        static Logger inst;   // construido en primer uso (thread-safe en C++11+)
        return inst;
    }

    void Init(const char* path) {
        if (m_ready) return;
        InitializeCriticalSection(&m_cs);
        // Ensure directory exists
        char dir[MAX_PATH];
        lstrcpynA(dir, path, MAX_PATH);
        // trim filename
        char* last = dir;
        for (char* p = dir; *p; ++p) if (*p == '\\' || *p == '/') last = p;
        *last = '\0';
        CreateDirectoryA(dir, nullptr);

        m_file = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ,
                             nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        m_ready = (m_file != INVALID_HANDLE_VALUE);
    }

    void Write(const char* level, const char* msg) {
        if (!m_ready) return;

        SYSTEMTIME st;
        GetLocalTime(&st);
        char line[2048];
        int n = wsprintfA(line, "[%02u:%02u:%02u.%03u] [%s] %s\r\n",
                          st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
                          level, msg);
        if (n <= 0) return;

        EnterCriticalSection(&m_cs);
        DWORD written = 0;
        WriteFile(m_file, line, (DWORD)n, &written, nullptr);
        LeaveCriticalSection(&m_cs);
        OutputDebugStringA(line);
    }

    // Overload for std::string call sites
    void Write(const char* level, const std::string& msg) {
        Write(level, msg.c_str());
    }

    ~Logger() {
        if (m_ready) {
            CloseHandle(m_file);
            DeleteCriticalSection(&m_cs);
        }
    }

private:
    Logger() = default;
    Logger(const Logger&) = delete;

    HANDLE         m_file  = INVALID_HANDLE_VALUE;
    CRITICAL_SECTION m_cs  = {};
    bool           m_ready = false;
};

} // namespace SimuFX

// Helpers — accept const char* directly
#define LOG_INFO(msg)  SimuFX::Logger::Instance().Write("INFO",  (msg))
#define LOG_WARN(msg)  SimuFX::Logger::Instance().Write("WARN",  (msg))
#define LOG_ERROR(msg) SimuFX::Logger::Instance().Write("ERROR", (msg))

// For call sites that pass std::string, provide inline converters
// Include <string> only where needed — NOT in logger.h itself
