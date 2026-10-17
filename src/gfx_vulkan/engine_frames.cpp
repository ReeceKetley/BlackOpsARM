#include "engine_frames.h"
#include "forward_opaque.h"
#include "image_uploads.h"
#include "target_bindings.h"
#include <cstdlib>
#include "platform/android/native_render_size.h"
#include "platform/android/engine_paths.h"
#include <chrono>
#include <android/log.h>
namespace {
const std::uint32_t SgsrVertex[]={
#include "sgsr_present.vert.spv.inc"
};
const std::uint32_t SgsrFragment[]={
#include "sgsr_present.frag.spv.inc"
};
}
namespace bo1::vulkan {
namespace {
std::uint64_t ProfileNow(){return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();}
double ProfileMs(std::uint64_t start){return double(ProfileNow()-start)/1e6;}
}
EngineFrames::EngineFrames(Presentation& p):renderer_(p),cache_(p.Device(),1024),passCache_(p.Device()),fullscreen_(p.Device()){}
EngineFrames::~EngineFrames() {
    std::string error;if(!Drain(error))std::abort();
    for(auto& slot:slots_) {
        if(slot.timestamps)vkDestroyQueryPool(renderer_.Device(),slot.timestamps,nullptr);
        if(slot.descriptors)vkDestroyDescriptorPool(renderer_.Device(),slot.descriptors,nullptr);
        for(auto& lane:slot.lanes){lane.storage.reset();if(lane.pool)vkDestroyCommandPool(renderer_.Device(),lane.pool,nullptr);}
    }
    const auto device=renderer_.Device();
    if(sgsr_.pipeline)vkDestroyPipeline(device,sgsr_.pipeline,nullptr);
    if(sgsr_.layout)vkDestroyPipelineLayout(device,sgsr_.layout,nullptr);
    if(sgsr_.set)vkDestroyDescriptorSetLayout(device,sgsr_.set,nullptr);
    if(sgsr_.vertex)vkDestroyShaderModule(device,sgsr_.vertex,nullptr);
    if(sgsr_.fragment)vkDestroyShaderModule(device,sgsr_.fragment,nullptr);
}
bool EngineFrames::SgsrPipeline(std::string& error) {
    const auto device=renderer_.Device();const VkRenderPass pass=renderer_.RenderPass();
    if(sgsr_.pipeline&&sgsr_.pass==pass)return true;
    auto check=[&](VkResult result,const char* action){if(result==VK_SUCCESS)return true;error=std::string("Native SGSR ")+action+": "+std::to_string(int(result));return false;};
    if(!sgsr_.vertex) {
        VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};info.codeSize=sizeof(SgsrVertex);info.pCode=SgsrVertex;
        if(!check(vkCreateShaderModule(device,&info,nullptr,&sgsr_.vertex),"vertex module"))return false;
        info.codeSize=sizeof(SgsrFragment);info.pCode=SgsrFragment;
        if(!check(vkCreateShaderModule(device,&info,nullptr,&sgsr_.fragment),"fragment module"))return false;
        const VkDescriptorSetLayoutBinding bindings[]={{0,VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},
            {1,VK_DESCRIPTOR_TYPE_SAMPLER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr}};
        VkDescriptorSetLayoutCreateInfo set{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};set.bindingCount=2;set.pBindings=bindings;
        if(!check(vkCreateDescriptorSetLayout(device,&set,nullptr,&sgsr_.set),"set layout"))return false;
        const VkPushConstantRange range{VK_SHADER_STAGE_FRAGMENT_BIT,0,16};
        VkPipelineLayoutCreateInfo layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};layout.setLayoutCount=1;layout.pSetLayouts=&sgsr_.set;
        layout.pushConstantRangeCount=1;layout.pPushConstantRanges=&range;
        if(!check(vkCreatePipelineLayout(device,&layout,nullptr,&sgsr_.layout),"pipeline layout"))return false;
    }
    if(sgsr_.pipeline){vkDestroyPipeline(device,sgsr_.pipeline,nullptr);sgsr_.pipeline=VK_NULL_HANDLE;}
    VkPipelineShaderStageCreateInfo stages[2]={{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO},{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO}};
    stages[0].stage=VK_SHADER_STAGE_VERTEX_BIT;stages[0].module=sgsr_.vertex;stages[0].pName="main";
    stages[1].stage=VK_SHADER_STAGE_FRAGMENT_BIT;stages[1].module=sgsr_.fragment;stages[1].pName="main";
    // Same vertex layout as the passthrough output program: float4 position, float2 uv.
    const VkVertexInputBindingDescription binding{0,24,VK_VERTEX_INPUT_RATE_VERTEX};
    const VkVertexInputAttributeDescription attributes[]={{0,0,VK_FORMAT_R32G32B32A32_SFLOAT,0},{1,0,VK_FORMAT_R32G32_SFLOAT,16}};
    VkPipelineVertexInputStateCreateInfo input{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    input.vertexBindingDescriptionCount=1;input.pVertexBindingDescriptions=&binding;input.vertexAttributeDescriptionCount=2;input.pVertexAttributeDescriptions=attributes;
    VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};assembly.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};viewport.viewportCount=viewport.scissorCount=1;
    VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    raster.polygonMode=VK_POLYGON_MODE_FILL;raster.cullMode=VK_CULL_MODE_NONE;raster.lineWidth=1;
    VkPipelineMultisampleStateCreateInfo samples{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};samples.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
    VkPipelineDepthStencilStateCreateInfo depth{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    VkPipelineColorBlendAttachmentState attachment{};attachment.colorWriteMask=0xf;
    VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};blend.attachmentCount=1;blend.pAttachments=&attachment;
    const VkDynamicState dynamic[]={VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};dynamicState.dynamicStateCount=2;dynamicState.pDynamicStates=dynamic;
    VkGraphicsPipelineCreateInfo info{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    info.stageCount=2;info.pStages=stages;info.pVertexInputState=&input;info.pInputAssemblyState=&assembly;info.pViewportState=&viewport;
    info.pRasterizationState=&raster;info.pMultisampleState=&samples;info.pDepthStencilState=&depth;info.pColorBlendState=&blend;
    info.pDynamicState=&dynamicState;info.layout=sgsr_.layout;info.renderPass=pass;
    if(!check(vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&info,nullptr,&sgsr_.pipeline),"pipeline"))return false;
    sgsr_.pass=pass;return true;
}
bool EngineFrames::Initialize(std::string& error) {
    VkPhysicalDeviceProperties device{};vkGetPhysicalDeviceProperties(renderer_.Physical(),&device);
    const char* userRoot=Sys_EngineUserRoot();
    if(!cache_.Initialize(error,userRoot&&*userRoot?std::string(userRoot)+"/native-pipeline-cache.bin":std::string(),&device))return false;
    if(!fullscreen_.Initialize(error))return false;
    for(unsigned linear=0;linear<2;++linear) {
        outputSamplers_[linear]=std::make_unique<texture_bindings::Sampler>(renderer_.Device());
        const unsigned decoded=(3u<<20)|(3u<<22)|(3u<<24)|((linear?2u:1u)<<8)|((linear?2u:1u)<<12);
        if(!outputSamplers_[linear]->Create(decoded,1,0,error))return false;
    }
    VkPhysicalDeviceProperties properties{};vkGetPhysicalDeviceProperties(renderer_.Physical(),&properties);
    alignment_=properties.limits.minUniformBufferOffsetAlignment;
    unsigned familyCount=0;vkGetPhysicalDeviceQueueFamilyProperties(renderer_.Physical(),&familyCount,nullptr);
    std::vector<VkQueueFamilyProperties> families(familyCount);vkGetPhysicalDeviceQueueFamilyProperties(renderer_.Physical(),&familyCount,families.data());
    if(renderer_.QueueFamily()<familyCount)timestampBits_=families[renderer_.QueueFamily()].timestampValidBits;
    timestampPeriod_=properties.limits.timestampPeriod;
    if(timestampBits_>64 || timestampPeriod_<=0)timestampBits_=0;
    for(auto& slot:slots_) {
        if(timestampBits_) {
            VkQueryPoolCreateInfo info{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};info.queryType=VK_QUERY_TYPE_TIMESTAMP;info.queryCount=pass_recording::ProfileQueryCount;
            if(vkCreateQueryPool(renderer_.Device(),&info,nullptr,&slot.timestamps)!=VK_SUCCESS) {error="Native scene timestamp query allocation failed";return false;}
        }
        slot.storage=std::make_unique<DrawStorage>(renderer_.Physical(),renderer_.Device());
        slot.uniforms=std::make_unique<Buffer>(renderer_.Physical(),renderer_.Device());
        if(!slot.uniforms->Create(4096,VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT|VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,error))return false;
        const VkDescriptorPoolSize sizes[]={{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,2048},{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,16384},{VK_DESCRIPTOR_TYPE_SAMPLER,16384}};
        VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};pool.maxSets=2048;pool.poolSizeCount=3;pool.pPoolSizes=sizes;
        if(vkCreateDescriptorPool(renderer_.Device(),&pool,nullptr,&slot.descriptors)!=VK_SUCCESS) {error="Native engine descriptor pool allocation failed";return false;}
        for(auto& lane:slot.lanes) {
            VkCommandPoolCreateInfo info{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};info.queueFamilyIndex=renderer_.QueueFamily();
            if(vkCreateCommandPool(renderer_.Device(),&info,nullptr,&lane.pool)!=VK_SUCCESS) {error="Native parallel lane command pool allocation failed";return false;}
            lane.storage=std::make_unique<DrawStorage>(renderer_.Physical(),renderer_.Device());
        }
    }
    return true;
}
bool EngineFrames::RetireLanes(Slot& slot,std::string& error,bool deferWorkers) {
    // Caller retired the slot's submission fence: every secondary, page and
    // packet of these lanes has finished on the GPU.
    for(std::size_t index=0;index<slot.lanes.size();++index) {
        auto& lane=slot.lanes[index];
        if(index&&deferWorkers){lane.retirePending=true;continue;}
        if(!lane.Retire(renderer_.Device(),cache_,error))return false;
    }
    return true;
}
void EngineFrames::Prewarm() {
    const auto generation=shader_assets::Generation();
    // Shader registrations re-trigger only while a map is loading (shortly
    // after the transition's cache clear), never as a gameplay stall.
    const bool loading=profileFrame_-prewarmClearFrame_<900;
    const bool shadersChanged=loading&&generation!=prewarmShaderGeneration_&&profileFrame_-prewarmFrame_>=30;
    if(!prewarmPending_&&!shadersChanged)return;
    prewarmPending_=false;prewarmShaderGeneration_=generation;prewarmFrame_=profileFrame_;
    const auto result=cache_.Prewarm([&](const std::array<unsigned,8>& key)->VkRenderPass {
        std::string error;const auto pass=passCache_.AcquireFromKey(renderer_.Device(),key,error);
        return pass?pass->PipelineHandle():VK_NULL_HANDLE;
    });
    if(result.built||result.failed)__android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan",
        "native_program_prewarm frame=%u recipes=%zu built=%u pending=%u failed=%u ms=%.1f",
        profileFrame_,cache_.Recipes(),result.built,result.pending,result.failed,result.ms);
}
FrameResult EngineFrames::Begin(GfxCmdBufState* state,std::string& error) {
    if(Active()||queries_||events_) {error="Native engine frame already active";return FrameResult::Failed;}
    Prewarm();
    const auto beginStart=ProfileNow();
    const auto uploadStart=ProfileNow();
    const bool timed=profileFrame_<3||(profileFrame_+1)%10==0;
    if(!image_uploads::Flush(error,timed))return FrameResult::Failed;
    uploadMs_=ProfileMs(uploadStart);
    const auto slotStart=ProfileNow();
    auto result=renderer_.BeginWork(context_,error);if(result!=FrameResult::Recording)return result;
    slotWaitMs_=ProfileMs(slotStart);const auto retireStart=ProfileNow();
    auto& slot=slots_[context_.slot];
    if(!RetireProfile(slot,error))return FrameResult::Failed;
    slot.windowProgram.reset();slot.windowImage.reset();
    slot.packets.clear();slot.passes.Retire(cache_);slot.queries.Retire();
    if(!slot.events.Retire(error))return FrameResult::Failed;
    retireMs_=ProfileMs(retireStart);const auto resetStart=ProfileNow();
    if(vkResetDescriptorPool(renderer_.Device(),slot.descriptors,0)!=VK_SUCCESS) {error="Native engine descriptor pool reset failed";return FrameResult::Failed;}
    if(!slot.storage->Reset(error))return FrameResult::Failed;
    if(!RetireLanes(slot,error,true))return FrameResult::Failed;
    for(auto& lane:slot.lanes)if(!lane.storage->Reset(error))return FrameResult::Failed;
    resetMs_=ProfileMs(resetStart);const auto prepareStart=ProfileNow();
    slot.cursor=0;submitError_.clear();
    queries_=std::make_unique<query_assets::RecordingScope>(context_.command,slot.queries);
    if(!queries_->Prepare(error))return FrameResult::Failed;
    prepareMs_=ProfileMs(prepareStart);
    events_=std::make_unique<event_fences::RecordingScope>(slot.events);
    passes_=std::make_unique<pass_recording::RecordingScope>(context_.command,renderer_.Physical(),renderer_.Device(),cache_,*slot.uniforms,slot.descriptors,slot.cursor,alignment_,slot.packets,slot.passes,slot.storage.get(),&passCache_,(profileFrame_<3||(profileFrame_+1)%10==0)?slot.timestamps:VK_NULL_HANDLE);
    passes_->SetFullscreenLibrary(&fullscreen_);
    if(pass_recording::ParallelRecordEnabled())passes_->SetParallelLanes(slot.lanes.data(),unsigned(slot.lanes.size()));
    if(slot.timestamps) {
        vkCmdResetQueryPool(context_.command,slot.timestamps,0,pass_recording::ProfileQueryCount);
        vkCmdWriteTimestamp(context_.command,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,slot.timestamps,0);
    }
    if(state&&!passes_->Bind(state,target_bindings::Acquire(state),error))return FrameResult::Failed;
    {const bool timed=profileFrame_<3||(profileFrame_+1)%10==0;material_dispatch::ResetProfile(timed);cache_.SetTiming(timed,profileFrame_+1);image_assets::SetModelLightingProfileTiming(timed);}
    beginMs_=ProfileMs(beginStart);recordStart_=ProfileNow();
    hitStart_=cache_.Hits();missStart_=cache_.Misses();
    keyStart_=cache_.KeyNanoseconds();createStart_=cache_.CreateNanoseconds();
    return FrameResult::Recording;
}
void EngineFrames::Submitted(void* self,unsigned slot,VkFence fence) {
    auto& owner=*static_cast<EngineFrames*>(self);
    if(!owner.slots_[slot].storage->Submitted(fence,owner.submitError_))return;
    for(auto& lane:owner.slots_[slot].lanes)if(!lane.storage->Submitted(fence,owner.submitError_))return;
    if(!owner.queries_->Submitted(fence,owner.submitError_))return;
    if(!owner.events_->Submitted(owner.submitError_))return;
    owner.queries_.reset();owner.events_.reset();
}
bool EngineFrames::Submit(bool wait,std::string& error) {
    if(!Active()||!queries_||queries_->active) {error="Native engine submission requires an active frame without an open query";return false;}
    if(!passes_->Finish(error))return false;passes_.reset();
    auto& slot=slots_[context_.slot];
    if(slot.timestamps)vkCmdWriteTimestamp(context_.command,VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,slot.timestamps,1);
    const double recordMs=ProfileMs(recordStart_);const auto submitStart=ProfileNow();
    // Diagnostic: every slow frame (not only sampled ones) with where its time
    // went; "outside" is render-thread time between scene frames (frontend
    // wait, other backend commands, window present).
    if(lastSubmit_&&submitStart>lastSubmit_) {
        const double interval=double(submitStart-lastSubmit_)/1e6;
        if(interval>45.0)__android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan",
            "native_frame_spike frame=%u interval_ms=%.1f begin_ms=%.1f upload_ms=%.1f slot_wait_ms=%.1f record_ms=%.1f outside_ms=%.1f create_ms=%.1f misses=%u",
            profileFrame_+1,interval,beginMs_,uploadMs_,slotWaitMs_,recordMs,interval-beginMs_-recordMs,
            double(cache_.CreateNanoseconds()-createStart_)/1e6,cache_.Misses()-missStart_);
    }
    lastSubmit_=submitStart;
    auto result=renderer_.SubmitFrame(error,Submitted,this);
    if(result!=FrameResult::Presented)return false;
    if(!submitError_.empty()){error=submitError_;return false;}
    // Persist new driver pipelines once creation has been quiet for a while.
    if(cache_.Misses()!=lastMisses_){lastMisses_=cache_.Misses();quietFrames_=0;}
    else if(lastMisses_!=savedMisses_&&++quietFrames_>=120) {
        std::string saveError;
        if(cache_.Save(saveError))savedMisses_=lastMisses_;
        else {__android_log_print(ANDROID_LOG_WARN,"BO1NativeVulkan","native_pipeline_cache_save_failed %s",saveError.c_str());savedMisses_=lastMisses_;}
    }
    const double submitMs=ProfileMs(submitStart);const auto waitStart=ProfileNow();
    if(wait&&!renderer_.WaitFrame(context_.slot,error))return false;
    const double waitMs=wait?ProfileMs(waitStart):0;
    auto& saved=slot.profile;
    if(saved.pending){error="Native scene profile was not retired before slot reuse";return false;}
    saved.pending=true;saved.frame=++profileFrame_;saved.begin=beginMs_;saved.upload=uploadMs_;
    saved.slotWait=slotWaitMs_;saved.retire=retireMs_;saved.reset=resetMs_;saved.prepare=prepareMs_;
    saved.record=recordMs;saved.submit=submitMs;saved.wait=waitMs;saved.submitClock=double(submitStart)/1e6;
    if(const auto window=cadence_.Observe(submitStart))
        __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan",
            "native_frame_cadence frame=%u frames=%u seconds=%.6f fps=%.3f p50_ms=%.3f p95_ms=%.3f p99_ms=%.3f max_ms=%.3f",
            saved.frame,window->frames,window->seconds,window->fps,window->p50,window->p95,window->p99,window->maximum);
    saved.key=double(cache_.KeyNanoseconds()-keyStart_)/1e6;saved.create=double(cache_.CreateNanoseconds()-createStart_)/1e6;
    saved.hits=cache_.Hits()-hitStart_;saved.misses=cache_.Misses()-missStart_;saved.evictions=cache_.Evictions()-evictStart_;
    saved.cacheSize=cache_.Size();saved.definitions=passCache_.Size();saved.draw=material_dispatch::ReadProfile();
    evictStart_=cache_.Evictions();
    return !wait||RetireProfile(slot,error);
}
bool EngineFrames::RetireProfile(Slot& slot,std::string& error) {
    const auto& saved=slot.profile;if(!saved.pending)return true;
    double gpuMs=-1;
    // Caller completed this slot's real fence (or device drain). Results are
    // read before query resets or resource retirement, never by adding a wait.
    if(slot.timestamps) {
        std::array<std::uint64_t,pass_recording::ProfileQueryCount> stamps{};
        const unsigned count=2+2*unsigned(slot.passes.profile.size());
        if(vkGetQueryPoolResults(renderer_.Device(),slot.timestamps,0,count,count*sizeof(std::uint64_t),stamps.data(),sizeof(std::uint64_t),VK_QUERY_RESULT_64_BIT)!=VK_SUCCESS) {
            error="Native scene timestamp results unavailable after completed fence";return false;
        }
        const auto mask=timestampBits_==64?~std::uint64_t(0):(std::uint64_t(1)<<timestampBits_)-1;
        gpuMs=double((stamps[1]-stamps[0])&mask)*timestampPeriod_/1e6;
        for(unsigned i=0;i<slot.passes.profile.size();++i) {
            const auto& p=slot.passes.profile[i];
            const double ms=double((stamps[p.query+1]-stamps[p.query])&mask)*timestampPeriod_/1e6;
            __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan",
                "native_pass_profile frame=%u pass=%u target=%d width=%u height=%u draws=%u indexed=%llu triangles=%llu merged=%llu fullscreen=%llu load_color=%u load_depth=%u gpu_ms=%.3f color=%s depth=%s truncated=%u",
                saved.frame,i,p.target,p.width,p.height,p.draws,(unsigned long long)p.indexedCommands,(unsigned long long)p.triangles,
                (unsigned long long)p.mergedRanges,(unsigned long long)p.fullscreenCommands,
                unsigned(p.loadColor),unsigned(p.loadDepth),ms,p.color.c_str(),p.depth.c_str(),unsigned(slot.passes.profileTruncated));
        }
    }
    if(saved.frame<=3||saved.frame%10==0) {
        __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan",
            "native_scene_profile frame=%u draws=%zu passes=%zu upload_ms=%.3f begin_ms=%.3f record_ms=%.3f submit_ms=%.3f fence_wait_ms=%.3f gpu_ms=%.3f key_ms=%.3f program_create_ms=%.3f cache_hits=%u cache_misses=%u evictions=%u cache_size=%zu renderpass_definitions=%zu timestamp_bits=%u submit_clock_ms=%.3f",
            saved.frame,slot.packets.size(),slot.passes.passes.size(),saved.upload,saved.begin,saved.record,saved.submit,saved.wait,gpuMs,
            saved.key,saved.create,saved.hits,saved.misses,saved.evictions,saved.cacheSize,saved.definitions,timestampBits_,saved.submitClock);
        __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan",
            "native_begin_profile frame=%u upload_ms=%.3f slot_ms=%.3f retire_ms=%.3f reset_ms=%.3f prepare_ms=%.3f",
            saved.frame,saved.upload,saved.slotWait,saved.retire,saved.reset,saved.prepare);
        if(saved.frame%300==0) {
            static std::uint64_t lastCreates,lastCreateBytes;
            const auto memory=ReadDeviceMemoryStats();
            __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan",
                "native_device_memory frame=%u buffers=%llu buffer_mb=%.1f buffer_requested_mb=%.1f small_buffers=%llu large_buffers=%llu large_mb=%.1f creates_300f=%llu create_mb_300f=%.1f images=%llu image_mb=%.1f",saved.frame,
                (unsigned long long)memory.bufferCount,memory.bufferBytes/1048576.0,memory.bufferRequested/1048576.0,
                (unsigned long long)memory.smallBuffers,(unsigned long long)memory.largeBuffers,memory.largeBufferBytes/1048576.0,
                (unsigned long long)(memory.bufferCreates-lastCreates),(memory.bufferCreateBytes-lastCreateBytes)/1048576.0,
                (unsigned long long)memory.imageCount,memory.imageBytes/1048576.0);
            lastCreates=memory.bufferCreates;lastCreateBytes=memory.bufferCreateBytes;
            __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan","native_draw_storage frame=%u pages=%u draws=%u used_kb=%.0f",
                saved.frame,slot.storage->Pages(),slot.storage->Draws(),slot.storage->Bytes()/1024.0);
        }
        const auto modelLighting=image_assets::ReadModelLightingProfile();
        __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan",
            "native_model_lighting_profile frame=%u enabled=%u batches_total=%llu changed_tiles_total=%llu unchanged_tiles_total=%llu snapshot_bytes_total=%llu sampled_cpu_ns_total=%llu",
            saved.frame,unsigned(image_assets::ModelLightingRegionsEnabled()),
            (unsigned long long)modelLighting.batches,(unsigned long long)modelLighting.changedTiles,
            (unsigned long long)modelLighting.unchangedTiles,(unsigned long long)modelLighting.snapshotBytes,
            (unsigned long long)modelLighting.cpuNs);
        const auto scratch=material_dispatch::ReadResourceScratchProfile();
        __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan",
            "native_resource_scratch_profile frame=%u enabled=%u leases_total=%llu nested_leases_total=%llu capacity_growths_total=%llu",
            saved.frame,unsigned(material_dispatch::ResourceScratchReuseEnabled()),(unsigned long long)scratch.leases,
            (unsigned long long)scratch.nestedLeases,(unsigned long long)scratch.capacityGrowths);
        const auto sampling=texture_bindings::ReadSamplingValidationProfile();
        __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan",
            "native_sampling_validation_profile frame=%u enabled=%u lookups_total=%llu slot_reuses_total=%llu slot_validations_total=%llu",
            saved.frame,unsigned(texture_bindings::SamplingValidationReuseEnabled()),(unsigned long long)sampling.lookups,
            (unsigned long long)sampling.slotReuses,(unsigned long long)sampling.slotValidations);
        const auto indexBindings=geometry_bindings::ReadIndexBindingProfile();
        __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan",
            "native_index_binding_profile frame=%u enabled=%u attempts_total=%llu hits_total=%llu",
            saved.frame,unsigned(geometry_bindings::IndexBindingReuseEnabled()),(unsigned long long)indexBindings.attempts,(unsigned long long)indexBindings.hits);
        const auto descriptorWrites=ReadDescriptorWriteStats();
        __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan",
            "native_descriptor_write_profile frame=%u enabled=%u lazy_skips_total=%llu write_builds_total=%llu",
            saved.frame,unsigned(LazyDescriptorWrites()),(unsigned long long)descriptorWrites.lazySkips,(unsigned long long)descriptorWrites.writeBuilds);
        const auto& p=saved.draw;
        __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan",
            "native_draw_profile frame=%u draws=%u reused=%u miss_state=%u miss_selection=%u miss_constants=%u miss_textures=%u miss_geometry=%u capture_ms=%.3f vertex_resolve_ms=%.3f state_ms=%.3f program_ms=%.3f upload_ms=%.3f descriptors_ms=%.3f geometry_ms=%.3f cached_groups=%llu cached_models=%llu cached_indices=%llu cached_uploads=%llu program_reuse=%llu full_prepares=%llu weak_constants=%llu strong_constants=%llu forward_draws=%llu omitted_groups_total=%llu geometry_arena_retentions=%llu geometry_arena_growths=%llu geometry_retained_draws=%llu geometry_offset_misses=%llu geometry_packet_hits=%llu geometry_buffer_misses=%llu",
            saved.frame,p.draws,p.reused,p.missState,p.missSelection,p.missConstants,p.missTextures,p.missGeometry,p.capture/1e6,p.resolve/1e6,p.state/1e6,p.program/1e6,p.upload/1e6,p.descriptors/1e6,p.geometry/1e6,
            (unsigned long long)p.cachedGroups,(unsigned long long)p.cachedModels,(unsigned long long)p.cachedIndices,(unsigned long long)p.cachedUploads,
            (unsigned long long)p.programReuse,(unsigned long long)p.fullPrepares,(unsigned long long)p.weakConstants,(unsigned long long)p.strongConstants,(unsigned long long)p.forwardDraws,(unsigned long long)forward_opaque::OmittedGroups(),
            (unsigned long long)p.geometryArenaRetentions,(unsigned long long)p.geometryArenaGrowths,(unsigned long long)p.geometryRetainedDraws,
            (unsigned long long)p.geometryOffsetMisses,(unsigned long long)p.geometryPacketHits,(unsigned long long)p.geometryBufferMisses);
    }
    slot.profile.pending=false;return true;
}
FrameResult EngineFrames::Present(std::shared_ptr<const image_assets::Record> image,std::string& error) {
    if(Active()||queries_||events_||!image||!image->texture) {
        error="Native window output needs a submitted scene and actual color image";return FrameResult::Failed;
    }
    const auto presentStart=ProfileNow();
    FrameContext frame{};auto result=renderer_.BeginFrame(frame,error);
    if(result!=FrameResult::Recording)return result;
    const double acquireMs=ProfileMs(presentStart);
    auto& slot=slots_[frame.slot];
    if(!RetireProfile(slot,error))return FrameResult::Failed;
    slot.windowProgram.reset();slot.windowImage.reset();slot.packets.clear();slot.passes.Retire(cache_);slot.queries.Retire();
    if(!slot.events.Retire(error))return FrameResult::Failed;
    if(windowGeneration_!=frame.generation) {
        // Swapchain rebuild drains the device before destroying its old pass.
        // Drop cached pass identities so a recycled handle cannot hit old PSOs.
        cache_.Clear();for(auto& old:slots_)old.windowProgram.reset();windowGeneration_=frame.generation;sgsr_.pass=VK_NULL_HANDLE;prewarmPending_=true;prewarmClearFrame_=profileFrame_;
    }
    VkImageLayout layout{};
    if(!image_usage::Sample(image->texture,layout,error))return FrameResult::Failed;
    const auto outputSampler=outputSamplers_[image->description.linearSampling?1:0]->Handle();
    if(vkResetDescriptorPool(renderer_.Device(),slot.descriptors,0)!=VK_SUCCESS) {
        error="Native output descriptor reset failed";return FrameResult::Failed;
    }
    // Backend-owned passthrough shaders; retail bytecode is never modified.
    static const unsigned vs[]={0xfffe0300,
        0x0200001f,0x80000000,0x900f0000,0x0200001f,0x80000005,0x90030001,
        0x0200001f,0x80000000,0xe00f0000,0x0200001f,0x80000005,0xe0030001,
        0x02000001,0xe00f0000,0x90e40000,0x02000001,0xe0030001,0x90e40001,0x0000ffff};
    static const unsigned ps[]={0xffff0300,0x0200001f,0x80000005,0x90030000,
        0x0200001f,0x90000000,0xa00f0800,0x03000042,0x800f0000,0x90e40000,0xa0e40800,
        0x02000001,0x800f0800,0x80e40000,0x0000ffff};
    struct Vertex{float position[4],uv[2];};
    Vertex vertices[]={{{-1,-1,0,1},{0,1}},{{-1,3,0,1},{0,-1}},{{3,-1,0,1},{2,1}}};
    std::array<float,16> transform{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};renderer_.PreRotateProjection(transform);
    for(auto& v:vertices) {const float x=v.position[0],y=v.position[1];v.position[0]=transform[0]*x+transform[1]*y;v.position[1]=transform[4]*x+transform[5]*y;}
    if(!slot.uniforms->Write(0,vertices,sizeof(vertices),error))return FrameResult::Failed;
    const VkVertexInputBindingDescription binding{0,sizeof(Vertex),VK_VERTEX_INPUT_RATE_VERTEX};
    const VkVertexInputAttributeDescription attributes[]={{0,0,VK_FORMAT_R32G32B32A32_SFLOAT,0},{1,0,VK_FORMAT_R32G32_SFLOAT,16}};
    ProgramDescription description;description.vertexBytecode=vs;description.vertexBytes=sizeof(vs);description.pixelBytecode=ps;description.pixelBytes=sizeof(ps);
    description.renderPass=renderer_.RenderPass();description.vertexBindings=&binding;description.vertexBindingCount=1;description.vertexAttributes=attributes;description.vertexAttributeCount=2;
    slot.windowProgram=cache_.Acquire(description,error);if(!slot.windowProgram)return FrameResult::Failed;
    ProgramResource texture{};texture.kind=ResourceKind::Texture;texture.image={VK_NULL_HANDLE,image->texture->View(),layout};
    ProgramResource sampler{};sampler.kind=ResourceKind::Sampler;sampler.image={outputSampler,VK_NULL_HANDLE,VK_IMAGE_LAYOUT_UNDEFINED};
    std::array<VkDescriptorSet,2> sets{};
    if(!slot.windowProgram->BindResources(slot.descriptors,{}, {texture,sampler},sets,error))return FrameResult::Failed;
    if(profileFrame_<=3||profileFrame_%100==0)__android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan",
        "native_output_extent frame=%u scene_width=%u scene_height=%u swap_width=%u swap_height=%u",
        profileFrame_,image->description.extent.width,image->description.extent.height,frame.extent.width,frame.extent.height);
    const bool quarterTurn=renderer_.Transform()&(VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR|VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR
        |VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90_BIT_KHR|VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270_BIT_KHR);
    const auto& source=image->description.extent;
    const auto display=renderer_.DisplayExtent();
    const auto fit=bo1::android::native_render_size::FitViewport(source.width,source.height,display.width,display.height,frame.extent.width,frame.extent.height,quarterTurn);
    if(!fit.width||!fit.height){error="Native output display extent unavailable";return FrameResult::Failed;}
    // Snapdragon GSR v1 replaces bilinear output when the scene is upscaled.
    const bool sgsr=native_fullscreen::SgsrUpscale()&&(source.width<fit.width||source.height<fit.height);
    VkDescriptorSet sgsrSet{};
    if(sgsr) {
        if(!SgsrPipeline(error))return FrameResult::Failed;
        VkDescriptorSetAllocateInfo allocate{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        allocate.descriptorPool=slot.descriptors;allocate.descriptorSetCount=1;allocate.pSetLayouts=&sgsr_.set;
        if(vkAllocateDescriptorSets(renderer_.Device(),&allocate,&sgsrSet)!=VK_SUCCESS){error="Native SGSR descriptor allocation failed";return FrameResult::Failed;}
        const VkDescriptorImageInfo imageInfo{VK_NULL_HANDLE,image->texture->View(),layout};
        const VkDescriptorImageInfo samplerInfo{outputSamplers_[1]->Handle(),VK_NULL_HANDLE,VK_IMAGE_LAYOUT_UNDEFINED};
        VkWriteDescriptorSet writes[2]{{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET},{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET}};
        writes[0].dstSet=sgsrSet;writes[0].dstBinding=0;writes[0].descriptorCount=1;writes[0].descriptorType=VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;writes[0].pImageInfo=&imageInfo;
        writes[1].dstSet=sgsrSet;writes[1].dstBinding=1;writes[1].descriptorCount=1;writes[1].descriptorType=VK_DESCRIPTOR_TYPE_SAMPLER;writes[1].pImageInfo=&samplerInfo;
        vkUpdateDescriptorSets(renderer_.Device(),2,writes,0,nullptr);
    }
    slot.windowImage=std::move(image);
    const VkClearColorValue clear{{0,0,0,1}};
    if(!renderer_.BeginWindowPass(clear,error))return FrameResult::Failed;
    if(sgsr) {
        const float viewportInfo[4]={1.0f/float(source.width),1.0f/float(source.height),float(source.width),float(source.height)};
        vkCmdBindPipeline(frame.command,VK_PIPELINE_BIND_POINT_GRAPHICS,sgsr_.pipeline);
        vkCmdBindDescriptorSets(frame.command,VK_PIPELINE_BIND_POINT_GRAPHICS,sgsr_.layout,0,1,&sgsrSet,0,nullptr);
        vkCmdPushConstants(frame.command,sgsr_.layout,VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(viewportInfo),viewportInfo);
    } else slot.windowProgram->Bind(frame.command,sets);
    const VkViewport viewport{float(fit.x),float(fit.y+fit.height),float(fit.width),-float(fit.height),0,1};
    const VkRect2D scissor{{0,0},frame.extent};vkCmdSetViewport(frame.command,0,1,&viewport);vkCmdSetScissor(frame.command,0,1,&scissor);
    const VkBuffer buffer=slot.uniforms->Handle();const VkDeviceSize offset=0;vkCmdBindVertexBuffers(frame.command,0,1,&buffer,&offset);vkCmdDraw(frame.command,3,1,0,0);
    if(!renderer_.EndWindowPass(error))return FrameResult::Failed;
    const auto presentSubmit=ProfileNow();
    result=renderer_.SubmitFrame(error);
    if(profileFrame_<=3||profileFrame_%10==0)__android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan",
        "native_window_profile frame=%u total_ms=%.3f acquire_ms=%.3f record_ms=%.3f submit_ms=%.3f",
        profileFrame_,ProfileMs(presentStart),acquireMs,double(presentSubmit-presentStart)/1e6-acquireMs,ProfileMs(presentSubmit));
    return result;
}
bool EngineFrames::Drain(std::string& error) {
    // An unsubmitted command buffer can retain recording metadata, but cannot
    // have GPU users. End tracked passes before dropping its image journal.
    passes_.reset();queries_.reset();events_.reset();
    if(vkDeviceWaitIdle(renderer_.Device())!=VK_SUCCESS) {error="Native engine frame retirement failed";return false;}
    for(auto& slot:slots_) {
        if(!RetireProfile(slot,error))return false;
        slot.windowProgram.reset();slot.windowImage.reset();
        slot.packets.clear();slot.passes.Retire(cache_);slot.queries.Retire();
        if(!slot.events.Retire(error))return false;
        if(!RetireLanes(slot,error))return false;
    }
    cache_.Clear();passCache_.Clear();return true;
}
}
