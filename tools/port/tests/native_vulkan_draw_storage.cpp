#include "draw_storage.h"
#include "graphics_program.h"
#include "attachment_pass.h"
#include "image_usage.h"
#include <cstdio>
#include <stdexcept>
#include <cmath>
namespace {
using namespace bo1::vulkan;
void Check(bool ok,const std::string& e){if(!ok)throw std::runtime_error(e);}
void Vk(VkResult r,const char* e){Check(r==VK_SUCCESS,std::string(e)+" result="+std::to_string(int(r)));}
struct Owner {
 VkDevice device;VkCommandPool commands{};VkEvent gate{};VkFence fence{};bool pending{};
 std::unique_ptr<DrawStorage> storage;std::unique_ptr<GraphicsProgram> program;std::unique_ptr<AttachmentPass> pass;
 std::shared_ptr<Texture2D> color;std::unique_ptr<Buffer> vertices,readback;
 ~Owner(){if(pending){if(gate)vkSetEvent(device,gate);vkDeviceWaitIdle(device);}program.reset();pass.reset();storage.reset();if(gate)vkDestroyEvent(device,gate,nullptr);if(fence)vkDestroyFence(device,fence,nullptr);if(commands)vkDestroyCommandPool(device,commands,nullptr);}
};
}
bool NativeVulkanDrawStorage(VkPhysicalDevice physical,VkDevice device,unsigned family) {
 using namespace bo1::vulkan;
 try {
  Owner owner{device};std::string error;VkPhysicalDeviceProperties properties{};vkGetPhysicalDeviceProperties(physical,&properties);
  const auto alignment=properties.limits.minUniformBufferOffsetAlignment;
  owner.storage=std::make_unique<DrawStorage>(physical,device,8192,1,4);
  DrawStorage::Allocation bad;Check(!owner.storage->Allocate(4096,alignment,bad,error),"Allocation before reset accepted");
  TextureDescription d;d.extent={4,1,1};d.format=VK_FORMAT_R32G32B32A32_SFLOAT;d.usage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  owner.color=std::make_shared<Texture2D>(physical,device);Check(owner.color->Create(d,error),error);owner.pass=std::make_unique<AttachmentPass>(device);Check(owner.pass->Create(owner.color,nullptr,false,error),error);
  owner.vertices=std::make_unique<Buffer>(physical,device);Check(owner.vertices->Create(48,VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,error),error);
  const float vertices[3][4]={{-1,-1,0,1},{-1,3,0,1},{3,-1,0,1}};Check(owner.vertices->Write(0,vertices,sizeof(vertices),error),error);
  owner.readback=std::make_unique<Buffer>(physical,device);Check(owner.readback->Create(128,VK_BUFFER_USAGE_TRANSFER_DST_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,error),error);
  const unsigned vs[]={0xfffe0300,0x0200001f,0x80000000,0x900f0000,0x0200001f,0x80000000,0xe00f0000,0x02000001,0xe00f0000,0x90e40000,0x0000ffff};
  const unsigned ps[]={0xffff0300,0x02000001,0x800f0800,0xa0e40000,0x0000ffff};
  const VkVertexInputBindingDescription binding{0,16,VK_VERTEX_INPUT_RATE_VERTEX};const VkVertexInputAttributeDescription attribute{0,0,VK_FORMAT_R32G32B32A32_SFLOAT,0};
  ProgramDescription program;program.vertexBytecode=vs;program.vertexBytes=sizeof(vs);program.pixelBytecode=ps;program.pixelBytes=sizeof(ps);program.renderPass=owner.pass->RenderPass();program.vertexBindings=&binding;program.vertexBindingCount=1;program.vertexAttributes=&attribute;program.vertexAttributeCount=1;
  owner.program=std::make_unique<GraphicsProgram>(device);Check(owner.program->Create(program,VK_NULL_HANDLE,error),error);
  VkCommandPoolCreateInfo cp{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};cp.queueFamilyIndex=family;cp.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;Vk(vkCreateCommandPool(device,&cp,nullptr,&owner.commands),"arena command pool");
  VkCommandBufferAllocateInfo ai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};ai.commandPool=owner.commands;ai.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;ai.commandBufferCount=1;VkCommandBuffer command{};Vk(vkAllocateCommandBuffers(device,&ai,&command),"arena commands");
  VkFenceCreateInfo fi{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};Vk(vkCreateFence(device,&fi,nullptr,&owner.fence),"arena fence");VkEventCreateInfo ei{VK_STRUCTURE_TYPE_EVENT_CREATE_INFO};Vk(vkCreateEvent(device,&ei,nullptr,&owner.gate),"arena host gate");
  VkQueue queue{};vkGetDeviceQueue(device,family,0,&queue);image_usage::Tracker tracker;VkBuffer previous[4]{};VkDescriptorPool previousPools[4]{};
  for(unsigned round=0;round<2;++round) {
   Check(owner.storage->Reset(error),error);Check(!owner.storage->Allocate(8193,alignment,bad,error),"Oversized page allocation accepted");Check(!owner.storage->Allocate(4096,0,bad,error),"Zero alignment accepted");
   if(round){Vk(vkResetCommandBuffer(command,0),"arena command reset");Vk(vkResetFences(device,1,&owner.fence),"arena fence reset");Vk(vkResetEvent(device,owner.gate),"arena gate reset");}
   VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};Vk(vkBeginCommandBuffer(command,&begin),"arena begin");image_usage::RecordingScope tracking(tracker,command);
   // Real queued work cannot finish until the host opens this gate. This makes
   // the retirement-rejection check deterministic rather than a GPU race.
   vkCmdWaitEvents(command,1,&owner.gate,VK_PIPELINE_STAGE_HOST_BIT,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,0,nullptr,0,nullptr,0,nullptr);
   VkClearColorValue zero{{0,0,0,0}};Check(owner.pass->BeginTracked(command,zero,error),error);
   for(unsigned i=0;i<4;++i) {
    DrawStorage::Allocation a;Check(owner.storage->Allocate(4096,alignment,a,error),error);Check(a.offset%alignment==0,"Unaligned constant page range");
    if(round)Check(a.buffer->Handle()==previous[i]&&a.descriptors==previousPools[i],"Retired pages were not reused");else{previous[i]=a.buffer->Handle();previousPools[i]=a.descriptors;if(i)Check(previous[i]!=previous[i-1]&&previousPools[i]!=previousPools[i-1],"Page/descriptor growth missing");}
    float bank[1024]{};bank[0]=float(round*4+i+1)/16;bank[1]=float(round+1)/4;bank[2]=.5f;bank[3]=1;
    Check(a.buffer->Write(a.offset,bank,sizeof(bank),error),error);
    // CPU source lifetime is independent of the later, deliberately gated GPU draw.
    for(auto& lane:bank)*static_cast<volatile float*>(&lane)=-999.0f;
    ProgramResource resource{};resource.kind=ResourceKind::Constants;resource.registerIndex=0;resource.buffer={a.buffer->Handle(),a.offset,4096};std::array<VkDescriptorSet,2> sets{};
    Check(owner.program->BindResources(a.descriptors,{}, {resource},sets,error),error);owner.program->Bind(command,sets);
    const VkViewport viewport{float(i),1,1,-1,0,1};const VkRect2D scissor{{int(i),0},{1,1}};vkCmdSetViewport(command,0,1,&viewport);vkCmdSetScissor(command,0,1,&scissor);const VkBuffer vb=owner.vertices->Handle();const VkDeviceSize offset=0;vkCmdBindVertexBuffers(command,0,1,&vb,&offset);vkCmdDraw(command,3,1,0,0);
   }
   Check(owner.storage->Pages()==4&&owner.storage->Draws()==4,"Draw page accounting mismatch");Check(!owner.storage->Allocate(4096,alignment,bad,error)&&!bad.buffer&&!bad.descriptors,"Page budget failure must clear output");Check(!owner.storage->Reset(error),"Recorded page reset accepted");
   Check(owner.pass->EndTracked(command,error),error);
   VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};barrier.image=owner.color->Handle();barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;barrier.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};barrier.oldLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;barrier.newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;barrier.srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&barrier);
   VkBufferImageCopy copy{};copy.bufferOffset=round*64;copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};copy.imageExtent={4,1,1};vkCmdCopyImageToBuffer(command,owner.color->Handle(),VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,owner.readback->Handle(),1,&copy);
   barrier.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;barrier.newLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;barrier.srcAccessMask=VK_ACCESS_TRANSFER_READ_BIT;barrier.dstAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,0,0,nullptr,0,nullptr,1,&barrier);
   VkMemoryBarrier host{VK_STRUCTURE_TYPE_MEMORY_BARRIER};host.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;host.dstAccessMask=VK_ACCESS_HOST_READ_BIT;vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&host,0,nullptr,0,nullptr);
   Check(tracking.Validate(error),error);Vk(vkEndCommandBuffer(command),"arena end");VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&command;Vk(vkQueueSubmit(queue,1,&submit,owner.fence),"arena real submit");owner.pending=true;tracking.Commit();Check(owner.storage->Submitted(owner.fence,error),error);
   Check(vkGetFenceStatus(device,owner.fence)==VK_NOT_READY&&!owner.storage->Reset(error),"Pending actual fence permitted page reuse");Vk(vkSetEvent(device,owner.gate),"arena open host gate");Vk(vkWaitForFences(device,1,&owner.fence,VK_TRUE,10000000000ull),"arena actual fence");owner.pending=false;
   std::printf("native_draw_storage=PASS round=%u pages=4 draws=4 actual_pending_rejected=1 fence_completed=1 reused=%u\n",round,round);
  }
  float pixels[32]{};Check(owner.readback->Read(0,pixels,sizeof(pixels),error),error);
  for(unsigned round=0;round<2;++round)for(unsigned i=0;i<4;++i){const float expected[4]={float(round*4+i+1)/16,float(round+1)/4,.5f,1};for(unsigned c=0;c<4;++c)Check(std::isfinite(pixels[(round*4+i)*4+c])&&std::fabs(pixels[(round*4+i)*4+c]-expected[c])<.000001f,"Draw storage GPU constant pixel mismatch");}
  Check(owner.storage->Reset(error),error);Check(owner.storage->Allocate(0,alignment,bad,error)&&bad.offset==0,"Constant-free draw allocation rejected");
  std::printf("native_draw_storage_pixels=PASS pixels=8 independent_ranges=1 pools_grow=1 retired_reuse=1 no_constants=1\n");return true;
 }catch(const std::exception& e){std::fprintf(stderr,"native_draw_storage=FAIL %s\n",e.what());return false;}
}
