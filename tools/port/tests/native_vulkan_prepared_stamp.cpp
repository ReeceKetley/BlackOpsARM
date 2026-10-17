#include "gfx_vulkan/bind_cache.h"
#include "gfx_vulkan/geometry_packet.h"
#include <cstdio>
#include <cstdlib>
using namespace bo1::vulkan;
static unsigned checks;
static void Check(bool value){++checks;if(!value)std::abort();}
int main(){
    PreparedProgramStamp base;int owner;
    Check(base==base);
    BindCache::LastDraw last;last.valid=true;last.state=&owner;last.pass=base.pass;last.programStamp=base;
    auto proof=[&](const PreparedProgramStamp& stamp,bool enabled=true,bool selection=true,bool constants=true,bool textures=true,bool offsets=true){
        return geometry_packet::Proven(enabled,last,&owner,base.pass,stamp,selection,constants,textures,offsets);
    };
    Check(proof(base));Check(!proof(base,false));Check(!proof(base,true,false));Check(!proof(base,true,true,false));
    Check(!proof(base,true,true,true,false));Check(!proof(base,true,true,true,true,false));
    last.valid=false;Check(!proof(base));last.valid=true;last.state=nullptr;Check(!proof(base));last.state=&owner;
    last.pass=reinterpret_cast<VkRenderPass>(1);Check(!proof(base));last.pass=base.pass;
    auto changed=[&](auto mutate){auto next=base;mutate(next);Check(!(next==base));Check(!proof(next));};
    changed([&](auto& s){s.cache=&owner;});changed([](auto& s){s.physical=reinterpret_cast<VkPhysicalDevice>(1);});
    changed([](auto& s){s.pass=reinterpret_cast<VkRenderPass>(1);});changed([](auto& s){s.pipelinePass=reinterpret_cast<VkRenderPass>(1);});
    changed([](auto& s){s.subpass=1;});changed([](auto& s){s.samples=VK_SAMPLE_COUNT_2_BIT;});
    changed([](auto& s){s.color=true;});changed([](auto& s){s.depth=true;});changed([](auto& s){s.stencil=true;});
    changed([](auto& s){s.relaxed=true;});changed([](auto& s){s.depthFormat=VK_FORMAT_D16_UNORM;});
    changed([&](auto& s){s.vertex=&owner;});changed([&](auto& s){s.pixel=&owner;});changed([&](auto& s){s.declaration=&owner;});
    changed([](auto& s){s.variant=1;});changed([](auto& s){s.words[0]=1;});changed([](auto& s){s.words[1]=1;});
    changed([](auto& s){s.bias=1;});changed([](auto& s){s.slope=1;});
    changed([](auto& s){s.streamMask=1;});changed([](auto& s){s.depthSamplerMask=1;});
    for(unsigned i=0;i<3;++i)changed([&](auto& s){s.strides[i]=16;});
    geometry_bindings::Snapshot a,b;
    auto vertex=std::make_shared<buffer_assets::Record>();
    auto index=std::make_shared<buffer_assets::Record>();
    // Non-owning identity tokens suffice here: this host test never calls GPU APIs.
    vertex->gpu=std::shared_ptr<Buffer>(reinterpret_cast<Buffer*>(1),[](Buffer*){});
    index->gpu=std::shared_ptr<Buffer>(reinterpret_cast<Buffer*>(2),[](Buffer*){});
    a.indices=index;a.streams[0]={vertex,0,44};a.streams[1]={vertex,0,20};b=a;
    Check(!geometry_packet::OffsetOnly(a,b));b.streams[0].offset=44;b.streams[1].offset=20;
    Check(geometry_packet::OffsetOnly(a,b));
    auto change=[&](auto mutate){auto next=b;mutate(next);Check(!geometry_packet::OffsetOnly(a,next));};
    change([](auto& v){v.indices.reset();});change([](auto& v){v.streams[0].buffer.reset();});
    change([](auto& v){v.streams[1].buffer.reset();});change([](auto& v){v.streams[1].stride=24;});
    auto different=std::make_shared<buffer_assets::Record>();
    different->gpu=std::shared_ptr<Buffer>(reinterpret_cast<Buffer*>(3),[](Buffer*){});
    change([&](auto& v){v.indices=different;});change([&](auto& v){v.streams[0].buffer=different;});
    change([&](auto& v){v.streams[1].buffer=different;});
    std::printf("native_prepared_stamp=PASS checks=%u all_pipeline_dependencies=1 bias_slope_regression=1\n",checks);
}
