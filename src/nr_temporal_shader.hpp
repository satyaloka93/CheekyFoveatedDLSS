#pragma once
namespace cheeky::foveated_dlss {
struct NrTemporalConstants {unsigned size[2],valid,inverted;float strength,padding[3];};
inline constexpr char nr_temporal_shader[] = R"(
Texture2D<float4> Proxy : register(t0);
Texture2D<float4> Neural : register(t1);
Texture2D<float2> Motion : register(t2);
Texture2D<float> Depth : register(t3);
Texture2D<float4> Previous : register(t4);
Texture2D<float> PreviousDepth : register(t5);
RWTexture2D<float4> Correction : register(u0);
RWTexture2D<float> SavedDepth : register(u1);
cbuffer Constants : register(b0) {uint2 Size;uint Valid;uint Inverted;float Strength;float3 Padding;};
float luminance(float3 c){return dot(c,float3(.2126,.7152,.0722));}
float3 fresh(int2 p){return Neural.Load(int3(p,0)).rgb-Proxy.Load(int3(p,0)).rgb;}
[numthreads(8,8,1)]
void main(uint3 id:SV_DispatchThreadID){
 if(any(id.xy>=Size))return;
 int2 p=int2(id.xy);
 float3 input=Proxy.Load(int3(p,0)).rgb;
 float3 delta=fresh(p);
 float z=Depth.Load(int3(p,0));
 float l=luminance(input);
 float3 result=delta;
 float2 mv=Motion.Load(int3(p,0));
 if(Valid && all(isfinite(mv)) && all(isfinite(delta)) && isfinite(z) && z>=0 && z<=1 && isfinite(l)) {
  float2 previous=float2(p)+mv*float2(Size);
  if(all(previous>=0) && all(previous<=float2(Size)-1)) {
   int2 base=int2(floor(previous));float2 f=frac(previous);
   float3 old=0;float confidence=0;
   [unroll]for(int y=0;y<2;++y)[unroll]for(int x=0;x<2;++x){
    int2 q=base+int2(x,y);float w=(x?f.x:1-f.x)*(y?f.y:1-f.y);
    if(w>0 && all(q>=0) && all(q<int2(Size))){
     float4 h=Previous.Load(int3(q,0));float hz=PreviousDepth.Load(int3(q,0));
     float d=Inverted?z:1-z,hd=Inverted?hz:1-hz;
     float depthError=abs(d-hd)/max(max(abs(d),abs(hd)),.00001);
     float colorError=abs(l-h.a)/max(max(abs(l),abs(h.a)),.02);
     if(all(isfinite(h)) && isfinite(hz) && hz>=0 && hz<=1){
      float c=(1-smoothstep(.01,.04,depthError))*(1-smoothstep(.05,.25,colorError));
      old+=h.rgb*(w*c);confidence+=w*c;
     }
    }
   }
   if(confidence>.001){
    // Clamp to current NR corrections, never replace rejected pixels with raw game color.
    float3 lo=delta,hi=delta;
    [unroll]for(int y=-1;y<=1;++y)[unroll]for(int x=-1;x<=1;++x){
     float3 v=fresh(clamp(p+int2(x,y),int2(0,0),int2(Size)-1));
     if(all(isfinite(v))){lo=min(lo,v);hi=max(hi,v);}
    }
    float3 prior=clamp(old/confidence,lo,hi);
    result=lerp(delta,prior,saturate(Strength)*.8*confidence);
   }
  }
 }
 // Nonfinite current samples are never promoted as reusable history.
 Correction[id.xy]=float4(result,all(isfinite(delta))&&isfinite(l)?l:asfloat(0x7fc00000));
 SavedDepth[id.xy]=z;
}
)";
}
