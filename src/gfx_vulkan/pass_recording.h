#pragma once
#include "attachment_pass.h"
#include "pointer_hash.h"
#include "render_targets.h"
#include "material_dispatch.h"
#include "native_fullscreen.h"
#include <cstdint>
#include <unordered_map>
struct GfxViewport;struct GfxCmdBufState;
namespace bo1::vulkan::pass_recording {
// Optional sampled profiling: timestamps bracket existing pass boundaries only.
// Query slots 0/1 belong to the enclosing scene. No extra GPU waits/barriers.
constexpr unsigned MaxProfilePasses=128;
constexpr unsigned ProfileQueryCount=2+2*MaxProfilePasses;
struct PassProfile {
    unsigned query{},width{},height{},draws{};int target{};
    bool loadColor{},loadDepth{};std::string color,depth;
};
struct Arena {
    std::vector<PassProfile> profile;bool profileTruncated{};
    std::vector<std::shared_ptr<AttachmentPass>> passes;
    std::vector<std::shared_ptr<const image_assets::Record>> copiedImages;
    std::vector<std::shared_ptr<Texture2D>> retainedTextures; // backend-owned copy targets
    // Caller retires the submission fence first. Shared definitions keep stable
    // keys; private definitions drop keys before their handles can be recycled.
    void Retire(ProgramCache& cache){for(auto& pass:passes)if(!pass->SharedRenderPass())cache.EraseRenderPass(pass->RenderPass());passes.clear();copiedImages.clear();retainedTextures.clear();profile.clear();profileTruncated=false;}
};
class RecordingScope {
public:
    RecordingScope(VkCommandBuffer,VkPhysicalDevice,VkDevice,ProgramCache&,Buffer&,VkDescriptorPool,
        VkDeviceSize& cursor,VkDeviceSize alignment,std::vector<material_dispatch::Packet>&,Arena&,DrawStorage* storage=nullptr,AttachmentPassCache* passCache=nullptr,VkQueryPool profilePool=VK_NULL_HANDLE);
    ~RecordingScope();RecordingScope(const RecordingScope&)=delete;RecordingScope& operator=(const RecordingScope&)=delete;
    bool Bind(GfxCmdBufState*,const GfxRenderTarget*,std::string&);
    bool Viewport(GfxCmdBufState*,const GfxViewport&,std::string&);
    bool InitializeColor(const GfxImage*,const VkClearColorValue&,std::string&);
    bool Resolve(GfxCmdBufState*,const GfxImage*,std::string&);
    bool ResolveDepth(GfxCmdBufState*,const GfxImage*,std::string&);
    bool Scissor(bool enabled,unsigned x,unsigned y,unsigned width,unsigned height,std::string&);
    bool Clear(unsigned,const float*,float,unsigned,const GfxViewport*,std::string&);
    bool Draw(const GfxCmdBufPrimState*,int,int,int,std::string&);
    // Native effect writing every texel of the state's selected colour target.
    // Inputs must be other generated images with preserved, ordered contents.
    bool NativeFullscreen(GfxCmdBufState*,native_fullscreen::Program,const native_fullscreen::Input*,unsigned count,
        const void* parameters,unsigned bytes,std::string&);
    void SetFullscreenLibrary(native_fullscreen::Library* library){fullscreen_=library;}
    // Sun shadow far-partition cache: a persistent backend image holding the
    // static casters' depth for one world-locked far projection (key). Store
    // copies that rectangle of the bound depth target into the cache; restore
    // copies it back when the cache holds the same key (restored=false if not).
    bool StoreSunShadowCache(GfxCmdBufState*,const VkRect2D&,std::uint64_t key,std::string&);
    bool RestoreSunShadowCache(GfxCmdBufState*,const VkRect2D&,std::uint64_t key,bool& restored,std::string&);
    // Records a merged pending draw before a command recorded outside this scope.
    void FlushPendingDraw(){FlushPending(binds_);}
    bool Finish(std::string&);
private:
    bool Copy(const std::shared_ptr<const image_assets::Record>&,const std::shared_ptr<const image_assets::Record>&,std::string&,VkImageAspectFlags copyAspects=0);
    bool Ensure(std::string&,unsigned discardMask=0,const float* clearColor=nullptr,float clearDepth=1,unsigned clearStencil=0,bool omitDepth=false);bool ApplyViewport(std::string&);
    VkQueryPool profilePool_{};int activeProfile_{-1};std::size_t profileDrawStart_{};
    VkCommandBuffer command_;VkPhysicalDevice physical_;VkDevice device_;ProgramCache& cache_;Buffer& uniforms_;VkDescriptorPool pool_;
    VkDeviceSize& cursor_;VkDeviceSize alignment_;std::vector<material_dispatch::Packet>& packets_;Arena& arena_;
    GfxCmdBufState* state_{};std::shared_ptr<const render_targets::Record> target_;std::shared_ptr<AttachmentPass> active_;
    bool scissorEnabled_{};VkRect2D scissor_{};
    // Content generations of attachment images within this frame recording
    // (depth and colour). A resolve repeats only when its source or destination
    // changed since the last identical copy; any recorded write advances it.
    struct DepthCopy {const Texture2D* source{};std::uint64_t sourceGeneration{},destinationGeneration{};};
    std::unordered_map<const Texture2D*,std::uint64_t,bo1::vulkan::PointerHash> depthGenerations_;
    std::unordered_map<const Texture2D*,DepthCopy,bo1::vulkan::PointerHash> depthCopies_;
    std::uint64_t depthSerial_{};
    BindCache binds_; // this frame's command buffer only
    native_fullscreen::Library* fullscreen_{};
    void MarkDepthWrite(const Texture2D* image){if(image)depthGenerations_[image]=++depthSerial_;}
    std::uint64_t DepthGeneration(const Texture2D* image)const{auto it=depthGenerations_.find(image);return it==depthGenerations_.end()?0:it->second;}
    AttachmentPassCache* passCache_{};DrawStorage* storage_{};RecordingScope* previous_;
};
RecordingScope* Current();
// Render thread, before device teardown; in-flight frames retain the image.
void ReleaseSunShadowCache();
}
