#include "engine_frames.h"
#include "image_uploads.h"
#include "target_bindings.h"
#include <cstdlib>
#include "platform/android/native_render_size.h"
#include "platform/android/engine_paths.h"
#include <chrono>
#include <android/log.h>
namespace bo1::vulkan {
namespace {
std::uint64_t ProfileNow(){return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();}
double ProfileMs(std::uint64_t start){return double(ProfileNow()-start)/1e6;}
}
EngineFrames::EngineFrames(Presentation& p):renderer_(p),cache_(p.Device(),512),passCache_(p.Device()),fullscreen_(p.Device()){}
EngineFrames::~EngineFrames() {
    std::string error;if(!Drain(error))std::abort();
    for(auto& slot:slots_) {
        if(slot.timestamps)vkDestroyQueryPool(renderer_.Device(),slot.timestamps,nullptr);
        if(slot.descriptors)vkDestroyDescriptorPool(renderer_.Device(),slot.descriptors,nullptr);
    }
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
    }
    return true;
}
FrameResult EngineFrames::Begin(GfxCmdBufState* state,std::string& error) {
    if(Active()||queries_||events_) {error="Native engine frame already active";return FrameResult::Failed;}
    const auto beginStart=ProfileNow();
    const auto uploadStart=ProfileNow();
    if(!image_uploads::Flush(error))return FrameResult::Failed;
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
    resetMs_=ProfileMs(resetStart);const auto prepareStart=ProfileNow();
    slot.cursor=0;submitError_.clear();
    queries_=std::make_unique<query_assets::RecordingScope>(context_.command,slot.queries);
    if(!queries_->Prepare(error))return FrameResult::Failed;
    prepareMs_=ProfileMs(prepareStart);
    events_=std::make_unique<event_fences::RecordingScope>(slot.events);
    passes_=std::make_unique<pass_recording::RecordingScope>(context_.command,renderer_.Physical(),renderer_.Device(),cache_,*slot.uniforms,slot.descriptors,slot.cursor,alignment_,slot.packets,slot.passes,slot.storage.get(),&passCache_,(profileFrame_<3||(profileFrame_+1)%10==0)?slot.timestamps:VK_NULL_HANDLE);
    passes_->SetFullscreenLibrary(&fullscreen_);
    if(slot.timestamps) {
        vkCmdResetQueryPool(context_.command,slot.timestamps,0,pass_recording::ProfileQueryCount);
        vkCmdWriteTimestamp(context_.command,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,slot.timestamps,0);
    }
    if(state&&!passes_->Bind(state,target_bindings::Acquire(state),error))return FrameResult::Failed;
    {const bool timed=profileFrame_<3||(profileFrame_+1)%10==0;material_dispatch::ResetProfile(timed);cache_.SetTiming(timed);}
    beginMs_=ProfileMs(beginStart);recordStart_=ProfileNow();
    hitStart_=cache_.Hits();missStart_=cache_.Misses();
    keyStart_=cache_.KeyNanoseconds();createStart_=cache_.CreateNanoseconds();
    return FrameResult::Recording;
}
void EngineFrames::Submitted(void* self,unsigned slot,VkFence fence) {
    auto& owner=*static_cast<EngineFrames*>(self);
    if(!owner.slots_[slot].storage->Submitted(fence,owner.submitError_))return;
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
                "native_pass_profile frame=%u pass=%u target=%d width=%u height=%u draws=%u load_color=%u load_depth=%u gpu_ms=%.3f color=%s depth=%s truncated=%u",
                saved.frame,i,p.target,p.width,p.height,p.draws,unsigned(p.loadColor),unsigned(p.loadDepth),ms,p.color.c_str(),p.depth.c_str(),unsigned(slot.passes.profileTruncated));
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
        const auto& p=saved.draw;
        __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan",
            "native_draw_profile frame=%u draws=%u reused=%u miss_state=%u miss_selection=%u miss_constants=%u miss_textures=%u miss_geometry=%u capture_ms=%.3f vertex_resolve_ms=%.3f state_ms=%.3f program_ms=%.3f upload_ms=%.3f descriptors_ms=%.3f geometry_ms=%.3f",
            saved.frame,p.draws,p.reused,p.missState,p.missSelection,p.missConstants,p.missTextures,p.missGeometry,p.capture/1e6,p.resolve/1e6,p.state/1e6,p.program/1e6,p.upload/1e6,p.descriptors/1e6,p.geometry/1e6);
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
        cache_.Clear();for(auto& old:slots_)old.windowProgram.reset();windowGeneration_=frame.generation;
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
    slot.windowImage=std::move(image);
    const VkClearColorValue clear{{0,0,0,1}};
    if(!renderer_.BeginWindowPass(clear,error))return FrameResult::Failed;
    slot.windowProgram->Bind(frame.command,sets);
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
    }
    cache_.Clear();passCache_.Clear();return true;
}
}
