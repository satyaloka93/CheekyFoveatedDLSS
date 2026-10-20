#pragma once
#include <array>
#include <atomic>
#include <cstring>

namespace cheeky {
// Publish only successfully enabled persistent hooks. Preserve the public
// entry point so other observers installed before OR after us remain reachable.
class StreamlineExportRoutes {
    static constexpr std::array names{
        "slEvaluateFeature", "slSetTag", "slSetTagForFrame",
        "slSetConstants", "slGetFeatureFunction"};
    std::array<std::atomic<void*>, names.size()> targets_{};
public:
    void publish(const char* name, void* target) noexcept {
        if (!name) return;
        for (std::size_t i = 0; i < names.size(); ++i)
            if (std::strcmp(name, names[i]) == 0)
                targets_[i].store(target, std::memory_order_release);
    }
    void* find(const char* name) const noexcept {
        if (!name) return nullptr;
        for (std::size_t i = 0; i < names.size(); ++i)
            if (std::strcmp(name, names[i]) == 0)
                return targets_[i].load(std::memory_order_acquire);
        return nullptr;
    }
    void clear() noexcept {
        for (auto& target : targets_) target.store(nullptr, std::memory_order_release);
    }
};
}
