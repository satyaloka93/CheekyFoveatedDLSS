#pragma once
#include "nr_temporal_shader.hpp"
#include "dlss_nr_lifetime.hpp"
#include "nr_guides.hpp"
#include <chrono>
#include <deque>

namespace cheeky::foveated_dlss {
// Called under the NR execution lock. Immutable descriptor slots are recycled
// only after every recording and every queue execution that uses them retires.
struct NrTemporalState {
    template<class T> using Ptr=Microsoft::WRL::ComPtr<T>;
    struct Slot {
        Ptr<ID3D12Resource> correction,depth;
        Ptr<ID3D12DescriptorHeap> heap;
        NrLifetime uses;
        NrHistoryTicket producer;
        unsigned width{},height{};
        std::uint64_t sequence{},bytes{};
        ~Slot(){NrTemporalState::allocated_bytes-=bytes;}
    };
    inline static std::uint64_t allocated_bytes{};
    static constexpr std::uint64_t budget=384ULL*1024*1024;
    std::deque<std::unique_ptr<Slot>> slots;
    Slot* previous{};
    Ptr<ID3D12RootSignature> root;
    Ptr<ID3D12PipelineState> pipeline;
    std::shared_ptr<NrHistoryOrder> order;
    std::chrono::steady_clock::time_point last{};
    std::uint64_t filtered{},fresh_frames{},fallbacks{};
    const char* reason{"Off"};
    NrTemporalState()=default;
    NrTemporalState(NrTemporalState&&)=default;
    NrTemporalState& operator=(NrTemporalState&&)=default;

