#include "material_state.h"
namespace bo1::vulkan {
namespace {
bool Factor(unsigned index,VkBlendFactor& result) {
    // Exact s_blendTable_64 D3D values1..10, mapped to Vulkan names.
    static const VkBlendFactor factors[]={VK_BLEND_FACTOR_ZERO,VK_BLEND_FACTOR_ZERO,VK_BLEND_FACTOR_ONE,VK_BLEND_FACTOR_SRC_COLOR,
        VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,VK_BLEND_FACTOR_SRC_ALPHA,VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,VK_BLEND_FACTOR_DST_ALPHA,
        VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,VK_BLEND_FACTOR_DST_COLOR,VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR};
    if(index<1||index>10)return false;result=factors[index];return true;
}
bool Op(unsigned index,VkBlendOp& result) {
    static const VkBlendOp ops[]={VK_BLEND_OP_ADD,VK_BLEND_OP_ADD,VK_BLEND_OP_SUBTRACT,VK_BLEND_OP_REVERSE_SUBTRACT,VK_BLEND_OP_MIN,VK_BLEND_OP_MAX};
    if(index<1||index>5)return false;result=ops[index];return true;
}
VkStencilOp StencilOp(unsigned index) {
    static const VkStencilOp ops[]={VK_STENCIL_OP_KEEP,VK_STENCIL_OP_ZERO,VK_STENCIL_OP_REPLACE,VK_STENCIL_OP_INCREMENT_AND_CLAMP,
        VK_STENCIL_OP_DECREMENT_AND_CLAMP,VK_STENCIL_OP_INVERT,VK_STENCIL_OP_INCREMENT_AND_WRAP,VK_STENCIL_OP_DECREMENT_AND_WRAP};return ops[index];
}
VkStencilOpState Stencil(unsigned pass,unsigned fail,unsigned zfail,unsigned compare) {
    return {StencilOp(fail),StencilOp(pass),StencilOp(zfail),static_cast<VkCompareOp>(compare),255,255,0};
}
}
bool ApplyMaterialState(unsigned w0,unsigned w1,ProgramDescription& description,std::string& error) {
    error.clear();auto result=description;
    if(w0&0x80000000u) {error="Native wireframe requires optional device feature policy";return false;}
    // Decode desired state here; draw capture resolves actual dvars and depth format.
    result.depthBias=(w1&0x30)?DepthBiasState{true,false,0,0}:DepthBiasState{};
    const unsigned cull=(w0>>14)&3;
    if(!cull) {error="Invalid canonical cull state";return false;}
    // Match the protected working DXVK D3D9 path: clockwise front face, negative viewport.
    result.frontFace=VK_FRONT_FACE_CLOCKWISE;
    result.cull=cull==1?VK_CULL_MODE_NONE:(cull==2?VK_CULL_MODE_BACK_BIT:VK_CULL_MODE_FRONT_BIT);
    result.color.colorWriteMask=((w0&0x8000000)?7:0)|((w0&0x10000000)?8:0);
    result.alphaTest={};
    if(!(w0&0x800)) {
        switch((w0>>12)&3) {
        case 1:result.alphaTest={5,0};break;
        case 2:result.alphaTest={7,1};break;
        case 3:result.alphaTest={7,128.0f/255.0f};break;
        default:error="Invalid canonical alpha-test function";return false;
        }
    }
    const unsigned rgbOp=(w0>>8)&7,alphaOp=(w0>>24)&7;
    result.color.blendEnable=rgbOp!=0;
    if(rgbOp) {
        if(!Factor(w0&15,result.color.srcColorBlendFactor)||!Factor((w0>>4)&15,result.color.dstColorBlendFactor)||!Op(rgbOp,result.color.colorBlendOp)) {
            error="Invalid canonical RGB blend state";return false;
        }
        if(alphaOp) {
            if(!Factor((w0>>16)&15,result.color.srcAlphaBlendFactor)||!Factor((w0>>20)&15,result.color.dstAlphaBlendFactor)||!Op(alphaOp,result.color.alphaBlendOp)) {
                error="Invalid canonical alpha blend state";return false;
            }
        } else {result.color.srcAlphaBlendFactor=result.color.srcColorBlendFactor;result.color.dstAlphaBlendFactor=result.color.dstColorBlendFactor;result.color.alphaBlendOp=result.color.colorBlendOp;}
    } else if(alphaOp) {error="Independent alpha blend requires RGB blend";return false;}
    result.depthTest=!(w1&2);result.depthWrite=(w1&1)!=0;
    static const VkCompareOp depth[]={VK_COMPARE_OP_ALWAYS,VK_COMPARE_OP_LESS,VK_COMPARE_OP_EQUAL,VK_COMPARE_OP_LESS_OR_EQUAL};
    result.depthCompare=depth[(w1>>2)&3];
    result.stencilTest=(w1&0x40)!=0;
    if((w1&0x80)&&!result.stencilTest) {error="Back stencil requires front stencil";return false;}
    result.frontStencil=Stencil((w1>>8)&7,(w1>>11)&7,(w1>>14)&7,(w1>>17)&7);
    result.backStencil=(w1&0x80)?Stencil((w1>>20)&7,(w1>>23)&7,(w1>>26)&7,w1>>29):result.frontStencil;
    description=result;return true;
}
}
