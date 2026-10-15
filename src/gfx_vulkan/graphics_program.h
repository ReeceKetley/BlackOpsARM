#pragma once
#include "shader_translate.h"
#include "bind_cache.h"
#include <vulkan/vulkan.h>
#include <array>
#include <memory>

namespace bo1::vulkan {
namespace shader_assets {struct Record;}
struct DepthBiasState {bool enabled{},resolved=true;float constantFactor{},slopeFactor{};};
struct ProgramDescription {
    // Optional immutable registered owners; key retains them against address reuse.
    std::shared_ptr<const shader_assets::Record> vertexOwner,pixelOwner;
    const void* vertexBytecode{};std::size_t vertexBytes{};
    const void* pixelBytecode{};std::size_t pixelBytes{};
    VkRenderPass renderPass{};unsigned subpass{};
    const VkVertexInputBindingDescription* vertexBindings{};unsigned vertexBindingCount{};
    const VkVertexInputAttributeDescription* vertexAttributes{};unsigned vertexAttributeCount{};
    VkPrimitiveTopology topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkSampleCountFlagBits samples=VK_SAMPLE_COUNT_1_BIT;
    VkCullModeFlags cull=VK_CULL_MODE_NONE;VkFrontFace frontFace=VK_FRONT_FACE_CLOCKWISE;
    ShaderAlphaTest alphaTest;
    unsigned pixelDepthSamplerMask{};
    unsigned vertexUbyteMask{};
    DepthBiasState depthBias;
    bool stencilTest{};VkStencilOpState frontStencil{},backStencil{};
    bool colorAttachment=true;
    bool depthTest{},depthWrite{};VkCompareOp depthCompare=VK_COMPARE_OP_LESS_OR_EQUAL;
    VkPipelineColorBlendAttachmentState color{VK_FALSE,VK_BLEND_FACTOR_ONE,VK_BLEND_FACTOR_ZERO,VK_BLEND_OP_ADD,
        VK_BLEND_FACTOR_ONE,VK_BLEND_FACTOR_ZERO,VK_BLEND_OP_ADD,15};
};
// Resolve canonical normalized D3D bias into fixed-depth Vulkan units.
bool ResolveDepthBias(ProgramDescription&,VkFormat,float normalizedBias,float slope,std::string&);
bool ValidateDepthBias(const DepthBiasState&,std::string&);
void SetCompatibleDescriptorReuse(bool enabled);
bool CompatibleDescriptorReuse();
struct ProgramResource {
    ResourceKind kind{};unsigned registerIndex{};
    VkDescriptorBufferInfo buffer{};VkDescriptorImageInfo image{};
};
// Immutable native pipeline/material layout. The caller owns device/cache/pass
// and must retire GPU work before destroying this object or its input resources.
class GraphicsProgram {
public:
    explicit GraphicsProgram(VkDevice device):device_(device){}
    ~GraphicsProgram();
    GraphicsProgram(const GraphicsProgram&)=delete;
    GraphicsProgram& operator=(const GraphicsProgram&)=delete;
    bool Create(const ProgramDescription&,VkPipelineCache,std::string& error,
        const std::array<ShaderTranslation,2>* translated=nullptr);
    // Descriptor pool belongs to a fence-retired frame arena. This object does
    // not reset/free the pool and never steals image/buffer ownership.
    bool BindResources(VkDescriptorPool,const std::vector<ProgramResource>& vertex,
        const std::vector<ProgramResource>& pixel,std::array<VkDescriptorSet,2>& sets,std::string& error) const;
    void Bind(VkCommandBuffer,const std::array<VkDescriptorSet,2>&) const;
    // Engine draw path: constants use dynamic offsets (resource offsets are
    // passed per draw), identical sets are reused from this frame's cache and
    // unchanged pipeline/set bindings are not re-recorded.
    bool Record(VkCommandBuffer,VkDescriptorPool,const std::vector<ProgramResource>& vertex,
        const std::vector<ProgramResource>& pixel,BindCache&,std::string& error) const;
    unsigned ConstantRows(unsigned stage)const{return shaders_[stage].constantRows;}
    // True when either stage samples a texture (descriptor content depends on it).
    bool SamplesTextures()const{
        for(const auto& stage:shaders_)for(const auto& b:stage.bindings)if(b.kind!=ResourceKind::Constants)return true;
        return false;}
private:
    VkDevice device_{};VkPipeline pipeline_{};VkPipelineLayout layout_{};unsigned dynamicCount_{};
    std::array<VkDescriptorSetLayout,2> setLayouts_{};
    std::array<std::uint64_t,2> layoutBindings_{};
    std::array<unsigned,2> stageDynamic_{};
    std::array<ShaderTranslation,2> shaders_;
};
}
