#include "resources.h"
#include "attachment_pass.h"
#include "image_usage.h"
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <vector>
#include <cstring>
namespace {
using namespace bo1::vulkan;
void Check(bool ok,const std::string& e){if(!ok)throw std::runtime_error(e);}
void Vk(VkResult r,const char* e){Check(r==VK_SUCCESS,std::string(e)+" result="+std::to_string(int(r)));}
struct Owner {
 VkDevice device;VkCommandPool pool{};VkFence fence{};bool pending{};
 std::vector<std::shared_ptr<Texture2D>> images;std::vector<std::unique_ptr<AttachmentPass>> passes;std::unique_ptr<Buffer> readback;
 ~Owner(){if(pending)vkDeviceWaitIdle(device);passes.clear();if(fence)vkDestroyFence(device,fence,nullptr);if(pool)vkDestroyCommandPool(device,pool,nullptr);}
};
}
bool NativeVulkanDepthCopy(VkPhysicalDevice physical,VkDevice device,unsigned family) {
 using namespace bo1::vulkan;
 try {
  Owner owner{device};std::string error;constexpr unsigned width=8,height=4;
  auto image=[&](unsigned w,VkImageUsageFlags usage){auto t=std::make_shared<Texture2D>(physical,device);TextureDescription d;d.extent={w,height,1};d.format=VK_FORMAT_D24_UNORM_S8_UINT;d.aspects=VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT;d.linearSampling=false;d.usage=usage;Check(t->Create(d,error),error);owner.images.push_back(t);return t;};
  constexpr VkImageUsageFlags usage=VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  const auto source=image(width,usage),destination=image(width,usage),wrongExtent=image(width+1,usage),noTransfer=image(width,VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
  auto pass=[&](std::shared_ptr<Texture2D> t,bool load){auto p=std::make_unique<AttachmentPass>(device);Check(p->Create(nullptr,t,true,error,false,int(load)),error);auto result=p.get();owner.passes.push_back(std::move(p));return result;};
  auto clear=pass(source,false),resume=pass(source,true);
  auto colourImage=std::make_shared<Texture2D>(physical,device);TextureDescription cd;
  cd.extent={width,height,1};cd.format=VK_FORMAT_R8G8B8A8_UNORM;cd.aspects=VK_IMAGE_ASPECT_COLOR_BIT;
  cd.usage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT;
  Check(colourImage->Create(cd,error),error);owner.images.push_back(colourImage);
  auto colourPass=std::make_unique<AttachmentPass>(device);
  Check(colourPass->Create(colourImage,nullptr,false,error),error);
  Check(!colourPass->HasDepth()&&resume->HasDepth(),"Actual attachment identity incorrect");
  auto withoutDepth=colourPass.get();owner.passes.push_back(std::move(colourPass));
  owner.readback=std::make_unique<Buffer>(physical,device);Check(owner.readback->Create(width*height*4*4,VK_BUFFER_USAGE_TRANSFER_DST_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,error),error);
  VkCommandPoolCreateInfo cp{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};cp.queueFamilyIndex=family;Vk(vkCreateCommandPool(device,&cp,nullptr,&owner.pool),"copy pool");
  VkCommandBufferAllocateInfo ai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};ai.commandPool=owner.pool;ai.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;ai.commandBufferCount=1;VkCommandBuffer command{};Vk(vkAllocateCommandBuffers(device,&ai,&command),"copy commands");
  VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};Vk(vkBeginCommandBuffer(command,&begin),"copy begin");
  Check(!image_usage::Copy(command,source,destination,error),"Missing scope accepted");
  image_usage::Tracker tracker;image_usage::RecordingScope tracking(tracker,command);
  Check(!image_usage::Copy(command,source,destination,error),"Undefined source accepted");
  Check(!image_usage::Copy(command,source,source,error),"Self copy accepted");
  Check(!image_usage::Copy(command,source,wrongExtent,error),"Mismatched extent accepted");
  Check(!image_usage::Copy(command,source,noTransfer,error),"Missing transfer usage accepted");
  VkClearColorValue colour{{0.2f,0.4f,0.6f,1}};Check(clear->BeginTracked(command,colour,error,0.25f,5),error);
  Check(!image_usage::Copy(command,source,destination,error),"Active source accepted");
  Check(clear->EndTracked(command,error),error);
  Check(image_usage::Copy(command,source,destination,error),error);
  Check(image_usage::CanLoad(source)&&image_usage::CanLoad(destination),"Copy did not preserve LOAD contents");
  VkImageLayout layout{};Check(image_usage::Sample(destination,layout,error)&&layout==VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,"Copy destination not sampleable");
  Check(tracker.submitted.empty(),"Recording published before submit");
  auto read=[&](std::shared_ptr<Texture2D> t,unsigned index){
   VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};b.image=t->Handle();b.srcQueueFamilyIndex=b.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;b.subresourceRange={VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT,0,1,0,1};b.oldLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;b.newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;b.srcAccessMask=VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT|VK_ACCESS_TRANSFER_WRITE_BIT|VK_ACCESS_SHADER_READ_BIT;b.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
   const VkPipelineStageFlags producers=VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_TRANSFER_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
   vkCmdPipelineBarrier(command,producers,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&b);
   VkBufferImageCopy c{};c.bufferOffset=index*width*height*4;c.imageSubresource={VK_IMAGE_ASPECT_DEPTH_BIT,0,0,1};c.imageExtent={width,height,1};vkCmdCopyImageToBuffer(command,t->Handle(),VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,owner.readback->Handle(),1,&c);
   c.bufferOffset=width*height*12+index*width*height;c.imageSubresource.aspectMask=VK_IMAGE_ASPECT_STENCIL_BIT;vkCmdCopyImageToBuffer(command,t->Handle(),VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,owner.readback->Handle(),1,&c);
   b.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;b.newLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;b.srcAccessMask=VK_ACCESS_TRANSFER_READ_BIT;b.dstAccessMask=VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
   vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,producers,0,0,nullptr,0,nullptr,1,&b);
  };
  read(destination,0);
  // Real colour-only work between depth production and resumed depth LOAD.
  // Subsequent depth/stencil readbacks prove the untouched half survived.
  Check(withoutDepth->BeginTracked(command,colour,error),error);
  VkClearAttachment ca{};ca.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT;ca.clearValue.color=colour;
  VkClearRect cr{{{0,0},{width,height}},0,1};vkCmdClearAttachments(command,1,&ca,1,&cr);
  Check(withoutDepth->EndTracked(command,error),error);
  Check(image_usage::CanLoad(source),"Colour-only pass discarded retained depth");
  Check(resume->BeginTracked(command,colour,error),error);
  Check(!image_usage::Copy(command,destination,source,error),"Active destination accepted");
  VkClearAttachment red{};red.aspectMask=VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT;red.clearValue.depthStencil={0.75f,9};
  VkClearRect rect{{{0,0},{width/2,height}},0,1};vkCmdClearAttachments(command,1,&red,1,&rect);Check(resume->EndTracked(command,error),error);
  Check(image_usage::Copy(command,source,destination,error),error);read(source,1);read(destination,2);
  VkMemoryBarrier host{VK_STRUCTURE_TYPE_MEMORY_BARRIER};host.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;host.dstAccessMask=VK_ACCESS_HOST_READ_BIT;vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&host,0,nullptr,0,nullptr);
  Check(tracking.Validate(error)&&tracker.submitted.empty(),"Copy journal validation/publication failed");Vk(vkEndCommandBuffer(command),"copy end");
  VkFenceCreateInfo fi{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};Vk(vkCreateFence(device,&fi,nullptr,&owner.fence),"copy fence");VkQueue queue{};vkGetDeviceQueue(device,family,0,&queue);VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&command;Vk(vkQueueSubmit(queue,1,&submit,owner.fence),"copy submit");owner.pending=true;tracking.Commit();
  Check(tracker.submitted.size()==3,"Copy submit state absent");Vk(vkWaitForFences(device,1,&owner.fence,VK_TRUE,10000000000ull),"copy actual fence");owner.pending=false;
  std::vector<unsigned char> pixels(width*height*4*4);Check(owner.readback->Read(0,pixels.data(),pixels.size(),error),error);
  for(unsigned i=0;i<3;++i){for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x){
   const bool changed=i&&x<width/2;std::uint32_t value{};std::memcpy(&value,&pixels[((i*height+y)*width+x)*4],4);
   const auto expected=std::uint32_t((changed?0.75:0.25)*16777215.0+0.5);value&=0xffffffu;
   const unsigned stencil=pixels[width*height*12+(i*height+y)*width+x];
   if(value+1<expected||value>expected+1||stencil!=(changed?9u:5u))throw std::runtime_error("Depth/stencil copy mismatch value="+std::to_string(value)+" expected="+std::to_string(expected)+" stencil="+std::to_string(stencil));
  }std::printf("native_depth_copy=PASS case=%u depth_stencil_pixels=32 fence_completed=1\n",i);}
  std::printf("native_depth_copy_journal=PASS ordered_load=1 colour_only_preserves_depth_stencil=1 overwrite=1 negatives=7 submit_publication=1\n");return true;
 }catch(const std::exception& e){std::fprintf(stderr,"native_depth_copy=FAIL %s\n",e.what());return false;}
}
