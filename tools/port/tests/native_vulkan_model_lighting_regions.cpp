#include "model_lighting_regions.h"
#include "upload_flights.h"
#include <cstdio>
#include <stdexcept>
bool NativeModelLightingRegionChecks();
namespace {
using namespace bo1::vulkan;
void Check(bool b,const std::string& e){if(!b)throw std::runtime_error(e);}
void Vk(VkResult r,const char* e){Check(r==VK_SUCCESS,std::string(e)+": "+std::to_string(int(r)));}
struct Owners {
    VkDevice device;VkCommandPool pool{};VkFence fence{};VkEvent gate{};bool submitted{};
    std::vector<std::shared_ptr<image_uploads::Flight>> flights;std::shared_ptr<Buffer> readback;
    ~Owners(){if(submitted){if(gate)vkSetEvent(device,gate);vkDeviceWaitIdle(device);}flights.clear();
        if(gate)vkDestroyEvent(device,gate,nullptr);if(fence)vkDestroyFence(device,fence,nullptr);if(pool)vkDestroyCommandPool(device,pool,nullptr);}
};
}
bool NativeVulkanModelLightingRegions(VkPhysicalDevice physical,VkDevice device,unsigned family) {
    using namespace bo1::vulkan;using namespace model_lighting_regions;
    try {
        if(!NativeModelLightingRegionChecks())return false;
        for(unsigned test=0;test<4;++test) {
        Owners owner{device};std::string error;const unsigned height=test?16:8;const VkDeviceSize size=256*height*16;
        auto texture=std::make_shared<Texture2D>(physical,device);TextureDescription d;
        d.type=VK_IMAGE_TYPE_3D;d.viewType=VK_IMAGE_VIEW_TYPE_3D;d.extent={256,height,4};
        d.format=VK_FORMAT_B8G8R8A8_UNORM;d.linearSampling=false;d.usage|=VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        Check(texture->Create(d,error),error);
        std::vector<unsigned char> pixels(size,17),expectedOld=pixels;
        auto first=std::make_shared<image_assets::Record>();first->texture=texture;first->description=d;
        first->sourceFormat=21;first->bytesPerTexel=4;first->revision=1;
        first->uploads={{0,0,UploadBytes::Copy(pixels.data(),pixels.size(),error)}};
        auto fillTile=[&](unsigned tile,unsigned char value){for(unsigned z=0;z<4;++z)for(unsigned y=0;y<4;++y)
            std::memset(pixels.data()+Offset(tile,height,z,y),value,16);};
        const unsigned firstTile=test==2?63:0,lastTile=test==0?127:test==1?2:test==2?64:255;
        fillTile(firstTile,55);auto second=std::make_shared<image_assets::Record>(*first);
        second->revision=2;second->regionBaseRevision=1;second->modelLightingRegions=true;second->changedTiles={firstTile};
        second->uploads[0].bytes=UploadBytes::Copy(pixels.data(),pixels.size(),error);
        fillTile(lastTile,99);auto third=std::make_shared<image_assets::Record>(*second);
        third->revision=3;third->regionBaseRevision=2;third->changedTiles={lastTile};
        third->uploads[0].bytes=UploadBytes::Copy(pixels.data(),pixels.size(),error);const auto expectedNew=pixels;
        auto pending=Merge(Pending(third),Pending(second)); // failed-flush restore order
        Check(CanPartial(pending,texture,1,true),"Actual initialized atlas ancestry rejected");
        std::vector<unsigned char> packed;std::vector<VkBufferImageCopy> regions;Box box;bool fullFallback=false;
        if(!test)Pack(*pending.record,pending.tiles,size,packed,regions);
        else if(PlanBox(*pending.record,pending.tiles,box))PackBox(*pending.record,box,size,packed,regions);
        else {fullFallback=true;const auto& bytes=pending.record->uploads[0].bytes;packed.assign(bytes->data(),bytes->data()+bytes->size());}
        const std::size_t expectedBytes=test==0?512:test==1?768:test==2?32768:65536;
        Check(packed.size()==expectedBytes&&fullFallback==(test==3),"Actual merged tile box/fallback plan mismatch");
        if(test&&!fullFallback)Check(regions.size()==1,"Box must issue exactly one Vulkan copy region");
        auto staging=std::make_shared<Buffer>(physical,device);
        Check(staging->Create(size+packed.size(),VK_BUFFER_USAGE_TRANSFER_SRC_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,error),error);
        Check(staging->Write(0,first->uploads[0].bytes->data(),size,error)&&staging->Write(size,packed.data(),packed.size(),error),error);
        pixels.assign(pixels.size(),222);packed.assign(packed.size(),223); // publication owns copied bytes
        auto readback=std::make_shared<Buffer>(physical,device);owner.readback=readback;Check(readback->Create(size*2,VK_BUFFER_USAGE_TRANSFER_DST_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,error),error);
        VkCommandPoolCreateInfo pi{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};pi.queueFamilyIndex=family;Vk(vkCreateCommandPool(device,&pi,nullptr,&owner.pool),"regions pool");
        VkCommandBufferAllocateInfo ai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};ai.commandPool=owner.pool;ai.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;ai.commandBufferCount=1;
        VkCommandBuffer command{};Vk(vkAllocateCommandBuffers(device,&ai,&command),"regions command");
        VkEventCreateInfo ei{VK_STRUCTURE_TYPE_EVENT_CREATE_INFO};Vk(vkCreateEvent(device,&ei,nullptr,&owner.gate),"regions gate");
        VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};Vk(vkBeginCommandBuffer(command,&bi),"regions begin");
        vkCmdWaitEvents(command,1,&owner.gate,VK_PIPELINE_STAGE_HOST_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,nullptr,0,nullptr,0,nullptr);
        Check(texture->UploadSubresource(command,staging->Handle(),0,0,0,VK_IMAGE_LAYOUT_UNDEFINED,error),error);
        auto copyOut=[&](VkDeviceSize offset){
            VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};b.image=texture->Handle();b.srcQueueFamilyIndex=b.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
            b.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};b.oldLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;b.newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            b.srcAccessMask=VK_ACCESS_SHADER_READ_BIT;b.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
            const auto stages=VK_PIPELINE_STAGE_VERTEX_SHADER_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            vkCmdPipelineBarrier(command,stages,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&b);
            VkBufferImageCopy c{};c.bufferOffset=offset;c.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};c.imageExtent=d.extent;
            vkCmdCopyImageToBuffer(command,texture->Handle(),VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,readback->Handle(),1,&c);
            b.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;b.newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            b.srcAccessMask=VK_ACCESS_TRANSFER_READ_BIT;b.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
            vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,stages,0,0,nullptr,0,nullptr,1,&b);
        };
        copyOut(0);
        if(fullFallback)Check(texture->UploadSubresource(command,staging->Handle(),size,0,0,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,error),error);
        else {
            auto invalid=regions;invalid[0].imageOffset.x=256;
            Check(!texture->UploadRegions(command,staging->Handle(),staging->Size(),invalid,error),"Out-of-bounds GPU region accepted");
            Check(!texture->UploadRegions(command,staging->Handle(),size,regions,error),"Out-of-bounds staging accepted");
            Check(texture->UploadRegions(command,staging->Handle(),staging->Size(),regions,error),error);
        }
        copyOut(size);
        VkMemoryBarrier host{VK_STRUCTURE_TYPE_MEMORY_BARRIER};host.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;host.dstAccessMask=VK_ACCESS_HOST_READ_BIT;
        vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&host,0,nullptr,0,nullptr);
        Vk(vkEndCommandBuffer(command),"regions end");VkFenceCreateInfo fi{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};Vk(vkCreateFence(device,&fi,nullptr,&owner.fence),"regions fence");
        auto flight=std::make_shared<image_uploads::Flight>();flight->records={first,third};flight->staging=staging;flight->fence=owner.fence;owner.flights.push_back(flight);
        std::weak_ptr<const image_assets::Record> weak=third;std::weak_ptr<Texture2D> weakTexture=texture;
        VkQueue queue{};vkGetDeviceQueue(device,family,0,&queue);VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};si.commandBufferCount=1;si.pCommandBuffers=&command;
        Vk(vkQueueSubmit(queue,1,&si,owner.fence),"regions submit");owner.submitted=true;
        first.reset();second.reset();third.reset();pending.record.reset();staging.reset();texture.reset();flight.reset();
        unsigned retired{};Check(image_uploads::RetireFlights(device,owner.flights,retired,error)&&!retired&&!weak.expired()&&!weakTexture.expired(),"Pending fence lost immutable owners");
        Vk(vkSetEvent(device,owner.gate),"regions release gate");Vk(vkWaitForFences(device,1,&owner.fence,VK_TRUE,10000000000ull),"regions actual completion");owner.submitted=false;
        std::vector<unsigned char> actual(size*2);Check(readback->Read(0,actual.data(),actual.size(),error),error);
        Check(!std::memcmp(actual.data(),expectedOld.data(),size),"Queued pre-update read changed");
        Check(!std::memcmp(actual.data()+size,expectedNew.data(),size),"Sparse update changed untouched texels or lost union");
        Check(image_uploads::RetireFlights(device,owner.flights,retired,error)&&retired==1&&weak.expired()&&weakTexture.expired(),"Completed fence did not retire owners");
        if(!test)std::printf("native_model_lighting_regions_gpu=PASS old_new_bytes=%llu region_bytes=512 untouched_preserved=1 source_overwrite=1 pending_fence_owners=1 fence_completed=1\n",(unsigned long long)(size*2));
        else std::printf("native_model_lighting_boxes_gpu=PASS case=%u staged_bytes=%zu region_count=1 full_fallback=%u holes_preserved=1 source_overwrite=1 pending_fence_owners=1 fence_completed=1\n",test,expectedBytes,unsigned(fullFallback));
        }
        return true;
    }catch(const std::exception& e){std::fprintf(stderr,"native_model_lighting_regions_gpu=FAIL %s\n",e.what());return false;}
}
