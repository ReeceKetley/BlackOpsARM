#pragma once
#include "resource_scratch.h"
#include "material_bindings.h"
#include "program_cache.h"
namespace bo1::vulkan::material_dispatch {
struct Packet {
    // Uniform bytes are already copied into fence-retained DrawStorage.
    // Only GPU-referenced resource owners must outlive command recording.
    std::shared_ptr<const texture_bindings::Snapshot> textures;
    std::shared_ptr<const geometry_bindings::Snapshot> geometry;
    std::shared_ptr<GraphicsProgram> program;
    std::array<VkDescriptorSet,2> sets{};
};
struct Pass {
    VkRenderPass handle{};unsigned subpass{};
    VkSampleCountFlagBits samples=VK_SAMPLE_COUNT_1_BIT;
    bool color=true,depth=false,stencil=false;
    VkFormat depthFormat=VK_FORMAT_UNDEFINED;
    VkRenderPass pipelineHandle{}; // optional retained compatible definition
    std::array<unsigned,8> key{};bool keyed{}; // AttachmentPassCache key (program recipes)
};
// Scope owns no GPU objects. The frame owner reserves its uniform buffer/pool
// and retains packets until the submission fence completes. Reset only after
// retirement. The caller begins the compatible pass and sets viewport/scissor.
class RecordingScope {
public:
    RecordingScope(const GfxCmdBufState*,VkCommandBuffer,VkPhysicalDevice,Pass,
        ProgramCache&,Buffer&,VkDescriptorPool,VkDeviceSize& cursor,VkDeviceSize alignment,
        std::vector<Packet>&,DrawStorage* storage=nullptr,BindCache* binds=nullptr,const unsigned* effectiveWords=nullptr);
    ~RecordingScope();
    RecordingScope(const RecordingScope&)=delete;
    RecordingScope& operator=(const RecordingScope&)=delete;
    const GfxCmdBufState* state;VkCommandBuffer command;VkPhysicalDevice physical;Pass pass;
    ProgramCache& cache;Buffer& uniforms;VkDescriptorPool pool;
    VkDeviceSize& cursor;VkDeviceSize alignment;std::vector<Packet>& packets;
    DrawStorage* storage;BindCache* binds;
    std::array<unsigned,2> effectiveWords{};bool overrideState{};
private:RecordingScope* previous_;
};
struct Profile {
    std::uint64_t capture{},resolve{},state{},program{},upload{},descriptors{},geometry{};
    unsigned draws{},reused{}; // reused: draws issued with the previous draw's bound state
    // First reason a draw could not reuse the previous draw's state.
    unsigned missState{},missSelection{},missConstants{},missTextures{},missGeometry{};
    std::uint64_t cachedGroups{},cachedModels{},cachedIndices{},cachedUploads{};
    std::uint64_t programReuse{},fullPrepares{},weakConstants{},strongConstants{};
    std::uint64_t forwardDraws{};
    std::uint64_t geometryOffsetMisses{},geometryPacketHits{},geometryBufferMisses{};
    std::uint64_t geometryArenaRetentions{},geometryArenaGrowths{},geometryRetainedDraws{};
};
// timed: record per-phase clock timings for this frame (draw counts always).
void ResetProfile(bool timed=false);Profile ReadProfile();
// Parallel scene recording: profiles are per thread. A worker starts with the
// recording thread's timing choice and its counts are added after the join.
bool ProfileTimed();void MergeProfile(const Profile&);
void SetResourceScratchReuse(bool enabled);bool ResourceScratchReuseEnabled();
ResourceScratchProfile ReadResourceScratchProfile();
void SetCachedGather(bool enabled);bool CachedGatherEnabled();
void SetConstantSnapshotReuse(bool enabled);bool ConstantSnapshotReuseEnabled();
void SetGeometryPacketReuse(bool enabled);bool GeometryPacketReuseEnabled();
void SetGeometryRetentionReuse(bool enabled);bool GeometryRetentionReuseEnabled();
void SetPreparedDrawReuse(bool enabled);bool PreparedDrawReuseEnabled();
void RecordCachedGroup(unsigned models,unsigned indices,unsigned uploads);
bool HasScope();
bool DrawIndexed(const GfxCmdBufPrimState*,int baseIndex,int vertexCount,int triCount,std::string&);
}
