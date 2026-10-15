#include "graphics_program.h"
#include <cmath>
namespace bo1::vulkan {
bool ValidateDepthBias(const DepthBiasState& bias,std::string& error) {
    if(!bias.resolved||!std::isfinite(bias.constantFactor)||!std::isfinite(bias.slopeFactor)) {
        error="Native polygon bias has not been resolved to finite depth-format units";return false;
    }
    if(!bias.enabled&&(bias.constantFactor!=0||bias.slopeFactor!=0)) {error="Disabled native polygon bias has nonzero factors";return false;}
    return true;
}
bool ResolveDepthBias(ProgramDescription& description,VkFormat format,float normalized,float slope,std::string& error) {
    if(!description.depthBias.enabled) {description.depthBias={};return true;}
    if(!std::isfinite(normalized)||!std::isfinite(slope)) {error="Non-finite canonical polygon offset";return false;}
    float units{};
    switch(format) {
    case VK_FORMAT_D16_UNORM:case VK_FORMAT_D16_UNORM_S8_UINT:units=65536.0f;break;
    case VK_FORMAT_X8_D24_UNORM_PACK32:case VK_FORMAT_D24_UNORM_S8_UINT:units=16777216.0f;break;
    default:error="Native constant polygon bias requires a supported fixed-depth attachment";return false;
    }
    DepthBiasState next{true,true,normalized*units,slope};
    if(!ValidateDepthBias(next,error))return false;
    description.depthBias=next;return true;
}
}
