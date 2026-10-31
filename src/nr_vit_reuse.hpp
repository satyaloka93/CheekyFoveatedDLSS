#pragma once
#include <cstdint>
#include <string>
struct ID3D12GraphicsCommandList;
namespace cheeky::foveated_dlss {
struct NrVitReuseSnapshot {
    std::uint64_t computed{}, reused{}, rejected{}, copied_bytes{};
    std::uint64_t retained_bytes{}, cache_entries{}, retired_buffers{};
    std::uint64_t split_runs{}, replayed{}, cache_misses{}, last_run_kernels{}, last_run_calls{};
    bool hooked{};
    std::string reason;
    std::string sequence;
    std::string miss_reason;
};
NrVitReuseSnapshot nr_vit_reuse_snapshot();
void nr_vit_reuse_shutdown() noexcept;
void nr_vit_reuse_submitted(ID3D12GraphicsCommandList*) noexcept;
void nr_vit_reuse_reset(ID3D12GraphicsCommandList*) noexcept;
}
