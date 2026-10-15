#include "pipeline_key.h"
#include "shader_assets.h"
#include <cstring>
#include <type_traits>
#include <utility>

namespace bo1::vulkan {
namespace {
// Hashes only accelerate lookup; equality still compares all owned bytes/fields.
// Mix complete words to avoid byte-at-a-time loops on every engine draw.
class Hasher {
public:
    void Bytes(const std::uint8_t* data,std::size_t count) {
        while(count>=8) {std::uint64_t word;std::memcpy(&word,data,8);U64(Little(word));data+=8;count-=8;}
        if(count) {std::uint64_t tail=0;std::memcpy(&tail,data,count);U64(Little(tail));}
    }
    void U64(std::uint64_t value) {
        value^=value>>30;value*=0xbf58476d1ce4e5b9ULL;
        value^=value>>27;value*=0x94d049bb133111ebULL;value^=value>>31;
        state_^=value+0x9e3779b97f4a7c15ULL+(state_<<6)+(state_>>2);
    }
    std::uint64_t State()const{return state_;}
private:
    static std::uint64_t Little(std::uint64_t value) {
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        return __builtin_bswap64(value);
#else
        return value;
#endif
    }
    std::uint64_t state_=0x243f6a8885a308d3ULL;
};

// Non-dispatchable handles are pointers on 64-bit targets and uint64_t on
// 32-bit targets; both reduce to a stable integer here.
template <typename Handle>
std::uint64_t HandleValue(Handle handle) {
    if constexpr (std::is_pointer<Handle>::value) return static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(handle));
    else return static_cast<std::uint64_t>(handle);
}

std::uint32_t FloatBits(float value) {
    std::uint32_t bits;
    static_assert(sizeof(bits) == sizeof(value), "float must be 32-bit");
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}

void CopyBytes(const void* data, std::size_t count, std::vector<std::uint8_t>& out) {
    if (!count) { out.clear(); return; }
    const std::uint8_t* bytes = static_cast<const std::uint8_t*>(data);
    out.assign(bytes, bytes + count);
}

bool PointerValid(const void* pointer, std::size_t count, const char* what, std::string& error) {
    if (count != 0 && pointer == nullptr) { error = std::string(what) + " is null with non-zero size/count"; return false; }
    return true;
}
}

