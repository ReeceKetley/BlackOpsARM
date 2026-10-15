#pragma once
#include "image_assets.h"
namespace bo1::vulkan::image_uploads {
struct Flight {
    std::shared_ptr<Buffer> staging;
    std::vector<std::shared_ptr<const image_assets::Record>> records;
    VkFence fence{};
};
// Caller serializes queue ownership and this container. A null fence means
// unsubmitted/failed work and remains retained until explicit device drain.
inline bool RetireFlights(VkDevice device,std::vector<std::shared_ptr<Flight>>& flights,unsigned& retired,std::string& error) {
    for(auto it=flights.begin();it!=flights.end();) {
        if(!(*it)->fence){++it;continue;}
        const auto status=vkGetFenceStatus(device,(*it)->fence);
        if(status==VK_NOT_READY){++it;continue;}
        if(status!=VK_SUCCESS){error="Native upload retirement fence failed";return false;}
        it=flights.erase(it);++retired;
    }
    return true;
}
}
