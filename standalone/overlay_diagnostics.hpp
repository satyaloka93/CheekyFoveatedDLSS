#pragma once
#include <Windows.h>
#include <atomic>
#include <cstdarg>
#include <cstdio>
#ifdef CHEEKY_OVERLAY_TEST_DESKTOP
extern void cheeky_overlay_test_diagnostic(const char*);
#endif
namespace cheeky::standalone {
inline void overlay_log(const char* format, ...) noexcept {
    char message[2048]{};
    va_list args; va_start(args,format);
    vsnprintf(message,sizeof(message),format,args); va_end(args);
#ifdef CHEEKY_OVERLAY_TEST_DESKTOP
    cheeky_overlay_test_diagnostic(message);
#else
    const auto host=GetModuleHandleW(L"CheekyFoveatedDLSSHost.dll");
    const auto log=host?reinterpret_cast<void(__cdecl*)(const char*) noexcept>(
        GetProcAddress(host,"CheekyOpenXRMenuLogDiagnostic")):nullptr;
    if(log)log(message); else OutputDebugStringA(message);
#endif
}
// Initialization may retry each Present. Keep errors useful without flooding disk.
inline bool overlay_failure(const char* operation, unsigned long code) noexcept {
    static std::atomic<ULONGLONG> last{};
    const auto now=GetTickCount64();
    auto previous=last.load();
    if((!previous || now-previous>=5000) && last.compare_exchange_strong(previous,now))
        overlay_log("[Overlay ERROR] %s: code=0x%08lX (HRESULT or Win32); retry errors limited to once/5s",operation,code);
    return false;
}
}
