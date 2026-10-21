#pragma once
#include <Windows.h>
#include <array>
#include <atomic>
namespace cheeky::foveated_dlss {
// Fixed by the host before interception starts; never changed by live controls.
inline std::atomic<bool> nr_previous_runtime{};
struct NrRuntimeModule {
    HMODULE module{}; DWORD error{}; std::array<wchar_t,32768> directory{};
    std::array<char, 4096> path{};
    std::array<char, 64> version{};
};
NrRuntimeModule load_nr_runtime_module() noexcept;
}
