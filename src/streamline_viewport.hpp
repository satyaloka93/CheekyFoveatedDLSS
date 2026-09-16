#pragma once

#include <array>
#include <cstdint>
#include <cstring>

namespace cheeky::foveated_dlss {

// Remember only constants accepted by Streamline. Repeated evaluations of a
// frame may reuse them, but changed constants must never be silently accepted.
template <typename Constants> struct AcceptedStreamlineConstants {
    std::uintptr_t frame{};
    std::uint32_t viewport{};
    Constants constants{};
    bool valid{};
    bool matches(std::uintptr_t f, std::uint32_t v, const Constants& c) const {
        return valid && frame == f && viewport == v &&
            std::memcmp(&constants, &c, sizeof(c)) == 0;
    }
};

// Lookup by the input handle, never by the last viewport tagged on a thread.
template <typename Viewport, typename Range>
auto find_streamline_view(const Range& entries, const void* const* inputs, std::uint32_t count)
    -> const typename Range::value_type* {
    if (!inputs || !count || count > 16U) return nullptr;
    for (const auto& entry : entries) {
        const Viewport* found{};
        for (std::uint32_t i{}; i < count; ++i) {
            const auto* candidate = static_cast<const Viewport*>(inputs[i]);
            if (!candidate || std::memcmp(&candidate->struct_type, &entry.viewport.struct_type,
                    sizeof(candidate->struct_type)) != 0) continue;
            if (found || candidate->next) return nullptr;
            found = candidate;
        }
        if (found && found->value == entry.viewport.value) return &entry;
    }
    return nullptr;
}

// Keep the original frame/viewport constants immutable. The private SR viewport
// is distinct from both the host and the peripheral DLAA namespace (0x40000000).
template <typename Viewport, std::size_t Capacity>
[[nodiscard]] bool prepare_streamline_sr_inputs(
    const void* const* inputs, std::uint32_t count, const Viewport& original,
    Viewport& cropped, std::array<const void*, Capacity>& redirected
) noexcept {
    if (inputs == nullptr || count == 0U || count > Capacity ||
        (original.value & 0x60000000U) != 0U) return false;
    cropped = original;
    cropped.next = nullptr;
    cropped.value |= 0x20000000U;
    bool found{};
    for (std::uint32_t i{}; i < count; ++i) {
        redirected[i] = inputs[i];
        // All Streamline input structures share the BaseStructure prefix.
        const auto* candidate = static_cast<const Viewport*>(inputs[i]);
        if (candidate == nullptr || std::memcmp(&candidate->struct_type,
                &original.struct_type, sizeof(original.struct_type)) != 0) continue;
        if (found || candidate->value != original.value || candidate->next != nullptr) return false;
        redirected[i] = &cropped;
        found = true;
    }
    return found;
}
} // namespace cheeky::foveated_dlss