bool PipelineKey::Create(const ProgramDescription& d, PipelineKey& out, std::string& error) {
    error.clear();
    if (!PointerValid(d.vertexBytecode, d.vertexBytes, "vertexBytecode", error) ||
        !PointerValid(d.pixelBytecode, d.pixelBytes, "pixelBytecode", error) ||
        !PointerValid(d.vertexBindings, d.vertexBindingCount, "vertexBindings", error) ||
        !PointerValid(d.vertexAttributes, d.vertexAttributeCount, "vertexAttributes", error)) return false;

    if(!ValidateDepthBias(d.depthBias,error))return false;
    PipelineKey k;
    // Only trust an owner when these are exactly its immutable bytes. Raw
    // descriptions keep the previous value-copy path used by previews/tests.
    const auto ownerMatches=[](const auto& owner,const void* bytes,std::size_t count){
        return !owner||(owner->bytecode.data()==bytes&&owner->bytecode.size()*4==count);
    };
    if(!ownerMatches(d.vertexOwner,d.vertexBytecode,d.vertexBytes)||!ownerMatches(d.pixelOwner,d.pixelBytecode,d.pixelBytes)) {
        error="Pipeline shader owner does not match supplied bytecode";return false;
    }
    k.vertexOwner_=d.vertexOwner;k.pixelOwner_=d.pixelOwner;
    if(!k.vertexOwner_)CopyBytes(d.vertexBytecode, d.vertexBytes, k.vertexBytecode_);
    if(!k.pixelOwner_)CopyBytes(d.pixelBytecode, d.pixelBytes, k.pixelBytecode_);
    k.renderPass_ = d.renderPass;
    k.subpass_ = d.subpass;
    k.pixelDepthSamplerMask_ = d.pixelDepthSamplerMask;
    k.vertexUbyteMask_=d.vertexUbyteMask;
    k.depthBiasEnabled_=d.depthBias.enabled;k.depthBiasConstantBits_=FloatBits(d.depthBias.constantFactor);k.depthBiasSlopeBits_=FloatBits(d.depthBias.slopeFactor);
    k.vertexBindings_.reserve(d.vertexBindingCount);
    for (unsigned i = 0; i < d.vertexBindingCount; ++i) {
        const VkVertexInputBindingDescription& b = d.vertexBindings[i];
        k.vertexBindings_.push_back({b.binding, b.stride, b.inputRate});
    }
    k.vertexAttributes_.reserve(d.vertexAttributeCount);
    for (unsigned i = 0; i < d.vertexAttributeCount; ++i) {
        const VkVertexInputAttributeDescription& a = d.vertexAttributes[i];
        k.vertexAttributes_.push_back({a.location, a.binding, a.format, a.offset});
    }
    k.topology_ = d.topology;
    k.samples_ = d.samples;
    k.cull_ = d.cull;
    k.frontFace_ = d.frontFace;
    k.alphaComparison_ = d.alphaTest.comparison;
    k.alphaReferenceBits_ = FloatBits(d.alphaTest.reference);
    k.stencilTest_ = d.stencilTest;
    const VkStencilOpState* stencils[2] = {&d.frontStencil, &d.backStencil};
    Stencil* keyStencils[2] = {&k.frontStencil_, &k.backStencil_};
    for (int i = 0; i < 2; ++i) {
        const VkStencilOpState& s = *stencils[i];
        *keyStencils[i] = {s.failOp, s.passOp, s.depthFailOp, s.compareOp, s.compareMask, s.writeMask, s.reference};
    }
    k.colorAttachment_ = d.colorAttachment;
    k.depthTest_ = d.depthTest;
    k.depthWrite_ = d.depthWrite;
    k.depthCompare_ = d.depthCompare;
    k.color_ = {d.color.blendEnable, d.color.srcColorBlendFactor, d.color.dstColorBlendFactor, d.color.colorBlendOp,
        d.color.srcAlphaBlendFactor, d.color.dstAlphaBlendFactor, d.color.alphaBlendOp, d.color.colorWriteMask};
    k.ComputeHash();
    out = std::move(k);
    return true;
}

void PipelineKey::ComputeHash() {
    Hasher h;
    // Variable-length members are length-prefixed so adjacent fields cannot alias.
    h.U64(HandleValue(vertexOwner_.get()));h.U64(HandleValue(pixelOwner_.get()));
    h.U64(vertexBytecode_.size()); h.Bytes(vertexBytecode_.data(), vertexBytecode_.size());
    h.U64(pixelBytecode_.size()); h.Bytes(pixelBytecode_.data(), pixelBytecode_.size());
    h.U64(HandleValue(renderPass_)); h.U64(subpass_);
    h.U64(vertexBindings_.size());
    for (const Binding& b : vertexBindings_) { h.U64(b.binding); h.U64(b.stride); h.U64(static_cast<std::uint64_t>(b.inputRate)); }
    h.U64(vertexAttributes_.size());
    for (const Attribute& a : vertexAttributes_) { h.U64(a.location); h.U64(a.binding); h.U64(static_cast<std::uint64_t>(a.format)); h.U64(a.offset); }
    h.U64(static_cast<std::uint64_t>(topology_)); h.U64(static_cast<std::uint64_t>(samples_));
    h.U64(cull_); h.U64(static_cast<std::uint64_t>(frontFace_));
    h.U64(alphaComparison_); h.U64(alphaReferenceBits_);
    h.U64(pixelDepthSamplerMask_);h.U64(vertexUbyteMask_);
    h.U64(depthBiasEnabled_);h.U64(depthBiasConstantBits_);h.U64(depthBiasSlopeBits_);
    h.U64(stencilTest_ ? 1u : 0u);
    for (const Stencil* s : {&frontStencil_, &backStencil_}) {
        h.U64(static_cast<std::uint64_t>(s->failOp)); h.U64(static_cast<std::uint64_t>(s->passOp));
        h.U64(static_cast<std::uint64_t>(s->depthFailOp)); h.U64(static_cast<std::uint64_t>(s->compareOp));
        h.U64(s->compareMask); h.U64(s->writeMask); h.U64(s->reference);
    }
    h.U64(colorAttachment_ ? 1u : 0u); h.U64(depthTest_ ? 1u : 0u); h.U64(depthWrite_ ? 1u : 0u);
    h.U64(static_cast<std::uint64_t>(depthCompare_));
    h.U64(color_.blendEnable);
    h.U64(static_cast<std::uint64_t>(color_.srcColorBlendFactor)); h.U64(static_cast<std::uint64_t>(color_.dstColorBlendFactor));
    h.U64(static_cast<std::uint64_t>(color_.colorBlendOp));
    h.U64(static_cast<std::uint64_t>(color_.srcAlphaBlendFactor)); h.U64(static_cast<std::uint64_t>(color_.dstAlphaBlendFactor));
    h.U64(static_cast<std::uint64_t>(color_.alphaBlendOp)); h.U64(color_.colorWriteMask);
    hash_ = static_cast<std::size_t>(h.State());
}

