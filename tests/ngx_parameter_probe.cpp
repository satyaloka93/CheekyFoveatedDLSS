#include "../src/ngx_parameter_overlay.hpp"
#include <cstdio>
using namespace cheeky::foveated_dlss;
int wmain(int argc, wchar_t** argv) {
    if(argc!=2) return 2;
    HMODULE dll=LoadLibraryW(argv[1]);
    if(!dll){printf("Load failed %lu\n",GetLastError());return 3;}
    auto alloc=reinterpret_cast<NgxResult(*)(NgxParameters**)>(GetProcAddress(dll,"NVSDK_NGX_D3D12_AllocateParameters"));
    auto destroy=reinterpret_cast<NgxResult(*)(NgxParameters*)>(GetProcAddress(dll,"NVSDK_NGX_D3D12_DestroyParameters"));
    if(!alloc || !destroy){puts("Missing allocator");return 4;}
    NgxParameters* real{};auto result=alloc(&real);
    printf("Allocate=%08x\n",result);if(!ngx_succeeded(result)||!real)return 5;
    NgxParameterOverlay overlay;
    for(auto* p:{real,static_cast<NgxParameters*>(&overlay)}) {
        p->Set("Color",reinterpret_cast<ID3D12Resource*>(uintptr_t(0x12340)));
        void* generic{};ID3D11Resource* d11{};ID3D12Resource* d12{};
        const auto a=p->Get("Color",&generic),b=p->Get("Color",&d11),c=p->Get("Color",&d12);
        printf("%s d12-set: void=%08x/%p d11=%08x/%p d12=%08x/%p\n",p==real?"NVIDIA":"overlay",a,generic,b,d11,c,d12);
        if(!ngx_succeeded(a)||!ngx_succeeded(c)||generic!=d12||generic!=reinterpret_cast<void*>(uintptr_t(0x12340)))return 6;
        p->Set("Color",reinterpret_cast<void*>(uintptr_t(0x23450)));
        d11=nullptr;d12=nullptr;generic=nullptr;
        const auto d=p->Get("Color",&generic),e=p->Get("Color",&d11),f=p->Get("Color",&d12);
        printf("%s void-set: void=%08x/%p d11=%08x/%p d12=%08x/%p\n",p==real?"NVIDIA":"overlay",d,generic,e,d11,f,d12);
        if(!ngx_succeeded(d)||!ngx_succeeded(f)||generic!=d12||generic!=reinterpret_cast<void*>(uintptr_t(0x23450)))return 7;
        p->Set("Color",reinterpret_cast<ID3D11Resource*>(uintptr_t(0x34560)));
        d11=nullptr;d12=nullptr;generic=nullptr;
        const auto g=p->Get("Color",&generic),h=p->Get("Color",&d11),i=p->Get("Color",&d12);
        printf("%s d11-set: void=%08x/%p d11=%08x/%p d12=%08x/%p\n",p==real?"NVIDIA":"overlay",g,generic,h,d11,i,d12);
    }
    destroy(real);FreeLibrary(dll);return 0;
}
