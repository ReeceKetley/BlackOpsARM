#include "native_vulkan_memory.h"
#include "native_vulkan_device.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <limits>
#include <vector>
namespace bo1::android::native_memory {
namespace {
bool AvailableRAM(VkDeviceSize& value) {
    auto file=std::fopen("/proc/meminfo","r");if(!file)return false;
    char line[256];bool found=false;
    while(std::fgets(line,sizeof(line),file)) {
        unsigned long long kb{};char unit[16]{};
        if(std::sscanf(line,"MemAvailable: %llu %15s",&kb,unit)==2&&!std::strcmp(unit,"kB")
            &&kb<=std::numeric_limits<VkDeviceSize>::max()/1024) {value=VkDeviceSize(kb)*1024;found=true;break;}
    }
    const bool clean=!std::ferror(file);std::fclose(file);return found&&clean;
}
bool Add(VkDeviceSize& total,VkDeviceSize value,std::string& error) {
    if(value>std::numeric_limits<VkDeviceSize>::max()-total){error="Vulkan memory budget overflow";return false;}
    total+=value;return true;
}}
bool Read(Snapshot& output,std::string& error) {
    output={};error.clear();auto renderer=native_device::Renderer();
    if(!renderer||!renderer->Device()||!renderer->Physical()) {error="Texture budget requires the real initialized Vulkan device";return false;}
    const auto physical=renderer->Physical();Snapshot sample;
    VkPhysicalDeviceProperties properties{};vkGetPhysicalDeviceProperties(physical,&properties);
    sample.uma=properties.deviceType==VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU||properties.deviceType==VK_PHYSICAL_DEVICE_TYPE_CPU;
    unsigned count{};
    if(vkEnumerateDeviceExtensionProperties(physical,nullptr,&count,nullptr)!=VK_SUCCESS) {error="Native memory extension enumeration failed";return false;}
    std::vector<VkExtensionProperties> extensions(count);
    if(vkEnumerateDeviceExtensionProperties(physical,nullptr,&count,extensions.data())!=VK_SUCCESS) {error="Native memory extension enumeration changed/failed";return false;}
    for(const auto& extension:extensions)if(!std::strcmp(extension.extensionName,VK_EXT_MEMORY_BUDGET_EXTENSION_NAME))sample.driverBudget=true;
    VkPhysicalDeviceMemoryBudgetPropertiesEXT budget{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT};
    VkPhysicalDeviceMemoryProperties2 memory{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2};
    memory.pNext=sample.driverBudget?&budget:nullptr;vkGetPhysicalDeviceMemoryProperties2(physical,&memory);
    for(unsigned heap=0;heap<memory.memoryProperties.memoryHeapCount;++heap) {
        const auto& info=memory.memoryProperties.memoryHeaps[heap];if(!(info.flags&VK_MEMORY_HEAP_DEVICE_LOCAL_BIT))continue;
        if(!Add(sample.localCapacity,info.size,error))return false;
        if(sample.driverBudget) {
            if(!budget.heapBudget[heap]||budget.heapBudget[heap]>info.size) {error="Driver reported an invalid Vulkan heap budget";return false;}
            const auto remaining=budget.heapBudget[heap]>budget.heapUsage[heap]?budget.heapBudget[heap]-budget.heapUsage[heap]:0;
            if(!Add(sample.driverAvailable,remaining,error))return false;
        }
    }
    if(!sample.localCapacity){error="Native texture budget has no device-local heap";return false;}
    sample.osKnown=AvailableRAM(sample.osAvailable);
    if(!sample.driverBudget&&!sample.osKnown) {error="Native memory availability is unknown";return false;}
    sample.textureAllowance=sample.driverBudget?sample.driverAvailable:std::min(sample.localCapacity,sample.osAvailable);
    if(sample.uma&&sample.osKnown)sample.textureAllowance=std::min(sample.textureAllowance,sample.osAvailable);
    output=sample;return true;
}
}
