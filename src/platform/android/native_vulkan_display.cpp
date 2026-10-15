#include "native_vulkan_display.h"
#include <algorithm>
namespace bo1::android::native_display {
bool Read(int display,Snapshot& output,std::string& error) {
    if(!(SDL_WasInit(SDL_INIT_VIDEO)&SDL_INIT_VIDEO)||display<0||display>=SDL_GetNumVideoDisplays()) {error="Native SDL display/video unavailable";return false;}
    Snapshot value;value.index=display;
    if(SDL_GetDisplayBounds(display,&value.bounds)||SDL_GetCurrentDisplayMode(display,&value.current)) {error=SDL_GetError();return false;}
    if(value.bounds.w<=0||value.bounds.h<=0||value.current.w<=0||value.current.h<=0||value.current.refresh_rate<0) {error="Native display has invalid current dimensions/timing";return false;}
    const int count=SDL_GetNumDisplayModes(display);if(count<0) {error=SDL_GetError();return false;}
    for(int i=0;i<count&&value.modes.size()<256;++i) {
        SDL_DisplayMode mode{};if(SDL_GetDisplayMode(display,i,&mode)) {error=SDL_GetError();return false;}
        if(mode.w<=0||mode.h<=0||mode.refresh_rate<0) {error="Native SDL mode has invalid dimensions/timing";return false;}
        value.modes.push_back(mode);
    }
    output=std::move(value);return true;
}
bool ProbeHardware(bo1::vulkan::HardwareInfo& output,std::string& error) {
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};app.pApplicationName="BlackOpsARM capability probe";app.apiVersion=VK_API_VERSION_1_1;
    VkInstanceCreateInfo create{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};create.pApplicationInfo=&app;
    VkInstance instance{};auto result=vkCreateInstance(&create,nullptr,&instance);
    if(result!=VK_SUCCESS){error="Native physical probe instance: "+std::to_string(int(result));return false;}
    struct Owner {VkInstance instance;~Owner(){vkDestroyInstance(instance,nullptr);}} owner{instance};
    unsigned count{};result=vkEnumeratePhysicalDevices(instance,&count,nullptr);
    if(result!=VK_SUCCESS||!count){error="Native physical probe has no enumerable GPU";return false;}
    std::vector<VkPhysicalDevice> devices(count);result=vkEnumeratePhysicalDevices(instance,&count,devices.data());
    if(result!=VK_SUCCESS||!count){error="Native physical probe enumeration failed";return false;}
    // Matches the current presenter's first physical-device selection. This
    // probe cannot claim surface/queue suitability before window creation.
    return bo1::vulkan::QueryHardware(devices[0],output,error);
}
}
