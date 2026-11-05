#pragma once
namespace cheeky::foveated_dlss {
inline constexpr char nr_meter_shader[] = R"(
Texture2D<float4> Source0 : register(t0);
RWTexture2D<float4> Output0 : register(u0);
cbuffer C : register(b0) { uint2 Size;uint2 SourceSize;uint2 SourceBase;uint2 ProxySize;float4 Padding0;uint4 Padding1;float4 Padding2;uint4 Padding3;float4 Padding4[4];uint ColorPipeline; };
float Luminance(float3 c){return dot(c,float3(.2126,.7152,.0722));}
float Decode(float c){return c<=.04045 ? c/12.92 : pow((c+.055)/1.055,2.4);}
float3 SignalConvert(float3 c,uint e,bool b){return float3(Decode(c.r),Decode(c.g),Decode(c.b));}
// Lecram-inspired, bounded same-frame metering. No readback or temporal history.
// Whole view sampling is independent of the gaze crop. Black/invalid samples
// cannot poison the reduction. All-black frames use neutral exposure.
groupshared float MeterLog[64];
groupshared uint MeterCount[64];
[numthreads(64, 1, 1)]
void MeterMain(uint lane : SV_GroupIndex) {
    float total=0.0; uint count=0;
    for(uint i=lane;i<2304U;i+=64U) {
        uint2 grid=uint2(i%64U,i/64U);
        uint2 p=min(uint2((float2(grid)+0.5)*float2(SourceSize)/float2(64,36)),SourceSize-1U);
        float3 rgb=Source0.Load(int3(SourceBase+p,0)).rgb;
        if(ColorPipeline==2U) rgb=SignalConvert(rgb,0U,false);
        float y=Luminance(rgb);
        if(isfinite(y) && y>0.000001) {total+=clamp(log2(y),-24.0,24.0);++count;}
    }
    MeterLog[lane]=total; MeterCount[lane]=count;
    GroupMemoryBarrierWithGroupSync();
    for(uint step=32U;step>0U;step>>=1U) {
        if(lane<step) {MeterLog[lane]+=MeterLog[lane+step];MeterCount[lane]+=MeterCount[lane+step];}
        GroupMemoryBarrierWithGroupSync();
    }
    if(lane==0U) {
        float mean=MeterCount[0]>0U ? exp2(MeterLog[0]/float(MeterCount[0])) : 0.0;
        float gain=mean>0.0 ? clamp(0.18/mean,0.0625,16.0) : 1.0;
        Output0[uint2(0,0)]=float4(gain,mean,float(MeterCount[0]),1.0);
    }
}

)";
}
