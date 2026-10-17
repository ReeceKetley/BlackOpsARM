#include "material_bindings.h"
#include "resource_scratch.h"
#include "constant_upload.h"
#include <algorithm>
#include <limits>
namespace bo1::vulkan::material_bindings {
bool UploadScratch(const Draw& draw,DrawStorage& storage,VkDeviceSize alignment,
    ResourceScratch& scratch,VkDescriptorPool& pool,std::string& error,const unsigned* constantRows,BindCache* cache) {
    if(!draw.constants||!draw.selection||!draw.selection->vertex||!draw.selection->pixel||!alignment
        ||alignment>std::numeric_limits<VkDeviceSize>::max()-4095){error="Invalid material draw-page capture/alignment";return false;}
    auto& lists=scratch.stages;lists[0].clear();lists[1].clear();scratch.added.clear();bool needed[2]{};
    for(unsigned stage=0;stage<2;++stage) {
        const auto& shader=stage?draw.selection->pixel:draw.selection->vertex;
        for(const auto& binding:shader->translation.bindings)if(binding.kind==ResourceKind::Constants) {
            if(binding.registerIndex!=0){error="Native integer/boolean constant banks not implemented";return false;}
            needed[stage]=true;
        }
        const bool sampled=std::any_of(shader->translation.bindings.begin(),shader->translation.bindings.end(),[](const auto& binding){return binding.kind!=ResourceKind::Constants;});
        if(sampled&&(!draw.textures||!texture_bindings::AppendResourcesWithScratch(*draw.textures,shader->translation,lists[stage],scratch.added,error)))return false;
    }
    // Only the rows each translated shader can read (all 256 when it indexes
    // constants dynamically) are copied and exposed to the GPU.
    VkDeviceSize size[2]{};
    for(unsigned stage=0;stage<2;++stage)if(needed[stage]) {
        const unsigned rows=constantRows?constantRows[stage]:256;
        if(!rows||rows>256){error="Invalid native constant row count";return false;}
        size[stage]=VkDeviceSize(rows)*16;
    }
    const void* data[2]={draw.constants->floats[0].data(),draw.constants->floats[1].data()};
    VkDescriptorBufferInfo ranges[2]{};
    if(!UploadConstantRanges(storage,alignment,data,size,cache,ranges,pool,error))return false;
    for(unsigned stage=0;stage<2;++stage)if(needed[stage])
        lists[stage].push_back({ResourceKind::Constants,0,ranges[stage],{}});
    return true;
}
bool Upload(const Draw& draw,DrawStorage& storage,VkDeviceSize alignment,
    std::vector<ProgramResource>& vs,std::vector<ProgramResource>& ps,VkDescriptorPool& pool,std::string& error,const unsigned* constantRows,BindCache* cache) {
    ResourceScratch scratch;
    if(!UploadScratch(draw,storage,alignment,scratch,pool,error,constantRows,cache))return false;
    vs=std::move(scratch.stages[0]);ps=std::move(scratch.stages[1]);return true;
}
}
