#include "geometry_bindings.h"
#include "material_dispatch.h"
#include "geometry_packet.h"
#include "attachment_pass.h"
#include "image_usage.h"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <stdexcept>
#include <unordered_map>
// Fixture registry only: records own real GPU buffers, but this does not test
// canonical registration/upload code. Unknown owners always fail closed.
namespace bo1::vulkan::buffer_assets {
std::unordered_map<const void*,std::shared_ptr<const Record>> fixtureRecords;
std::shared_ptr<const Record> Acquire(const void* owner){auto i=fixtureRecords.find(owner);return i==fixtureRecords.end()?nullptr:i->second;}
}
namespace {
using namespace bo1::vulkan;
void Check(bool value,const std::string& e){if(!value)throw std::runtime_error(e);}
void Vk(VkResult r,const char* e){Check(r==VK_SUCCESS,std::string(e)+": "+std::to_string(int(r)));}
struct Owner {
    VkDevice device;VkCommandPool commands{};VkFence fence{};VkEvent gate{};VkDescriptorPool descriptors{};bool pending{};
    std::shared_ptr<Texture2D> color;std::shared_ptr<GraphicsProgram> program;std::unique_ptr<AttachmentPass> pass;
    std::unique_ptr<Buffer> readback;
    std::vector<material_dispatch::Packet> packets;
    std::vector<std::shared_ptr<const geometry_bindings::Snapshot>> outerArena,innerArena;
    ~Owner(){if(pending){if(gate)vkSetEvent(device,gate);vkDeviceWaitIdle(device);}
        packets.clear();outerArena.clear();innerArena.clear();pass.reset();program.reset();
        if(descriptors)vkDestroyDescriptorPool(device,descriptors,nullptr);if(gate)vkDestroyEvent(device,gate,nullptr);
        if(fence)vkDestroyFence(device,fence,nullptr);if(commands)vkDestroyCommandPool(device,commands,nullptr);}
};
}
bool NativeVulkanGeometryRetention(VkPhysicalDevice physical,VkDevice device,unsigned family) {
    using namespace bo1::vulkan;namespace geometry=geometry_bindings;
    struct RestoreIndexOption {bool saved=geometry::IndexBindingReuseEnabled();
        ~RestoreIndexOption(){geometry::SetIndexBindingReuse(saved);buffer_assets::fixtureRecords.clear();}} restoreIndexOption;
    try {
        Owner owner{device};std::string error;TextureDescription d;d.extent={5,1,1};d.format=VK_FORMAT_R32G32B32A32_SFLOAT;
        d.usage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        owner.color=std::make_shared<Texture2D>(physical,device);Check(owner.color->Create(d,error),error);
        owner.pass=std::make_unique<AttachmentPass>(device);Check(owner.pass->Create(owner.color,nullptr,false,error),error);
        owner.readback=std::make_unique<Buffer>(physical,device);Check(owner.readback->Create(80,VK_BUFFER_USAGE_TRANSFER_DST_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,error),error);
        float vertices[6][4]={{-1,-1,0,1},{-1,3,0,1},{3,-1,0,1},{9,-1,0,1},{9,3,0,1},{13,-1,0,1}};
        float auxiliary[6][5]{};unsigned short indices[6]={0,1,2,0,1,2};
        auto aux=std::make_shared<Buffer>(physical,device);
        Check(aux->Create(sizeof(auxiliary),VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,error)&&aux->Write(0,auxiliary,sizeof(auxiliary),error),error);
        auto vb=std::make_shared<Buffer>(physical,device),ib=std::make_shared<Buffer>(physical,device);
        Check(vb->Create(sizeof(vertices),VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,error)
            &&ib->Create(sizeof(indices),VK_BUFFER_USAGE_INDEX_BUFFER_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,error),error);
        Check(vb->Write(0,vertices,sizeof(vertices),error)&&ib->Write(0,indices,sizeof(indices),error),error);
        auto vertex=std::make_shared<buffer_assets::Record>(),index=std::make_shared<buffer_assets::Record>();
        vertex->kind=buffer_assets::Kind::Vertex;vertex->payloadBytes=sizeof(vertices);vertex->gpu=vb;
        auto auxiliaryRecord=std::make_shared<buffer_assets::Record>();auxiliaryRecord->kind=buffer_assets::Kind::Vertex;auxiliaryRecord->payloadBytes=sizeof(auxiliary);auxiliaryRecord->gpu=aux;
        index->kind=buffer_assets::Kind::Index16;index->payloadBytes=sizeof(indices);index->gpu=ib;
        std::shared_ptr<const geometry::Snapshot> snapshot,fresh;
        {
            unsigned char stateId{},cloneId{},vertexId{},indexId{},auxId{};
            auto* state=reinterpret_cast<const GfxCmdBufPrimState*>(&stateId);
            auto* clone=reinterpret_cast<const GfxCmdBufPrimState*>(&cloneId);
            struct Cleanup {const GfxCmdBufPrimState *a,*b;~Cleanup(){geometry::Release(a);geometry::Release(b);buffer_assets::fixtureRecords.clear();}} cleanup{state,clone};
            buffer_assets::fixtureRecords[&auxId]=auxiliaryRecord;
            buffer_assets::fixtureRecords[&vertexId]=vertex;buffer_assets::fixtureRecords[&indexId]=index;
            geometry::SetIndexBindingReuse(false);
            Check(geometry::SetStreams(state,{{{&vertexId,0,16},{&auxId,0,20},{}}},error)&&geometry::SetIndex(state,&indexId,error),error);
            auto eager=geometry::Acquire(state);
            Check(geometry::SetIndex(state,&indexId,error)&&geometry::Acquire(state)!=eager,"disabled index reuse retained identity");
            geometry::SetIndexBindingReuse(true);snapshot=geometry::Acquire(state);
            const auto beforeIndex=geometry::ReadIndexBindingProfile();
            Check(geometry::SetIndex(state,&indexId,error)&&geometry::Acquire(state)==snapshot,"unchanged index lost identity");
            geometry::Clone(state,clone);
            Check(geometry::SetIndex(clone,&indexId,error)&&geometry::Acquire(clone)==snapshot,"clone unchanged index lost identity");
            auto replacement=std::make_shared<buffer_assets::Record>(*index);replacement->serial=index->serial+1;
            buffer_assets::fixtureRecords[&indexId]=replacement;
            Check(geometry::SetIndex(clone,&indexId,error),error);fresh=geometry::Acquire(clone);
            Check(fresh!=snapshot&&fresh->indices==replacement&&snapshot->indices==index
                &&fresh->indices->gpu==snapshot->indices->gpu,"same buffer new record reused stale snapshot");
            Check(geometry::Acquire(state)==snapshot,"clone publication mutated captured source");
            buffer_assets::fixtureRecords.erase(&indexId);
            Check(!geometry::SetIndex(clone,&indexId,error),"released owner accepted stale capture");
            Check(!geometry::SetIndex(clone,&vertexId,error),"vertex owner accepted as index");
            Check(!geometry::SetIndex(nullptr,&vertexId,error),"null state accepted");
            const auto afterIndex=geometry::ReadIndexBindingProfile();
            Check(afterIndex.attempts-beforeIndex.attempts==6&&afterIndex.hits-beforeIndex.hits==2,"index no-op counters mismatch");
        }
        auto wrong=std::make_shared<geometry::Snapshot>(*snapshot);wrong->streams[0].offset=4;
        const unsigned vs[]={0xfffe0300,0x0200001f,0x80000000,0x900f0000,0x0200001f,0x80000000,0xe00f0000,0x0200001f,0x80000005,0x900f0001,0x03000002,0xe00f0000,0x90e40000,0x90e40001,0x0000ffff};
        const unsigned ps[]={0xffff0300,0x05000051,0xa00f0000,0x3e800000,0x3f000000,0x3f400000,0x3f800000,0x02000001,0x800f0800,0xa0e40000,0x0000ffff};
        VkVertexInputBindingDescription binding[2]={{0,16,VK_VERTEX_INPUT_RATE_VERTEX},{1,20,VK_VERTEX_INPUT_RATE_VERTEX}};VkVertexInputAttributeDescription attribute[2]={{0,0,VK_FORMAT_R32G32B32A32_SFLOAT,0},{1,1,VK_FORMAT_R32G32B32A32_SFLOAT,0}};
        ProgramDescription description;description.vertexBytecode=vs;description.vertexBytes=sizeof(vs);description.pixelBytecode=ps;description.pixelBytes=sizeof(ps);
        description.renderPass=owner.pass->RenderPass();description.vertexBindings=binding;description.vertexBindingCount=2;description.vertexAttributes=attribute;description.vertexAttributeCount=2;
        owner.program=std::make_shared<GraphicsProgram>(device);Check(owner.program->Create(description,VK_NULL_HANDLE,error),error);
        VkDescriptorPoolSize poolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1};VkDescriptorPoolCreateInfo dpi{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};dpi.maxSets=2;dpi.poolSizeCount=1;dpi.pPoolSizes=&poolSize;
        Vk(vkCreateDescriptorPool(device,&dpi,nullptr,&owner.descriptors),"geometry descriptor pool");std::array<VkDescriptorSet,2> sets{};
        Check(owner.program->BindResources(owner.descriptors,{}, {},sets,error),error);
        VkCommandPoolCreateInfo ci{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};ci.queueFamilyIndex=family;Vk(vkCreateCommandPool(device,&ci,nullptr,&owner.commands),"geometry pool");
        VkCommandBufferAllocateInfo ai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};ai.commandPool=owner.commands;ai.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;ai.commandBufferCount=1;
        VkCommandBuffer command{};Vk(vkAllocateCommandBuffers(device,&ai,&command),"geometry command");
        VkEventCreateInfo ei{VK_STRUCTURE_TYPE_EVENT_CREATE_INFO};Vk(vkCreateEvent(device,&ei,nullptr,&owner.gate),"geometry gate");
        VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};Vk(vkBeginCommandBuffer(command,&begin),"geometry begin");
        vkCmdWaitEvents(command,1,&owner.gate,VK_PIPELINE_STAGE_HOST_BIT,VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,0,nullptr,0,nullptr,0,nullptr);
        image_usage::Tracker tracker;image_usage::RecordingScope tracked(tracker,command);VkClearColorValue clear{{0,0,0,0}};
        Check(owner.pass->BeginTracked(command,clear,error),error);owner.program->Bind(command,sets);
        VkViewport viewport{0,1,5,-1,0,1};vkCmdSetViewport(command,0,1,&viewport);BindCache binds;
        auto scissor=[&](int x){FlushPending(binds);VkRect2D r{{x,0},{1,1}};vkCmdSetScissor(command,0,1,&r);};
        // Identity-only command-state addresses; the geometry API never dereferences them.
        unsigned char outerIdentity{},innerIdentity{};auto* outer=reinterpret_cast<const GfxCmdBufPrimState*>(&outerIdentity);auto* inner=reinterpret_cast<const GfxCmdBufPrimState*>(&innerIdentity);
        geometry::ResetRecordingProfile();scissor(0);
        {
            geometry::RecordingScope outerScope(outer,command,owner.outerArena,snapshot,&binds);
            Check(geometry::DrawIndexed(outer,0,3,1,error),error);
            {
                geometry::RecordingScope innerScope(inner,command,owner.innerArena,snapshot,&binds);
                Check(!geometry::DrawIndexed(outer,0,3,1,error),"Nested mismatched scope accepted");
                Check(geometry::DrawIndexed(inner,0,3,1,error),error);
            }
            Check(geometry::DrawIndexed(outer,0,3,1,error),"Outer geometry scope was not restored");
        }
        Check(!geometry::DrawIndexed(outer,0,3,1,error),"Missing standalone scope accepted");
        const auto legacy=geometry::ReadRecordingProfile();Check(legacy.arenaRetentions==3&&legacy.arenaGrowths>0,"Standalone retention disappeared");
        material_dispatch::Packet packet;packet.geometry=snapshot;packet.program=owner.program;owner.packets.push_back(packet);packet={};
        scissor(1);
        Check(geometry::DrawIndexedRetained(command,fresh,owner.packets[0].geometry,&binds,0,3,1,error),error);
        Check(geometry::DrawIndexedRetained(command,fresh,owner.packets[0].geometry,&binds,3,3,1,error,true),error);
        Check(binds.mergedRanges==1&&binds.pending.count==6,"Retained contiguous draw did not merge");scissor(2);
        Check(geometry::DrawIndexedRetained(command,fresh,owner.packets[0].geometry,&binds,0,3,1,error),error);
        const auto before=geometry::ReadRecordingProfile();const auto pendingCount=binds.pending.count;const auto commands=binds.indexedCommands;
        Check(!geometry::DrawIndexedRetained(command,fresh,owner.packets[0].geometry,&binds,6,3,1,error),"Invalid index range accepted");
        Check(!geometry::DrawIndexedRetained(command,fresh,owner.packets[0].geometry,&binds,0,7,1,error),"Invalid vertex range accepted");
        Check(!geometry::DrawIndexedRetained(command,fresh,wrong,&binds,0,3,1,error),"Mismatched frame owner accepted");
        Check(!geometry::DrawIndexedRetained(command,fresh,{},&binds,0,3,1,error),"Absent frame owner accepted");
        Check(binds.pending.count==pendingCount&&binds.indexedCommands==commands
            &&geometry::ReadRecordingProfile().retainedDraws==before.retainedDraws,"Rejected geometry changed command/counter state");scissor(3);
        Check(geometry::DrawIndexedRetained(command,fresh,owner.packets[0].geometry,&binds,0,3,1,error),error);FlushPending(binds);
        const auto profile=geometry::ReadRecordingProfile();Check(profile.retainedDraws==4&&profile.arenaRetentions==legacy.arenaRetentions&&profile.arenaGrowths==legacy.arenaGrowths,"Retained draws grew standalone ownership arena");
        // The pending draw points at an off-screen triangle. Reuse must flush
        // it before rebinding both streams to the visible triangle, without
        // merging the adjacent index ranges across different offsets.
        auto shifted=std::make_shared<geometry::Snapshot>(*snapshot);
        shifted->streams[0].offset=48;shifted->streams[1].offset=60;
        Check(geometry_packet::OffsetOnly(*shifted,*snapshot),"Offset-only two-stream draw rejected");
        auto changedGpu=std::make_shared<geometry::Snapshot>(*shifted);
        auto alternate=std::make_shared<buffer_assets::Record>(*vertex);alternate->gpu=aux;
        changedGpu->streams[0].buffer=alternate;
        Check(!geometry_packet::OffsetOnly(*changedGpu,*snapshot),"Different GPU backing accepted");
        changedGpu->streams[0]=shifted->streams[0];changedGpu->streams[1].stride=16;
        Check(!geometry_packet::OffsetOnly(*changedGpu,*snapshot),"Different secondary stride accepted");
        changedGpu->streams[1]={};Check(!geometry_packet::OffsetOnly(*changedGpu,*snapshot),"Different stream mask accepted");
        changedGpu.reset();alternate.reset();
        scissor(4);
        Check(geometry_packet::Record(command,shifted,owner.packets,binds,0,3,1,error),error);
        const auto issued=binds.indexedCommands,merged=binds.mergedRanges;
        Check(geometry_packet::Record(command,snapshot,owner.packets,binds,3,3,1,error),error);
        Check(binds.indexedCommands==issued+1&&binds.mergedRanges==merged
            &&binds.vertexOffsets[0]==0&&binds.vertexOffsets[1]==0,"Offset change did not flush/rebind both streams");
        const auto beforeInvalid=binds.indexedCommands;
        Check(!geometry_packet::Record(command,shifted,owner.packets,binds,0,4,1,error),"Offset packet invalid range accepted");
        Check(binds.indexedCommands==beforeInvalid,"Invalid packet emitted commands");
        FlushPending(binds);
        std::weak_ptr<const geometry::Snapshot> weakShifted=shifted;
        shifted.reset();
        Check(owner.pass->EndTracked(command,error),error);
        VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};b.image=owner.color->Handle();b.srcQueueFamilyIndex=b.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
        b.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};b.oldLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;b.newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        b.srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;b.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
        vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&b);
        VkBufferImageCopy copy{};copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};copy.imageExtent={5,1,1};vkCmdCopyImageToBuffer(command,owner.color->Handle(),VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,owner.readback->Handle(),1,&copy);
        VkMemoryBarrier host{VK_STRUCTURE_TYPE_MEMORY_BARRIER};host.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;host.dstAccessMask=VK_ACCESS_HOST_READ_BIT;
        vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&host,0,nullptr,0,nullptr);Check(tracked.Validate(error),error);
        Vk(vkEndCommandBuffer(command),"geometry end");VkFenceCreateInfo fi{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};Vk(vkCreateFence(device,&fi,nullptr,&owner.fence),"geometry fence");
        std::weak_ptr<Buffer> weakVertex=vb,weakIndex=ib,weakAux=aux;std::weak_ptr<const geometry::Snapshot> weak=snapshot;
        VkQueue queue{};vkGetDeviceQueue(device,family,0,&queue);VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&command;
        Vk(vkQueueSubmit(queue,1,&submit,owner.fence),"geometry submit");owner.pending=true;tracked.Commit();
        std::memset(vertices,0,sizeof(vertices));std::memset(indices,0xff,sizeof(indices));
        vb.reset();ib.reset();aux.reset();auxiliaryRecord.reset();vertex.reset();index.reset();snapshot.reset();fresh.reset();wrong.reset();geometry::Release(outer);geometry::Release(inner);
        Check(vkGetFenceStatus(device,owner.fence)==VK_NOT_READY&&!weak.expired()&&!weakVertex.expired()&&!weakIndex.expired()&&!weakAux.expired()&&!weakShifted.expired(),"Pending geometry lost fence-retained owners");
        // Standalone arena retention and Packet retention independently own the
        // same backing; dropping the former must not invalidate native packets.
        owner.outerArena.clear();owner.innerArena.clear();Check(!weakVertex.expired(),"Packet did not retain GPU vertex buffer");
        Vk(vkSetEvent(device,owner.gate),"geometry release gate");Vk(vkWaitForFences(device,1,&owner.fence,VK_TRUE,10000000000ull),"geometry completed fence");owner.pending=false;
        float pixels[20]{};Check(owner.readback->Read(0,pixels,sizeof(pixels),error),error);const float expected[4]={0.25f,0.5f,0.75f,1};
        for(unsigned i=0;i<20;++i)Check(std::isfinite(pixels[i])&&std::fabs(pixels[i]-expected[i%4])<0.00001f,"Geometry retention GPU pixel mismatch");
        owner.packets.clear();Check(weak.expired()&&weakVertex.expired()&&weakIndex.expired()&&weakAux.expired()&&weakShifted.expired(),"Fence-retired geometry owners leaked");
        std::puts("native_geometry_packet=PASS pixels=5 multistream=2 offsets=2 pending_flush=1 changed_gpu_stride_mask_rejected=1 invalid_range=1 fence_retention=1");
        std::puts("native_index_binding_reuse=PASS fixture_registry=1 real_gpu_records=1 repeated_identity=1 clone=1 same_buffer_new_record=1 released_wrongkind_null_rejected=1 captured_immutable=1 fence_readback=1");
        std::printf("native_geometry_retention=PASS pixels=5 legacy_retentions=3 retained_draws=6 retained_arena_growths=0 invalid_ranges=2 mismatched_owners=2 nested_scopes=1 pending_fence=1 source_release=1\n");return true;
    }catch(const std::exception& e){std::fprintf(stderr,"native_geometry_retention=FAIL %s\n",e.what());return false;}
}
