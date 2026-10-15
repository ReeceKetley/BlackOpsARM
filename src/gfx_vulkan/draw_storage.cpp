#include "draw_storage.h"
#include <limits>
namespace bo1::vulkan {
DrawStorage::DrawStorage(VkPhysicalDevice physical,VkDevice device,VkDeviceSize bytes,unsigned draws,unsigned maximum)
 :physical_(physical),device_(device),pageBytes_(bytes),drawsPerPage_(draws),maxPages_(maximum){}
DrawStorage::~DrawStorage(){for(auto& p:pages_)if(p.descriptors)vkDestroyDescriptorPool(device_,p.descriptors,nullptr);}
unsigned DrawStorage::Draws()const {unsigned n=0;for(const auto& p:pages_)n+=p.draws;return n;}
bool DrawStorage::Reset(std::string& error) {
 if(recording_&&dirty_){error="Native draw storage cannot reset recorded readers";return false;}
 if(fence_&&vkGetFenceStatus(device_,fence_)!=VK_SUCCESS){error="Native draw storage submission fence has not retired";return false;}
 for(auto& p:pages_) {
  if(vkResetDescriptorPool(device_,p.descriptors,0)!=VK_SUCCESS){error="Native draw descriptor page reset failed";return false;}
  p.cursor=0;p.draws=0;
 }
 page_=0;fence_=VK_NULL_HANDLE;recording_=true;dirty_=false;return true;
}
bool DrawStorage::CreatePage(std::string& error) {
 if(!physical_||!device_||!pageBytes_||!drawsPerPage_||drawsPerPage_>std::numeric_limits<unsigned>::max()/32||pages_.size()>=maxPages_) {
  error="Native draw storage page budget/configuration exceeded pages="+std::to_string(pages_.size());return false;
 }
 Page p;p.buffer=std::make_unique<Buffer>(physical_,device_);
 if(!p.buffer->Create(pageBytes_,VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,error))return false;
 // Two stage sets and up to16 sampled images/samplers per stage per draw.
 // Native full-screen effects also take combined samplers and one uniform block.
 const VkDescriptorPoolSize sizes[]={{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,2*drawsPerPage_},{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,32*drawsPerPage_},{VK_DESCRIPTOR_TYPE_SAMPLER,32*drawsPerPage_},
  {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,8*drawsPerPage_},{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,drawsPerPage_}};
 VkDescriptorPoolCreateInfo info{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};info.maxSets=2*drawsPerPage_;info.poolSizeCount=5;info.pPoolSizes=sizes;
 if(vkCreateDescriptorPool(device_,&info,nullptr,&p.descriptors)!=VK_SUCCESS){error="Native draw descriptor page allocation failed";return false;}
 pages_.push_back(std::move(p));return true;
}
bool DrawStorage::Allocate(VkDeviceSize bytes,VkDeviceSize alignment,Allocation& output,std::string& error) {
 output={};
 if(!recording_||!alignment||bytes>pageBytes_){error="Invalid native draw page allocation/recording";return false;}
 for(;;) {
  if(page_==pages_.size()&&!CreatePage(error))return false;
  auto& p=pages_[page_];
  if(p.cursor>std::numeric_limits<VkDeviceSize>::max()-(alignment-1)){error="Native draw page alignment overflow";return false;}
  const auto offset=((p.cursor+alignment-1)/alignment)*alignment;
  if(p.draws<drawsPerPage_&&offset<=pageBytes_&&bytes<=pageBytes_-offset) {
   output={p.buffer.get(),offset,p.descriptors};p.cursor=offset+bytes;++p.draws;dirty_=true;return true;
  }
  if(page_+1>=maxPages_){error="Native draw storage page budget exhausted pages="+std::to_string(pages_.size())+" draws="+std::to_string(Draws());return false;}
  ++page_;
 }
}
bool DrawStorage::Submitted(VkFence fence,std::string& error) {
 if(!recording_||!fence){error="Native draw storage needs its real submitted fence";return false;}
 fence_=fence;recording_=false;return true;
}
}
