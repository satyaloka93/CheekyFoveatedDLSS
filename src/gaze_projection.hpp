#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace cheeky::foveated_dlss {
struct GazeProjection {
    // Signed view-space tangents, independent of left/right handed depth.
    float left{}, right{}, up{}, down{};
    bool valid{};
};

// UV in an asymmetric headset eye is not UV in the game's render camera.
// Ported from the maintained fork's 44ac27f, with finite-FOV validation.
inline bool reproject_gaze_center(const GazeProjection& from, const GazeProjection& to,
    float u, float v, float& out_u, float& out_v) {
    const auto valid = [](const GazeProjection& p) {
        return p.valid && std::isfinite(p.left) && std::isfinite(p.right) &&
            std::isfinite(p.up) && std::isfinite(p.down) &&
            p.right > p.left && p.up > p.down;
    };
    if (!valid(from) || !valid(to) || !std::isfinite(u) || !std::isfinite(v)) return false;
    out_u = (from.left + u * (from.right - from.left) - to.left) / (to.right - to.left);
    out_v = (to.up - (from.up - v * (from.up - from.down))) / (to.up - to.down);
    return std::isfinite(out_u) && std::isfinite(out_v);
}

inline bool projection_forward_center(const GazeProjection& p, float& u, float& v) {
    if (!p.valid || !std::isfinite(p.left) || !std::isfinite(p.right) ||
        !std::isfinite(p.up) || !std::isfinite(p.down) ||
        p.left >= 0.F || p.right <= 0.F || p.down >= 0.F || p.up <= 0.F) return false;
    u = -p.left / (p.right - p.left);
    v = p.up / (p.up - p.down);
    return std::isfinite(u) && std::isfinite(v);
}

inline GazeProjection gaze_projection_from_matrix(const float* m) {
    for (unsigned i = 0; i < 16; ++i) if (!std::isfinite(m[i])) return {};
    // Streamline uses row vectors and row-major, unjittered matrices.
    // Accept only conventional perspective projections, not arbitrary transforms.
    for (const auto i : {1, 2, 3, 4, 6, 7, 12, 13, 15})
        if (std::abs(m[i]) > 0.0001F) return {};
    if (m[0] <= 0 || m[5] <= 0 || std::abs(std::abs(m[11]) - 1.F) > 0.0001F) return {};
    const float sign = m[11];
    GazeProjection result{(-1.F - m[8] * sign) / m[0],
        (1.F - m[8] * sign) / m[0], (1.F - m[9] * sign) / m[5],
        (-1.F - m[9] * sign) / m[5], true};
    result.valid = result.left < 0 && result.right > 0 && result.down < 0 && result.up > 0 &&
        std::isfinite(result.left) && std::isfinite(result.right) &&
        std::isfinite(result.up) && std::isfinite(result.down);
    return result;
}

inline bool gaze_projection_matches(const GazeProjection& a, const GazeProjection& b) {
    if (!a.valid || !b.valid) return false;
    const auto close = [](float x, float y) {
        return std::isfinite(x) && std::isfinite(y) &&
            std::abs(x - y) <= 0.001F * (1.F + (std::max)(std::abs(x), std::abs(y)));
    };
    return close(a.left, b.left) && close(a.right, b.right) &&
        close(a.up, b.up) && close(a.down, b.down);
}

struct GazeProjectionMatch { unsigned count{}; unsigned index{UINT32_MAX}; };
inline GazeProjectionMatch match_gaze_projection_eyes(const GazeProjection& camera,
    const std::array<GazeProjection, 2>& eyes) {
    GazeProjectionMatch result{};
    if (!eyes[0].valid || !eyes[1].valid) return result;
    for (unsigned i = 0; i < 2; ++i) if (gaze_projection_matches(camera, eyes[i])) {
        ++result.count; result.index = i;
    }
    return result;
}

