#pragma once
#include <vulkan/vulkan.h>
#include <string>
namespace bo1::vulkan {
struct HardwareInfo {
    VkPhysicalDeviceProperties properties{};VkPhysicalDeviceFeatures available{};
    VkDeviceSize deviceLocalHeapBytes{};bool mipCubemaps{};
    bool depthShadowComparison{},depthShadowLinear{};
    // 16-bit depth shadow maps: half the attachment/copy/sampling bandwidth of
    // D24S8 (shadow maps never use stencil).
    bool depthShadow16{},depthShadow16Linear{};
    unsigned enabledAnisotropy=1; // Current device enables no optional anisotropy.
};
bool QueryHardware(VkPhysicalDevice,HardwareInfo&,std::string&);
}
