#pragma once
#include <d3d12.h>
#include <cstdint>
// Optional, versioned cross-DLL diagnostic ABI. Resource states must be the
// states at the call site; calls never change rendering parameters.
struct CvrVectorPlane { ID3D12Resource* resource; uint32_t state,x,y,width,height; };
struct CvrVectorPacket {
    uint32_t size,version,stage,frame,eye;
    CvrVectorPlane plane[3]; // color, motion, depth
    float motionScale[2],jitterUV[2]; // stored MV -> full-view UV displacement
    uint32_t reset,depthInverted,motionJittered,regionCropped;
    const void* constants; uint32_t constantsBytes;
};
using CvrVectorRecordFn=void(*)(ID3D12GraphicsCommandList*,const CvrVectorPacket*);
using CvrVectorContextFn=uint64_t(*)(); // 0 disarmed; otherwise 1+(frame*2+eye)
#ifdef CVR_VECTOR_AUDIT_PORT
#include <memory>
#include <vector>
#include <string>
namespace cvr::vector_audit {
struct Recording;
using Submission=std::vector<std::shared_ptr<Recording>>;
void Arm();
std::string Status();
void Begin(uint32_t frame,uint32_t eye);
void End();
void Poll();
Submission Prepare(UINT,ID3D12CommandList* const*);
void Submitted(ID3D12CommandQueue*,Submission);
void Reset(ID3D12GraphicsCommandList*);
}
#endif
