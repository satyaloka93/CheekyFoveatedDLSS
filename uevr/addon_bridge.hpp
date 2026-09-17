#pragma once
#include "runtime_api.hpp"

// Copied frame data only. No callbacks or UEVR-owned projection pointers are retained.
struct CheekyAddonUEVRFrame {
    std::uint32_t size{sizeof(CheekyAddonUEVRFrame)}, abi{1};
    std::uint32_t renderer{UINT32_MAX}, rendering_mode{UINT32_MAX};
    CheekyUEVRStereoProjection stereo{};
    void* openvr_compositor{}; // Consumed synchronously by the existing OpenVR adapter.
};
using CheekyAddonPublishUEVRFn = bool (*)(const CheekyAddonUEVRFrame*);

inline bool valid_addon_uevr_frame(const CheekyAddonUEVRFrame* frame) noexcept {
    return frame && frame->size == sizeof(*frame) && frame->abi == 1 &&
        frame->stereo.size == sizeof(frame->stereo) && frame->stereo.abi == 1 &&
        frame->stereo.active <= 1;
}
