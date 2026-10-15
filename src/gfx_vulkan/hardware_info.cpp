#include "hardware_info.h"
#include <limits>
namespace bo1::vulkan {
bool QueryHardware(VkPhysicalDevice physical,HardwareInfo& output,std::string& error) {
    if(!physical){error="Native physical device unavailable";return false;}
    HardwareInfo info;vkGetPhysicalDeviceProperties(physical,&info.properties);vkGetPhysicalDeviceFeatures(physical,&info.available);
    const auto& limits=info.properties.limits;
    if(info.properties.apiVersion<VK_API_VERSION_1_1||limits.maxUniformBufferRange<4096||limits.maxBoundDescriptorSets<2
        ||limits.maxPerStageDescriptorSamplers<16||limits.maxPerStageDescriptorSampledImages<16||limits.maxVertexInputAttributes<16) {
        error="Device lacks current native T5 shader/resource minimums";return false;
    }
    VkPhysicalDeviceMemoryProperties memory{};vkGetPhysicalDeviceMemoryProperties(physical,&memory);
    for(unsigned i=0;i<memory.memoryHeapCount;++i)if(memory.memoryHeaps[i].flags&VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
        if(memory.memoryHeaps[i].size>std::numeric_limits<VkDeviceSize>::max()-info.deviceLocalHeapBytes){error="Device heap total overflow";return false;}
        info.deviceLocalHeapBytes+=memory.memoryHeaps[i].size;
    }
    VkImageFormatProperties cube{};
    const auto result=vkGetPhysicalDeviceImageFormatProperties(physical,VK_FORMAT_R8G8B8A8_UNORM,VK_IMAGE_TYPE_2D,VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT,VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,&cube);
    info.mipCubemaps=result==VK_SUCCESS&&cube.maxMipLevels>1&&cube.maxArrayLayers>=6;
    VkFormatProperties depth{};vkGetPhysicalDeviceFormatProperties(physical,VK_FORMAT_D24_UNORM_S8_UINT,&depth);
    VkImageFormatProperties shadow{};
    const auto shadowUsage=VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    const auto depthFeatures=VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT|VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_FORMAT_FEATURE_TRANSFER_SRC_BIT|VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
    info.depthShadowComparison=(depth.optimalTilingFeatures&depthFeatures)==depthFeatures
        &&vkGetPhysicalDeviceImageFormatProperties(physical,VK_FORMAT_D24_UNORM_S8_UINT,VK_IMAGE_TYPE_2D,
            VK_IMAGE_TILING_OPTIMAL,shadowUsage,0,&shadow)==VK_SUCCESS
        &&(shadow.sampleCounts&VK_SAMPLE_COUNT_1_BIT)&&shadow.maxMipLevels&&shadow.maxArrayLayers;
    info.depthShadowLinear=info.depthShadowComparison&&(depth.optimalTilingFeatures&VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT);
    output=info;return true;
}
}
