#include "pass_recording.h"
#include "image_usage.h"
#include "target_bindings.h"
#include "forward_opaque.h"
#include "gfx_d3d/rb_state.h"
#include "gfx_d3d/r_rendertarget.h"
#include <cmath>
#include <cstring>
#include <array>
#include <algorithm>
#include <limits>
#include <cstdlib>
#include <atomic>
#include <android/log.h>
#include <string>
#include <vector>
namespace bo1::vulkan::pass_recording {
namespace {thread_local RecordingScope* current{};std::atomic<bool> g_constantUploadReuse{true};}
void SetConstantUploadReuse(bool enabled){g_constantUploadReuse.store(enabled,std::memory_order_relaxed);}
void SetForwardOpaque(bool enabled){forward_opaque::SetEnabled(enabled);}
RecordingScope* Current(){return current;}
namespace {void BeginColorResolveFrame();}
RecordingScope::RecordingScope(VkCommandBuffer c,VkPhysicalDevice p,VkDevice d,ProgramCache& cache,Buffer& uniforms,VkDescriptorPool pool,
    VkDeviceSize& cursor,VkDeviceSize alignment,std::vector<material_dispatch::Packet>& packets,Arena& arena,DrawStorage* storage,AttachmentPassCache* passCache,VkQueryPool profilePool)
    :command_(c),physical_(p),device_(d),cache_(cache),uniforms_(uniforms),pool_(pool),cursor_(cursor),alignment_(alignment),packets_(packets),arena_(arena),primary_(c),binds_(ownBinds_),passCache_(passCache),storage_(storage),previous_(current){profilePool_=profilePool;binds_.compatibleSets=CompatibleDescriptorReuse();binds_.reuseConstants=g_constantUploadReuse.load(std::memory_order_relaxed);current=this;BeginColorResolveFrame();}
RecordingScope::RecordingScope(const RecordingScope& parent,ParallelLane& lane)
    :command_(VK_NULL_HANDLE),physical_(parent.physical_),device_(parent.device_),cache_(parent.cache_),uniforms_(parent.uniforms_),
    pool_(parent.pool_),cursor_(lane.cursor),alignment_(parent.alignment_),packets_(lane.packets),arena_(lane.arena),
    parent_(&parent),chunk_(true),lane_(&lane),binds_(lane.binds),storage_(lane.storage.get()),previous_(current){
    binds_.compatibleSets=parent.binds_.compatibleSets;binds_.reuseConstants=parent.binds_.reuseConstants;current=this;
}
bool ParallelLane::Retire(VkDevice device,ProgramCache& cache,std::string& error) {
    packets.clear();arena.Retire(cache);binds=BindCache{};cursor=0;
    if(used&&vkResetCommandPool(device,pool,0)!=VK_SUCCESS) {error="Native parallel lane command pool reset failed";return false;}
    used=0;retirePending=false;return true;
}
bool ParallelLane::Acquire(VkDevice device,VkCommandBuffer& command,std::string& error) {
    if(!pool) {error="Native parallel lane has no command pool";return false;}
    if(used==buffers.size()) {
        VkCommandBufferAllocateInfo info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        info.commandPool=pool;info.level=VK_COMMAND_BUFFER_LEVEL_SECONDARY;info.commandBufferCount=1;
        VkCommandBuffer created{};if(vkAllocateCommandBuffers(device,&info,&created)!=VK_SUCCESS) {error="Native secondary command buffer allocation failed";return false;}
        buffers.push_back(created);
    }
    command=buffers[used++];return true;
}
namespace {
bool BeginSecondary(VkCommandBuffer command,const AttachmentPass& pass,std::string& error) {
    VkCommandBufferInheritanceInfo inheritance{VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO};
    inheritance.renderPass=pass.RenderPass();inheritance.subpass=0;inheritance.framebuffer=pass.Framebuffer();
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags=VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT|VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;begin.pInheritanceInfo=&inheritance;
    if(vkBeginCommandBuffer(command,&begin)!=VK_SUCCESS) {error="Native secondary command buffer begin failed";return false;}
    return true;
}
// Attachment pairs that hosted parallel recording: their passes begin with
// secondary-command-buffer contents (render thread only; addresses are only a
// preference, never ownership).
struct ParallelHost {const Texture2D* color{};const Texture2D* depth{};};
std::vector<ParallelHost> g_parallelHosts;
bool IsParallelHost(const Texture2D* color,const Texture2D* depth) {
    for(const auto& host:g_parallelHosts)if(host.color==color&&host.depth==depth)return true;
    return false;
}
void NoteParallelHost(const Texture2D* color,const Texture2D* depth) {
    if(IsParallelHost(color,depth))return;
    if(g_parallelHosts.size()>=8)g_parallelHosts.erase(g_parallelHosts.begin());
    g_parallelHosts.push_back({color,depth});
}
}
bool RecordingScope::StartMainSecondary(std::string& error) {
    if(!lanes_||!laneCount_||!active_) {error="Native secondary pass recording has no lane or pass";return false;}
    VkCommandBuffer command{};if(!lanes_[0].Acquire(device_,command,error)||!BeginSecondary(command,*active_,error))return false;
    command_=command;ForgetBoundState(binds_);return true;
}
bool RecordingScope::CloseMainSecondary(std::string& error) {
    FlushPending(binds_);
    if(command_==primary_) {error="Native secondary pass recording lost its secondary";return false;}
    if(vkEndCommandBuffer(command_)!=VK_SUCCESS) {error="Native secondary command buffer end failed";return false;}
    executes_.push_back(command_);command_=primary_;ForgetBoundState(binds_);return true;
}
bool RecordingScope::BeginParallel(GfxCmdBufState* state,bool& ready,std::string& error) {
    ready=false;
    if(chunk_||!lanes_||laneCount_<2)return true;
    if(!state||!Bind(state,target_bindings::Acquire(state),error))return false;
    if(!Ensure(error))return false;
    if(target_->depth&&!active_->HasDepth())return true; // a colour-only pass would split on the first depth draw
    const auto* color=target_->color?target_->color->texture.get():nullptr;
    const auto* depth=target_->depth?target_->depth->texture.get():nullptr;
    if(!secondaryPass_) {
        // This pass began inline. Prefer secondaries for it from now on; switch
        // now only when ending it keeps both attachments loadable.
        NoteParallelHost(color,depth);
        if(depth&&!(target_->depth->texture->Description().usage&VK_IMAGE_USAGE_SAMPLED_BIT))return true;
        if(!Finish(error)||!Ensure(error))return false;
        if(!secondaryPass_)return true;
    }
    if(!CloseMainSecondary(error))return false;
    ready=true;return true;
}
bool RecordingScope::EndParallel(const std::vector<VkCommandBuffer>& ordered,std::string& error) {
    if(chunk_||!secondaryPass_||!active_||command_!=primary_) {error="Native parallel recording ended outside its pass";return false;}
    executes_.insert(executes_.end(),ordered.begin(),ordered.end());
    // Chunk draws may write both attachments (no per-draw generation tracking).
    if(target_->color)MarkDepthWrite(target_->color->texture.get());
    if(active_->HasDepth())MarkDepthWrite(target_->depth->texture.get());
    return StartMainSecondary(error);
}
bool RecordingScope::BeginChunk(GfxCmdBufState* state,std::string& error) {
    if(!chunk_||!parent_||!parent_->active_||!parent_->secondaryPass_||command_||!state) {error="Native parallel chunk has no parallel parent pass";return false;}
    if(target_bindings::Acquire(state)!=target_bindings::Acquire(parent_->state_)) {error="Native parallel chunk state has another target";return false;}
    if(lane_->retirePending) {
        // First use this frame by the owning thread: the binds being reset are
        // this scope's binds_, which holds nothing yet.
        if(!lane_->Retire(device_,cache_,error))return false;
        binds_.compatibleSets=parent_->binds_.compatibleSets;binds_.reuseConstants=parent_->binds_.reuseConstants;
    }
    target_=parent_->target_;active_=parent_->active_;scissorEnabled_=parent_->scissorEnabled_;scissor_=parent_->scissor_;
    if(!lane_->Acquire(device_,command_,error)||!BeginSecondary(command_,*active_,error)){command_=VK_NULL_HANDLE;return false;}
    ForgetBoundState(binds_);
    state_=state;return true;
}
namespace {
// Diagnostic: which materials write depth after a scene depth resolve (each
// such write forces another resolve and pass split). Render thread only.
unsigned g_depthWriteDiagnosticFrame;
std::atomic<bool> g_singleDepthResolve{};
std::atomic<bool> g_lazyColorResolve{};
// Colour resolve destinations: shader reads at the start of this frame and
// during the previous frame. Render thread only; keyed by the destination.
struct ColorResolveState {std::weak_ptr<Texture2D> image;std::uint64_t readsAtFrameStart{},previousFrameReads{~0ull};};
std::unordered_map<const Texture2D*,ColorResolveState,bo1::vulkan::PointerHash> g_colorResolves;
void BeginColorResolveFrame() {
    for(auto it=g_colorResolves.begin();it!=g_colorResolves.end();) {
        const auto image=it->second.image.lock();
        if(!image){it=g_colorResolves.erase(it);continue;}
        it->second.previousFrameReads=image->Reads()-it->second.readsAtFrameStart;
        it->second.readsAtFrameStart=image->Reads();++it;
    }
}
}
void SetLazyColorResolve(bool enabled){g_lazyColorResolve.store(enabled,std::memory_order_relaxed);}
namespace {std::atomic<bool> g_parallelRecord{};}
void SetParallelRecord(bool enabled){g_parallelRecord.store(enabled,std::memory_order_relaxed);}
bool ParallelRecordEnabled(){return g_parallelRecord.load(std::memory_order_relaxed);}
void SetSingleDepthResolve(bool enabled){g_singleDepthResolve.store(enabled,std::memory_order_relaxed);}
RecordingScope::~RecordingScope(){
    std::string error;if(!Finish(error))std::abort();current=previous_;
    if(profilePool_)__android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan",
        "native_descriptor_profile compatible=%u hits=%llu misses=%llu pipeline_binds=%llu pipeline_skips=%llu bind_calls=%llu bound_sets=%llu content_hits=%llu content_misses=%llu",
        unsigned(binds_.compatibleSets),(unsigned long long)binds_.setHits,(unsigned long long)binds_.setMisses,
        (unsigned long long)binds_.pipelineBinds,(unsigned long long)binds_.pipelineSkips,
        (unsigned long long)binds_.descriptorBinds,(unsigned long long)binds_.descriptorSetsBound,
        (unsigned long long)binds_.descriptorContentHits,(unsigned long long)binds_.descriptorContentMisses);
    if(profilePool_)__android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan",
        "native_constant_upload_profile enabled=%u hits=%llu misses=%llu reused_bytes=%llu",
        unsigned(binds_.reuseConstants),(unsigned long long)binds_.constantUploadHits,
        (unsigned long long)binds_.constantUploadMisses,(unsigned long long)binds_.constantUploadBytesReused);
    if(profilePool_)__android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan",
        "native_command_profile packets=%zu indexed=%llu triangles=%llu merged=%llu fullscreen=%llu",
        packets_.size(),(unsigned long long)binds_.indexedCommands,(unsigned long long)binds_.indexedTriangles,
        (unsigned long long)binds_.mergedRanges,(unsigned long long)binds_.fullscreenCommands);
    if(!postResolveDepthWriters_.empty()&&(g_depthWriteDiagnosticFrame++%200)==0) {
        std::string names;for(const auto& n:postResolveDepthWriters_){names+=n;names+=' ';}
        __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan","post_resolve_depth_writers draws=%u materials=%s",postResolveDepthWrites_,names.c_str());
    }
}
bool RecordingScope::Finish(std::string& error){
    FlushPending(binds_);
    if(chunk_) {
        // A chunk ends its secondary, never the parent's pass. Any later
        // operation needing a pass fails in Ensure.
        if(command_&&!chunkEnded_){chunkEnded_=true;if(vkEndCommandBuffer(command_)!=VK_SUCCESS){error="Native parallel chunk end failed";return false;}}
        active_.reset();return true;
    }
    if(secondaryPass_) {
        if(command_!=primary_&&!CloseMainSecondary(error))return false;
        if(!executes_.empty())vkCmdExecuteCommands(primary_,unsigned(executes_.size()),executes_.data());
        executes_.clear();secondaryPass_=false;ForgetBoundState(binds_);
    }
    if(active_&&!active_->EndTracked(command_,error))return false;
    if(activeProfile_>=0) {
        auto& p=arena_.profile[activeProfile_];p.draws=unsigned(packets_.size()-profileDrawStart_);
        p.indexedCommands=binds_.indexedCommands-profileIndexedStart_;p.triangles=binds_.indexedTriangles-profileTriangleStart_;
        p.mergedRanges=binds_.mergedRanges-profileMergedStart_;p.fullscreenCommands=binds_.fullscreenCommands-profileFullscreenStart_;
        vkCmdWriteTimestamp(command_,VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,profilePool_,p.query+1);
        activeProfile_=-1;
    }
    active_.reset();return true;
}
bool RecordingScope::Bind(GfxCmdBufState* state,const GfxRenderTarget* target,std::string& error) {
    if(!target_bindings::Select(state,target,error))return false;
    auto record=render_targets::Acquire(target);if(!state||!record||(!record->color&&!record->depth)){error="Native target switch has no real attachments";return false;}
    if(record!=target_){if(chunk_){error="Native parallel chunk cannot switch render targets";return false;}if(!Finish(error))return false;target_=record;}
    state_=state;return true;
}
bool RecordingScope::Ensure(std::string& error,unsigned discardMask,const float* clearColor,float clearDepth,unsigned clearStencil,bool omitDepth) {
    if(active_)return true;if(!target_||!state_){error="Native pass has no selected target";return false;}
    if(chunk_){error="Native parallel chunk cannot open a pass";return false;}
    FlushPending(binds_);
    auto color=target_->color?target_->color->texture:nullptr,depth=target_->depth&&!omitDepth?target_->depth->texture:nullptr;
    auto pass=std::make_shared<AttachmentPass>(device_,passCache_);
    const bool loadColor=!(discardMask&1)&&image_usage::CanLoad(color),loadDepth=!(discardMask&2)&&image_usage::CanLoad(depth);
    const bool preserveDepth=depth&&(depth->Description().usage&VK_IMAGE_USAGE_SAMPLED_BIT);
    if(!pass->Create(color,depth,preserveDepth,error,loadColor,int(loadDepth)))return false;
    if(profilePool_) {
        if(arena_.profile.size()<MaxProfilePasses) {
            PassProfile p;p.query=2+2*unsigned(arena_.profile.size());p.target=int(state_->renderTargetId);
            const auto& extent=(color?color:depth)->Description().extent;p.width=extent.width;p.height=extent.height;
            p.loadColor=loadColor;p.loadDepth=loadDepth;
            if(target_->color)p.color=target_->color->createdName;
            if(depth)p.depth=target_->depth->createdName;
            activeProfile_=int(arena_.profile.size());profileDrawStart_=packets_.size();arena_.profile.push_back(std::move(p));
            profileIndexedStart_=binds_.indexedCommands;profileTriangleStart_=binds_.indexedTriangles;
            profileMergedStart_=binds_.mergedRanges;profileFullscreenStart_=binds_.fullscreenCommands;
            vkCmdWriteTimestamp(command_,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,profilePool_,arena_.profile.back().query);
        } else arena_.profileTruncated=true;
    }
    VkClearColorValue value{{0,0,0,0}};if((discardMask&1)&&clearColor)for(unsigned i=0;i<4;++i)value.float32[i]=clearColor[i];
    const bool secondary=lanes_&&laneCount_>1&&IsParallelHost(color.get(),depth.get());
    if(!pass->BeginTracked(command_,value,error,(discardMask&2)?clearDepth:1.0f,(discardMask&4)?clearStencil:0,
        secondary?VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS:VK_SUBPASS_CONTENTS_INLINE))return false;
    if(depth&&!loadDepth)MarkDepthWrite(depth.get()); // CLEAR load op replaces contents
    if(color&&!loadColor)MarkDepthWrite(color.get());
    arena_.passes.push_back(pass);active_=std::move(pass);
    if(secondary){secondaryPass_=true;if(!StartMainSecondary(error))return false;}
    return true;
}
bool RecordingScope::ApplyViewport(std::string& error) {
    const auto& v=state_->viewport;const auto& size=(target_->color?target_->color:target_->depth)->description.extent;
    if(v.x<0||v.y<0||v.width<=0||v.height<=0||unsigned(v.x)>size.width||unsigned(v.y)>size.height||unsigned(v.width)>size.width-unsigned(v.x)||unsigned(v.height)>size.height-unsigned(v.y)
        ||!std::isfinite(state_->depthRangeNear)||!std::isfinite(state_->depthRangeFar)||state_->depthRangeNear<0||state_->depthRangeFar>1||state_->depthRangeNear>state_->depthRangeFar) {
        error="Native viewport exceeds attachment or depth range";return false;}
    VkViewport viewport{float(v.x),float(v.y+v.height),float(v.width),-float(v.height),state_->depthRangeNear,state_->depthRangeFar};
    VkRect2D scissor{{0,0},{size.width,size.height}};
    if(scissorEnabled_) {
        const auto x=std::min(unsigned(scissor_.offset.x),size.width),y=std::min(unsigned(scissor_.offset.y),size.height);
        scissor={{int(x),int(y)},{std::min(scissor_.extent.width,size.width-x),std::min(scissor_.extent.height,size.height-y)}};
    }
    auto& b=binds_;
    if(b.viewportValid&&!std::memcmp(&b.viewport,&viewport,sizeof viewport)&&!std::memcmp(&b.scissor,&scissor,sizeof scissor))return true;
    FlushPending(b);vkCmdSetViewport(command_,0,1,&viewport);vkCmdSetScissor(command_,0,1,&scissor);
    b.viewport=viewport;b.scissor=scissor;b.viewportValid=true;return true;
}
bool RecordingScope::Viewport(GfxCmdBufState* state,const GfxViewport& v,std::string& error) {
    // Canonical draw callbacks clone command states. A viewport can be the
    // first operation on the new state before Draw binds its inherited target.
    // Snapshot first: depth-range callers pass state->viewport by reference.
    const auto requested=v;
    if(!state){error="Native viewport has no command state";return false;}
    const auto previous=state->viewport;
    if(!target_bindings::Viewport(state,requested,error))return false;
    if(!Bind(state,target_bindings::Acquire(state),error)){state->viewport=previous;return false;}
    state->viewport=requested;
    // Dynamic viewport/scissor commands are valid outside a pass. Open the
    // attachment pass only when a clear/draw actually needs it.
    if(!ApplyViewport(error)){state->viewport=previous;return false;}return true;
}
bool RecordingScope::InitializeColor(const GfxImage* image,const VkClearColorValue& color,std::string& error) {
    FlushPending(binds_);
    const auto record=image_assets::Acquire(image);
    if(!record||!record->texture||!record->uploads.empty()
        ||record->description.aspects!=VK_IMAGE_ASPECT_COLOR_BIT) {
        error="Native initial colour contents require an actual generated colour attachment";return false;
    }
    if(image_usage::Known(record->texture)) {
        VkImageLayout layout{};return image_usage::Sample(record->texture,layout,error);
    }
    if(active_) {error="Native initial colour clear must precede scene passes";return false;}
    auto pass=std::make_shared<AttachmentPass>(device_,passCache_);
    if(!pass->Create(record->texture,nullptr,false,error)||!pass->BeginTracked(command_,color,error))return false;
    // Retain the real pass/image through this frame's successful submission.
    arena_.passes.push_back(pass);MarkDepthWrite(record->texture.get());
    return pass->EndTracked(command_,error);
}
bool RecordingScope::Resolve(GfxCmdBufState* state,const GfxImage* destination,std::string& error) {
    const auto owner=target_bindings::Acquire(state);
    if(!state||!owner||!Bind(state,owner,error))return false;
    const auto dst=image_assets::Acquire(destination);
    if(!target_->color||!dst||!dst->uploads.empty()) {error="Native scene resolve requires actual generated colour images";return false;}
    // As for depth: an identical repeated resolve (neither image changed since
    // the last copy between them) is skipped along with its pass split.
    const Texture2D* source=target_->color->texture.get();const Texture2D* copied=dst->texture.get();
    const auto previous=depthCopies_.find(copied);
    if(previous!=depthCopies_.end()&&previous->second.source==source
        &&previous->second.sourceGeneration==DepthGeneration(source)
        &&previous->second.destinationGeneration==DepthGeneration(copied)&&image_usage::CanLoad(dst->texture))return true;
    // Optional (r_nativeLazyResolve): a destination nobody read during the
    // whole previous frame, nor so far this frame, needs no copy (nor its scene
    // pass split). A reader that appears sees an older copy for that frame;
    // every resolve copies again from the next frame on.
    if(g_lazyColorResolve.load(std::memory_order_relaxed)) {
        auto found=g_colorResolves.find(copied);
        if(found==g_colorResolves.end()||found->second.image.lock()!=dst->texture)
            g_colorResolves[copied]={dst->texture,dst->texture->Reads(),~0ull};
        else if(!found->second.previousFrameReads&&found->second.readsAtFrameStart==dst->texture->Reads()
            &&image_usage::CanLoad(dst->texture))return true;
    }
    if(!Finish(error))return false;
    // Retain the immutable owners before recording transfer commands. Frame
    // retirement releases them only after the real submission fence completes.
    arena_.copiedImages.push_back(target_->color);arena_.copiedImages.push_back(dst);
    if(!Copy(target_->color,dst,error))return false;
    MarkDepthWrite(copied);depthCopies_[copied]={source,DepthGeneration(source),DepthGeneration(copied)};return true;
}
bool RecordingScope::ResolveDepth(GfxCmdBufState* state,const GfxImage* destination,std::string& error) {
    const auto owner=target_bindings::Acquire(state);
    if(!state||!owner||!Bind(state,owner,error))return false;
    const auto dst=image_assets::Acquire(destination);
    if(!target_->depth||!dst||!dst->uploads.empty()||!(dst->description.aspects&VK_IMAGE_ASPECT_DEPTH_BIT)) {
        error="Native depth resolve needs actual generated depth images";return false;
    }
    // Repeating an identical copy is a no-op: skip it (and the pass split it
    // forces) when neither image's contents changed since that copy.
    const Texture2D* source=target_->depth->texture.get();const Texture2D* copied=dst->texture.get();
    // Optional (r_nativeDepthResolveOnce): later resolves in a frame keep the
    // first copy, avoiding a copy plus a scene pass split; depth written after
    // the first resolve is then absent from the sampled float Z.
    if(depthResolved_&&g_singleDepthResolve.load(std::memory_order_relaxed)&&image_usage::CanLoad(dst->texture))return true;
    const auto previous=depthCopies_.find(copied);
    if(previous!=depthCopies_.end()&&previous->second.source==source
        &&previous->second.sourceGeneration==DepthGeneration(source)
        &&previous->second.destinationGeneration==DepthGeneration(copied)&&image_usage::CanLoad(dst->texture))return true;
    if(!Finish(error))return false;
    arena_.copiedImages.push_back(target_->depth);arena_.copiedImages.push_back(dst);
    // Resolved scene depth is sampled only through depth-aspect views (the
    // INTZ float-Z consumer); its stencil plane has no reader, so copy depth.
    if(!Copy(target_->depth,dst,error,VK_IMAGE_ASPECT_DEPTH_BIT))return false;
    MarkDepthWrite(copied);depthCopies_[copied]={source,DepthGeneration(source),DepthGeneration(copied)};depthResolved_=true;return true;
}
bool RecordingScope::Copy(const std::shared_ptr<const image_assets::Record>& src,const std::shared_ptr<const image_assets::Record>& dst,std::string& error,VkImageAspectFlags copyAspects) {
    FlushPending(binds_);
    int profile=-1;
    if(profilePool_) {
        if(arena_.profile.size()<MaxProfilePasses) {
            PassProfile p;p.query=2+2*unsigned(arena_.profile.size());
            p.target=(src->description.aspects&VK_IMAGE_ASPECT_DEPTH_BIT)?-2:-1;
            p.width=src->description.extent.width;p.height=src->description.extent.height;
            p.color=src->createdName;p.depth=dst->createdName;
            profile=int(arena_.profile.size());arena_.profile.push_back(std::move(p));
            vkCmdWriteTimestamp(command_,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,profilePool_,arena_.profile.back().query);
        } else arena_.profileTruncated=true;
    }
    const bool result=image_usage::Copy(command_,src->texture,dst->texture,error,copyAspects);
    if(profile>=0)vkCmdWriteTimestamp(command_,VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,profilePool_,arena_.profile[profile].query+1);
    return result;
}
namespace {
// Render thread only. Contents are valid for `key` once a store was recorded;
// recording order (one queue) makes later restores observe it.
struct SunShadowCacheImage {std::shared_ptr<Texture2D> image;std::uint64_t key{};bool valid{};} g_sunShadowCache[2];
}
namespace {std::atomic<bool> g_nearShadowCache{false};std::atomic<bool> g_nearShadowDeclDedup{false};std::atomic<bool> g_sunBModelCache{true};std::atomic<bool> g_farBrushShadows{true};std::atomic<std::uint64_t> g_sunShadowCacheGeneration{1};}
void SetSunBModelCache(bool enabled){g_sunBModelCache.store(enabled,std::memory_order_relaxed);}
bool SunBModelCacheEnabled(){return g_sunBModelCache.load(std::memory_order_relaxed);}
void SetFarBrushShadows(bool enabled){g_farBrushShadows.store(enabled,std::memory_order_relaxed);}
bool FarBrushShadowsEnabled(){return g_farBrushShadows.load(std::memory_order_relaxed);}
std::uint64_t SunShadowCacheGeneration(){return g_sunShadowCacheGeneration.load(std::memory_order_relaxed);}
void SetNearShadowCache(bool enabled){g_nearShadowCache.store(enabled,std::memory_order_relaxed);}
bool NearShadowCacheEnabled(){return g_nearShadowCache.load(std::memory_order_relaxed);}
void SetNearShadowDeclDedup(bool enabled){g_nearShadowDeclDedup.store(enabled,std::memory_order_relaxed);}
bool NearShadowDeclDedupEnabled(){return g_nearShadowDeclDedup.load(std::memory_order_relaxed);}
void ReleaseNearShadowCache(){g_sunShadowCache[0]={};}
void ReleaseSunShadowCache(){g_sunShadowCache[0]={};g_sunShadowCache[1]={};g_sunShadowCacheGeneration.fetch_add(1,std::memory_order_relaxed);}
bool RecordingScope::StoreSunShadowCache(GfxCmdBufState* state,const VkRect2D& rect,std::uint64_t key,std::string& error,unsigned partition) {
    if(partition>=2){error="Invalid shadow cache partition";return false;}
    const auto owner=target_bindings::Acquire(state);
    if(!state||!owner||!Bind(state,owner,error))return false;
    if(!target_->depth){error="Native sun shadow cache needs a bound depth target";return false;}
    if(!Finish(error))return false;
    const auto& shadow=target_->depth->texture;auto& cache=g_sunShadowCache[partition];
    auto description=shadow->Description();
    description.extent={rect.extent.width,rect.extent.height,1};
    description.usage=VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    if(!cache.image||cache.image->Description().format!=description.format||cache.image->Description().aspects!=description.aspects
        ||cache.image->Description().extent.width!=description.extent.width||cache.image->Description().extent.height!=description.extent.height) {
        auto image=std::make_shared<Texture2D>(physical_,device_);
        if(!image->Create(description,error))return false;
        cache={std::move(image),0,false};
    }
    cache.valid=false;
    arena_.copiedImages.push_back(target_->depth);arena_.retainedTextures.push_back(cache.image);
    if(!image_usage::CopyRegion(command_,shadow,rect.offset,cache.image,{0,0},rect.extent,VK_IMAGE_ASPECT_DEPTH_BIT,error))return false;
    cache.key=key;cache.valid=true;return true;
}
bool RecordingScope::RestoreSunShadowCache(GfxCmdBufState* state,const VkRect2D& rect,std::uint64_t key,bool& restored,std::string& error,unsigned partition) {
    restored=false;
    if(partition>=2){error="Invalid shadow cache partition";return false;}
    const auto owner=target_bindings::Acquire(state);
    if(!state||!owner||!Bind(state,owner,error))return false;
    if(!target_->depth){error="Native sun shadow cache needs a bound depth target";return false;}
    auto& cache=g_sunShadowCache[partition];
    if(!cache.valid||cache.key!=key||!cache.image||cache.image->Description().extent.width!=rect.extent.width
        ||cache.image->Description().extent.height!=rect.extent.height
        ||cache.image->Description().format!=target_->depth->texture->Description().format
        ||cache.image->Description().aspects!=target_->depth->texture->Description().aspects)return true;
    if(!Finish(error))return false;
    arena_.copiedImages.push_back(target_->depth);arena_.retainedTextures.push_back(cache.image);
    if(!image_usage::CopyRegion(command_,cache.image,{0,0},target_->depth->texture,rect.offset,rect.extent,VK_IMAGE_ASPECT_DEPTH_BIT,error))return false;
    MarkDepthWrite(target_->depth->texture.get());restored=true;return true;
}
bool RecordingScope::Scissor(bool enabled,unsigned x,unsigned y,unsigned width,unsigned height,std::string& error) {
    const auto maximum=unsigned(std::numeric_limits<int>::max());
    if(x>maximum||y>maximum||width>maximum||height>maximum) {error="Native scissor coordinates exceed signed Vulkan range";return false;}
    scissorEnabled_=enabled;scissor_={{int(x),int(y)},{width,height}};
    // Scissor is recording-scope state, independent of a command-state owner.
    // The previous draw may have used a callback-local state that is now gone.
    // Every Draw binds its real owner and ApplyViewport emits this scissor
    // before drawing. Do not dereference the previous owner's viewport here.
    return true;
}
bool RecordingScope::Clear(unsigned mask,const float* color,float depth,unsigned stencil,const GfxViewport* viewport,std::string& error) {
    FlushPending(binds_);
    if(!mask||(mask&~7u)||!color||!std::isfinite(depth)||depth<0||depth>1||!target_||!state_){if(error.empty())error="Invalid native clear arguments";return false;}
    // Canonical shadow callbacks clear a D3D NULL dummy color surface too.
    // Native shadow targets omit that dummy; retain the actual depth clear.
    if((mask&3)==3&&!target_->color&&target_->depth&&state_
        &&(state_->renderTargetId==R_RENDERTARGET_SHADOWMAP_SUN||state_->renderTargetId==R_RENDERTARGET_SHADOWMAP_SPOT))mask&=~1u;
    if(((mask&1)&&!target_->color)||((mask&2)&&!target_->depth)||((mask&4)&&(!target_->depth||!(target_->depth->description.aspects&VK_IMAGE_ASPECT_STENCIL_BIT)))) {
        error="Native clear requests an absent attachment aspect";return false;}
    const auto& size=(target_->color?target_->color:target_->depth)->description.extent;
    GfxViewport region=viewport?*viewport:GfxViewport{0,0,int(size.width),int(size.height)};
    if(region.x<0||region.y<0||region.width<=0||region.height<=0||unsigned(region.x)>size.width||unsigned(region.y)>size.height||unsigned(region.width)>size.width-unsigned(region.x)||unsigned(region.height)>size.height-unsigned(region.y)) {
        error="Native clear rectangle exceeds target";return false;}
    // A depth/stencil clear must reopen a previously colour-only pass.
    if((mask&6)&&active_&&!active_->HasDepth()&&!Finish(error))return false;
    unsigned discardMask=0;
    if(!active_&&region.x==0&&region.y==0&&unsigned(region.width)==size.width&&unsigned(region.height)==size.height) {
        // A full first clear makes earlier contents unnecessary. Preserve any
        // uncleared stencil aspect; partial/late clears keep their draw command.
        // Untouched stencil still holds the zeros of its first clear, so a
        // CLEAR load op (stencil 0) reproduces it exactly instead of loading.
        discardMask=mask&1;
        if((mask&2)&&(!(target_->depth->description.aspects&VK_IMAGE_ASPECT_STENCIL_BIT)||(mask&4)
            ||!target_->depth->texture->StencilTouched()))discardMask|=mask&6;
    }
    // Non-zero stencil contents end the untouched-stencil guarantee (above).
    if((mask&4)&&stencil)target_->depth->texture->MarkStencilTouched();
    if(!Ensure(error,discardMask,color,depth,stencil))return false;
    mask&=~discardMask;if(!mask)return true;
    VkClearAttachment attachments[2]{};unsigned count=0;
    if(mask&1){auto& a=attachments[count++];a.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT;for(unsigned i=0;i<4;++i)a.clearValue.color.float32[i]=color[i];}
    if(mask&6){auto& a=attachments[count++];a.aspectMask=(mask&2?VK_IMAGE_ASPECT_DEPTH_BIT:0)|(mask&4?VK_IMAGE_ASPECT_STENCIL_BIT:0);a.clearValue.depthStencil={depth,stencil};}
    VkClearRect rect{{{region.x,region.y},{unsigned(region.width),unsigned(region.height)}},0,1};vkCmdClearAttachments(command_,count,attachments,1,&rect);
    if(mask&1)MarkDepthWrite(target_->color->texture.get());
    if(mask&6)MarkDepthWrite(target_->depth->texture.get());return true;
}
bool RecordingScope::NativeFullscreen(GfxCmdBufState* state,native_fullscreen::Program program,const native_fullscreen::Input* inputs,
    unsigned count,const void* parameters,unsigned bytes,std::string& error) {
    if(!fullscreen_||!storage_||!inputs||count!=native_fullscreen::ImageCount(program)||count>8||!parameters||!bytes) {
        error="Native fullscreen effect has no library/storage or invalid inputs";return false;
    }
    const auto owner=target_bindings::Acquire(state);
    if(!state||!owner||!Bind(state,owner,error))return false;
    if(!target_->color) {error="Native fullscreen effect needs a colour target";return false;}
    std::array<VkDescriptorImageInfo,8> images{};
    for(unsigned i=0;i<count;++i) {
        const auto record=image_assets::Acquire(inputs[i].image);
        if(!record||!record->texture||record->texture==target_->color->texture) {error="Native fullscreen input missing or aliases its target";return false;}
        VkImageLayout layout{};if(!image_usage::Sample(record->texture,layout,error))return false;
        images[i]={fullscreen_->Sampler(inputs[i].point),record->texture->View(),layout};
        arena_.copiedImages.push_back(record); // retained until the frame fence
    }
    FlushPending(binds_);
    // The effect overwrites every texel: start a fresh colour-only pass whose
    // CLEAR load op replaces loading the previous contents.
    if(!Finish(error))return false;
    static const float zero[4]{};
    if(!Ensure(error,1u,zero,1,0,true))return false;
    VkPipeline pipeline{};VkPipelineLayout layout{};VkDescriptorSetLayout setLayout{};
    if(!fullscreen_->Acquire(program,active_->PipelineRenderPass(),pipeline,layout,setLayout,error))return false;
    DrawStorage::Allocation allocation;
    if(!storage_->Allocate(bytes,alignment_,allocation,error)||!allocation.buffer->Write(allocation.offset,parameters,bytes,error))return false;
    VkDescriptorSet set{};VkDescriptorSetAllocateInfo allocate{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocate.descriptorPool=allocation.descriptors;allocate.descriptorSetCount=1;allocate.pSetLayouts=&setLayout;
    if(vkAllocateDescriptorSets(device_,&allocate,&set)!=VK_SUCCESS) {error="Native fullscreen descriptor allocation failed";return false;}
    VkWriteDescriptorSet writes[9]{};const VkDescriptorBufferInfo buffer{allocation.buffer->Handle(),allocation.offset,bytes};
    for(unsigned i=0;i<count;++i) {
        auto& w=writes[i];w.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;w.dstSet=set;w.dstBinding=i;w.descriptorCount=1;
        w.descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;w.pImageInfo=&images[i];
    }
    auto& w=writes[count];w.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;w.dstSet=set;w.dstBinding=8;w.descriptorCount=1;
    w.descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;w.pBufferInfo=&buffer;
    vkUpdateDescriptorSets(device_,count+1,writes,0,nullptr);
    const auto& extent=target_->color->description.extent;
    const VkViewport viewport{0,0,float(extent.width),float(extent.height),0,1};const VkRect2D scissor{{0,0},{extent.width,extent.height}};
    vkCmdBindPipeline(command_,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline);
    vkCmdBindDescriptorSets(command_,VK_PIPELINE_BIND_POINT_GRAPHICS,layout,0,1,&set,0,nullptr);
    vkCmdSetViewport(command_,0,1,&viewport);vkCmdSetScissor(command_,0,1,&scissor);
    vkCmdDraw(command_,3,1,0,0);++binds_.fullscreenCommands;MarkDepthWrite(target_->color->texture.get());
    // Engine draws must rebind their pipeline and sets; viewport is re-applied per draw.
    binds_.pipeline=VK_NULL_HANDLE;binds_.layout=VK_NULL_HANDLE;binds_.sets={};binds_.offsetCount=~0u;binds_.last.valid=false;binds_.viewportValid=false;
    return true;
}
bool RecordingScope::Draw(const GfxCmdBufPrimState* state,int base,int vertices,int triangles,std::string& error) {
    auto owner=target_bindings::Owner(state);
    if(!owner||!Bind(const_cast<GfxCmdBufState*>(owner),target_bindings::Acquire(owner),error))return false;
    unsigned words[2]={state_->activeStateBits[0],state_->activeStateBits[1]};
    // Only the explicitly scoped main-lit opaque base pass can replace its
    // omitted prepass. Cloak, FloatZ and all other draw scopes stay canonical.
    const bool forward=forward_opaque::Apply(state_,words);
    // Only omit depth when the canonical draw disables every use of it. Keep
    // an already-open depth pass to avoid splitting geometry around a quad.
    const bool omitDepth=target_->color&&(words[1]&0xf3u)==2u;
    if(active_&&!active_->HasDepth()&&!omitDepth&&target_->depth&&!Finish(error))return false;
    if(!state_||&state_->prim!=state||!Ensure(error,0,nullptr,1,0,omitDepth)||!ApplyViewport(error)){if(error.empty())error="Native material draw has no matching selected target";return false;}
    material_dispatch::Pass pass;pass.handle=active_->RenderPass();pass.pipelineHandle=active_->PipelineRenderPass();pass.color=bool(target_->color);pass.depth=active_->HasDepth();pass.stencil=pass.depth&&(target_->depth->description.aspects&VK_IMAGE_ASPECT_STENCIL_BIT);
    pass.depthFormat=pass.depth?target_->depth->description.format:VK_FORMAT_UNDEFINED;
    pass.keyed=active_->PassKey(pass.key);
    // Effective depth-write (bit 0) or any stencil use (bit 6) may change the
    // attachment's depth/stencil contents; material_state decodes the same bits.
    if(pass.depth&&(words[1]&0x41u)) {
        MarkDepthWrite(target_->depth->texture.get());
        if(depthResolved_) {
            ++postResolveDepthWrites_;
            const char* name=state_->material?state_->material->info.name:"?";
            if(postResolveDepthWriters_.size()<12&&std::find(postResolveDepthWriters_.begin(),postResolveDepthWriters_.end(),name)==postResolveDepthWriters_.end())
                postResolveDepthWriters_.push_back(name?name:"?");
        }
    }
    if(pass.color)MarkDepthWrite(target_->color->texture.get()); // any colour draw may change it
    if(pass.stencil&&(words[1]&0x40u))target_->depth->texture->MarkStencilTouched();
    material_dispatch::RecordingScope recording(state_,command_,physical_,pass,cache_,uniforms_,pool_,cursor_,alignment_,packets_,storage_,storage_?&binds_:nullptr,forward?words:nullptr);
    return material_dispatch::DrawIndexed(state,base,vertices,triangles,error);
}
}
