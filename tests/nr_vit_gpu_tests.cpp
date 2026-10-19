#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <iostream>
#include <stdexcept>
#include "../src/nr_vit_reuse.inl"
using namespace cheeky::foveated_dlss;
using Microsoft::WRL::ComPtr;
void require(HRESULT hr){if(FAILED(hr))throw std::runtime_error("HRESULT "+std::to_string(hr));}
std::vector<std::pair<void*,unsigned>> recorded;
void* fail_function{};
int __cdecl record_launch(ID3D12GraphicsCommandList*,const nr_vit::Launch* kernels,unsigned count){recorded.emplace_back(kernels[0].fn,count);return kernels[0].fn==fail_function?-1:0;}
int main(){try{
    ComPtr<IDXGIFactory6> factory;require(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));
    ComPtr<ID3D12Device> device;
    for(unsigned i=0;;++i){ComPtr<IDXGIAdapter1> a;if(factory->EnumAdapters1(i,&a)==DXGI_ERROR_NOT_FOUND)break;DXGI_ADAPTER_DESC1 desc{};a->GetDesc1(&desc);
        if(desc.VendorId==0x10de && SUCCEEDED(D3D12CreateDevice(a.Get(),D3D_FEATURE_LEVEL_12_0,IID_PPV_ARGS(&device)))){std::wcout<<desc.Description<<L"\n";break;}}
    if(!device)throw std::runtime_error("NVIDIA D3D12 device required");
    nr_vit::initialize();auto& s=nr_vit::state();if(!s.stats.hooked)throw std::runtime_error(s.stats.reason);
    ComPtr<ID3D12CommandQueue> queue;D3D12_COMMAND_QUEUE_DESC q{};require(device->CreateCommandQueue(&q,IID_PPV_ARGS(&queue)));
    ComPtr<ID3D12CommandAllocator> allocator;require(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)));
    ComPtr<ID3D12GraphicsCommandList> list;require(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list)));
    constexpr std::uint64_t bytes=240*2048;
    auto buffer=[&](D3D12_HEAP_TYPE type,D3D12_RESOURCE_STATES state){
        D3D12_HEAP_PROPERTIES heap{};heap.Type=type;D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;d.Width=bytes;d.Height=1;d.DepthOrArraySize=1;d.MipLevels=1;d.SampleDesc.Count=1;d.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        if(type==D3D12_HEAP_TYPE_DEFAULT)d.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        ComPtr<ID3D12Resource> r;require(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&d,state,nullptr,IID_PPV_ARGS(&r)));return r;
    };
    auto upload=buffer(D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ);
    auto src=buffer(D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_COPY_DEST);
    auto dst=buffer(D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    auto readback=buffer(D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
    void* mapped{};D3D12_RANGE empty{};require(upload->Map(0,&empty,&mapped));
    for(std::uint64_t i=0;i<bytes/4;++i)static_cast<unsigned*>(mapped)[i]=unsigned(i*2654435761U)^0xa563b1efU;upload->Unmap(0,nullptr);
    auto transition=[&](ID3D12Resource* r,D3D12_RESOURCE_STATES before,D3D12_RESOURCE_STATES after){D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition={r,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,before,after};list->ResourceBarrier(1,&b);};
    list->CopyBufferRegion(src.Get(),0,upload.Get(),0,bytes);transition(src.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    nr_vit::Cache cache;
    if(!nr_vit::prepare(s,device.Get(),cache,bytes))throw std::runtime_error("Copy setup failed");
    if(nr_vit::copy(s,list.Get(),device.Get(),cache.buffer->GetGPUVirtualAddress(),src->GetGPUVirtualAddress(),bytes)!=0)throw std::runtime_error("Save failed");
    if(nr_vit::copy(s,list.Get(),device.Get(),dst->GetGPUVirtualAddress(),cache.buffer->GetGPUVirtualAddress(),bytes)!=0)throw std::runtime_error("Restore failed");
    transition(dst.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);list->CopyBufferRegion(readback.Get(),0,dst.Get(),0,bytes);require(list->Close());
    ID3D12CommandList* lists[]{list.Get()};queue->ExecuteCommandLists(1,lists);
    ComPtr<ID3D12Fence> fence;require(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)));require(queue->Signal(fence.Get(),1));
    HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);if(!event)throw std::runtime_error("Event failed");require(fence->SetEventOnCompletion(1,event));
    const auto waited=WaitForSingleObject(event,10000);CloseHandle(event);if(waited!=WAIT_OBJECT_0)throw std::runtime_error("GPU timeout");
    D3D12_RANGE range{0,bytes};require(readback->Map(0,&range,&mapped));
    for(std::uint64_t i=0;i<bytes/4;++i)if(static_cast<unsigned*>(mapped)[i]!=(unsigned(i*2654435761U)^0xa563b1efU))throw std::runtime_error("GPU copy mismatch");
    readback->Unmap(0,&empty);
    std::cout<<"PASS: private ViT save/restore GPU readback, "<<bytes<<" bytes. Does not validate NR model sequence or quality.\n";
    require(allocator->Reset());require(list->Reset(allocator.Get(),nullptr));
    auto original_launch=s.launch;s.launch=record_launch;
    auto check=[](bool ok){if(!ok)throw std::runtime_error("Dispatch ordering regression");};
    struct Params{std::uint64_t src,dst;unsigned rows,cols;} params{src->GetGPUVirtualAddress(),dst->GetGPUVirtualAddress(),12,20};
    nr_vit::Launch kernels[5]{};
    for(unsigned i=0;i<5;++i){kernels[i].fn=reinterpret_cast<void*>(std::uintptr_t(100+i));kernels[i].params=&params;kernels[i].size=sizeof params;kernels[i].grid={240*2,1,1};kernels[i].block={256,1,1};}
    s.kernels[kernels[1].fn]={nr_vit::Role::start,false};s.kernels[kernels[2].fn]={nr_vit::Role::inner,false};s.kernels[kernels[3].fn]={nr_vit::Role::end,false};
    unsigned assertions{};
    auto assert_case=[&](bool ok,const char* label){++assertions;if(!ok)throw std::runtime_error(label);};
    auto begin=[&](std::uint64_t frame,bool reset=false,std::uint64_t view=42){
        recorded.clear();nr_vit::context={true,reset,view,frame,device.Get(),list.Get()};
    };
    auto end=[&](){return nr_vit::finish_context();};
    auto invoke=[&](unsigned first,unsigned count){return nr_vit::launch_hook(list.Get(),kernels+first,count);};
    auto split=[&](std::uint64_t frame,bool reset=false,std::uint64_t view=42){
        begin(frame,reset,view);int rc=0;for(unsigned i=0;i<5 && !rc;++i)rc=invoke(i,1);
        const int finished=end();return rc?rc:finished;
    };
    auto packed=[&](std::uint64_t frame){begin(frame);const int rc=invoke(0,5);const int finished=end();return rc?rc:finished;};
    assert_case(split(10)==0 && recorded.size()==6 && s.stats.computed==1,"split run computed and cached");
    nr_vit_reuse_submitted(list.Get());
    const auto copy_fn=s.devices[device.Get()].copy;
    assert_case(split(11)==0 && recorded.size()==3 && recorded[1].first==copy_fn && s.stats.reused==1,"split run reused");
    assert_case(s.stats.last_run_kernels==3 && s.stats.last_run_calls==3 && s.stats.split_runs==2,"split census counters");
    assert_case(packed(12)==0 && recorded.size()==6,"packed run computed");
    nr_vit_reuse_submitted(list.Get());
    assert_case(packed(13)==0 && recorded.size()==3 && s.stats.reused==2,"API partition does not alter learned trace");
    assert_case(split(14)==0,"cache seed before reset");
    nr_vit_reuse_submitted(list.Get());
    assert_case(split(15,true)==0 && recorded.size()==6 && s.stats.reused==2,"reset computes instead of reusing");
    assert_case(split(16)==0,"cache seed before discard");
    nr_vit_reuse_reset(list.Get());
    assert_case(split(17)==0 && recorded.size()==6 && s.stats.reused==2,"discarded cache is not reused");
    // Independent view/pass keys do not consume another view's private output.
    assert_case(split(18,false,43)==0 && split(18,false,44)==0,"independent view cache creation");
    nr_vit_reuse_submitted(list.Get());
    assert_case(split(19,false,43)==0 && split(19,false,44)==0 && s.stats.reused==4,"independent view reuse");
    // Mismatch after Start has been skipped: replay current Start, not last-frame input.
    assert_case(split(20)==0,"seed for changed interior parameters");
    nr_vit_reuse_submitted(list.Get());begin(21);
    assert_case(invoke(0,1)==0 && invoke(1,1)==0 && recorded.size()==1,"prefix skipped provisionally");
    auto changed=params;changed.src+=16;kernels[2].params=&changed;
    assert_case(invoke(2,1)==0 && invoke(3,1)==0 && invoke(4,1)==0 && end()==0,"parameter change recovers");
    assert_case(recorded.size()==6 && recorded[1].first==kernels[1].fn && recorded[2].first==kernels[2].fn && s.stats.replayed==1 && !s.disabled,"prefix replay occurs before changed kernel");
    kernels[2].params=&params;
    // Unknown kernel in the middle: compute skipped prefix BEFORE the unknown command.
    assert_case(split(22)==0,"seed for interrupted run");
    nr_vit_reuse_submitted(list.Get());begin(23);
    assert_case(invoke(1,1)==0 && invoke(2,1)==0 && recorded.empty(),"skip two interior launches");
    assert_case(invoke(0,1)==0 && end()==0 && recorded.size()==3 && recorded[0].first==kernels[1].fn && recorded[2].first==kernels[0].fn && s.disabled,"interrupted run replays prefix before foreign command");
    s.disabled=false;nr_vit::invalidate(s);
    // Missing End at evaluation return: recover and invalidate, never leave a skipped prefix.
    assert_case(split(24)==0,"seed missing-End case");nr_vit_reuse_submitted(list.Get());begin(25);
    assert_case(invoke(1,1)==0 && invoke(2,1)==0 && end()!=0 && recorded.size()==2 && s.disabled,"missing End recovers pending prefix and fails NR safely");
    s.disabled=false;nr_vit::invalidate(s);
    assert_case(split(26)==0,"seed restore-error case");nr_vit_reuse_submitted(list.Get());
    fail_function=copy_fn;
    assert_case(split(27)==0 && recorded.size()==6 && recorded[1].first==copy_fn &&
        recorded[2].first==kernels[1].fn && recorded[3].first==kernels[2].fn &&
        recorded[4].first==kernels[3].fn && recorded[5].first==kernels[4].fn && s.disabled,"failed restore replays entire current run and preserves suffix");
    fail_function=nullptr;s.launch=original_launch;require(list->Close());
    s.disabled=false;
    auto slot=[&](std::uint64_t key){nr_vit::Scope scope(true,"310.9.16.0",device.Get(),list.Get(),key,false);const auto frame=nr_vit::context.frame;scope.finish();return frame;};
    assert_case(slot(42)==0 && slot(42|(1ULL<<63))==0 && slot(43)==0,"Scope shares first stereo round across passes");
    assert_case(slot(42)==1 && slot(42|(1ULL<<62))==1 && slot(43)==1,"Scope advances stereo round without presentation callbacks");
    const auto targets=s.hook_targets;nr_vit_reuse_shutdown();
    for(auto* target:targets)assert_case(MH_EnableHook(target)==MH_ERROR_NOT_CREATED,"NVAPI hook removed at shutdown");
    std::cout<<"PASS: "<<assertions<<" streaming dispatch, replay, view isolation, submission and teardown checks.\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
