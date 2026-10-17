#include "texture_bindings.h"
#include "attachment_pass.h"
#include "image_usage.h"
#include <cstdio>
#include <stdexcept>
// This fixture supplies explicit snapshots; asset-registry calls fail closed.
namespace bo1::vulkan::image_assets {
std::shared_ptr<const Record> Acquire(const GfxImage*){return {};}
std::string MissingOwnerDiagnostic(const GfxImage*){return "Fixture has no asset registry";}
}
namespace {
using namespace bo1::vulkan;
void Check(bool ok,const std::string& e){if(!ok)throw std::runtime_error(e);}
void Vk(VkResult r,const char* e){Check(r==VK_SUCCESS,std::string(e)+" result="+std::to_string(int(r)));}
struct Owner {
 VkDevice device;VkCommandPool pool{};VkFence fence{};VkDescriptorPool descriptors{};bool pending{};
 std::shared_ptr<texture_bindings::Sampler> samplerOwner;
 std::shared_ptr<Texture2D> source,target,noSample;std::unique_ptr<AttachmentPass> producer,consumer;
 std::unique_ptr<GraphicsProgram> program;std::unique_ptr<Buffer> vertices,readback;
 ~Owner(){if(pending)vkDeviceWaitIdle(device);program.reset();consumer.reset();producer.reset();if(descriptors)vkDestroyDescriptorPool(device,descriptors,nullptr);if(fence)vkDestroyFence(device,fence,nullptr);if(pool)vkDestroyCommandPool(device,pool,nullptr);}
};
}
bool NativeVulkanSamplingValidation(VkPhysicalDevice physical,VkDevice device,unsigned family){
 using namespace bo1::vulkan;using namespace texture_bindings;
 struct Restore{bool enabled=SamplingValidationReuseEnabled();~Restore(){SetSamplingValidationReuse(enabled);}} restore;
 try{
  Owner o{device};std::string error;unsigned cases=0;
  auto make=[&](VkImageUsageFlags usage){auto t=std::make_shared<Texture2D>(physical,device);TextureDescription d;d.extent={1,1,1};d.format=VK_FORMAT_R8G8B8A8_UNORM;d.usage=usage;Check(t->Create(d,error),error);return t;};
  o.source=make(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT);o.target=make(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT);o.noSample=make(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
  auto sampler=Sampler::Acquire(device,0x03f01100,1,0,error);Check(bool(sampler),error);o.samplerOwner=sampler;
  auto record=std::make_shared<image_assets::Record>();record->texture=o.source;record->description=o.source->Description();record->createdName="sampling-fixture";
  Snapshot snapshot;snapshot.slots[0]={record,sampler};snapshot.slots[1]=snapshot.slots[0];
  ShaderTranslation reflected;reflected.bindings={{ResourceKind::Texture,0,1,0,2},{ResourceKind::Sampler,0,1,1,2},{ResourceKind::Texture,1,1,2,2},{ResourceKind::Sampler,1,1,3,2}};
  bool scratchMode=false;std::vector<ProgramResource> addedScratch;
  auto check=[&](bool expected){std::vector<ProgramResource> resources(1);resources[0].registerIndex=99;error.clear();bool ok=scratchMode?AppendResourcesWithScratch(snapshot,reflected,resources,addedScratch,error):AppendResources(snapshot,reflected,resources,error);Check(ok==expected,"Unexpected sampling validation: "+error);Check(resources[0].registerIndex==99&&resources.size()==(ok?5u:1u),"Append failure was not atomic");++cases;return resources;};
  // Compare both modes, including no recording scope, and preserve the legacy
  // complete-upload exception without inventing any generated-image contents.
  for(bool useScratch:{false,true})for(bool enabled:{false,true}){scratchMode=useScratch;SetSamplingValidationReuse(enabled);check(false);record->uploads.resize(1);check(true);record->description.mipLevels=2;check(false);record->description.mipLevels=1;record->uploads.clear();}
  VkCommandPoolCreateInfo cp{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};cp.queueFamilyIndex=family;Vk(vkCreateCommandPool(device,&cp,nullptr,&o.pool),"sampling pool");
  VkCommandBufferAllocateInfo ai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};ai.commandPool=o.pool;ai.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;ai.commandBufferCount=1;VkCommandBuffer command{};Vk(vkAllocateCommandBuffers(device,&ai,&command),"sampling command");
  VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};Vk(vkBeginCommandBuffer(command,&begin),"sampling begin");
  image_usage::Tracker tracker;image_usage::RecordingScope tracking(tracker,command);
  for(bool useScratch:{false,true})for(bool enabled:{false,true}){
   scratchMode=useScratch;SetSamplingValidationReuse(enabled);check(false);record->uploads.resize(1);check(true);record->description.mipLevels=2;check(false);
   auto& state=tracking.pending[o.source.get()];state.image=o.source;state.layout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;state.contents=true;
   check(true); // Known generated state retains the original incomplete-upload bypass.
   state.active=true;check(false);state.active=false;state.contents=false;check(false);state.contents=true;state.layout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;check(false);
   state.layout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
   auto bad=std::make_shared<image_assets::Record>(*record);bad->texture=o.noSample;tracking.pending[o.noSample.get()]={o.noSample,state.layout,true,false};snapshot.slots[1].image=bad;check(false);snapshot.slots[1].image=record;
   // A later invalid slot must not publish any earlier resolved resources.
   snapshot.slots[1].sampler.reset();check(false);snapshot.slots[1].sampler=sampler;
   record->description.mipLevels=1;record->uploads.clear();tracking.pending.clear();
  }
  o.producer=std::make_unique<AttachmentPass>(device);Check(o.producer->Create(o.source,nullptr,false,error),error);
  o.consumer=std::make_unique<AttachmentPass>(device);Check(o.consumer->Create(o.target,nullptr,false,error),error);
  VkClearColorValue colour{{0.2f,0.4f,0.6f,1}};Check(o.producer->BeginTracked(command,colour,error),error);SetSamplingValidationReuse(true);check(false);Check(o.producer->EndTracked(command,error),error);
  auto before=ReadSamplingValidationProfile();auto resources=check(true);auto after=ReadSamplingValidationProfile();Check(after.lookups-before.lookups==2&&after.slotReuses-before.slotReuses==2,"Aliased slots must validate independently once");
  Check(resources[1].image.imageLayout==VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL&&resources[2].image.imageLayout==VK_IMAGE_LAYOUT_UNDEFINED,"Texture/sampler layout changed");
  // Actual generated attachment -> texture shader -> independent target pixels.
  const unsigned vs[]={0xfffe0300,0x0200001f,0x80000000,0x900f0000,0x0200001f,0x80000000,0xe00f0000,0x02000001,0xe00f0000,0x90e40000,0x0000ffff};
  const unsigned ps[]={0xffff0300,0x0200001f,0x90000000,0xa00f0800,0x05000051,0xa00f0000,0x3f000000,0x3f000000,0,0,0x02000001,0x800f0001,0xa0e40000,0x03000042,0x800f0000,0x80e40001,0xa0e40800,0x02000001,0x800f0800,0x80e40000,0x0000ffff};
  ShaderTranslation translated;Check(TranslateShader(ps,sizeof(ps),translated),translated.diagnostics);
  VkVertexInputBindingDescription binding{0,16,VK_VERTEX_INPUT_RATE_VERTEX};VkVertexInputAttributeDescription attribute{0,0,VK_FORMAT_R32G32B32A32_SFLOAT,0};
  ProgramDescription description;description.vertexBytecode=vs;description.vertexBytes=sizeof(vs);description.pixelBytecode=ps;description.pixelBytes=sizeof(ps);description.renderPass=o.consumer->PipelineRenderPass();description.vertexBindings=&binding;description.vertexBindingCount=1;description.vertexAttributes=&attribute;description.vertexAttributeCount=1;
  o.program=std::make_unique<GraphicsProgram>(device);Check(o.program->Create(description,VK_NULL_HANDLE,error),error);
  o.vertices=std::make_unique<Buffer>(physical,device);Check(o.vertices->Create(48,VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,error),error);const float triangle[12]={-1,-1,0,1,-1,3,0,1,3,-1,0,1};Check(o.vertices->Write(0,triangle,sizeof(triangle),error),error);
  o.readback=std::make_unique<Buffer>(physical,device);Check(o.readback->Create(4,VK_BUFFER_USAGE_TRANSFER_DST_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,error),error);
  VkDescriptorPoolSize sizes[]={{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,2},{VK_DESCRIPTOR_TYPE_SAMPLER,2},{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,2}};VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};pool.maxSets=2;pool.poolSizeCount=3;pool.pPoolSizes=sizes;Vk(vkCreateDescriptorPool(device,&pool,nullptr,&o.descriptors),"sampling descriptors");
  resources.clear();Check(AppendResourcesWithScratch(snapshot,translated,resources,addedScratch,error),error);std::array<VkDescriptorSet,2> sets{};Check(o.program->BindResources(o.descriptors,{},resources,sets,error),error);
  for(auto& resource:resources)resource={};resources.clear();
  for(auto& resource:addedScratch)resource={};addedScratch.clear();
  VkClearColorValue zero{};Check(o.consumer->BeginTracked(command,zero,error),error);VkViewport viewport{0,0,1,1,0,1};VkRect2D scissor{{0,0},{1,1}};vkCmdSetViewport(command,0,1,&viewport);vkCmdSetScissor(command,0,1,&scissor);o.program->Bind(command,sets);auto vertex=o.vertices->Handle();VkDeviceSize offset=0;vkCmdBindVertexBuffers(command,0,1,&vertex,&offset);vkCmdDraw(command,3,1,0,0);Check(o.consumer->EndTracked(command,error),error);
  VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};barrier.image=o.target->Handle();barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;barrier.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};barrier.oldLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;barrier.newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;barrier.srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&barrier);
  VkBufferImageCopy copy{};copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};copy.imageExtent={1,1,1};vkCmdCopyImageToBuffer(command,o.target->Handle(),VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,o.readback->Handle(),1,&copy);
  VkMemoryBarrier host{VK_STRUCTURE_TYPE_MEMORY_BARRIER};host.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;host.dstAccessMask=VK_ACCESS_HOST_READ_BIT;vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&host,0,nullptr,0,nullptr);
  Check(tracking.Validate(error),error);Vk(vkEndCommandBuffer(command),"sampling end");VkFenceCreateInfo fence{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};Vk(vkCreateFence(device,&fence,nullptr,&o.fence),"sampling fence");VkQueue queue{};vkGetDeviceQueue(device,family,0,&queue);VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&command;Vk(vkQueueSubmit(queue,1,&submit,o.fence),"sampling submit");o.pending=true;tracking.Commit();Vk(vkWaitForFences(device,1,&o.fence,VK_TRUE,10000000000ull),"sampling wait");o.pending=false;
  unsigned char pixel[4];Check(o.readback->Read(0,pixel,4,error),error);Check(pixel[0]==51&&pixel[1]==102&&pixel[2]==153&&pixel[3]==255,"Generated sampling pixel mismatch");
  std::puts("native_resource_scratch_texture=PASS wrapper_and_scratch_validation=1 destination_transaction=1 retry_after_failure=1 cpu_vectors_overwritten=1 gpu_pixels=1");
  std::printf("native_sampling_validation=PASS cases=%u gpu_pixels=1 alias_lookups=2 slot_reuses=2 fence_completed=1\n",cases);return true;
 }catch(const std::exception& e){std::fprintf(stderr,"native_sampling_validation=FAIL %s\n",e.what());return false;}
}
