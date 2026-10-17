#pragma once
#include "resources.h"
#include <memory>
#include <array>
#include <vector>
namespace bo1::vulkan {
// Device/render-thread owner of immutable render-pass definitions only. No
// framebuffer/image is retained here. Exact formats/load/store/layout policies
// share one handle; identities live until a drained Clear, never handle recycling.
struct SharedAttachmentRenderPass {
    VkDevice device{};VkRenderPass handle{};
    std::array<unsigned,8> key{}; // AttachmentPassCache definition key
    // Earlier canonical compatible definition, never self-owned. Retain it even
    // when a pass outlives the cache so pipeline creation cannot see a dead handle.
    std::shared_ptr<SharedAttachmentRenderPass> pipelinePass;
    VkRenderPass PipelineHandle()const{return pipelinePass?pipelinePass->handle:handle;}
    ~SharedAttachmentRenderPass();
};
// Reference-counted framebuffer: destroyed when the cache and every pass that
// recorded it (retained until its frame fence) have released it.
struct SharedFramebuffer {
    VkDevice device{};VkFramebuffer handle{};
    ~SharedFramebuffer();
};
class AttachmentPassCache {
public:
    explicit AttachmentPassCache(VkDevice device):device_(device){}
    std::shared_ptr<SharedAttachmentRenderPass> Acquire(VkDevice,const std::array<unsigned,8>&,const VkRenderPassCreateInfo&,std::string&);
    // Defines the pass AttachmentPass::Create would make for this key (used to
    // rebuild recorded programs before their pass is first used).
    std::shared_ptr<SharedAttachmentRenderPass> AcquireFromKey(VkDevice,const std::array<unsigned,8>&,std::string&);
    void Clear(){entries_.clear();framebuffers_.clear();} // Caller clears program keys at a drained boundary.
    std::size_t Size()const{return entries_.size();}
    // Framebuffers are reused per (render pass, attachment textures). Entries hold
    // weak texture owners, so a recreated image never matches a stale entry.
    std::shared_ptr<SharedFramebuffer> Framebuffer(VkRenderPass,const std::shared_ptr<Texture2D>& color,
        const std::shared_ptr<Texture2D>& depth,const VkFramebufferCreateInfo&,std::string&);
private:
    VkDevice device_{};
    struct Entry {std::array<unsigned,8> key;std::shared_ptr<SharedAttachmentRenderPass> pass;std::vector<std::uint32_t> compatibility;};
    std::vector<Entry> entries_;
    struct FramebufferEntry {VkRenderPass pass{};std::weak_ptr<Texture2D> color,depth;const Texture2D* colorKey{};const Texture2D* depthKey{};std::shared_ptr<SharedFramebuffer> framebuffer;};
    std::vector<FramebufferEntry> framebuffers_;unsigned lookups_{};
};
// Clear-pass owner for color and/or depth. Shared images and this framebuffer
// must survive submitted users. Further scene passes need explicit load policy.
class AttachmentPass {
public:
    explicit AttachmentPass(VkDevice device,AttachmentPassCache* cache=nullptr):device_(device),cache_(cache){}
    ~AttachmentPass();AttachmentPass(const AttachmentPass&)=delete;AttachmentPass& operator=(const AttachmentPass&)=delete;
    bool Create(std::shared_ptr<Texture2D> color,std::shared_ptr<Texture2D> depth,bool preserveDepth,std::string&,bool loadExisting=false,int depthLoad=-1);
    // contents: SECONDARY_COMMAND_BUFFERS when the pass body is recorded into
    // secondary command buffers (parallel scene recording).
    bool BeginTracked(VkCommandBuffer,const VkClearColorValue&,std::string&,float depth=1,unsigned stencil=0,
        VkSubpassContents contents=VK_SUBPASS_CONTENTS_INLINE)const;
    bool EndTracked(VkCommandBuffer,std::string&)const;
    void Begin(VkCommandBuffer,const VkClearColorValue&,float depth=1,unsigned stencil=0,VkSubpassContents contents=VK_SUBPASS_CONTENTS_INLINE)const;
    void End(VkCommandBuffer command)const{vkCmdEndRenderPass(command);}
    VkRenderPass RenderPass()const{return pass_;}
    VkFramebuffer Framebuffer()const{return framebuffer_;}
    // Pipeline compilation/cache only; Begin/End and framebuffer retain the
    // actual definition, including its clear/load/store/layout semantics.
    VkRenderPass PipelineRenderPass()const{return sharedPass_?sharedPass_->PipelineHandle():pass_;}
    bool PassKey(std::array<unsigned,8>& key)const{if(!sharedPass_)return false;key=sharedPass_->key;return true;}
    bool HasDepth()const{return bool(depth_);}
    bool SharedRenderPass()const{return bool(sharedPass_);}
private:
    AttachmentPassCache* cache_{};std::shared_ptr<SharedAttachmentRenderPass> sharedPass_;std::shared_ptr<SharedFramebuffer> sharedFramebuffer_;
    bool loadExisting_{},loadDepth_{},preserveDepth_{};
    VkDevice device_{};VkRenderPass pass_{};VkFramebuffer framebuffer_{};
    std::shared_ptr<Texture2D> color_,depth_;VkExtent2D extent_{};
};
}
