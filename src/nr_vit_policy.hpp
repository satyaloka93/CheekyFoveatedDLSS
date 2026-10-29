#pragma once
#include <cstdint>
#include <cstring>
#include <span>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <limits>
namespace cheeky::foveated_dlss::nr_vit {
// A repeated primary view starts a new render round. Extra NR passes use the
// primary view's slot. L,R,L,R and alternating-eye L,R both keep eye phases
// aligned without treating command-queue submissions as frame boundaries.
class RoundClock {
    std::uint64_t round_{};
    std::unordered_set<std::uint64_t> seen_;
    std::unordered_map<std::uint64_t,std::uint64_t> slots_;
public:
    std::uint64_t next(std::uint64_t base,bool primary) {
        if(!primary){const auto it=slots_.find(base);return it==slots_.end()?UINT64_MAX:it->second;}
        if(seen_.contains(base) || seen_.size()>=64){++round_;seen_.clear();}
        if(slots_.size()>=64 && !slots_.contains(base)){++round_;seen_.clear();slots_.clear();}
        seen_.insert(base);slots_[base]=round_;return round_;
    }
};
enum class Role { none, start, inner, end };
inline bool reuse_enabled(bool master, unsigned every, unsigned plain, bool fp8) {
    return master && (fp8 ? every : plain) == 2;
}
inline Role role(std::string_view name) {
    if (!name.starts_with("cc_vit_1d_")) return Role::none;
    name.remove_prefix(10);
    if (name == "repack_2d_to_1d" || name == "repack_2d_to_1d_fp8") return Role::start;
    if (name == "repack_1d_to_2d" || name == "repack_1d_to_2d_fp8") return Role::end;
    return Role::inner;
}
// No deferred launches: a whole contiguous run must be present in this API call.
inline bool complete_run(std::span<const Role> roles, std::size_t& first, std::size_t& last) {
    bool inside=false, found=false;
    for (std::size_t i=0;i<roles.size();++i) {
        switch(roles[i]) {
        case Role::start: if(inside || found)return false; inside=true;first=i;break;
        case Role::inner: if(!inside)return false;break;
        case Role::end: if(!inside || i<=first+1)return false; inside=false;found=true;last=i;break;
        case Role::none: if(inside)return false;break;
        }
    }
    return found && !inside;
}
// F5 v0.1.32 CheckOutput contract: rows*cols tokens, 1024 channels, one word/thread.
inline bool output(const void* params, unsigned size, std::uint64_t threads,
    bool fp8, std::uint64_t& dst, std::uint64_t& bytes) {
    struct Params {std::uint64_t src,dst;std::uint32_t rows,cols;} p{};
    if(!params || size!=sizeof p)return false;
    std::memcpy(&p,params,sizeof p);
    const std::uint64_t tokens=std::uint64_t(p.rows)*p.cols;
    if(!p.dst || (p.dst&15) || !tokens || tokens>16384)return false;
    bytes=tokens*1024*(fp8?1:2);dst=p.dst;
    return threads==bytes/4;
}
inline bool reuse_due(bool valid, bool reset, std::uint64_t saved_frame,
    std::uint64_t frame, std::uint64_t signature, std::uint64_t saved_signature) {
    return valid && !reset && (frame&1) && frame>saved_frame && frame-saved_frame==1 && signature==saved_signature;
}
}