class GazeProjectionCache {
    struct Entry { std::uint32_t viewport; std::uintptr_t frame; std::uint64_t time;
        GazeProjection projection; };
    std::vector<Entry> entries_;
public:
    GazeProjection recent(std::uint32_t viewport, std::uint64_t now) const {
        for (auto i = entries_.rbegin(); i != entries_.rend(); ++i)
            if (i->viewport == viewport)
                return now >= i->time && now - i->time <= 100 ? i->projection : GazeProjection{};
        return {};
    }
    void record(std::uint32_t viewport, std::uintptr_t frame, std::uint64_t now, GazeProjection projection) {
        // Constants for frame N+1 may arrive before frame N is evaluated.
        // Retain bounded in-flight frames, while invalidation replaces the
        // exact viewport/frame publication rather than borrowing another eye.
        std::erase_if(entries_, [=](const auto& e) { return e.viewport == viewport && e.frame == frame; });
        if (entries_.size() >= 32) entries_.erase(entries_.begin());
        entries_.push_back({viewport, frame, now, projection});
    }
    GazeProjection find(std::uint32_t viewport, std::uintptr_t frame, std::uint64_t now) const {
        for (const auto& e : entries_)
            if (frame && e.viewport == viewport && e.frame == frame && now >= e.time && now - e.time <= 100)
                return e.projection;
        return {};
    }
    void forget(std::uint32_t viewport) {
        std::erase_if(entries_, [=](const auto& e) { return e.viewport == viewport; });
    }
};

// A lower NGX evaluation is nested synchronously inside slEvaluateFeature.
// Cyberpunk 0.1.8 can omit VRCAM slSetConstants. Its exported render windows
// are MAIN then VRCAM, l/r/d/u. Only use the missing VRCAM window after
// validating MAIN against a recent observed camera. Never mirror a UV.
inline GazeProjection cyberpunk_missing_camera(std::uint32_t viewport,
    std::uint32_t vrcam, int mode, const float* windows, const GazeProjection& main) {
    if (mode != 1 || !vrcam || viewport != vrcam || !windows) return {};
    const GazeProjection primary{windows[0], windows[1], windows[3], windows[2], true};
    const GazeProjection secondary{windows[4], windows[5], windows[7], windows[6], true};
    float u{}, v{};
    if (!projection_forward_center(primary, u, v) ||
        !projection_forward_center(secondary, u, v) || !gaze_projection_matches(primary, main)) return {};
    return secondary;
}

// Carry only that call's frame/viewport-matched camera, never a last-seen eye.
// Thread-local RAII also prevents another render thread or later call inheriting it.
struct NativeGazeProjectionContext {
    bool active{};
    GazeProjection projection{};
    std::uint32_t viewport{UINT32_MAX};
};
inline thread_local NativeGazeProjectionContext native_gaze_projection{};
class ScopedNativeGazeProjection {
    NativeGazeProjectionContext previous_;
public:
    ScopedNativeGazeProjection(GazeProjection projection, std::uint32_t viewport)
        : previous_(native_gaze_projection) {
        native_gaze_projection = {true, projection, viewport};
    }
    ~ScopedNativeGazeProjection() { native_gaze_projection = previous_; }
    ScopedNativeGazeProjection(const ScopedNativeGazeProjection&) = delete;
    ScopedNativeGazeProjection& operator=(const ScopedNativeGazeProjection&) = delete;
};

struct GazeProjectionContext { std::uint64_t view{}; GazeProjection projection{};
    std::uint32_t viewport{UINT32_MAX}; };
inline thread_local GazeProjectionContext active_gaze_projection{};
class ScopedGazeProjection {
    GazeProjectionContext previous_;
public:
    ScopedGazeProjection(std::uint64_t view, GazeProjection projection,
        std::uint32_t viewport = UINT32_MAX) : previous_(active_gaze_projection) {
        active_gaze_projection = {view, projection, viewport};
    }
    ~ScopedGazeProjection() { active_gaze_projection = previous_; }
    ScopedGazeProjection(const ScopedGazeProjection&) = delete;
    ScopedGazeProjection& operator=(const ScopedGazeProjection&) = delete;
};
} // namespace cheeky::foveated_dlss
