#pragma once
#include "afw_compatibility.hpp"
#include <Windows.h>
#include <atomic>
#include <mutex>

namespace cheeky::foveated_dlss {
// Optional, versioned host handshake. Only final OpenXR targets may consume
// this data; never paint it into PDAFW's persistent input/output framebuffers.
inline std::atomic<ULONGLONG> afw_border_host_time{};
inline std::atomic<unsigned> afw_border_host_width{}, afw_border_host_height{};
inline std::mutex afw_border_mutex;
struct AfwFinalBorder {
    float bounds[2][5]{};
    ULONGLONG time{};
    std::uint64_t generation{};
    unsigned width{}, height{};
};
inline AfwFinalBorder afw_final_border;

inline bool afw_final_border_supported(const Settings& settings) noexcept {
    const auto now = GetTickCount64();
    const auto heartbeat = afw_border_host_time.load(std::memory_order_acquire);
    const auto projection = afw_stereo_projection();
    return settings.eye_independent_coverage && settings.afw_source_eye < 2 &&
        settings.afw_mask.count == 1 && projection.valid && heartbeat &&
        now >= heartbeat && now - heartbeat < 250 &&
        projection.output_width == afw_border_host_width.load() &&
        projection.output_height == afw_border_host_height.load();
}

inline void publish_afw_final_border(const Settings& settings) noexcept {
    if (!afw_final_border_supported(settings)) return;
    const auto projection = afw_stereo_projection();
    AfwFinalBorder next{};
    if (settings.alignment_border_enabled) {
        const auto* bounds = settings.afw_mask.bounds[0];
        for (unsigned eye = 0; eye < 2; ++eye) {
            const auto lo = afw_project_between_eyes({bounds[0], bounds[1], 1},
                projection.projections[settings.afw_source_eye], projection.projections[eye]);
            const auto hi = afw_project_between_eyes({bounds[2], bounds[3], 1},
                projection.projections[settings.afw_source_eye], projection.projections[eye]);
            next.bounds[eye][0] = lo.u; next.bounds[eye][1] = lo.v;
            next.bounds[eye][2] = hi.u; next.bounds[eye][3] = hi.v;
            next.bounds[eye][4] = settings.roundness;
        }
        next.time = GetTickCount64(); next.generation = projection.generation;
        next.width = projection.output_width; next.height = projection.output_height;
    }
    std::lock_guard lock(afw_border_mutex);
    afw_final_border = next;
}

inline unsigned read_afw_final_border(unsigned eye, unsigned width, unsigned height,
    float* bounds, unsigned count) noexcept {
    if (eye > 1 || !bounds || count != 5 || !width || !height) return 0;
    const auto now = GetTickCount64();
    afw_border_host_width.store(width); afw_border_host_height.store(height);
    afw_border_host_time.store(now, std::memory_order_release);
    const auto projection = afw_stereo_projection();
    std::lock_guard lock(afw_border_mutex);
    const auto& value = afw_final_border;
    if (!projection.valid || value.generation != projection.generation || !value.time ||
        now < value.time || now - value.time > 100 || value.width != width || value.height != height) return 0;
    std::copy_n(value.bounds[eye], 5, bounds);
    return 1;
}
}
