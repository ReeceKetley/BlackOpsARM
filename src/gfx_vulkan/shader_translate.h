#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace bo1::vulkan {
struct ShaderAlphaTest { unsigned comparison=8;float reference=0; };
enum class ShaderStage { Vertex, Fragment };
enum class ResourceKind { Constants, Texture, Sampler };
// Each stage has its own descriptor set: VS=0, PS=1. Constants retain
// their D3D register-set identity; image/sampler slots retain their s#.
struct ShaderBinding {
    ResourceKind kind;
    unsigned registerIndex, set, binding, dimension;
};
struct ShaderVarying {
    std::string semantic;
    unsigned semanticIndex, registerIndex, mask, usedMask, systemValue, componentType;
};
struct ShaderTranslation {
    ShaderStage stage{};
    std::vector<std::uint32_t> spirv;
    std::vector<ShaderBinding> bindings;
    std::vector<ShaderVarying> inputs, outputs;
    std::string diagnostics;
    // Float constant rows (c0..cN-1) the SPIR-V can read. 256 whenever the
    // block is indexed dynamically or its declaration is not recognised.
    unsigned constantRows=256;
};
// Original SM3 bytecode stays untouched. Output and reflection own their memory.
// On failure no partially usable program is published.
bool TranslateShader(const void* bytecode, std::size_t bytes,
                     ShaderTranslation& output,
                     const std::vector<ShaderVarying>* nextStageInputs=nullptr,
                     const ShaderAlphaTest* alphaTest=nullptr,unsigned depthSamplerMask=0,unsigned vertexUbyteMask=0);
// Honour the SM1-3 _pp partial-precision hints as SPIR-V RelaxedPrecision in
// subsequently translated shaders (fp16 ALU/texture paths on mobile GPUs).
void SetRelaxedPrecision(bool enabled);
bool RelaxedPrecisionEnabled();
// SM3 output/input register indices need not agree. Link by semantic rather
// than assuming TEXCOORD0 occupies the same register in both stages.
bool TranslateShaderPair(const void* vertex, std::size_t vertexBytes,
                         const void* fragment, std::size_t fragmentBytes,
                         ShaderTranslation& vs, ShaderTranslation& ps,const ShaderAlphaTest* alphaTest=nullptr,unsigned pixelDepthSamplerMask=0,unsigned vertexUbyteMask=0);
}
