#pragma once
#include "resources.h"
#include <memory>
#include <vector>
namespace bo1::vulkan {
// One queue/frame owner. A real submitted fence gates all pool/range reuse.
class DrawStorage {
public:
 struct Allocation {Buffer* buffer{};VkDeviceSize offset{};VkDescriptorPool descriptors{};};
 // Pages fill by bytes: per-draw constants are small, so a low draw cap per
 // page left most of each 1 MB page unused (and pinned) on mobile.
 DrawStorage(VkPhysicalDevice,VkDevice,VkDeviceSize pageBytes=1024*1024,unsigned drawsPerPage=1024,unsigned maxPages=64);
 ~DrawStorage();DrawStorage(const DrawStorage&)=delete;DrawStorage& operator=(const DrawStorage&)=delete;
 bool Reset(std::string&); // First use or after the submitted fence signals.
 bool Allocate(VkDeviceSize bytes,VkDeviceSize alignment,Allocation&,std::string&);
 bool Submitted(VkFence,std::string&); // Call only after successful vkQueueSubmit.
 unsigned Pages()const{return unsigned(pages_.size());}
 unsigned Draws()const;
 VkDeviceSize Bytes()const;
private:
 struct Page {std::unique_ptr<Buffer> buffer;VkDescriptorPool descriptors{};VkDeviceSize cursor{};unsigned draws{};};
 bool CreatePage(std::string&);
 VkPhysicalDevice physical_{};VkDevice device_{};VkDeviceSize pageBytes_{};unsigned drawsPerPage_{},maxPages_{},page_{};
 std::vector<Page> pages_;VkFence fence_{};bool recording_{},dirty_{};
 // Pages a frame actually used since the last trim; extra pages are freed.
 unsigned usedPeak_{},resets_{};
};
}
