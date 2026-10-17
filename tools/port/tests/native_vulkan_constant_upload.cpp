#include "constant_upload.h"
#include "material_bindings.h"
#include "resource_scratch.h"
#include "graphics_program.h"
#include "attachment_pass.h"
#include "image_usage.h"
#include "constant_state.h"
#include <cstdio>
#include <cmath>
#include <stdexcept>
namespace {
using namespace bo1::vulkan;
void Require(bool ok,const std::string& reason){if(!ok)throw std::runtime_error(reason);}
void Vk(VkResult result,const char* reason){Require(result==VK_SUCCESS,std::string(reason)+": "+std::to_string(int(result)));}
struct Owner {
 VkDevice device;VkCommandPool commands{};VkFence fence{};VkEvent gate{};bool pending{};
 std::unique_ptr<DrawStorage> storage;std::shared_ptr<GraphicsProgram> program;
 std::shared_ptr<Texture2D> color;std::unique_ptr<AttachmentPass> pass;std::unique_ptr<Buffer> vertices,readback;
 ~Owner(){if(pending){vkSetEvent(device,gate);vkDeviceWaitIdle(device);}program.reset();pass.reset();storage.reset();if(gate)vkDestroyEvent(device,gate,nullptr);if(fence)vkDestroyFence(device,fence,nullptr);if(commands)vkDestroyCommandPool(device,commands,nullptr);}
};
}
bool NativeVulkanConstantUpload(VkPhysicalDevice physical,VkDevice device,unsigned family) {
 using namespace bo1::vulkan;
 struct RestoreDescriptorOptions {
  bool lazy=LazyDescriptorWrites(),contents=DescriptorContentReuse();
  ~RestoreDescriptorOptions(){SetLazyDescriptorWrites(lazy);SetDescriptorContentReuse(contents);}
 } restoreOptions;
 try {
  SetDescriptorContentReuse(false); // Force contents misses and exercise the written-set map.
  const auto initialWriteStats=ReadDescriptorWriteStats();
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
  owner.program=std::make_shared<GraphicsProgram>(device);Require(owner.program->Create(program,VK_NULL_HANDLE,error),error);
  auto vertexRecord=std::make_shared<shader_assets::Record>(),pixelRecord=std::make_shared<shader_assets::Record>();
  Require(TranslateShader(vs,sizeof(vs),vertexRecord->translation)&&TranslateShader(ps,sizeof(ps),pixelRecord->translation),"upload fixture reflection failed");
  auto selection=std::make_shared<material_bindings::Selection>();selection->vertex=vertexRecord;selection->pixel=pixelRecord;
  {
   BindCache first,second;ResourceScratch* reusable{};
   {
    ResourceScratchLease outer(&first,true);reusable=outer.Get();outer.Get()->stages[0].resize(1);outer.Get()->stages[0][0].registerIndex=77;
    auto* address=outer.Get()->stages[0].data();
    {ResourceScratchLease inner(&first,true),other(&second,true);
     Require(inner.Get()!=outer.Get()&&other.Get()!=outer.Get(),"nested/independent scratch aliased");
     inner.Get()->stages[0].resize(100);other.Get()->stages[0].resize(200);
     Require(outer.Get()->stages[0].data()==address&&outer.Get()->stages[0][0].registerIndex==77,"pool growth invalidated outer lease");}
   }
   ResourceScratchLease again(&first,true);Require(again.Get()==reusable&&again.Get()->stages[0].empty()&&again.Get()->stages[0].capacity()>=1,"scratch lease lost reusable capacity");
  }
  Require(owner.program->ConstantRows(0)==1&&owner.program->ConstantRows(1)==1,"reflected c0 float banks must expose one row");
  const unsigned plainVs[]={0xfffe0300,0x0200001f,0x80000000,0x900f0000,0x0200001f,0x80000000,0xe00f0000,0x02000001,0xe00f0000,0x90e40000,0x0000ffff};
  const unsigned localPs[]={0xffff0300,0x05000051,0xa00f0000,0x3e800000,0x3f000000,0x3f400000,0x3f800000,0x02000001,0x800f0800,0xa0e40000,0x0000ffff};
  auto plain=program;plain.vertexBytecode=plainVs;plain.vertexBytes=sizeof(plainVs);plain.pixelBytecode=localPs;plain.pixelBytes=sizeof(localPs);
  GraphicsProgram noBanks(device);Require(noBanks.Create(plain,VK_NULL_HANDLE,error),error);
  Require(noBanks.ConstantRows(0)==0&&noBanks.ConstantRows(1)==0,"constant-free stages must not expose conservative 256-row default");
  std::puts("native_float_bank_reflection=PASS used_rows_vs=1 used_rows_ps=1 absent_rows_vs=0 absent_rows_ps=0");
  VkCommandPoolCreateInfo cp{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};cp.queueFamilyIndex=family;cp.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;Vk(vkCreateCommandPool(device,&cp,nullptr,&owner.commands),"constant commands");
  VkCommandBufferAllocateInfo ai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};ai.commandPool=owner.commands;ai.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;ai.commandBufferCount=1;VkCommandBuffer command{};Vk(vkAllocateCommandBuffers(device,&ai,&command),"constant command allocation");
  VkFenceCreateInfo fi{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};Vk(vkCreateFence(device,&fi,nullptr,&owner.fence),"constant fence");VkEventCreateInfo ei{VK_STRUCTURE_TYPE_EVENT_CREATE_INFO};Vk(vkCreateEvent(device,&ei,nullptr,&owner.gate),"constant gate");
  VkQueue queue{};vkGetDeviceQueue(device,family,0,&queue);image_usage::Tracker tracker;VkBuffer firstBuffer{};
  for(unsigned round=0;round<2;++round) {
   SetLazyDescriptorWrites(round!=0);
   Require(owner.storage->Reset(error),error);BindCache cache;
   if(round){Vk(vkResetCommandBuffer(command,0),"constant reset");Vk(vkResetFences(device,1,&owner.fence),"constant fence reset");Vk(vkResetEvent(device,owner.gate),"constant gate reset");}
   VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};Vk(vkBeginCommandBuffer(command,&begin),"constant begin");image_usage::RecordingScope tracking(tracker,command);
   vkCmdWaitEvents(command,1,&owner.gate,VK_PIPELINE_STAGE_HOST_BIT,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,0,nullptr,0,nullptr,0,nullptr);
   VkClearColorValue clear{{0,0,0,0}};Require(owner.pass->BeginTracked(command,clear,error),error);
   VkDescriptorBufferInfo previous[2]{};
   auto snapshot=std::make_shared<constant_state::Snapshot>();
   PreparedProgramStamp prepared;prepared.physical=physical;prepared.pass=prepared.pipelinePass=owner.pass->RenderPass();
   prepared.samples=VK_SAMPLE_COUNT_1_BIT;prepared.color=true;prepared.streamMask=1;prepared.strides[0]=16;
   prepared.vertex=vs;prepared.pixel=ps;
   cache.prepared={prepared,{},owner.program};
   auto changed=prepared;changed.bias=1;Require(!cache.prepared.Find(changed,true),"changed bias reused prepared program");
   Require(!cache.prepared.Find(prepared,false),"disabled prepared reuse returned program");
   for(unsigned i=0;i<6;++i) {
    auto retainedProgram=cache.prepared.Find(prepared,true);Require(retainedProgram==owner.program,"prepared program owner lost");
    float vertex[8]={float(round+1)/8+(i>=2?.125f:0),0,0,.5f,0,0,0,0};float pixel[8]={0,i>=1?.5f:.25f,0,.5f,0,0,0,0};
    std::memcpy(snapshot->floats[0].data(),vertex,sizeof(vertex));std::memcpy(snapshot->floats[1].data(),pixel,sizeof(pixel));++snapshot->revision;
    if(i==3)snapshot->floats[0][255][0]=123; // unused row must permit bound-state reuse
    const void* data[2]={snapshot->floats[0].data(),snapshot->floats[1].data()};const VkDeviceSize sizes[2]={16,i>=4?32u:16u};VkDescriptorBufferInfo ranges[2]{};VkDescriptorPool pool{};cache.reuseConstants=i!=5;
    ResourceScratchLease scratch(&cache,round!=0);
    std::vector<ProgramResource> localVertex,localPixel;
    auto& uploadedVertex=scratch.Get()?scratch.Get()->stages[0]:localVertex;
    auto& uploadedPixel=scratch.Get()?scratch.Get()->stages[1]:localPixel;
    material_bindings::Draw uploadDraw;uploadDraw.selection=selection;uploadDraw.constants=snapshot;
    const unsigned uploadRows[2]={unsigned(sizes[0]/16),unsigned(sizes[1]/16)};
    if(i==0) {
     auto missingPixel=std::make_shared<shader_assets::Record>(*pixelRecord);
     missingPixel->translation.bindings.push_back({ResourceKind::Texture,0,1,16,2});
     auto badSelection=std::make_shared<material_bindings::Selection>(*selection);badSelection->pixel=missingPixel;
     auto badDraw=uploadDraw;badDraw.selection=badSelection;
     std::vector<ProgramResource> untouchedV(1),untouchedP(1);untouchedV[0].registerIndex=77;untouchedP[0].registerIndex=88;
     VkDescriptorPool unusedPool{};
     Require(!material_bindings::Upload(badDraw,*owner.storage,alignment,untouchedV,untouchedP,unusedPool,error,uploadRows,&cache)
        &&untouchedV.size()==1&&untouchedP.size()==1&&untouchedV[0].registerIndex==77&&untouchedP[0].registerIndex==88,"failed public upload changed outputs");
     if(scratch.Get())Require(!material_bindings::UploadScratch(badDraw,*owner.storage,alignment,*scratch.Get(),unusedPool,error,uploadRows,&cache),"bad scratch upload succeeded");
    }
    const bool boundReuse=cache.UploadedConstantsMatch(snapshot,snapshot->revision,owner.storage.get(),data,true);
    if(i==1||i==2)Require(!boundReuse,"changed used row incorrectly reused itself");
    if(i==3) {
     Require(boundReuse,"unused row prevented bound constant reuse");ranges[0]=previous[0];ranges[1]=previous[1];
    } else {
     const bool uploaded=scratch.Get()?material_bindings::UploadScratch(uploadDraw,*owner.storage,alignment,*scratch.Get(),pool,error,uploadRows,&cache)
         :material_bindings::Upload(uploadDraw,*owner.storage,alignment,uploadedVertex,uploadedPixel,pool,error,uploadRows,&cache);
     Require(uploaded,error);Require(uploadedVertex.size()==1&&uploadedPixel.size()==1,"upload stage resources mismatch");
     ranges[0]=uploadedVertex[0].buffer;ranges[1]=uploadedPixel[0].buffer;
    }
    if(i==0){Require(cache.constantUploadMisses==2&&cache.constantUploadHits==0,"fresh frame reused stale constants");if(round)Require(ranges[0].buffer==firstBuffer,"retired page not reused");else firstBuffer=ranges[0].buffer;}
    if(i==1||i==3||i==4)Require(ranges[0].buffer==previous[0].buffer&&ranges[0].offset==previous[0].offset,"unchanged VS reallocated");
    if(i==2||i==3)Require(ranges[1].buffer==previous[1].buffer&&ranges[1].offset==previous[1].offset,"unchanged PS reallocated across pages");
    previous[0]=ranges[0];previous[1]=ranges[1];
    const unsigned rows[2]={unsigned(sizes[0]/16),unsigned(sizes[1]/16)};
    cache.RememberConstants(snapshot,snapshot->revision,owner.storage.get(),data,rows,true);
    Require(i==5?bool(cache.last.constants):!cache.last.constants,"constant snapshot fallback ownership mismatch");
    snapshot->floats[0][0].fill(-99);snapshot->floats[1][0].fill(-99);++snapshot->revision; // GPU must use uploaded bytes.
    Require(!cache.UploadedConstantsMatch(snapshot,snapshot->revision,owner.storage.get(),data,true),"in-place mutation compared against itself");
    ProgramResource v{},p{};v.kind=p.kind=ResourceKind::Constants;v.buffer=ranges[0];p.buffer=ranges[1];
    if(i!=3) {
     const auto before=ReadDescriptorWriteStats();const auto misses=cache.setMisses,hits=cache.setHits;
     Require(retainedProgram->Record(command,pool,uploadedVertex,uploadedPixel,cache,error),error);
     const auto after=ReadDescriptorWriteStats();
     Require(after.writeBuilds-before.writeBuilds==(round?cache.setMisses-misses:2),"lazy descriptor writes built on map hit");
     Require(after.lazySkips-before.lazySkips==(round?cache.setHits-hits:0),"lazy descriptor skip count mismatch");
     if(i==0) {
      Require(!retainedProgram->Record(command,pool,{v,v},{p},cache,error),"duplicate resource accepted");
      Require(!retainedProgram->Record(command,pool,{},{p},cache,error),"missing resource accepted");
      auto invalid=v;invalid.buffer.offset=0x100000000ull;
      Require(!retainedProgram->Record(command,pool,{invalid},{p},cache,error),"oversized dynamic offset accepted");
      invalid=v;invalid.buffer.buffer=VK_NULL_HANDLE;
      Require(!retainedProgram->Record(command,pool,{invalid},{p},cache,error),"null buffer accepted");
     }
    }
    // Descriptor recording consumes CPU descriptions synchronously.
    for(auto& resource:uploadedVertex)resource={};for(auto& resource:uploadedPixel)resource={};
    uploadedVertex.clear();uploadedPixel.clear();
    const VkViewport viewport{float(i),1,1,-1,0,1};const VkRect2D scissor{{int(i),0},{1,1}};vkCmdSetViewport(command,0,1,&viewport);vkCmdSetScissor(command,0,1,&scissor);
    const VkBuffer vb=owner.vertices->Handle();const VkDeviceSize offset=0;vkCmdBindVertexBuffers(command,0,1,&vb,&offset);vkCmdDraw(command,3,1,0,0);
   }
   Require(cache.constantUploadHits==3&&cache.constantUploadMisses==7,"constant reuse accounting mismatch");
   Require(owner.storage->Pages()==3&&owner.storage->Draws()==5,"descriptor reservation/page rollover mismatch");Require(!owner.storage->Reset(error),"reset accepted recorded readers");
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
  Require(ReadDescriptorWriteStats().lazySkips>initialWriteStats.lazySkips,"fixture missed lazy written-map hit path");
  std::puts("native_resource_scratch=PASS eager_scratch_pixels=12 nested_pool=1 capacity_reuse=1 public_failure_transaction=1 retry_after_failure=1 cpu_vectors_overwritten=1 fence_reset=1");
  std::puts("native_lazy_descriptors=PASS eager_and_lazy_pixels=12 map_hits=1 changed_offsets_buffers_ranges=1 invalid_resources_rejected=1 fence_reset=1");
  std::puts("native_constant_upload=PASS pixels=12 stages=2 independent_changes=1 same_bytes=1 range_change=1 disabled=1 page_rollover=1 fresh_cache_after_fence=1");
  std::puts("native_constant_snapshot=PASS weak_identity=1 changed_revision=1 unused_row_bound_draw=1 overwritten_cpu=1 fence_readback=1");
  std::puts("native_prepared_program=PASS retained_owner=1 changed_bias_rejected=1 disabled_rejected=1 changed_constants_pixels=12");return true;
 }catch(const std::exception& e){std::fprintf(stderr,"native_constant_upload=FAIL %s\n",e.what());return false;}
}