bool PipelineKey::SameStencil(const Stencil& a, const Stencil& b) {
    return a.failOp == b.failOp && a.passOp == b.passOp && a.depthFailOp == b.depthFailOp &&
        a.compareOp == b.compareOp && a.compareMask == b.compareMask &&
        a.writeMask == b.writeMask && a.reference == b.reference;
}

bool PipelineKey::SameBlend(const Blend& a, const Blend& b) {
    return a.blendEnable == b.blendEnable && a.srcColorBlendFactor == b.srcColorBlendFactor &&
        a.dstColorBlendFactor == b.dstColorBlendFactor && a.colorBlendOp == b.colorBlendOp &&
        a.srcAlphaBlendFactor == b.srcAlphaBlendFactor && a.dstAlphaBlendFactor == b.dstAlphaBlendFactor &&
        a.alphaBlendOp == b.alphaBlendOp && a.colorWriteMask == b.colorWriteMask;
}

bool PipelineKey::operator==(const PipelineKey& o) const {
    // The hash is only a fast reject; every field is still compared below.
    if (hash_ != o.hash_) return false;
    if (renderPass_ != o.renderPass_ || subpass_ != o.subpass_ || pixelDepthSamplerMask_ != o.pixelDepthSamplerMask_ || vertexUbyteMask_!=o.vertexUbyteMask_) return false;
    if (topology_ != o.topology_ || samples_ != o.samples_ || cull_ != o.cull_ || frontFace_ != o.frontFace_) return false;
    if(depthBiasEnabled_!=o.depthBiasEnabled_||depthBiasConstantBits_!=o.depthBiasConstantBits_||depthBiasSlopeBits_!=o.depthBiasSlopeBits_)return false;
    if (alphaComparison_ != o.alphaComparison_ || alphaReferenceBits_ != o.alphaReferenceBits_) return false;
    if (stencilTest_ != o.stencilTest_ || !SameStencil(frontStencil_, o.frontStencil_) || !SameStencil(backStencil_, o.backStencil_)) return false;
    if (colorAttachment_ != o.colorAttachment_ || depthTest_ != o.depthTest_ || depthWrite_ != o.depthWrite_ || depthCompare_ != o.depthCompare_) return false;
    if (!SameBlend(color_, o.color_)) return false;
    if (vertexBindings_.size() != o.vertexBindings_.size() || vertexAttributes_.size() != o.vertexAttributes_.size()) return false;
    for (std::size_t i = 0; i < vertexBindings_.size(); ++i) {
        const Binding& a = vertexBindings_[i]; const Binding& b = o.vertexBindings_[i];
        if (a.binding != b.binding || a.stride != b.stride || a.inputRate != b.inputRate) return false;
    }
    for (std::size_t i = 0; i < vertexAttributes_.size(); ++i) {
        const Attribute& a = vertexAttributes_[i]; const Attribute& b = o.vertexAttributes_[i];
        if (a.location != b.location || a.binding != b.binding || a.format != b.format || a.offset != b.offset) return false;
    }
    return vertexOwner_==o.vertexOwner_&&pixelOwner_==o.pixelOwner_
        &&vertexBytecode_ == o.vertexBytecode_ && pixelBytecode_ == o.pixelBytecode_;
}
}
