#include "gfx_vulkan/constant_state.h"
#include "gfx_vulkan/bind_cache.h"
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
    updated.reset();
    // Exercise the production LastDraw helper with the real COW state map.
    BindCache cache;const unsigned rows[2]={1,1};int storage;
    auto snapshot=constant_state::Acquire(a);const auto weakAddress=snapshot.get();
    const void* data[2]={snapshot->floats[0].data(),snapshot->floats[1].data()};
    for(unsigned stage=0;stage<2;++stage)cache.constantUploads[stage].Remember(&storage,
        {reinterpret_cast<VkBuffer>(std::uintptr_t(1)),stage*16u,16},data[stage]);
    cache.RememberConstants(snapshot,snapshot->revision,&storage,data,rows,true);
    cache.constantUploads[0].buffer.offset+=16;
    check(!cache.UploadedConstantsMatch(snapshot,snapshot->revision,&storage,data,true)); // identity/revision cannot hide replaced range
    cache.constantUploads[0].buffer.offset-=16;
    check(!cache.last.constants&&cache.UploadedConstantsMatch(snapshot,snapshot->revision,&storage,data,true));
    snapshot.reset();check(constant_state::Set(a,ShaderStage::Vertex,0,1,first,error));
    snapshot=constant_state::Acquire(a);data[0]=snapshot->floats[0].data();data[1]=snapshot->floats[1].data();
    check(snapshot.get()==weakAddress); // LastDraw did not force an 8 KiB clone.
    check(!cache.UploadedConstantsMatch(snapshot,snapshot->revision,&storage,data,true)); // no self-comparison
    cache.constantUploads[0].Remember(&storage,{reinterpret_cast<VkBuffer>(std::uintptr_t(1)),32,16},data[0]);
    check(!cache.UploadedConstantsMatch(snapshot,snapshot->revision,&storage,data,true)); // replaced mirror
    cache.RememberConstants(snapshot,snapshot->revision,&storage,data,rows,true);
    snapshot.reset();check(constant_state::Set(a,ShaderStage::Vertex,255,1,first,error));
    snapshot=constant_state::Acquire(a);data[0]=snapshot->floats[0].data();data[1]=snapshot->floats[1].data();
    check(snapshot.get()==weakAddress&&cache.UploadedConstantsMatch(snapshot,snapshot->revision,&storage,data,true));
    check(!cache.UploadedConstantsMatch(snapshot,snapshot->revision,&owners,data,true));
    check(!cache.UploadedConstantsMatch(snapshot,snapshot->revision,&storage,data,false));
    auto readerSnapshot=snapshot;snapshot.reset();check(constant_state::Set(a,ShaderStage::Vertex,0,1,second,error));
    snapshot=constant_state::Acquire(a);check(snapshot!=readerSnapshot&&readerSnapshot->floats[0][0][0]==1);
    data[0]=snapshot->floats[0].data();data[1]=snapshot->floats[1].data();
    cache.RememberConstants(snapshot,snapshot->revision,&storage,data,rows,false);
    check(cache.last.constants==snapshot&&!cache.last.constantStorage);
    auto strongAddress=snapshot.get();snapshot.reset();check(constant_state::Set(a,ShaderStage::Vertex,0,1,first,error));
    check(constant_state::Acquire(a).get()!=strongAddress&&static_cast<const constant_state::Snapshot*>(cache.last.constants.get())->floats[0][0][0]==5);
    snapshot=constant_state::Acquire(a);data[0]=snapshot->floats[0].data();data[1]=snapshot->floats[1].data();
    cache.RememberConstants(snapshot,snapshot->revision,nullptr,data,rows,true);check(cache.last.constants==snapshot);
    cache.reuseConstants=false;cache.RememberConstants(snapshot,snapshot->revision,&storage,data,rows,true);check(cache.last.constants==snapshot);
    cache={};snapshot.reset();readerSnapshot.reset();
    auto expired=std::make_shared<constant_state::Snapshot>();const unsigned noRows[2]={0,0};
    const void* emptyData[2]={expired->floats[0].data(),expired->floats[1].data()};
    cache.RememberConstants(expired,0,&storage,emptyData,noRows,true);expired.reset();
    check(cache.last.constantIdentity.expired()&&!cache.last.constants);
    auto replacement=std::make_shared<constant_state::Snapshot>();
    check(cache.UploadedConstantsMatch(replacement,0,&storage,emptyData,true)); // no bank reads, no dangling dereference
    cache={};
    std::atomic<bool> done=false,valid=true;
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
    std::printf("native_constant_ownership=pass checks=%u capture_clone_private_reuse_concurrent_readers=pass weak_upload_identity=pass cpu_fixture_only=1\n",checks);
}
