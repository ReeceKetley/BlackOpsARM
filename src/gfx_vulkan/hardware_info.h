#pragma once
#include <vulkan/vulkan.h>
#include <string>
namespace bo1::vulkan {
struct HardwareInfo {
    VkPhysicalDeviceProperties properties{};VkPhysicalDeviceFeatures available{};
    VkDeviceSize deviceLocalHeapBytes{};bool mipCubemaps{};
    bool depthShadowComparison{},depthShadowLinear{};
    unsigned enabledAnisotropy=1; // Current device enables no optional anisotropy.
};
bool QueryHardware(VkPhysicalDevice,HardwareInfo&,std::string&);
}
