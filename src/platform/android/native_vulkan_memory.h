#pragma once
#include <vulkan/vulkan.h>
#include <string>
namespace bo1::android::native_memory {
struct Snapshot {
    VkDeviceSize localCapacity{},driverAvailable{},osAvailable{},textureAllowance{};
    bool driverBudget{},osKnown{},uma{};
};
// A policy snapshot/estimate, never a guarantee that a future allocation works.
bool Read(Snapshot&,std::string& error);
}
