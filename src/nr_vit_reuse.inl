// Experimental F5-derived D3D12-only ViT reuse. Included once by dlss_nr.cpp.
// See NR-F5-REUSE.txt for provenance, restrictions and validation requirements.
#include "nr_vit_reuse.hpp"
#include "nr_vit_policy.hpp"
#include "nr_vit_copy_kernel.hpp"
#include <MinHook.h>
#include <wrl/client.h>
#include <map>
#include <array>
#include <string>
#include <vector>
#include <sstream>

namespace cheeky::foveated_dlss {
namespace nr_vit {
using Microsoft::WRL::ComPtr;
// NVIDIA nvapi.h at 9b181ea572f680327fe01a14a0f1f41c78034104,
// experimental D3D12 CU ABI (not an official DLSS-NR option).
struct Dim {unsigned x,y,z;};
struct Launch {void* fn;Dim grid,block;unsigned shared;const void* params;unsigned size;};
static_assert(sizeof(Launch)==56 && offsetof(Launch,params)==40);
using CreateModule=int(__cdecl*)(ID3D12Device*,const void*,unsigned,void**);
using CreateFunction=int(__cdecl*)(ID3D12Device*,void*,const char*,void**);
using LaunchChain=int(__cdecl*)(ID3D12GraphicsCommandList*,const Launch*,unsigned);
using Destroy=int(__cdecl*)(ID3D12Device*,void*);
struct Kernel {Role role{};bool fp8{};std::string name;};
struct OwnedLaunch {
    Launch launch{};
    std::vector<unsigned char> params;
    unsigned call{};
    explicit OwnedLaunch(const Launch& k,unsigned api_call):launch(k),call(api_call) {
        const auto* begin=static_cast<const unsigned char*>(k.params);
        params.assign(begin,begin+k.size);launch.params=nullptr;
    }
    Launch get()const {auto k=launch;k.params=params.data();return k;}
    bool matches(const Launch& k)const {
        return launch.fn==k.fn && launch.shared==k.shared && launch.size==k.size &&
            std::memcmp(&launch.grid,&k.grid,sizeof(Dim))==0 &&
            std::memcmp(&launch.block,&k.block,sizeof(Dim))==0 &&
            std::memcmp(params.data(),k.params,k.size)==0;
    }
};
struct Cache {
    ComPtr<ID3D12Resource> buffer;
    std::uint64_t bytes{},dst{},signature{},frame{},last_evaluation{};
    bool valid{},submitted{};
    ID3D12GraphicsCommandList* recorded_list{};
    std::vector<OwnedLaunch> pattern;
};
struct Device {
    ComPtr<ID3D12Device> owner;
    void* module{};void* copy{};
};
struct State {
    std::recursive_mutex mutex;
    std::map<void*,Kernel> kernels;
    std::map<ID3D12Device*,Device> devices;
    std::map<std::pair<ID3D12Device*,std::uint64_t>,Cache> caches;
    std::vector<ComPtr<ID3D12Resource>> retired;
    std::uint64_t allocated{};
    CreateModule create_module{};CreateFunction create_function{};LaunchChain launch{};
    Destroy destroy_function{},destroy_module{};
    bool attempted{},disabled{};
    std::array<void*,4> hook_targets{};
    NrVitReuseSnapshot stats;
    RoundClock clock;
};
// GPU-owned allocations are retained until process exit, bounded to 256 MiB.
State& state(){static auto* s=new State;return *s;}
struct Context {
    bool active{},reset{};std::uint64_t key{},frame{};ID3D12Device* device{};ID3D12GraphicsCommandList* list{};
    bool inside{},skipping{},fp8{},aborted{};
    unsigned calls{},runs{},first_call{};
    std::size_t param_bytes{};
    std::vector<OwnedLaunch> run;
};
thread_local Context context;
thread_local bool issued_work{};
void invalidate(State& s){for(auto& [key,c]:s.caches)c.valid=false;}
void barrier(ID3D12GraphicsCommandList* c){D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;c->ResourceBarrier(1,&b);}
int __cdecl create_hook(ID3D12Device* d,void* m,const char* name,void** out) {
    auto& s=state();const int rc=s.create_function(d,m,name,out);
    if(rc==0 && out && *out) {std::lock_guard lock(s.mutex);s.kernels[*out]={role(name?name:""),name && std::string_view(name).ends_with("_fp8"),name?name:""};}
    return rc;
}
int __cdecl destroy_function_hook(ID3D12Device* d,void* f){auto& s=state();std::lock_guard lock(s.mutex);s.kernels.erase(f);invalidate(s);return s.destroy_function(d,f);}
int __cdecl destroy_module_hook(ID3D12Device* d,void* m){auto& s=state();std::lock_guard lock(s.mutex);invalidate(s);return s.destroy_module(d,m);}
bool prepare(State& s,ID3D12Device* d,Cache& cache,std::uint64_t bytes) {
    auto& dev=s.devices[d];
    if(!dev.copy) {
        dev.owner=d;
        if(!s.create_module || s.create_module(d,DlssNrVitCopyKernel::fatbin,sizeof(DlssNrVitCopyKernel::fatbin),&dev.module)!=0 ||
           s.create_function(d,dev.module,"nr_vit_copy16",&dev.copy)!=0)return false;
    }
    if(cache.buffer && cache.bytes==bytes)return true;
    if(s.allocated+bytes>256ULL*1024*1024)return false;
    D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;desc.Width=bytes;
    desc.Height=1;desc.DepthOrArraySize=1;desc.MipLevels=1;desc.SampleDesc.Count=1;
    desc.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    ComPtr<ID3D12Resource> buffer;
    if(FAILED(d->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,nullptr,IID_PPV_ARGS(&buffer))))return false;
    if(cache.buffer)s.retired.push_back(cache.buffer);
    cache.buffer=buffer;cache.bytes=bytes;cache.valid=false;s.allocated+=bytes;return true;
}
int copy(State& s,ID3D12GraphicsCommandList* list,ID3D12Device* d,std::uint64_t dst,std::uint64_t src,std::uint64_t bytes) {
    struct Params {std::uint64_t dst,src,n;} p{dst,src,bytes/16};
    Launch k{};k.fn=s.devices.at(d).copy;k.grid={unsigned((std::min)((p.n+255)/256,std::uint64_t(1024))),1,1};
    k.block={256,1,1};k.params=&p;k.size=sizeof p;
    barrier(list);const int rc=s.launch(list,&k,1);barrier(list);return rc;
}
void census(State& s) {
    const auto& c=context;
    s.stats.last_run_kernels=c.run.size();
    s.stats.last_run_calls=c.calls-c.first_call+1;
    if(!s.stats.sequence.empty())return;
    std::ostringstream out;
    for(const auto& owned:c.run) {
        const auto k=owned.get();const auto found=s.kernels.find(k.fn);
        out<<"call "<<owned.call<<": "<<(found==s.kernels.end()?"unknown":found->second.name)
           <<" params="<<k.size<<" grid="<<k.grid.x<<','<<k.grid.y<<','<<k.grid.z
           <<" block="<<k.block.x<<','<<k.block.y<<','<<k.block.z<<"; ";
        if(out.tellp()>7800){out<<"[bounded census truncated]";break;}
    }
    s.stats.sequence=out.str();
}
int real(State& s,ID3D12GraphicsCommandList* list,const Launch* k,unsigned n) {
    issued_work=true;return s.launch(list,k,n);
}
// Own the CURRENT skipped parameters, never replay last frame's parameters.
// Barriers serialize the replayed prefix before the first non-skipped command.
int replay(State& s) {
    auto& c=context;
    if(!c.skipping)return 0;
    c.skipping=false;
    ++s.stats.replayed;
    for(const auto& owned:c.run) {
        auto k=owned.get();barrier(c.list);
        const int rc=real(s,c.list,&k,1);barrier(c.list);
        if(rc!=0)return rc;
    }
    return 0;
}
void reject(State& s,const char* why) {
    auto& c=context;
    s.disabled=true;invalidate(s);++s.stats.rejected;s.stats.reason=why;
    c.aborted=true;c.inside=false;
}
bool output_contract(const Launch& end,bool fp8,std::uint64_t& dst,std::uint64_t& bytes) {
    std::uint64_t threads=1;
    for(unsigned n:{end.grid.x,end.grid.y,end.grid.z,end.block.x,end.block.y,end.block.z}) {
        if(!n || threads>(1ULL<<32)/n)return false;
        threads*=n;
    }
    return output(end.params,end.size,threads,fp8,dst,bytes);
}
int one(State& s,ID3D12GraphicsCommandList* list,const Launch& k) {
    auto& c=context;
    const auto found=s.kernels.find(k.fn);
    static const Kernel empty;
    const Kernel& info=found==s.kernels.end()?empty:found->second;
    auto& cache=s.caches[{c.device,c.key}];
    if(info.role==Role::none) {
        if(c.inside) {
            const int rc=replay(s);census(s);
            reject(s,"Non-ViT kernel interrupted run; skipped prefix replayed, reuse disabled");
            if(rc!=0)return rc;
        }
        return real(s,list,&k,1);
    }
    if(!k.params || !k.size || k.size>65536 || c.run.size()>=256 || c.param_bytes+k.size>1024*1024) {
        const int rc=replay(s);census(s);
        reject(s,"Unexpected or oversized ViT parameters; reuse disabled");
        return rc!=0?rc:real(s,list,&k,1);
    }
    if(info.role==Role::start) {
        if(c.inside || c.runs!=0) {
            const int rc=replay(s);census(s);
            reject(s,"Multiple/nested ViT runs in one evaluation; reuse disabled");
            return rc!=0?rc:real(s,list,&k,1);
        }
        ++c.runs;c.inside=true;c.fp8=info.fp8;c.first_call=c.calls;
        c.skipping=reuse_due(cache.valid && cache.submitted,c.reset,cache.frame,c.frame,0,0) &&
            cache.last_evaluation!=c.frame && !cache.pattern.empty() && cache.pattern.front().matches(k);
        if(!c.skipping && (c.frame&1)) {
            ++s.stats.cache_misses;
            s.stats.miss_reason=!cache.valid?"No valid prior output":!cache.submitted?"Prior cache recording not observed at submission":
                c.reset?"NR history reset":cache.last_evaluation==c.frame?"Repeated view/pass in render round":
                cache.frame+1!=c.frame?"Prior cache is not from preceding render round":
                cache.pattern.empty()?"No learned sequence":"Start kernel parameters/geometry changed";
        }
        cache.valid=false;cache.last_evaluation=c.frame;
    } else if(!c.inside || info.fp8!=c.fp8) {
        const int rc=replay(s);census(s);
        reject(s,"Orphan ViT kernel or mixed precision sequence; reuse disabled");
        return rc!=0?rc:real(s,list,&k,1);
    }
    // Validate every kernel/geometry/parameter blob against the previously
    // computed run. A cache miss replays the prefix, then computes the remainder.
    if(c.skipping && (c.run.size()>=cache.pattern.size() || !cache.pattern[c.run.size()].matches(k))) {
        const int rc=replay(s);++s.stats.cache_misses;
        s.stats.miss_reason="Kernel parameters/geometry changed at index "+std::to_string(c.run.size());
        if(rc!=0){reject(s,"Prefix replay failed");return rc;}
    }
    std::uint64_t dst{},bytes{};
    if(info.role==Role::end && (c.run.size()<2 || !output_contract(k,c.fp8,dst,bytes))) {
        const int rc=replay(s);census(s);
        reject(s,"Unsupported ViT output contract; prefix replayed, normal NR");
        return rc!=0?rc:real(s,list,&k,1);
    }
    c.run.emplace_back(k,c.calls);c.param_bytes+=k.size;
    if(info.role!=Role::end)return c.skipping?0:real(s,list,&k,1);

    census(s);
    if(c.calls>c.first_call)++s.stats.split_runs;
    if(c.skipping && (c.run.size()!=cache.pattern.size() || cache.dst!=dst || cache.bytes!=bytes)) {
        const int rc=replay(s);++s.stats.cache_misses;
        // replay includes the current End, so do not launch it twice.
        if(rc!=0){reject(s,"Whole-run replay failed");return rc;}
        c.inside=false;cache.valid=false;
        return 0;
    }
    if(c.skipping) {
        const int restored=copy(s,list,c.device,dst,cache.buffer->GetGPUVirtualAddress(),bytes);
        if(restored==0) {
            c.skipping=false;cache.valid=true;
            ++s.stats.reused;s.stats.copied_bytes+=bytes;
            s.stats.reason="Active: evaluation-scoped streaming ViT reuse";
        } else {
            const int rc=replay(s);
            reject(s,"Restore failed; whole current run replayed, reuse disabled");
            if(rc!=0)return rc;
        }
    } else {
        const int rc=real(s,list,&k,1);
        if(rc!=0){reject(s,"Computed ViT end failed");return rc;}
        ++s.stats.computed;
        if(!prepare(s,c.device,cache,bytes) ||
           copy(s,list,c.device,cache.buffer->GetGPUVirtualAddress(),dst,bytes)!=0) {
            reject(s,"Private cache save failed; normal NR, reuse disabled");
        } else {
            cache.pattern=std::move(c.run);cache.valid=true;cache.submitted=false;
            cache.recorded_list=list;cache.frame=c.frame;cache.dst=dst;
            s.stats.copied_bytes+=bytes;
            s.stats.reason="Computed streaming ViT run; cache saved";
        }
    }
    c.inside=false;c.run.clear();
    return 0;
}
int launch_impl(ID3D12GraphicsCommandList* list,const Launch* kernels,unsigned count) {
    auto& s=state();
    if(!context.active)return s.launch(list,kernels,count);
    std::lock_guard lock(s.mutex);
    if(context.list!=list) {
        const int rc=replay(s);reject(s,"Command list changed within evaluation; reuse disabled");
        return rc!=0?rc:real(s,list,kernels,count);
    }
    if(s.disabled)return real(s,list,kernels,count);
    if(!kernels || !count || count>4096) {
        const int rc=replay(s);reject(s,"Invalid launch chain; reuse disabled");
        return rc!=0?rc:real(s,list,kernels,count);
    }
    ++context.calls;
    // Leave non-ViT chains untouched outside a run.
    if(!context.inside) {
        bool saw=false;
        for(unsigned i=0;i<count;++i){auto it=s.kernels.find(kernels[i].fn);saw|=it!=s.kernels.end() && it->second.role!=Role::none;}
        if(!saw)return real(s,list,kernels,count);
    }
    for(unsigned i=0;i<count;++i) {
        if(s.disabled)return real(s,list,kernels+i,count-i);
        const int rc=one(s,list,kernels[i]);
        if(rc!=0)return rc;
    }
    return 0;
}
int finish_context() noexcept {
    try {
        auto& s=state();std::lock_guard lock(s.mutex);int rc=0;
        if(context.active && context.inside) {
            rc=replay(s);census(s);
            reject(s,"Evaluation ended before ViT End; prefix replayed, reuse disabled");
            if(rc==0)rc=-1;
        }
        if(context.active && !context.runs && !s.disabled)s.stats.reason="No ViT start observed in this evaluation";
        context={};return rc;
    } catch(...) {context={};return -1;}
}
int __cdecl launch_hook(ID3D12GraphicsCommandList* list,const Launch* kernels,unsigned count) {
    issued_work=false;
    try {return launch_impl(list,kernels,count);}
    catch(...) {
        auto& s=state();std::lock_guard lock(s.mutex);
        // An allocation failure mid-stream must not let a decoder consume an
        // absent bottleneck. Replay only owned skipped launches, then fail NR.
        const bool had_pending=context.skipping;
        int rc=-1;
        try {rc=replay(s);}catch(...){}
        reject(s,"Streaming interceptor exception; reuse disabled");
        if(had_pending || issued_work || rc!=0)return -1;
        return s.launch(list,kernels,count);
    }
}
void initialize() {
    auto& s=state();std::lock_guard lock(s.mutex);if(s.attempted)return;s.attempted=true;
    s.stats.reason="NVAPI hooks unavailable";
    auto module=LoadLibraryExW(L"nvapi64.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);
    if(!module)return;
    using Query=void*(__cdecl*)(unsigned);
    auto query=reinterpret_cast<Query>(GetProcAddress(module,"nvapi_QueryInterface"));if(!query)return;
    s.create_module=reinterpret_cast<CreateModule>(query(0xad1a677d));
    void* targets[]{query(0xe2436e22),query(0x24973538),query(0xdf295ea6),query(0x41c65285)};
    void* detours[]{reinterpret_cast<void*>(create_hook),reinterpret_cast<void*>(launch_hook),reinterpret_cast<void*>(destroy_function_hook),reinterpret_cast<void*>(destroy_module_hook)};
    void** originals[]{reinterpret_cast<void**>(&s.create_function),reinterpret_cast<void**>(&s.launch),reinterpret_cast<void**>(&s.destroy_function),reinterpret_cast<void**>(&s.destroy_module)};
    const auto init=MH_Initialize();if(init!=MH_OK && init!=MH_ERROR_ALREADY_INITIALIZED)return;
    unsigned created{};
    for(;created<4;++created)if(!targets[created] || MH_CreateHook(targets[created],detours[created],originals[created])!=MH_OK)break;
    if(created!=4){for(unsigned i=0;i<created;++i)MH_RemoveHook(targets[i]);return;}
    // Function pointers already obtained by another caller also pass through these entry hooks.
    for(unsigned i=0;i<4;++i)if(MH_EnableHook(targets[i])!=MH_OK){for(unsigned j=0;j<4;++j){MH_DisableHook(targets[j]);MH_RemoveHook(targets[j]);}return;}
    std::copy(std::begin(targets),std::end(targets),s.hook_targets.begin());
    s.stats.hooked=true;s.stats.reason="Ready; reuse off";
}
struct Scope {
    Scope(bool enabled,const char* version,ID3D12Device* device,ID3D12GraphicsCommandList* list,std::uint64_t key,bool reset) {
        auto& s=state();std::lock_guard lock(s.mutex);
        const bool supported=version && std::strcmp(version,"310.9.16.0")==0;
        if(!enabled || !supported || !s.stats.hooked || s.disabled) {
            invalidate(s);context={};
            if(enabled && !supported)s.stats.reason="Unsupported NR DLL (requires 310.9.16.0); normal NR";
            return;
        }
        constexpr auto pass_mask=std::uint64_t(3)<<62;
        const auto slot=s.clock.next(key&~pass_mask,(key&pass_mask)==0);
        context={true,reset || slot==UINT64_MAX,key,slot,device,list};
    }
    int finish() noexcept {return finish_context();}
    ~Scope(){if(context.active)static_cast<void>(finish_context());}
};
}
void nr_vit_reuse_submitted(ID3D12GraphicsCommandList* list) noexcept {
    if(!list)return;auto& s=nr_vit::state();std::lock_guard lock(s.mutex);
    for(auto& [key,c]:s.caches)if(c.valid && c.recorded_list==list)c.submitted=true;
}
void nr_vit_reuse_reset(ID3D12GraphicsCommandList* list) noexcept {
    if(!list)return;auto& s=nr_vit::state();std::lock_guard lock(s.mutex);
    for(auto& [key,c]:s.caches)if(c.recorded_list==list && !c.submitted)c.valid=false;
}
void nr_vit_reuse_shutdown() noexcept {
    auto& s=nr_vit::state();
    // Called by the interception owner after processing stops, including when
    // native submission observers remain installed to retire pending GPU work.
    for(auto* target:s.hook_targets)if(target){MH_DisableHook(target);MH_RemoveHook(target);}
    std::lock_guard lock(s.mutex);s.hook_targets={};s.stats.hooked=false;
    s.disabled=true;nr_vit::invalidate(s);
    s.stats.reason="Interception stopped; restart game to use F5 reuse";
}
NrVitReuseSnapshot nr_vit_reuse_snapshot(){auto& s=nr_vit::state();std::lock_guard lock(s.mutex);return s.stats;}
}
