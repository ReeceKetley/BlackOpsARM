#include "pass_recording.h"
#include "image_usage.h"
#include "target_bindings.h"
#include "gfx_d3d/rb_state.h"
#include "gfx_d3d/r_rendertarget.h"
#include <cmath>
#include <cstring>
#include <array>
#include <algorithm>
#include <limits>
#include <cstdlib>
namespace bo1::vulkan::pass_recording {
namespace {thread_local RecordingScope* current{};}
RecordingScope* Current(){return current;}
RecordingScope::RecordingScope(VkCommandBuffer c,VkPhysicalDevice p,VkDevice d,ProgramCache& cache,Buffer& uniforms,VkDescriptorPool pool,
    VkDeviceSize& cursor,VkDeviceSize alignment,std::vector<material_dispatch::Packet>& packets,Arena& arena,DrawStorage* storage,AttachmentPassCache* passCache,VkQueryPool profilePool)
    :command_(c),physical_(p),device_(d),cache_(cache),uniforms_(uniforms),pool_(pool),cursor_(cursor),alignment_(alignment),packets_(packets),arena_(arena),passCache_(passCache),storage_(storage),previous_(current){profilePool_=profilePool;current=this;}
RecordingScope::~RecordingScope(){std::string error;if(!Finish(error))std::abort();current=previous_;}
bool RecordingScope::Finish(std::string& error){
    FlushPending(binds_);
    if(active_&&!active_->EndTracked(command_,error))return false;
    if(activeProfile_>=0) {
        auto& p=arena_.profile[activeProfile_];p.draws=unsigned(packets_.size()-profileDrawStart_);
        vkCmdWriteTimestamp(command_,VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,profilePool_,p.query+1);
        activeProfile_=-1;
    }
    active_.reset();return true;
}
bool RecordingScope::Bind(GfxCmdBufState* state,const GfxRenderTarget* target,std::string& error) {
    if(!target_bindings::Select(state,target,error))return false;
    auto record=render_targets::Acquire(target);if(!state||!record||(!record->color&&!record->depth)){error="Native target switch has no real attachments";return false;}
    if(record!=target_){if(!Finish(error))return false;target_=record;}
    state_=state;return true;
}
bool RecordingScope::Ensure(std::string& error,unsigned discardMask,const float* clearColor,float clearDepth,unsigned clearStencil,bool omitDepth) {
    if(active_)return true;if(!target_||!state_){error="Native pass has no selected target";return false;}
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
            vkCmdWriteTimestamp(command_,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,profilePool_,arena_.profile.back().query);
        } else arena_.profileTruncated=true;
    }
    VkClearColorValue value{{0,0,0,0}};if((discardMask&1)&&clearColor)for(unsigned i=0;i<4;++i)value.float32[i]=clearColor[i];
    if(!pass->BeginTracked(command_,value,error,(discardMask&2)?clearDepth:1.0f,(discardMask&4)?clearStencil:0))return false;
    if(depth&&!loadDepth)MarkDepthWrite(depth.get()); // CLEAR load op replaces contents
    if(color&&!loadColor)MarkDepthWrite(color.get());
    arena_.passes.push_back(pass);active_=std::move(pass);return true;
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
    const auto previous=depthCopies_.find(copied);
    if(previous!=depthCopies_.end()&&previous->second.source==source
        &&previous->second.sourceGeneration==DepthGeneration(source)
        &&previous->second.destinationGeneration==DepthGeneration(copied)&&image_usage::CanLoad(dst->texture))return true;
    if(!Finish(error))return false;
    arena_.copiedImages.push_back(target_->depth);arena_.copiedImages.push_back(dst);
    // Resolved scene depth is sampled only through depth-aspect views (the
    // INTZ float-Z consumer); its stencil plane has no reader, so copy depth.
    if(!Copy(target_->depth,dst,error,VK_IMAGE_ASPECT_DEPTH_BIT))return false;
    MarkDepthWrite(copied);depthCopies_[copied]={source,DepthGeneration(source),DepthGeneration(copied)};return true;
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
struct SunShadowCacheImage {std::shared_ptr<Texture2D> image;std::uint64_t key{};bool valid{};} g_sunShadowCache;
}
void ReleaseSunShadowCache(){g_sunShadowCache={};}
bool RecordingScope::StoreSunShadowCache(GfxCmdBufState* state,const VkRect2D& rect,std::uint64_t key,std::string& error) {
    const auto owner=target_bindings::Acquire(state);
    if(!state||!owner||!Bind(state,owner,error))return false;
    if(!target_->depth){error="Native sun shadow cache needs a bound depth target";return false;}
    if(!Finish(error))return false;
    const auto& shadow=target_->depth->texture;auto& cache=g_sunShadowCache;
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
bool RecordingScope::RestoreSunShadowCache(GfxCmdBufState* state,const VkRect2D& rect,std::uint64_t key,bool& restored,std::string& error) {
    restored=false;
    const auto owner=target_bindings::Acquire(state);
    if(!state||!owner||!Bind(state,owner,error))return false;
    if(!target_->depth){error="Native sun shadow cache needs a bound depth target";return false;}
    auto& cache=g_sunShadowCache;
    if(!cache.valid||cache.key!=key||!cache.image||cache.image->Description().extent.width!=rect.extent.width
        ||cache.image->Description().extent.height!=rect.extent.height)return true;
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
    if(!fullscreen_->Acquire(program,active_->RenderPass(),pipeline,layout,setLayout,error))return false;
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
    vkCmdDraw(command_,3,1,0,0);MarkDepthWrite(target_->color->texture.get());
    // Engine draws must rebind their pipeline and sets; viewport is re-applied per draw.
    binds_.pipeline=VK_NULL_HANDLE;binds_.layout=VK_NULL_HANDLE;binds_.sets={};binds_.offsetCount=~0u;binds_.last.valid=false;binds_.viewportValid=false;
    return true;
}
bool RecordingScope::Draw(const GfxCmdBufPrimState* state,int base,int vertices,int triangles,std::string& error) {
    auto owner=target_bindings::Owner(state);
    if(!owner||!Bind(const_cast<GfxCmdBufState*>(owner),target_bindings::Acquire(owner),error))return false;
    // Only omit depth when the canonical draw disables every use of it. Keep
    // an already-open depth pass to avoid splitting geometry around a quad.
    const bool omitDepth=target_->color&&(state_->activeStateBits[1]&0xf3u)==2u;
    if(active_&&!active_->HasDepth()&&!omitDepth&&target_->depth&&!Finish(error))return false;
    if(!state_||&state_->prim!=state||!Ensure(error,0,nullptr,1,0,omitDepth)||!ApplyViewport(error)){if(error.empty())error="Native material draw has no matching selected target";return false;}
    material_dispatch::Pass pass;pass.handle=active_->RenderPass();pass.color=bool(target_->color);pass.depth=active_->HasDepth();pass.stencil=pass.depth&&(target_->depth->description.aspects&VK_IMAGE_ASPECT_STENCIL_BIT);
    pass.depthFormat=pass.depth?target_->depth->description.format:VK_FORMAT_UNDEFINED;
    // Canonical depth-write (bit 0) or any stencil use (bit 6) may change the
    // attachment's depth/stencil contents; material_state decodes the same bits.
    if(pass.depth&&(state_->activeStateBits[1]&0x41u))MarkDepthWrite(target_->depth->texture.get());
    if(pass.color)MarkDepthWrite(target_->color->texture.get()); // any colour draw may change it
    if(pass.stencil&&(state_->activeStateBits[1]&0x40u))target_->depth->texture->MarkStencilTouched();
    material_dispatch::RecordingScope recording(state_,command_,physical_,pass,cache_,uniforms_,pool_,cursor_,alignment_,packets_,storage_,storage_?&binds_:nullptr);
    return material_dispatch::DrawIndexed(state,base,vertices,triangles,error);
}
}
