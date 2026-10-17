#pragma once
#include <vulkan/vulkan.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <unordered_map>
#include <vector>
namespace bo1::vulkan {
class GraphicsProgram;
struct ResourceScratchPool;
// Inputs to the native material preparation step. Resource offsets/contents do
// not affect the pipeline; presence and stride do. Owners are retained beside
// this stamp, so the immutable shader/declaration addresses cannot be recycled.
struct PreparedProgramStamp {
    const void* cache{};VkPhysicalDevice physical{};
    VkRenderPass pass{},pipelinePass{};unsigned subpass{};VkSampleCountFlagBits samples{};
    bool color{},depth{},stencil{},relaxed{},depthOnly{};VkFormat depthFormat{};
    const void* vertex{};const void* pixel{};const void* declaration{};unsigned variant{};
    std::array<unsigned,2> words{};float bias{},slope{};
    unsigned streamMask{},depthSamplerMask{};std::array<unsigned,3> strides{};
    bool operator==(const PreparedProgramStamp&)const=default;
};
// Per-command-buffer record of what is currently bound, plus descriptor sets
// already written this frame. Owned by one frame's recording scope; its sets
// come from that frame's fence-retired pools, so it never outlives them.
struct DescriptorSetKey {
    // Null layout selects structural compatibility: words also identify the
    // stage and every binding/type (count=1, flags=0, no immutable samplers).
    // Non-null keeps the original exact-handle diagnostic path.
    VkDescriptorSetLayout layout{};
    std::vector<std::uint64_t> words; // binding, type, handles, range/layout
    bool operator==(const DescriptorSetKey& o)const{return layout==o.layout&&words==o.words;}
};
struct DescriptorSetKeyHash {
    std::size_t operator()(const DescriptorSetKey& k)const {
        std::uint64_t h=0x9e3779b97f4a7c15ull^reinterpret_cast<std::uintptr_t>(k.layout);
        for(auto w:k.words){h^=w+0x9e3779b97f4a7c15ull+(h<<6)+(h>>2);}
        return std::size_t(h);
    }
};
// Vertex attribute routing for one declaration variant, vertex shader and
// set of stream strides. Cached per frame: the owners are retained by that
// frame's draws, so their addresses cannot be reused within it.
struct VertexResolveKey {
    const void* declaration{};const void* translation{};unsigned variant{},streamMask{};std::array<std::uint32_t,3> strides{};
    bool operator==(const VertexResolveKey& o)const{return declaration==o.declaration&&translation==o.translation&&variant==o.variant&&streamMask==o.streamMask&&strides==o.strides;}
};
struct VertexResolveKeyHash {
    std::size_t operator()(const VertexResolveKey& k)const {
        std::uint64_t h=reinterpret_cast<std::uintptr_t>(k.declaration)*0x9e3779b97f4a7c15ull^reinterpret_cast<std::uintptr_t>(k.translation);
        h^=(std::uint64_t(k.variant)<<32|k.streamMask)+0x9e3779b97f4a7c15ull+(h<<6)+(h>>2);
        for(auto s:k.strides)h^=s+0x9e3779b97f4a7c15ull+(h<<6)+(h>>2);
        return std::size_t(h);
    }
};
struct VertexResolveValue {std::vector<VkVertexInputAttributeDescription> attributes;unsigned ubyteMask{};};
struct BindCache {
    std::shared_ptr<ResourceScratchPool> resourceScratch;
    struct PreparedProgram {
        PreparedProgramStamp stamp;
        std::shared_ptr<const void> selection;
        std::shared_ptr<GraphicsProgram> program;
        std::shared_ptr<GraphicsProgram> Find(const PreparedProgramStamp& wanted,bool enabled)const {
            return enabled&&stamp==wanted?program:nullptr;
        }
    } prepared;
    struct DescriptorContents {
        VkBuffer buffer{};VkDeviceSize range{};
        VkImageView imageView{};VkSampler sampler{};VkImageLayout imageLayout{};
        bool operator==(const DescriptorContents& other)const {
            return buffer==other.buffer&&range==other.range&&imageView==other.imageView
                &&sampler==other.sampler&&imageLayout==other.imageLayout;
        }
    };
    struct RecentDescriptorStage {
        // Unique immutable program token prevents a recycled program address
        // from matching another reflection. Frame pools own the actual sets.
        std::shared_ptr<const void> identity;
        VkDescriptorSet set{};
        std::vector<DescriptorContents> contents;
    } recentDescriptorStages[2];
    std::uint64_t descriptorContentHits{},descriptorContentMisses{};
    // Latest immutable constant range per stage, valid only for this recording
    // scope. DrawStorage retains every page until the frame fence retires;
    // destroy this cache before resetting that storage. Keep comparison bytes,
    // not the COW constant bank (retaining it would force another bank copy).
    struct ConstantUpload {
        const void* storage{};
        VkDescriptorBufferInfo buffer{};
        std::vector<unsigned char> bytes;
        bool Matches(const void* owner,const void* data,VkDeviceSize size)const {
            return storage==owner&&buffer.buffer&&buffer.range==size&&bytes.size()==size
                &&!std::memcmp(bytes.data(),data,std::size_t(size));
        }
        void Remember(const void* owner,const VkDescriptorBufferInfo& range,const void* data) {
            bytes.resize(std::size_t(range.range));
            std::memcpy(bytes.data(),data,bytes.size());storage=owner;buffer=range;
        }
    } constantUploads[2];
    bool reuseConstants=true;
    std::uint64_t constantUploadHits{},constantUploadMisses{},constantUploadBytesReused{};
    VkPipeline pipeline{};VkPipelineLayout layout{};
    std::array<VkDescriptorSet,2> sets{};std::array<std::uint32_t,2> offsets{};unsigned offsetCount=~0u;
    std::array<VkBuffer,3> vertexBuffers{};std::array<VkDeviceSize,3> vertexOffsets{};VkBuffer indexBuffer{};
    std::unordered_map<DescriptorSetKey,VkDescriptorSet,DescriptorSetKeyHash> written;
    std::unordered_map<VertexResolveKey,VertexResolveValue,VertexResolveKeyHash> vertexResolves;
    // Identity of the previous engine draw. When the next draw has the same
    // state owner, render pass, state bits and the same (copy-on-write)
    // selection/constant/texture/geometry snapshots, its pipeline, sets and
    // buffers are already bound: only the index range differs. Strong owners,
    // or a weak identity plus immutable uploaded bytes for constants, prevent
    // a freed snapshot address being mistaken for a later one.
    struct LastDraw {
        PreparedProgramStamp programStamp;
        bool valid{};const void* state{};VkRenderPass pass{};unsigned words[2]{};std::uint64_t revision{};
        std::shared_ptr<const void> selection,constants,textures,geometry;
        std::weak_ptr<const void> constantIdentity;
        const void* constantStorage{};
        VkDescriptorBufferInfo constantBuffers[2]{};
        bool samplesTextures{true}; // the bound program reads its texture slots
        unsigned constantRows[2]{}; // float constant rows the bound program reads per stage
    } last;
    // Drop the CPU bank only when every used row has an immutable uploaded
    // comparison copy. Other readers still retain normal COW ownership.
    void RememberConstants(const std::shared_ptr<const void>& snapshot,std::uint64_t revision,
        const void* storage,const void* const data[2],const unsigned rows[2],bool enabled) {
        bool uploaded=enabled&&reuseConstants&&storage;
        for(unsigned stage=0;stage<2&&uploaded;++stage)
            if(rows[stage])uploaded=constantUploads[stage].Matches(storage,data[stage],VkDeviceSize(rows[stage])*16);
        last.revision=revision;last.constantIdentity=snapshot;
        last.constantStorage=uploaded?storage:nullptr;
        last.constants=uploaded?std::shared_ptr<const void>{}:snapshot;
        for(unsigned stage=0;stage<2;++stage) {
            last.constantRows[stage]=rows[stage];
            last.constantBuffers[stage]=uploaded?constantUploads[stage].buffer:VkDescriptorBufferInfo{};
        }
    }
    bool UploadedConstantsMatch(const std::shared_ptr<const void>& snapshot,std::uint64_t revision,
        const void* storage,const void* const data[2],bool enabled)const {
        if(!snapshot||!enabled||!reuseConstants||!storage||last.constantStorage!=storage)return false;
        // The mirrors are mutable cache entries; first prove they still describe
        // the ranges bound by LastDraw, including after another upload path.
        for(unsigned stage=0;stage<2;++stage)if(last.constantRows[stage]) {
            const auto& upload=constantUploads[stage];const auto& saved=last.constantBuffers[stage];
            if(upload.storage!=storage||!saved.buffer||upload.buffer.buffer!=saved.buffer
                ||upload.buffer.offset!=saved.offset||upload.buffer.range!=saved.range
                ||saved.range!=VkDeviceSize(last.constantRows[stage])*16||upload.bytes.size()!=saved.range)return false;
        }
        if(revision==last.revision&&snapshot==last.constantIdentity.lock())return true;
        // Never read the old weak snapshot here: it may be this same bank after
        // an in-place Set. Only the uploaded bytes represent the previous draw.
        for(unsigned stage=0;stage<2;++stage)if(last.constantRows[stage]
            &&!constantUploads[stage].Matches(storage,data[stage],VkDeviceSize(last.constantRows[stage])*16))return false;
        return true;
    }
    // An indexed draw not yet recorded. Draws with identical bound state whose
    // index ranges continue it (D3D9-style appends to the dynamic index buffer)
    // extend it instead of issuing another draw. Anything else that records a
    // command first calls FlushPending.
    struct PendingDraw {bool active{};VkCommandBuffer command{};std::uint32_t first{},count{};} pending;
    VkViewport viewport{};VkRect2D scissor{};bool viewportValid{};
    std::array<std::uint64_t,2> layoutBindings{};std::array<std::uint32_t,2> stageOffsets{};
    std::uint64_t descriptorBinds{},descriptorSetsBound{};
    std::uint64_t indexedCommands{},indexedTriangles{},mergedRanges{},fullscreenCommands{};
    bool compatibleSets=true;
    std::uint64_t setHits{},setMisses{},pipelineBinds{},pipelineSkips{};
};
// A newly begun (secondary) command buffer has nothing bound: forget what the
// cache believes is bound. Frame-scoped descriptor sets and resolves remain.
inline void ForgetBoundState(BindCache& cache) {
    cache.pipeline=VK_NULL_HANDLE;cache.layout=VK_NULL_HANDLE;cache.sets={};cache.offsets={};cache.offsetCount=~0u;
    cache.vertexBuffers={};cache.vertexOffsets={};cache.indexBuffer=VK_NULL_HANDLE;cache.viewportValid=false;
    cache.layoutBindings={~std::uint64_t(0),~std::uint64_t(0)};cache.stageOffsets={};cache.last.valid=false;
}
inline void FlushPending(BindCache& cache) {
    if(!cache.pending.active)return;
    vkCmdDrawIndexed(cache.pending.command,cache.pending.count,1,cache.pending.first,0,0);
    ++cache.indexedCommands;cache.indexedTriangles+=cache.pending.count/3;cache.pending.active=false;
}
}
