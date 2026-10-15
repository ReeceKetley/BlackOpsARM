#pragma once
#include <vulkan/vulkan.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <unordered_map>
#include <vector>
namespace bo1::vulkan {
// Per-command-buffer record of what is currently bound, plus descriptor sets
// already written this frame. Owned by one frame's recording scope; its sets
// come from that frame's fence-retired pools, so it never outlives them.
struct DescriptorSetKey {
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
struct BindCache {
    VkPipeline pipeline{};VkPipelineLayout layout{};
    std::array<VkDescriptorSet,2> sets{};std::array<std::uint32_t,2> offsets{};unsigned offsetCount=~0u;
    std::array<VkBuffer,3> vertexBuffers{};std::array<VkDeviceSize,3> vertexOffsets{};VkBuffer indexBuffer{};
    std::unordered_map<DescriptorSetKey,VkDescriptorSet,DescriptorSetKeyHash> written;
    // Identity of the previous engine draw. When the next draw has the same
    // state owner, render pass, state bits and the same (copy-on-write)
    // selection/constant/texture/geometry snapshots, its pipeline, sets and
    // buffers are already bound: only the index range differs. Owners are held
    // so a freed snapshot address can never be mistaken for a later one.
    struct LastDraw {
        bool valid{};const void* state{};VkRenderPass pass{};unsigned words[2]{};std::uint64_t revision{};
        std::shared_ptr<const void> selection,constants,textures,geometry;
        bool samplesTextures{true}; // the bound program reads its texture slots
        unsigned constantRows[2]{}; // float constant rows the bound program reads per stage
    } last;
    // An indexed draw not yet recorded. Draws with identical bound state whose
    // index ranges continue it (D3D9-style appends to the dynamic index buffer)
    // extend it instead of issuing another draw. Anything else that records a
    // command first calls FlushPending.
    struct PendingDraw {bool active{};VkCommandBuffer command{};std::uint32_t first{},count{};} pending;
    VkViewport viewport{};VkRect2D scissor{};bool viewportValid{};
    std::uint64_t setHits{},setMisses{},pipelineBinds{},pipelineSkips{};
};
inline void FlushPending(BindCache& cache) {
    if(!cache.pending.active)return;
    vkCmdDrawIndexed(cache.pending.command,cache.pending.count,1,cache.pending.first,0,0);cache.pending.active=false;
}
}
