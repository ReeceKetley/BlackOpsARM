#include "gfx_vulkan/constant_state.h"
#include <cstdio>
#include <thread>
#include <atomic>
#include <cstdlib>
// Only backend selection is replaced: all constant storage/ownership code is
// the production implementation. Opaque command owners are never dereferenced.
namespace bo1::vulkan::shader_assets { bool Active(){return true;} }
using namespace bo1::vulkan;
static unsigned checks;
static void check(bool ok){++checks;if(!ok){std::fprintf(stderr,"constant check %u failed\n",checks);std::abort();}}
int main(){
    alignas(16) char owners[2]{};
    auto a=reinterpret_cast<GfxCmdBufState*>(&owners[0]);
    auto b=reinterpret_cast<GfxCmdBufState*>(&owners[1]);
    float first[4]{1,2,3,4},second[4]{5,6,7,8};std::string error;
    check(constant_state::Set(a,ShaderStage::Vertex,255,1,first,error));
    auto captured=constant_state::Acquire(a);check(captured&&captured->floats[0][255][0]==1);
    check(constant_state::Set(a,ShaderStage::Vertex,255,1,second,error));
    auto newer=constant_state::Acquire(a);check(newer!=captured&&captured->floats[0][255][0]==1&&newer->floats[0][255][0]==5);
    constant_state::Clone(a,b);newer.reset();
    check(constant_state::Set(a,ShaderStage::Fragment,1,1,first,error));
    auto cloned=constant_state::Acquire(b);check(cloned->floats[1][1][0]==0);
    auto privateOwner=constant_state::Acquire(a);auto address=privateOwner.get();privateOwner.reset();
    check(constant_state::Set(a,ShaderStage::Fragment,1,1,second,error));
    auto updated=constant_state::Acquire(a);check(updated.get()==address&&updated->floats[1][1][0]==5);
    const auto revision=updated->revision;
    check(constant_state::Set(a,ShaderStage::Fragment,1,1,second,error));
    check(constant_state::Acquire(a)==updated&&updated->revision==revision);
    check(!constant_state::Set(a,ShaderStage::Fragment,255,2,second,error));
    check(!constant_state::Set(a,ShaderStage::Fragment,256,1,second,error));
    check(!constant_state::Set(nullptr,ShaderStage::Vertex,0,1,first,error));
    check(!constant_state::Set(a,ShaderStage::Vertex,0,0,first,error));
    check(!constant_state::Set(a,ShaderStage::Vertex,0,1,nullptr,error));
    check(constant_state::Acquire(a)==updated);
    updated.reset();std::atomic<bool> done=false,valid=true;
    std::thread reader([&]{while(!done.load()){
        auto held=constant_state::Acquire(a);const auto value=held->floats[1][1];
        for(unsigned spin=0;spin<20;++spin)std::this_thread::yield();
        if(held->floats[1][1]!=value)valid=false;
    }});
    for(unsigned i=0;i<10000;++i){float values[4]{float(i),float(i+1),float(i+2),float(i+3)};check(constant_state::Set(a,ShaderStage::Fragment,1,1,values,error));}
    done=true;reader.join();check(valid);
    constant_state::Release(a);constant_state::Release(b);
    check(!constant_state::Acquire(a)&&!constant_state::Acquire(b));
    check(captured->floats[0][255][0]==1&&cloned->floats[0][255][0]==5);
    std::printf("native_constant_ownership=pass checks=%u capture_clone_private_reuse_concurrent_readers=pass cpu_fixture_only=1\n",checks);
}
