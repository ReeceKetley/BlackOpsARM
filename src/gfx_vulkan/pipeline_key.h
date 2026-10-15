#pragma once
#include "graphics_program.h"
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace bo1::vulkan {
// Value-semantic identity of a ProgramDescription for pipeline reuse. The key
// retains immutable registered shader owners (or copies raw bytecode) and the vertex binding/attribute
// arrays, so it stays valid after the description's pointers are gone. It makes
// no Vulkan calls and does not own, create or destroy any Vulkan object; the
// render-pass handle is compared and hashed by value only, and the caller must
// keep any cache entry's lifetime consistent with that render pass.
//
// Owned shader identities compare by retained record; raw bytes compare by value.
// Equality compares every field explicitly (no raw struct comparison, so struct
// padding never participates). The cached hash is only a fast reject: two keys
// are equal only when all contents match. Floating-point alpha reference is
// compared by bit pattern so equality and hashing always agree.
class PipelineKey {
public:
    PipelineKey() = default;
    // Retains matching immutable shader owners or copies raw pointed-to data.
    // Fails (and leaves `out`
    // unchanged) if a byte/element count is non-zero while its pointer is null.
    static bool Create(const ProgramDescription& description, PipelineKey& out, std::string& error);

    VkRenderPass RenderPass() const { return renderPass_; }
    std::size_t Hash() const { return hash_; }
    bool operator==(const PipelineKey& other) const;
    bool operator!=(const PipelineKey& other) const { return !(*this == other); }

private:
    struct Stencil {
        VkStencilOp failOp{}, passOp{}, depthFailOp{};
        VkCompareOp compareOp{};
        std::uint32_t compareMask{}, writeMask{}, reference{};
    };
    struct Blend {
        VkBool32 blendEnable{};
        VkBlendFactor srcColorBlendFactor{}, dstColorBlendFactor{};
        VkBlendOp colorBlendOp{};
        VkBlendFactor srcAlphaBlendFactor{}, dstAlphaBlendFactor{};
        VkBlendOp alphaBlendOp{};
        VkColorComponentFlags colorWriteMask{};
    };
    struct Binding {
        std::uint32_t binding{}, stride{};
        VkVertexInputRate inputRate{};
    };
    struct Attribute {
        std::uint32_t location{}, binding{};
        VkFormat format{};
        std::uint32_t offset{};
    };

    static bool SameStencil(const Stencil& a, const Stencil& b);
    static bool SameBlend(const Blend& a, const Blend& b);
    void ComputeHash();

    std::shared_ptr<const shader_assets::Record> vertexOwner_,pixelOwner_;
    std::vector<std::uint8_t> vertexBytecode_, pixelBytecode_;
    VkRenderPass renderPass_{};
    std::uint32_t subpass_{};
    unsigned pixelDepthSamplerMask_{};
    unsigned vertexUbyteMask_{};
    bool depthBiasEnabled_{};std::uint32_t depthBiasConstantBits_{},depthBiasSlopeBits_{};
    std::vector<Binding> vertexBindings_;
    std::vector<Attribute> vertexAttributes_;
    VkPrimitiveTopology topology_{};
    VkSampleCountFlagBits samples_{};
    VkCullModeFlags cull_{};
    VkFrontFace frontFace_{};
    std::uint32_t alphaComparison_{};
    std::uint32_t alphaReferenceBits_{};
    bool stencilTest_{};
    Stencil frontStencil_{}, backStencil_{};
    bool colorAttachment_{};
    bool depthTest_{}, depthWrite_{};
    VkCompareOp depthCompare_{};
    Blend color_{};
    std::size_t hash_{};
};

struct PipelineKeyHash {
    std::size_t operator()(const PipelineKey& key) const { return key.Hash(); }
};
}
