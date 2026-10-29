#include "../src/nr_vit_policy.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
using namespace cheeky::foveated_dlss::nr_vit;
int main(){
    unsigned checks=0;
    auto check=[&](bool ok){++checks;if(!ok)throw std::runtime_error("F5 policy check "+std::to_string(checks));};
    for(bool fp8 : {false,true}) {
        check(!reuse_enabled(true,1,1,fp8));
        check(!reuse_enabled(false,2,2,fp8));
        check(reuse_enabled(true,2,2,fp8));
        check(reuse_enabled(true,2,1,fp8)==fp8);
        check(reuse_enabled(true,1,2,fp8)!=fp8);
    }
    check(role("cc_vit_1d_repack_2d_to_1d_fp8")==Role::start);
    check(role("cc_vit_1d_repack_1d_to_2d")==Role::end);
    check(role("cc_vit_1d_attention_fp8")==Role::inner);
    check(role("cc_decoder")==Role::none);
    std::size_t first{},last{};
    std::array valid{Role::none,Role::start,Role::inner,Role::end,Role::none};
    check(complete_run(valid,first,last) && first==1 && last==3);
    check(!complete_run(std::array{Role::start,Role::inner},first,last));
    check(!complete_run(std::array{Role::inner,Role::end},first,last));
    check(!complete_run(std::array{Role::start,Role::none,Role::end},first,last));
    check(!complete_run(std::array{Role::start,Role::inner,Role::end,Role::start,Role::inner,Role::end},first,last));
    struct Params{std::uint64_t src,dst;std::uint32_t rows,cols;} p{16,32,12,20};
    std::uint64_t dst{},bytes{};
    check(output(&p,sizeof p,240*256,true,dst,bytes) && bytes==240*1024 && dst==32);
    check(output(&p,sizeof p,240*512,false,dst,bytes) && bytes==240*2048);
    check(!output(&p,sizeof p,240*256,false,dst,bytes));
    check(!output(&p,sizeof p-1,240*256,true,dst,bytes));
    p.dst=33;check(!output(&p,sizeof p,240*256,true,dst,bytes));
    p.dst=32;p.rows=0;check(!output(&p,sizeof p,0,true,dst,bytes));
    p.rows=65535;p.cols=65535;check(!output(&p,sizeof p,0,true,dst,bytes));
    check(reuse_due(true,false,10,11,5,5));
    check(!reuse_due(true,true,10,11,5,5));
    check(!reuse_due(false,false,10,11,5,5));
    check(!reuse_due(true,false,10,11,6,5));
    check(!reuse_due(true,false,10,13,5,5));
    check(!reuse_due(true,false,11,12,5,5));
    check(!reuse_due(true,false,11,11,5,5));
    for(unsigned eye=0;eye<2;++eye)for(unsigned pass=0;pass<3;++pass)check(reuse_due(true,false,20,21,100+eye*3+pass,100+eye*3+pass));
    RoundClock clock;
    check(clock.next(7,false)==UINT64_MAX);
    check(clock.next(7,true)==0 && clock.next(7,false)==0);
    check(clock.next(8,true)==0 && clock.next(8,false)==0);
    check(clock.next(7,true)==1 && clock.next(7,false)==1);
    check(clock.next(8,true)==1 && clock.next(8,false)==1);
    check(clock.next(7,true)==2 && clock.next(7,true)==3);
    check(clock.next(8,true)==3 && clock.next(8,false)==3);
    RoundClock mono;
    check(mono.next(7,true)==0 && mono.next(7,true)==1 && mono.next(7,false)==1);
    std::cout<<checks<<" F5 policy checks passed\n";
}