    void invalidate(){previous=nullptr;}
    void collect(bool disable=false){
        if(disable){previous=nullptr;reason="Off";}
        for(auto it=slots.begin();it!=slots.end();){
            (*it)->uses.collect();
            if((*it).get()!=previous && (*it)->uses.empty() && disable){it=slots.erase(it);}else ++it;
        }
    }
    static void barrier(ID3D12GraphicsCommandList* list,ID3D12Resource* r,D3D12_RESOURCE_STATES a,D3D12_RESOURCE_STATES b){
        D3D12_RESOURCE_BARRIER v{};v.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        v.Transition={r,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,a,b};list->ResourceBarrier(1,&v);
    }
    bool initialize(ID3D12Device* device){
        if(pipeline)return true;
        order=nr_history_order(device);if(!order)return false;
        D3D12_DESCRIPTOR_RANGE ranges[2]{{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,6,0,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,2,0,0,6}};
        D3D12_ROOT_PARAMETER p[2]{};p[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;p[0].DescriptorTable={2,ranges};
        p[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;p[1].Constants={0,0,sizeof(NrTemporalConstants)/4};
        D3D12_ROOT_SIGNATURE_DESC d{};d.NumParameters=2;d.pParameters=p;
        Ptr<ID3DBlob> blob,errors;
        if(FAILED(D3D12SerializeRootSignature(&d,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&errors)) ||
            FAILED(device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root))))return false;
        D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={d3d_shaders::nr_temporal.data,d3d_shaders::nr_temporal.size};
        return SUCCEEDED(device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&pipeline)));
    }
    // Heap layout: 6 filter SRVs, 2 UAVs, then 3 codec SRVs (original, proxy, correction)
    Slot* acquire(ID3D12Device* device,unsigned w,unsigned h){
        for(auto& s:slots)if(s.get()!=previous && s->uses.empty()){
            if(s->width==w && s->height==h)return s.get();
        }
        for(auto it=slots.begin();it!=slots.end();) {
            if(it->get()!=previous && (*it)->uses.empty()) {it=slots.erase(it);}else ++it;
        }
        if(slots.size()>=3){reason="History slots still in use";return nullptr;}
        D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;desc.Width=w;desc.Height=h;
        desc.DepthOrArraySize=desc.MipLevels=1;desc.SampleDesc.Count=1;desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        desc.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;
        auto bytes=device->GetResourceAllocationInfo(0,1,&desc).SizeInBytes;
        desc.Format=DXGI_FORMAT_R32_FLOAT;bytes+=device->GetResourceAllocationInfo(0,1,&desc).SizeInBytes;
        if(bytes>budget || allocated_bytes>budget-bytes){reason="384 MiB global history budget";return nullptr;}
        auto s=std::make_unique<Slot>();s->width=w;s->height=h;
        D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;
        if(FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,nullptr,IID_PPV_ARGS(&s->depth))))return nullptr;
        desc.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;
        if(FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,nullptr,IID_PPV_ARGS(&s->correction))))return nullptr;
        D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.NumDescriptors=15;hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        if(FAILED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&s->heap))))return nullptr;
        s->correction->SetName(L"Cheeky NR temporal correction");s->depth->SetName(L"Cheeky NR temporal depth");
        s->bytes=bytes;allocated_bytes+=bytes;auto* result=s.get();slots.push_back(std::move(s));return result;
    }
    Slot* dispatch(ID3D12GraphicsCommandList* list,ID3D12Resource* original,ID3D12Resource* proxy,
        ID3D12Resource* neural,ID3D12Resource* motion,ID3D12Resource* depth,
        ID3D12Resource* exposure,ID3D12Resource* meter,ID3D12Resource* game,
        unsigned w,unsigned h,std::uint64_t sequence,bool reset,bool inverted,float strength){
        collect();Ptr<ID3D12Device> device;
        if(FAILED(proxy->GetDevice(IID_PPV_ARGS(&device))) || !initialize(device.Get())){reason="Temporal pipeline setup failed";++fallbacks;invalidate();return nullptr;}
        const auto now=std::chrono::steady_clock::now();
        const char* fresh_reason = reset ? "Explicit or geometry reset" : !previous ? "No previous history" :
            previous->width!=w || previous->height!=h ? "History dimensions changed" :
            previous->sequence+1!=sequence ? "Nonconsecutive history" :
            now-last>=std::chrono::milliseconds(250) ? "History older than 250 ms" :
            !nr_history_ready(previous->producer,order) ? "History not submitted once or timeline invalid" : nullptr;
        bool valid=fresh_reason==nullptr;
        Slot* out=acquire(device.Get(),w,h);
        if(!out){++fallbacks;invalidate();return nullptr;}
        auto ticket=nr_history_attach(list,order);
        if(!ticket || !out->uses.record(list) || (valid && !previous->uses.record(list))){reason="History submission tracking unavailable";++fallbacks;invalidate();return nullptr;}
        const auto step=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        auto cpu=out->heap->GetCPUDescriptorHandleForHeapStart();
        auto srv=[&](ID3D12Resource* r,DXGI_FORMAT format){D3D12_SHADER_RESOURCE_VIEW_DESC d{};d.Format=format;d.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;d.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;d.Texture2D.MipLevels=1;device->CreateShaderResourceView(r,&d,cpu);cpu.ptr+=step;};
        srv(proxy,proxy->GetDesc().Format);srv(neural,neural->GetDesc().Format);srv(motion,motion->GetDesc().Format);srv(depth,depth->GetDesc().Format);
        srv(valid?previous->correction.Get():nullptr,DXGI_FORMAT_R16G16B16A16_FLOAT);srv(valid?previous->depth.Get():nullptr,DXGI_FORMAT_R32_FLOAT);
        for(auto* r:{out->correction.Get(),out->depth.Get()}){D3D12_UNORDERED_ACCESS_VIEW_DESC d{};d.Format=r->GetDesc().Format;d.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;device->CreateUnorderedAccessView(r,nullptr,&d,cpu);cpu.ptr+=step;}
        srv(original,original->GetDesc().Format);srv(proxy,proxy->GetDesc().Format);srv(out->correction.Get(),DXGI_FORMAT_R16G16B16A16_FLOAT);
        srv(exposure,exposure?exposure->GetDesc().Format:DXGI_FORMAT_R32_FLOAT);srv(meter,DXGI_FORMAT_R32G32B32A32_FLOAT);
        for(unsigned i=0;i<2;++i){D3D12_UNORDERED_ACCESS_VIEW_DESC d{};d.Format=game->GetDesc().Format;d.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;device->CreateUnorderedAccessView(game,nullptr,&d,cpu);cpu.ptr+=step;}
        for(auto* r:{out->correction.Get(),out->depth.Get()})barrier(list,r,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        ID3D12DescriptorHeap* heaps[]{out->heap.Get()};list->SetDescriptorHeaps(1,heaps);list->SetComputeRootSignature(root.Get());list->SetPipelineState(pipeline.Get());
        list->SetComputeRootDescriptorTable(0,out->heap->GetGPUDescriptorHandleForHeapStart());
        NrTemporalConstants c{{w,h},valid?1U:0U,inverted?1U:0U,strength,{}};
        list->SetComputeRoot32BitConstants(1,sizeof(c)/4,&c,0);list->Dispatch((w+7)/8,(h+7)/8,1);
        for(auto* r:{out->correction.Get(),out->depth.Get()})barrier(list,r,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        out->producer=ticket;out->sequence=sequence;previous=out;last=now;
        if(valid){++filtered;reason="Motion-reprojected history";}else{++fresh_frames;reason=fresh_reason;}
        return out;
    }
};
}
