#include "upload_flights.h"
#include <cstdio>
#include <stdexcept>
#include <cstring>
namespace {
using namespace bo1::vulkan;
void Check(bool ok,const std::string& error){if(!ok)throw std::runtime_error(error);}
void Vk(VkResult result,const char* label){Check(result==VK_SUCCESS,std::string(label)+": "+std::to_string(int(result)));}
struct Owner {
    VkDevice device;VkCommandPool pool{};VkFence fence{};VkEvent gate{};bool pending{};
    std::vector<std::shared_ptr<image_uploads::Flight>> flights;std::unique_ptr<Buffer> readback;
    ~Owner(){if(pending){vkSetEvent(device,gate);vkDeviceWaitIdle(device);}flights.clear();if(gate)vkDestroyEvent(device,gate,nullptr);if(fence)vkDestroyFence(device,fence,nullptr);if(pool)vkDestroyCommandPool(device,pool,nullptr);}
};
}
bool NativeVulkanUploadFlights(VkPhysicalDevice physical,VkDevice device,unsigned family) {
    using namespace bo1::vulkan;
    try {
        Owner owner{device};std::string error;unsigned retired=0;
        VkCommandPoolCreateInfo cp{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};cp.queueFamilyIndex=family;cp.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        Vk(vkCreateCommandPool(device,&cp,nullptr,&owner.pool),"flight pool");
        VkCommandBufferAllocateInfo allocation{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};allocation.commandPool=owner.pool;allocation.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;allocation.commandBufferCount=1;
        VkCommandBuffer command{};Vk(vkAllocateCommandBuffers(device,&allocation,&command),"flight command");
        VkFenceCreateInfo fi{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};Vk(vkCreateFence(device,&fi,nullptr,&owner.fence),"flight fence");
        VkEventCreateInfo ei{VK_STRUCTURE_TYPE_EVENT_CREATE_INFO};Vk(vkCreateEvent(device,&ei,nullptr,&owner.gate),"flight gate");
        VkQueue queue{};vkGetDeviceQueue(device,family,0,&queue);
        for(unsigned round=0;round<2;++round) {
            if(round){Vk(vkResetCommandBuffer(command,0),"flight reset");Vk(vkResetFences(device,1,&owner.fence),"flight fence reset");Vk(vkResetEvent(device,owner.gate),"flight gate reset");}
            auto staging=std::make_shared<Buffer>(physical,device);
            Check(staging->Create(16,VK_BUFFER_USAGE_TRANSFER_SRC_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,error),error);
            unsigned char expected[16];for(unsigned i=0;i<16;++i)expected[i]=static_cast<unsigned char>(round*32+i);
            Check(staging->Write(0,expected,16,error),error);
            auto record=std::make_shared<image_assets::Record>();record->texture=std::make_shared<Texture2D>(physical,device);
            TextureDescription description;description.extent={4,1,1};description.format=VK_FORMAT_R8G8B8A8_UNORM;
            description.usage=VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_SAMPLED_BIT;
            Check(record->texture->Create(description,error),error);
            owner.readback=std::make_unique<Buffer>(physical,device);auto& readback=*owner.readback;Check(readback.Create(16,VK_BUFFER_USAGE_TRANSFER_DST_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,error),error);
            auto flight=std::make_shared<image_uploads::Flight>();flight->staging=staging;flight->records.push_back(record);owner.flights.push_back(flight);
            std::weak_ptr<Buffer> weakStaging=staging;std::weak_ptr<const image_assets::Record> weakRecord=record;
            Check(image_uploads::RetireFlights(device,owner.flights,retired,error)&&owner.flights.size()==1,"Unsubmitted owner was retired");
            VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};Vk(vkBeginCommandBuffer(command,&begin),"flight begin");
            vkCmdWaitEvents(command,1,&owner.gate,VK_PIPELINE_STAGE_HOST_BIT,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,0,nullptr,0,nullptr,0,nullptr);
            Check(record->texture->UploadSubresource(command,staging->Handle(),0,0,0,VK_IMAGE_LAYOUT_UNDEFINED,error),error);
            VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};barrier.image=record->texture->Handle();barrier.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
            barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;barrier.oldLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;barrier.newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
            vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&barrier);
            VkBufferImageCopy copy{};copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};copy.imageExtent={4,1,1};
            vkCmdCopyImageToBuffer(command,record->texture->Handle(),VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,readback.Handle(),1,&copy);
            Vk(vkEndCommandBuffer(command),"flight end");VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&command;
            Vk(vkQueueSubmit(queue,1,&submit,owner.fence),"flight submit");owner.pending=true;flight->fence=owner.fence;
            staging.reset();record.reset();flight.reset();
            Check(vkGetFenceStatus(device,owner.fence)==VK_NOT_READY,"Host gate did not keep real copy pending");
            Check(image_uploads::RetireFlights(device,owner.flights,retired,error)&&owner.flights.size()==1&&!weakStaging.expired()&&!weakRecord.expired(),"Pending GPU owners were released");
            Vk(vkSetEvent(device,owner.gate),"flight release gate");Vk(vkWaitForFences(device,1,&owner.fence,VK_TRUE,10000000000ull),"flight actual fence");owner.pending=false;
            unsigned char actual[16]{};Check(readback.Read(0,actual,16,error)&&!std::memcmp(expected,actual,16),"Actual asynchronous upload/readback bytes differ");
            Check(image_uploads::RetireFlights(device,owner.flights,retired,error)&&owner.flights.empty()&&weakStaging.expired()&&weakRecord.expired(),"Completed sole owners were not released");
            std::printf("native_upload_flights=PASS round=%u bytes=16 actual_pending_retained=1 fence_completed=1 owners_released=1\n",round);
        }
        Check(retired==2,"Flight retirement count differs");return true;
    }catch(const std::exception& e){std::printf("native_upload_flights=FAIL %s\n",e.what());return false;}
}
