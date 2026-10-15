#include <vulkan/vulkan.h>
#include <cstdio>
#include <vector>
int main() {
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};app.pApplicationName="Native vertex capability query";app.apiVersion=VK_API_VERSION_1_1;
    VkInstanceCreateInfo create{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};create.pApplicationInfo=&app;VkInstance instance{};
    auto result=vkCreateInstance(&create,nullptr,&instance);if(result!=VK_SUCCESS)return 1;
    unsigned count{};result=vkEnumeratePhysicalDevices(instance,&count,nullptr);if(result!=VK_SUCCESS||!count){vkDestroyInstance(instance,nullptr);return 2;}
    std::vector<VkPhysicalDevice> devices(count);result=vkEnumeratePhysicalDevices(instance,&count,devices.data());if(result!=VK_SUCCESS){vkDestroyInstance(instance,nullptr);return 3;}
    const VkFormat formats[]={VK_FORMAT_R32G32_SFLOAT,VK_FORMAT_R32G32B32_SFLOAT,VK_FORMAT_R32G32B32A32_SFLOAT,VK_FORMAT_B8G8R8A8_UNORM,VK_FORMAT_R8G8B8A8_USCALED,VK_FORMAT_R8G8B8A8_UNORM,VK_FORMAT_R16G16B16A16_SNORM,VK_FORMAT_R16G16_SFLOAT,VK_FORMAT_R8G8B8A8_UINT};
    const char* names[]={"FLOAT2","FLOAT3","FLOAT4","D3DCOLOR_BGRA","UBYTE4_USCALED","UBYTE4N_RGBA","SHORT4N","FLOAT16_2","RGBA_UINT_alternative"};
    for(auto device:devices) {
        VkPhysicalDeviceProperties props{};vkGetPhysicalDeviceProperties(device,&props);std::printf("vertex_caps_device=%s vendor=%u api=%u\n",props.deviceName,props.vendorID,props.apiVersion);
        for(unsigned i=0;i<sizeof(formats)/sizeof(*formats);++i){VkFormatProperties p{};vkGetPhysicalDeviceFormatProperties(device,formats[i],&p);std::printf("vertex_caps format=%u name=%s supported=%u buffer_features=%u\n",unsigned(formats[i]),names[i],!!(p.bufferFeatures&VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT),unsigned(p.bufferFeatures));}
    }
    vkDestroyInstance(instance,nullptr);return 0;
}
