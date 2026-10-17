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
    std::uint64_t indexedCommands{},triangles{},mergedRanges{},fullscreenCommands{};
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
// Parallel scene recording (r_nativeParallelRecord): one lane per recording
// thread. Each holds that thread's secondary command buffers, constant/
// descriptor pages, packets and pass retention for one frame slot. The frame
// owner resets a lane only after the slot's submission fence retired.
struct ParallelLane {
    VkCommandPool pool{};std::vector<VkCommandBuffer> buffers;unsigned used{};
    std::unique_ptr<DrawStorage> storage;std::vector<material_dispatch::Packet> packets;Arena arena;
    BindCache binds; // frame-scoped sets/resolves shared by this lane's chunks
    VkDeviceSize cursor{};
    // Set once the slot's fence retired: the owning recording thread frees the
    // previous use (packets, passes, sets, command buffers) before reusing it,
    // keeping that work off the render thread. Storage resets stay with the frame.
    bool retirePending{};
    bool Acquire(VkDevice,VkCommandBuffer&,std::string&);
    bool Retire(VkDevice,ProgramCache&,std::string&);
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
    // Independent sun shadow partition caches: persistent backend images holding
    // static casters' depth for an exact projection key. Partition defaults to far.
    // copies that rectangle of the bound depth target into the cache; restore
    // copies it back when the cache holds the same key (restored=false if not).
    bool StoreSunShadowCache(GfxCmdBufState*,const VkRect2D&,std::uint64_t key,std::string&,unsigned partition=1);
    bool RestoreSunShadowCache(GfxCmdBufState*,const VkRect2D&,std::uint64_t key,bool& restored,std::string&,unsigned partition=1);
    // Records a merged pending draw before a command recorded outside this scope.
    void FlushPendingDraw(){FlushPending(binds_);}
    // The command buffer receiving this scope's commands now (a secondary
    // inside a parallel-hosting pass, otherwise the frame command buffer).
    VkCommandBuffer Command()const{return command_;}
    // Opens the selected target's pass before an occlusion query begins, so
    // the query and the draws it brackets share one command buffer (a
    // secondary when the pass hosts parallel recording).
    bool OpenForQuery(std::string& error){if(active_||chunk_||!target_||!state_)return true;return Ensure(error);}
    // Parallel scene recording. Lanes[0] belongs to this (recording) thread.
    void SetParallelLanes(ParallelLane* lanes,unsigned count){lanes_=lanes;laneCount_=count;}
    unsigned ParallelLaneCount()const{return laneCount_;}
    ParallelLane* Lane(unsigned index)const{return index<laneCount_?&lanes_[index]:nullptr;}
    // Opens the pass `state` draws into with its body in secondary command
    // buffers and closes this thread's current one. ready=false without an
    // error when that is impossible now without losing attachment contents;
    // the caller then records serially (the pass is preferred next frame).
    bool BeginParallel(GfxCmdBufState*,bool& ready,std::string&);
    // Executes the chunks' secondaries in list order and resumes recording.
    bool EndParallel(const std::vector<VkCommandBuffer>& ordered,std::string&);
    // Chunk scope (any thread): records one contiguous part of a draw list into
    // a new secondary of `lane` inside the parent's open pass. It can neither
    // switch targets nor open/end passes. Use between BeginParallel/EndParallel.
    RecordingScope(const RecordingScope& parent,ParallelLane& lane);
    bool BeginChunk(GfxCmdBufState*,std::string&);
    VkCommandBuffer ChunkCommand()const{return chunk_?command_:VK_NULL_HANDLE;}
    bool Finish(std::string&);
private:
    bool Copy(const std::shared_ptr<const image_assets::Record>&,const std::shared_ptr<const image_assets::Record>&,std::string&,VkImageAspectFlags copyAspects=0);
    bool Ensure(std::string&,unsigned discardMask=0,const float* clearColor=nullptr,float clearDepth=1,unsigned clearStencil=0,bool omitDepth=false);bool ApplyViewport(std::string&);
    bool StartMainSecondary(std::string&);bool CloseMainSecondary(std::string&);
    ParallelLane* lanes_{};unsigned laneCount_{};VkCommandBuffer primary_{};
    bool secondaryPass_{}; // active_ was begun with SECONDARY_COMMAND_BUFFERS contents
    std::vector<VkCommandBuffer> executes_; // ordered secondaries of the active pass
    const RecordingScope* parent_{};bool chunk_{},chunkEnded_{};ParallelLane* lane_{};
    VkQueryPool profilePool_{};int activeProfile_{-1};std::size_t profileDrawStart_{};
    std::uint64_t profileIndexedStart_{},profileTriangleStart_{},profileMergedStart_{},profileFullscreenStart_{};
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
    BindCache ownBinds_;BindCache& binds_; // this frame's command buffer, or the chunk's lane
    bool depthResolved_{};unsigned postResolveDepthWrites_{};std::vector<std::string> postResolveDepthWriters_; // diagnostic
    native_fullscreen::Library* fullscreen_{};
    void MarkDepthWrite(const Texture2D* image){if(image)depthGenerations_[image]=++depthSerial_;}
    std::uint64_t DepthGeneration(const Texture2D* image)const{auto it=depthGenerations_.find(image);return it==depthGenerations_.end()?0:it->second;}
    AttachmentPassCache* passCache_{};DrawStorage* storage_{};RecordingScope* previous_;
};
RecordingScope* Current();
// Render thread, before device teardown; in-flight frames retain the image.
void ReleaseSunShadowCache();
void ReleaseNearShadowCache();
void SetNearShadowCache(bool enabled);
bool NearShadowCacheEnabled();
void SetNearShadowDeclDedup(bool enabled);
bool NearShadowDeclDedupEnabled();
// Parallel scene recording (r_nativeParallelRecord): frames offer their lanes
// to the recording scope only while enabled (any thread may set it).
constexpr unsigned MaxParallelLanes=4;
void SetParallelRecord(bool enabled);bool ParallelRecordEnabled();
// Per-frame policy read by ResolveDepth (any thread may set it).
void SetSingleDepthResolve(bool enabled);
void SetLazyColorResolve(bool enabled);
void SetConstantUploadReuse(bool enabled);
void SetForwardOpaque(bool enabled);
void SetSunBModelCache(bool enabled);
bool SunBModelCacheEnabled();
void SetFarBrushShadows(bool enabled);
bool FarBrushShadowsEnabled();
std::uint64_t SunShadowCacheGeneration();
}
