#include "constant_upload.h"
#include "graphics_program.h"
#include "attachment_pass.h"
#include "image_usage.h"
#include <cstdio>
#include <cmath>
#include <stdexcept>
namespace {
using namespace bo1::vulkan;
void Require(bool ok,const std::string& reason){if(!ok)throw std::runtime_error(reason);}
void Vk(VkResult result,const char* reason){Require(result==VK_SUCCESS,std::string(reason)+": "+std::to_string(int(result)));}
struct Owner {
 VkDevice device;VkCommandPool commands{};VkFence fence{};VkEvent gate{};bool pending{};
 std::unique_ptr<DrawStorage> storage;std::unique_ptr<GraphicsProgram> program;
 std::shared_ptr<Texture2D> color;std::unique_ptr<AttachmentPass> pass;std::unique_ptr<Buffer> vertices,readback;
 ~Owner(){if(pending){vkSetEvent(device,gate);vkDeviceWaitIdle(device);}program.reset();pass.reset();storage.reset();if(gate)vkDestroyEvent(device,gate,nullptr);if(fence)vkDestroyFence(device,fence,nullptr);if(commands)vkDestroyCommandPool(device,commands,nullptr);}
};
}
bool NativeVulkanConstantUpload(VkPhysicalDevice physical,VkDevice device,unsigned family) {
 using namespace bo1::vulkan;
 try {
  Owner owner{device};std::string error;VkPhysicalDeviceProperties properties{};vkGetPhysicalDeviceProperties(physical,&properties);
  const auto alignment=properties.limits.minUniformBufferOffsetAlignment;
  owner.storage=std::make_unique<DrawStorage>(physical,device,4096,2,8);
  TextureDescription description;description.extent={6,1,1};description.format=VK_FORMAT_R32G32B32A32_SFLOAT;description.usage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  owner.color=std::make_shared<Texture2D>(physical,device);Require(owner.color->Create(description,error),error);
  owner.pass=std::make_unique<AttachmentPass>(device);Require(owner.pass->Create(owner.color,nullptr,false,error),error);
  owner.vertices=std::make_unique<Buffer>(physical,device);Require(owner.vertices->Create(48,VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,error),error);
  const float vertices[3][4]={{-1,-1,0,1},{-1,3,0,1},{3,-1,0,1}};Require(owner.vertices->Write(0,vertices,sizeof(vertices),error),error);
  owner.readback=std::make_unique<Buffer>(physical,device);Require(owner.readback->Create(192,VK_BUFFER_USAGE_TRANSFER_DST_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,error),error);
  // VS emits c0 as TEXCOORD0; PS adds its independent c0 to that varying.
  const unsigned vs[]={0xfffe0300,0x0200001f,0x80000000,0x900f0000,0x0200001f,0x80000000,0xe00f0000,0x0200001f,0x80000005,0xe00f0001,0x02000001,0xe00f0000,0x90e40000,0x02000001,0xe00f0001,0xa0e40000,0x0000ffff};
  const unsigned ps[]={0xffff0300,0x0200001f,0x80000005,0x900f0000,0x03000002,0x800f0800,0x90e40000,0xa0e40000,0x0000ffff};
  const VkVertexInputBindingDescription binding{0,16,VK_VERTEX_INPUT_RATE_VERTEX};const VkVertexInputAttributeDescription attribute{0,0,VK_FORMAT_R32G32B32A32_SFLOAT,0};
  ProgramDescription program;program.vertexBytecode=vs;program.vertexBytes=sizeof(vs);program.pixelBytecode=ps;program.pixelBytes=sizeof(ps);program.renderPass=owner.pass->RenderPass();program.vertexBindings=&binding;program.vertexBindingCount=1;program.vertexAttributes=&attribute;program.vertexAttributeCount=1;
  owner.program=std::make_unique<GraphicsProgram>(device);Require(owner.program->Create(program,VK_NULL_HANDLE,error),error);
  VkCommandPoolCreateInfo cp{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};cp.queueFamilyIndex=family;cp.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;Vk(vkCreateCommandPool(device,&cp,nullptr,&owner.commands),"constant commands");
  VkCommandBufferAllocateInfo ai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};ai.commandPool=owner.commands;ai.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;ai.commandBufferCount=1;VkCommandBuffer command{};Vk(vkAllocateCommandBuffers(device,&ai,&command),"constant command allocation");
  VkFenceCreateInfo fi{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};Vk(vkCreateFence(device,&fi,nullptr,&owner.fence),"constant fence");VkEventCreateInfo ei{VK_STRUCTURE_TYPE_EVENT_CREATE_INFO};Vk(vkCreateEvent(device,&ei,nullptr,&owner.gate),"constant gate");
  VkQueue queue{};vkGetDeviceQueue(device,family,0,&queue);image_usage::Tracker tracker;VkBuffer firstBuffer{};
  for(unsigned round=0;round<2;++round) {
   Require(owner.storage->Reset(error),error);BindCache cache;
   if(round){Vk(vkResetCommandBuffer(command,0),"constant reset");Vk(vkResetFences(device,1,&owner.fence),"constant fence reset");Vk(vkResetEvent(device,owner.gate),"constant gate reset");}
   VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};Vk(vkBeginCommandBuffer(command,&begin),"constant begin");image_usage::RecordingScope tracking(tracker,command);
   vkCmdWaitEvents(command,1,&owner.gate,VK_PIPELINE_STAGE_HOST_BIT,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,0,nullptr,0,nullptr,0,nullptr);
   VkClearColorValue clear{{0,0,0,0}};Require(owner.pass->BeginTracked(command,clear,error),error);
   VkDescriptorBufferInfo previous[2]{};
   for(unsigned i=0;i<6;++i) {
    float vertex[8]={float(round+1)/8+(i>=2?.125f:0),0,0,.5f,0,0,0,0};float pixel[8]={0,i>=1?.5f:.25f,0,.5f,0,0,0,0};
    const void* data[2]={vertex,pixel};const VkDeviceSize sizes[2]={16,i>=4?32u:16u};VkDescriptorBufferInfo ranges[2]{};VkDescriptorPool pool{};cache.reuseConstants=i!=5;
    Require(UploadConstantRanges(*owner.storage,alignment,data,sizes,&cache,ranges,pool,error),error);
    if(i==0){Require(cache.constantUploadMisses==2&&cache.constantUploadHits==0,"fresh frame reused stale constants");if(round)Require(ranges[0].buffer==firstBuffer,"retired page not reused");else firstBuffer=ranges[0].buffer;}
    if(i==1||i==3||i==4)Require(ranges[0].buffer==previous[0].buffer&&ranges[0].offset==previous[0].offset,"unchanged VS reallocated");
    if(i==2||i==3)Require(ranges[1].buffer==previous[1].buffer&&ranges[1].offset==previous[1].offset,"unchanged PS reallocated across pages");
    previous[0]=ranges[0];previous[1]=ranges[1];
    for(auto& value:vertex)value=-99;for(auto& value:pixel)value=-99; // GPU must use uploaded bytes.
    ProgramResource v{},p{};v.kind=p.kind=ResourceKind::Constants;v.buffer=ranges[0];p.buffer=ranges[1];
    Require(owner.program->Record(command,pool,{v},{p},cache,error),error);
    const VkViewport viewport{float(i),1,1,-1,0,1};const VkRect2D scissor{{int(i),0},{1,1}};vkCmdSetViewport(command,0,1,&viewport);vkCmdSetScissor(command,0,1,&scissor);
    const VkBuffer vb=owner.vertices->Handle();const VkDeviceSize offset=0;vkCmdBindVertexBuffers(command,0,1,&vb,&offset);vkCmdDraw(command,3,1,0,0);
   }
   Require(cache.constantUploadHits==5&&cache.constantUploadMisses==7,"constant reuse accounting mismatch");
   Require(owner.storage->Pages()==3&&owner.storage->Draws()==6,"descriptor reservation/page rollover mismatch");Require(!owner.storage->Reset(error),"reset accepted recorded readers");
   Require(owner.pass->EndTracked(command,error),error);
   VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};barrier.image=owner.color->Handle();barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;barrier.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};barrier.oldLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;barrier.newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;barrier.srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&barrier);
   VkBufferImageCopy copy{};copy.bufferOffset=round*96;copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};copy.imageExtent={6,1,1};vkCmdCopyImageToBuffer(command,owner.color->Handle(),VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,owner.readback->Handle(),1,&copy);
   barrier.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;barrier.newLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;barrier.srcAccessMask=VK_ACCESS_TRANSFER_READ_BIT;barrier.dstAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,0,0,nullptr,0,nullptr,1,&barrier);
   VkMemoryBarrier host{VK_STRUCTURE_TYPE_MEMORY_BARRIER};host.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;host.dstAccessMask=VK_ACCESS_HOST_READ_BIT;vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&host,0,nullptr,0,nullptr);
   Require(tracking.Validate(error),error);Vk(vkEndCommandBuffer(command),"constant end");VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&command;Vk(vkQueueSubmit(queue,1,&submit,owner.fence),"constant submit");owner.pending=true;tracking.Commit();Require(owner.storage->Submitted(owner.fence,error),error);
   Require(vkGetFenceStatus(device,owner.fence)==VK_NOT_READY&&!owner.storage->Reset(error),"pending fence allowed reset");Vk(vkSetEvent(device,owner.gate),"constant open gate");Vk(vkWaitForFences(device,1,&owner.fence,VK_TRUE,10000000000ull),"constant retire");owner.pending=false;
  }
  float pixels[48]{};Require(owner.readback->Read(0,pixels,sizeof(pixels),error),error);
  for(unsigned round=0;round<2;++round)for(unsigned i=0;i<6;++i){const float expected[4]={float(round+1)/8+(i>=2?.125f:0),i>=1?.5f:.25f,0,1};for(unsigned c=0;c<4;++c)Require(std::isfinite(pixels[(round*6+i)*4+c])&&std::fabs(pixels[(round*6+i)*4+c]-expected[c])<.000001f,"constant upload GPU pixel mismatch");}
  std::puts("native_constant_upload=PASS pixels=12 stages=2 independent_changes=1 same_bytes=1 range_change=1 disabled=1 page_rollover=1 fresh_cache_after_fence=1");return true;
 }catch(const std::exception& e){std::fprintf(stderr,"native_constant_upload=FAIL %s\n",e.what());return false;}
}
