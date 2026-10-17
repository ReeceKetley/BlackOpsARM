#include "graphics_program.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#if defined(__ANDROID__)
#include <android/log.h>
#endif

namespace bo1::vulkan {
namespace {
std::atomic<bool> compatibleDescriptorReuse{true};
std::atomic<bool> depthOnlyPipelines{false};
// True when the fragment SPIR-V can change depth results: it discards
// (kill/terminate/demote) or writes the FragDepth built-in.
bool FragmentAffectsDepth(const std::vector<std::uint32_t>& spirv) {
    if(spirv.size()<5)return true;
    for(std::size_t i=5;i<spirv.size();) {
        const unsigned count=spirv[i]>>16,opcode=spirv[i]&0xffffu;
        if(!count||i+count>spirv.size())return true;
        if(opcode==252||opcode==4416||opcode==5380)return true; // OpKill, OpTerminateInvocation, OpDemoteToHelperInvocation
        if(opcode==71&&count>=4&&spirv[i+2]==11&&spirv[i+3]==22)return true; // OpDecorate BuiltIn FragDepth
        i+=count;
    }
    return false;
}
std::atomic<bool> descriptorContentReuse{true};
std::atomic<bool> lazyDescriptorWrites{false};
thread_local DescriptorWriteStats descriptorWriteStats;
BindCache::DescriptorContents Contents(const ProgramResource& resource) {
    if(resource.kind==ResourceKind::Constants)return {resource.buffer.buffer,resource.buffer.range};
    return {VK_NULL_HANDLE,0,resource.image.imageView,resource.image.sampler,resource.image.imageLayout};
}
VkDescriptorType Type(ResourceKind kind) {
    // Constants are dynamic: one descriptor serves every draw's offset.
    return kind==ResourceKind::Constants?VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
        kind==ResourceKind::Texture?VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:VK_DESCRIPTOR_TYPE_SAMPLER;
}
bool Result(VkResult value,const char* action,std::string& error) {
    if(value==VK_SUCCESS)return true;
    error=std::string(action)+": VkResult="+std::to_string(int(value));return false;
}
}
void SetLazyDescriptorWrites(bool value){lazyDescriptorWrites.store(value,std::memory_order_relaxed);}
bool LazyDescriptorWrites(){return lazyDescriptorWrites.load(std::memory_order_relaxed);}
DescriptorWriteStats ReadDescriptorWriteStats(){return descriptorWriteStats;}
void SetDepthOnlyPipelines(bool enabled){depthOnlyPipelines.store(enabled,std::memory_order_relaxed);}
bool DepthOnlyPipelines(){return depthOnlyPipelines.load(std::memory_order_relaxed);}
void SetCompatibleDescriptorReuse(bool enabled){compatibleDescriptorReuse.store(enabled,std::memory_order_relaxed);}
bool CompatibleDescriptorReuse(){return compatibleDescriptorReuse.load(std::memory_order_relaxed);}
void SetDescriptorContentReuse(bool enabled){descriptorContentReuse.store(enabled,std::memory_order_relaxed);}
bool DescriptorContentReuse(){return descriptorContentReuse.load(std::memory_order_relaxed);}
GraphicsProgram::~GraphicsProgram() {
    if(pipeline_)vkDestroyPipeline(device_,pipeline_,nullptr);
    if(layout_)vkDestroyPipelineLayout(device_,layout_,nullptr);
    for(auto l:setLayouts_)if(l)vkDestroyDescriptorSetLayout(device_,l,nullptr);
}
bool GraphicsProgram::Create(const ProgramDescription& d,VkPipelineCache cache,std::string& error,const std::array<ShaderTranslation,2>* cached) {
    error.clear();
    if(!device_||!d.renderPass||pipeline_||layout_||setLayouts_[0]||setLayouts_[1]) {
        error="Invalid device/pass or program already initialized";return false;
    }
    const auto createStart=std::chrono::steady_clock::now();
    if(!ValidateDepthBias(d.depthBias,error))return false;
    if(cached)shaders_=*cached;
    else if(!TranslateShaderPair(d.vertexBytecode,d.vertexBytes,d.pixelBytecode,d.pixelBytes,shaders_[0],shaders_[1],&d.alphaTest,d.pixelDepthSamplerMask,d.vertexUbyteMask)) {
        error=shaders_[0].diagnostics+shaders_[1].diagnostics;if(error.empty())error="Native shader pair translation failed";return false;
    }
    const auto translated=std::chrono::steady_clock::now();
    for(unsigned stage=0;stage<2;++stage) {
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        for(const auto& b:shaders_[stage].bindings) {
            bindings.push_back({b.binding,Type(b.kind),1,stage?VK_SHADER_STAGE_FRAGMENT_BIT:VK_SHADER_STAGE_VERTEX_BIT,nullptr});
            // The SM3 translator fixes type by binding: CBV 0..2, SRV
            // 16..31, sampler 32..47. With count=1, flags=0 and fixed stage
            // visibility this bitset exactly describes each stage layout.
            if(!((b.kind==ResourceKind::Constants&&b.binding<=2)
                ||(b.kind==ResourceKind::Texture&&b.binding>=16&&b.binding<32)
                ||(b.kind==ResourceKind::Sampler&&b.binding>=32&&b.binding<48))) {
                error="Unexpected native descriptor layout mapping";return false;
            }
            layoutBindings_[stage]|=std::uint64_t(1)<<b.binding;
            if(b.kind==ResourceKind::Constants){++dynamicCount_;++stageDynamic_[stage];}
            if(b.kind==ResourceKind::Constants&&b.registerIndex==0)
                floatConstantRows_[stage]=shaders_[stage].constantRows;
        }
        std::sort(bindings.begin(),bindings.end(),[](const auto& a,const auto& b){return a.binding<b.binding;});
        VkDescriptorSetLayoutCreateInfo info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        info.bindingCount=unsigned(bindings.size());info.pBindings=bindings.data();
        if(!Result(vkCreateDescriptorSetLayout(device_,&info,nullptr,&setLayouts_[stage]),"descriptor layout",error))return false;
    }
    VkPipelineLayoutCreateInfo layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layout.setLayoutCount=2;layout.pSetLayouts=setLayouts_.data();
    if(!Result(vkCreatePipelineLayout(device_,&layout,nullptr,&layout_),"pipeline layout",error))return false;
    VkShaderModule modules[2]{};
    for(unsigned stage=0;stage<2;++stage) {
        VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        info.codeSize=shaders_[stage].spirv.size()*4;info.pCode=shaders_[stage].spirv.data();
        if(!Result(vkCreateShaderModule(device_,&info,nullptr,&modules[stage]),"shader module",error)) {
            if(modules[0])vkDestroyShaderModule(device_,modules[0],nullptr);return false;
        }
    }
    VkPipelineShaderStageCreateInfo stages[2]={{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO},{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO}};
    stages[0].stage=VK_SHADER_STAGE_VERTEX_BIT;stages[0].module=modules[0];stages[0].pName="main";
    stages[1].stage=VK_SHADER_STAGE_FRAGMENT_BIT;stages[1].module=modules[1];stages[1].pName="main";
    VkPipelineVertexInputStateCreateInfo input{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    input.vertexBindingDescriptionCount=d.vertexBindingCount;input.pVertexBindingDescriptions=d.vertexBindings;
    input.vertexAttributeDescriptionCount=d.vertexAttributeCount;input.pVertexAttributeDescriptions=d.vertexAttributes;
    VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};assembly.topology=d.topology;
    VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};viewport.viewportCount=viewport.scissorCount=1;
    VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    raster.polygonMode=VK_POLYGON_MODE_FILL;raster.lineWidth=1;raster.cullMode=d.cull;raster.frontFace=d.frontFace;
    raster.depthBiasEnable=d.depthBias.enabled;
    raster.depthBiasConstantFactor=d.depthBias.constantFactor;raster.depthBiasSlopeFactor=d.depthBias.slopeFactor;
    raster.depthBiasClamp=0; // No optional clamp feature required.
    VkPipelineMultisampleStateCreateInfo samples{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};samples.rasterizationSamples=d.samples;
    VkPipelineDepthStencilStateCreateInfo depth{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    depth.depthTestEnable=d.depthTest;depth.depthWriteEnable=d.depthWrite;depth.depthCompareOp=d.depthCompare;
    depth.stencilTestEnable=d.stencilTest;depth.front=d.frontStencil;depth.back=d.backStencil;
    VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};blend.attachmentCount=d.colorAttachment?1:0;blend.pAttachments=d.colorAttachment?&d.color:nullptr;
    const VkDynamicState states[]={VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};dynamic.dynamicStateCount=2;dynamic.pDynamicStates=states;
    VkGraphicsPipelineCreateInfo info{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    const bool depthOnly=d.depthOnly&&!d.colorAttachment&&!FragmentAffectsDepth(shaders_[1].spirv);
    info.stageCount=depthOnly?1:2;info.pStages=stages;info.pVertexInputState=&input;info.pInputAssemblyState=&assembly;
    info.pViewportState=&viewport;info.pRasterizationState=&raster;info.pMultisampleState=&samples;info.pDepthStencilState=&depth;
    info.pColorBlendState=&blend;info.pDynamicState=&dynamic;info.layout=layout_;info.renderPass=d.renderPass;info.subpass=d.subpass;
    const auto driverStart=std::chrono::steady_clock::now();
    const VkResult status=vkCreateGraphicsPipelines(device_,cache,1,&info,nullptr,&pipeline_);
#if defined(__ANDROID__)
    const auto driverEnd=std::chrono::steady_clock::now();
    const auto ms=[](auto a,auto b){return std::chrono::duration<double,std::milli>(b-a).count();};
    __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan",
        "native_pipeline_compile translate_ms=%.3f layout_module_ms=%.3f driver_ms=%.3f total_ms=%.3f result=%d depth_only=%d",
        ms(createStart,translated),ms(translated,driverStart),ms(driverStart,driverEnd),ms(createStart,driverEnd),int(status),depthOnly?1:0);
#endif
    for(auto module:modules)vkDestroyShaderModule(device_,module,nullptr);
    // The pipeline owns the compiled code; only the reflection is used now.
    for(auto& shader:shaders_)std::vector<std::uint32_t>().swap(shader.spirv);
    return Result(status,"graphics pipeline",error);
}
bool GraphicsProgram::BindResources(VkDescriptorPool pool,const std::vector<ProgramResource>& vertex,
    const std::vector<ProgramResource>& pixel,std::array<VkDescriptorSet,2>& sets,std::string& error) const {
    sets={};error.clear();if(!pipeline_||!pool) { error="Program or frame descriptor pool unavailable";return false; }
    std::vector<VkWriteDescriptorSet> writes;
    std::vector<unsigned> stages;
    for(unsigned stage=0;stage<2;++stage)for(const auto& binding:shaders_[stage].bindings) {
        const auto& resources=stage?pixel:vertex;
        const ProgramResource* selected{};
        for(const auto& r:resources)if(r.kind==binding.kind&&r.registerIndex==binding.registerIndex) {
            if(selected) { error="Duplicate material resource";return false; }selected=&r;
        }
        if(!selected) { error="Missing material resource at stage"+std::to_string(stage)+" binding"+std::to_string(binding.binding);return false; }
        if((binding.kind==ResourceKind::Constants&&(!selected->buffer.buffer||!selected->buffer.range))
            ||(binding.kind==ResourceKind::Texture&&(!selected->image.imageView
                ||(selected->image.imageLayout!=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL&&selected->image.imageLayout!=VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL&&selected->image.imageLayout!=VK_IMAGE_LAYOUT_GENERAL)))
            ||(binding.kind==ResourceKind::Sampler&&!selected->image.sampler)) { error="Invalid material resource";return false; }
        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};write.dstBinding=binding.binding;
        write.descriptorCount=1;write.descriptorType=Type(binding.kind);
        if(binding.kind==ResourceKind::Constants)write.pBufferInfo=&selected->buffer;else write.pImageInfo=&selected->image;
        writes.push_back(write);stages.push_back(stage);
    }
    VkDescriptorSetAllocateInfo allocation{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocation.descriptorPool=pool;allocation.descriptorSetCount=2;allocation.pSetLayouts=setLayouts_.data();
    if(!Result(vkAllocateDescriptorSets(device_,&allocation,sets.data()),"frame descriptors",error)) { sets={};return false; }
    for(unsigned i=0;i<writes.size();++i)writes[i].dstSet=sets[stages[i]];
    vkUpdateDescriptorSets(device_,unsigned(writes.size()),writes.data(),0,nullptr);return true;
}
void GraphicsProgram::Bind(VkCommandBuffer command,const std::array<VkDescriptorSet,2>& sets) const {
    // BindResources writes absolute descriptor offsets; dynamic offsets are 0.
    const std::uint32_t zeros[2]{};
    vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline_);
    vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,layout_,0,2,sets.data(),dynamicCount_,zeros);
}
bool GraphicsProgram::Record(VkCommandBuffer command,VkDescriptorPool pool,const std::vector<ProgramResource>& vertex,
    const std::vector<ProgramResource>& pixel,BindCache& cache,std::string& error) const {
    error.clear();if(!pipeline_||!pool||dynamicCount_>2) { error="Program or frame descriptor pool unavailable";return false; }
    FlushPending(cache); // pipeline/set binds below change the pending draw's state
    std::array<VkDescriptorSet,2> sets{};std::array<std::uint32_t,2> offsets{};unsigned offsetCount=0;
    thread_local std::vector<VkWriteDescriptorSet> writes;thread_local std::vector<VkDescriptorBufferInfo> buffers;
    thread_local std::vector<VkDescriptorImageInfo> images;thread_local DescriptorSetKey key;
    thread_local std::vector<const ProgramResource*> selectedResources;
    const bool reuseContents=DescriptorContentReuse();
    const bool lazyWrites=LazyDescriptorWrites();
    for(unsigned stage=0;stage<2;++stage) {
        const auto& resources=stage?pixel:vertex;
        auto& recent=cache.recentDescriptorStages[stage];
        bool same=reuseContents&&recent.identity==descriptorIdentity_&&recent.set
            &&recent.contents.size()==shaders_[stage].bindings.size();
        selectedResources.clear();selectedResources.reserve(shaders_[stage].bindings.size());
        // Index this stage's resources by (kind, register) once instead of
        // scanning them for every binding; duplicates are still rejected when a
        // binding selects them, as before.
        const ProgramResource* table[3][64]{};unsigned char duplicate[3][64]{};bool overflow=false;
        for(const auto& r:resources) {
            const unsigned kind=unsigned(r.kind);
            if(kind>=3||r.registerIndex>=64){overflow=true;continue;}
            if(table[kind][r.registerIndex])duplicate[kind][r.registerIndex]=1;else table[kind][r.registerIndex]=&r;
        }
        // Validation remains mandatory even on a contents hit. In particular,
        // changed dynamic offsets, duplicate slots and invalid layouts cannot
        // hide behind a previously valid descriptor set.
        for(const auto& binding:shaders_[stage].bindings) {
            const ProgramResource* selected{};
            const unsigned kind=unsigned(binding.kind);
            if(kind<3&&binding.registerIndex<64&&!overflow) {
                if(duplicate[kind][binding.registerIndex]) { error="Duplicate material resource";return false; }
                selected=table[kind][binding.registerIndex];
            } else {
                for(const auto& r:resources)if(r.kind==binding.kind&&r.registerIndex==binding.registerIndex) {
                    if(selected) { error="Duplicate material resource";return false; }selected=&r;
                }
            }
            if(!selected) { error="Missing material resource at stage"+std::to_string(stage)+" binding"+std::to_string(binding.binding);return false; }
            if((binding.kind==ResourceKind::Constants&&(!selected->buffer.buffer||!selected->buffer.range||selected->buffer.offset>0xffffffffull))
                ||(binding.kind==ResourceKind::Texture&&(!selected->image.imageView
                    ||(selected->image.imageLayout!=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL&&selected->image.imageLayout!=VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL&&selected->image.imageLayout!=VK_IMAGE_LAYOUT_GENERAL)))
                ||(binding.kind==ResourceKind::Sampler&&!selected->image.sampler)) { error="Invalid material resource";return false; }
            if(same&&!(recent.contents[selectedResources.size()]==Contents(*selected)))same=false;
            selectedResources.push_back(selected);
            if(binding.kind==ResourceKind::Constants)offsets[offsetCount++]=std::uint32_t(selected->buffer.offset);
        }
        if(same){sets[stage]=recent.set;++cache.setHits;++cache.descriptorContentHits;continue;}
        if(reuseContents)++cache.descriptorContentMisses;
        auto buildWrites=[&] {
            writes.clear();buffers.clear();images.clear();
            buffers.reserve(shaders_[stage].bindings.size());images.reserve(shaders_[stage].bindings.size());
            unsigned selectedIndex=0;
            for(const auto& binding:shaders_[stage].bindings) {
                const auto* selected=selectedResources[selectedIndex++];
                VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};write.dstBinding=binding.binding;
                write.descriptorCount=1;write.descriptorType=Type(binding.kind);
                if(binding.kind==ResourceKind::Constants)buffers.push_back({selected->buffer.buffer,0,selected->buffer.range});
                else images.push_back(selected->image);
                writes.push_back(write);
            }
            ++descriptorWriteStats.writeBuilds;
        };
        if(!lazyWrites)buildWrites();
        key.layout=cache.compatibleSets?VK_NULL_HANDLE:setLayouts_[stage];key.words.clear();
        // Layout flags/counts/stage visibility and these binding/type words
        // fully describe the existing structural descriptor-cache identity.
        key.words.push_back(stage?VK_SHADER_STAGE_FRAGMENT_BIT:VK_SHADER_STAGE_VERTEX_BIT);
        unsigned selectedIndex=0;
        for(const auto& binding:shaders_[stage].bindings) {
            const auto* selected=selectedResources[selectedIndex++];
            key.words.push_back((std::uint64_t(binding.binding)<<8)|unsigned(binding.kind));
            if(binding.kind==ResourceKind::Constants) {
                key.words.push_back(std::uint64_t(reinterpret_cast<std::uintptr_t>(selected->buffer.buffer)));key.words.push_back(selected->buffer.range);
            } else {
                key.words.push_back(std::uint64_t(reinterpret_cast<std::uintptr_t>(selected->image.imageView)));
                key.words.push_back(std::uint64_t(reinterpret_cast<std::uintptr_t>(selected->image.sampler)));
                key.words.push_back(unsigned(selected->image.imageLayout));
            }
        }
        auto found=cache.written.find(key);
        if(found!=cache.written.end()) {sets[stage]=found->second;++cache.setHits;if(lazyWrites)++descriptorWriteStats.lazySkips;}
        else {
            if(lazyWrites)buildWrites();
            VkDescriptorSetAllocateInfo allocation{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
            allocation.descriptorPool=pool;allocation.descriptorSetCount=1;allocation.pSetLayouts=&setLayouts_[stage];
            if(!Result(vkAllocateDescriptorSets(device_,&allocation,&sets[stage]),"frame descriptors",error))return false;
            // Point each write at stable storage only after both vectors are final.
            std::size_t b=0,i=0;
            for(auto& write:writes) {
                write.dstSet=sets[stage];
                if(write.descriptorType==VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)write.pBufferInfo=&buffers[b++];else write.pImageInfo=&images[i++];
            }
            if(!writes.empty())vkUpdateDescriptorSets(device_,unsigned(writes.size()),writes.data(),0,nullptr);
            cache.written.emplace(key,sets[stage]);++cache.setMisses;
        }
        if(reuseContents) {
            recent.identity=descriptorIdentity_;recent.set=sets[stage];recent.contents.clear();
            recent.contents.reserve(selectedResources.size());
            for(const auto* selected:selectedResources)recent.contents.push_back(Contents(*selected));
        }
    }
    if(offsetCount!=dynamicCount_) { error="Dynamic constant offsets do not match program layout";return false; }
    if(cache.pipeline!=pipeline_) {vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline_);cache.pipeline=pipeline_;++cache.pipelineBinds;}
    else ++cache.pipelineSkips;
    const bool singleBank=stageDynamic_[0]<=1&&stageDynamic_[1]<=1;
    if(!singleBank)cache.layoutBindings={~std::uint64_t(0),~std::uint64_t(0)};
    if(cache.compatibleSets&&singleBank) {
        std::array<std::uint32_t,2> stageOffsets{};unsigned cursor=0;
        for(unsigned stage=0;stage<2;++stage)if(stageDynamic_[stage])stageOffsets[stage]=offsets[cursor++];
        const bool compatible0=cache.layout&&cache.layoutBindings[0]==layoutBindings_[0];
        const bool compatible1=compatible0&&cache.layoutBindings[1]==layoutBindings_[1];
        const bool bind0=!compatible0||cache.sets[0]!=sets[0]||cache.stageOffsets[0]!=stageOffsets[0];
        const bool bind1=!compatible1||cache.sets[1]!=sets[1]||cache.stageOffsets[1]!=stageOffsets[1];
        if(bind0&&bind1) {
            vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,layout_,0,2,sets.data(),offsetCount,offsets.data());
            ++cache.descriptorBinds;cache.descriptorSetsBound+=2;
        } else if(bind0||bind1) {
            const unsigned stage=bind1?1:0;
            vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,layout_,stage,1,&sets[stage],stageDynamic_[stage],&stageOffsets[stage]);
            ++cache.descriptorBinds;++cache.descriptorSetsBound;
        }
        cache.layoutBindings=layoutBindings_;cache.stageOffsets=stageOffsets;
    } else if(cache.layout!=layout_||cache.sets!=sets||cache.offsetCount!=offsetCount||cache.offsets!=offsets) {
        vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,layout_,0,2,sets.data(),offsetCount,offsets.data());
        ++cache.descriptorBinds;cache.descriptorSetsBound+=2;
    }
    cache.layout=layout_;cache.sets=sets;cache.offsets=offsets;cache.offsetCount=offsetCount;
    return true;
}
}
