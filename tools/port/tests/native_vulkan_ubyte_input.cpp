#include "graphics_program.h"
#include "pipeline_key.h"
#include "resources.h"
#include "attachment_pass.h"
#include "image_usage.h"
#include <cstdio>
#include <cmath>
#include <stdexcept>
#include <memory>
#include <cstring>
namespace {
using namespace bo1::vulkan;
void Check(bool ok,const std::string& error){if(!ok)throw std::runtime_error(error);}
void Vk(VkResult result,const char* label){Check(result==VK_SUCCESS,std::string(label)+" VkResult="+std::to_string(int(result)));}
struct Owner {
    VkDevice device;VkCommandPool pool{};VkDescriptorPool descriptors{};VkFence fence{};bool pending{};
    std::shared_ptr<Texture2D> color;std::unique_ptr<Buffer> vertices,readback;
    std::unique_ptr<AttachmentPass> pass;std::vector<std::unique_ptr<GraphicsProgram>> programs;
    ~Owner(){if(pending)vkDeviceWaitIdle(device);programs.clear();pass.reset();if(fence)vkDestroyFence(device,fence,nullptr);if(descriptors)vkDestroyDescriptorPool(device,descriptors,nullptr);if(pool)vkDestroyCommandPool(device,pool,nullptr);}
};
struct Vertex {float position[4];unsigned char raw[4],other[4];};
}
bool NativeVulkanUbyteDraw(VkPhysicalDevice physical,VkDevice device,unsigned family,const char* directory) {
    using namespace bo1::vulkan;
    try {
        Owner owner{device};std::string error;constexpr unsigned cases=6,width=256;constexpr VkDeviceSize vertexBytes=sizeof(Vertex)*3;
        VkFormatProperties props{};vkGetPhysicalDeviceFormatProperties(physical,VK_FORMAT_R8G8B8A8_UNORM,&props);Check(props.bufferFeatures&VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT,"Actual UNORM byte vertex fetch unavailable");
        TextureDescription texture;texture.extent={width,1,1};texture.format=VK_FORMAT_R32G32B32A32_SFLOAT;texture.usage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        owner.color=std::make_shared<Texture2D>(physical,device);Check(owner.color->Create(texture,error),error);
        owner.pass=std::make_unique<AttachmentPass>(device);Check(owner.pass->Create(owner.color,nullptr,false,error),error);
        owner.vertices=std::make_unique<Buffer>(physical,device);Check(owner.vertices->Create(cases*width*vertexBytes,VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,error),error);
        owner.readback=std::make_unique<Buffer>(physical,device);Check(owner.readback->Create(cases*width*16,VK_BUFFER_USAGE_TRANSFER_DST_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,error),error);
        VkDescriptorPoolCreateInfo dp{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};dp.maxSets=cases*2;Vk(vkCreateDescriptorPool(device,&dp,nullptr,&owner.descriptors),"ubyte descriptors");
        VkCommandPoolCreateInfo cp{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};cp.queueFamilyIndex=family;Vk(vkCreateCommandPool(device,&cp,nullptr,&owner.pool),"ubyte pool");
        VkCommandBufferAllocateInfo ai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};ai.commandPool=owner.pool;ai.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;ai.commandBufferCount=1;VkCommandBuffer command{};Vk(vkAllocateCommandBuffers(device,&ai,&command),"ubyte commands");
        VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};begin.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;Vk(vkBeginCommandBuffer(command,&begin),"ubyte begin");
        image_usage::Tracker tracker;image_usage::RecordingScope tracking(tracker,command);
        const unsigned pixel[]={0xffff0300,0x0200001f,0x80000005,0x900f0000,0x02000001,0x800f0800,0x90e40000,0x0000ffff};
        const VkVertexInputBindingDescription binding{0,sizeof(Vertex),VK_VERTEX_INPUT_RATE_VERTEX};
        const VkVertexInputAttributeDescription attributes[]={{0,0,VK_FORMAT_R32G32B32A32_SFLOAT,0},{5,0,VK_FORMAT_R8G8B8A8_UNORM,16},{2,0,VK_FORMAT_R8G8B8A8_UNORM,20}};
        ShaderTranslation bad;
        Check(!TranslateShader(pixel,sizeof(pixel),bad,nullptr,nullptr,0,32)&&bad.spirv.empty(),"Pixel UBYTE mask must reject/clear");
        for(unsigned test=0;test<cases;++test) {
            std::vector<unsigned> vertex={0xfffe0300,0x0200001f,0x80000000,0x900f0000,
                0x0200001f,0x80000005,test==4?0x90030005u:0x900f0005u,
                0x0200001f,0x80010005,0x900f0002,
                0x0200001f,0x80000000,0xe00f0000,0x0200001f,0x80000005,0xe00f0001,
                0x02000001,0xe00f0000,0x90e40000};
            if(test==2||test==3)vertex.insert(vertex.end(),{0x03000002,0xe00f0001,0x90e40005,0x90e40002});
            else vertex.insert(vertex.end(),{0x02000001,0xe00f0001,test==1?0x911b0005u:test==4?0x90000005u:0x90e40005u});
            vertex.push_back(0x0000ffff);const auto original=vertex;
            const unsigned mask=test==5?0:test==3?36:32;
            if(test==0) {
                Check(!TranslateShader(vertex.data(),vertex.size()*4,bad,nullptr,nullptr,0,0x10000)&&bad.spirv.empty(),"Out-of-range UBYTE mask must reject");
                Check(!TranslateShader(vertex.data(),vertex.size()*4,bad,nullptr,nullptr,0,0x8000)&&bad.spirv.empty(),"Unrouted UBYTE mask must reject");
            }
            ProgramDescription description;description.vertexBytecode=vertex.data();description.vertexBytes=vertex.size()*4;description.pixelBytecode=pixel;description.pixelBytes=sizeof(pixel);description.vertexUbyteMask=mask;
            description.renderPass=owner.pass->RenderPass();description.vertexBindings=&binding;description.vertexBindingCount=1;description.vertexAttributes=attributes;description.vertexAttributeCount=3;
            PipelineKey key,other;Check(PipelineKey::Create(description,key,error),error);auto changed=description;changed.vertexUbyteMask=mask^32;
            Check(PipelineKey::Create(changed,other,error)&&key!=other,"UBYTE specialization missing from cache identity");
            ShaderTranslation translated;Check(TranslateShader(vertex.data(),vertex.size()*4,translated,nullptr,nullptr,0,mask),translated.diagnostics);
            if(directory){const auto path=std::string(directory)+"/ubyte-case"+std::to_string(test)+".spv";auto file=std::fopen(path.c_str(),"wb");Check(file!=nullptr,"UBYTE SPIR-V dump open");const auto n=std::fwrite(translated.spirv.data(),4,translated.spirv.size(),file);const auto closed=std::fclose(file);Check(n==translated.spirv.size()&&!closed,"UBYTE SPIR-V dump write");}
            auto program=std::make_unique<GraphicsProgram>(device);Check(program->Create(description,VK_NULL_HANDLE,error),error);Check(vertex==original,"Original SM3 bytes changed");
            std::array<VkDescriptorSet,2> sets{};Check(program->BindResources(owner.descriptors,{}, {},sets,error),error);
            VkClearColorValue clear{{0,0,0,0}};Check(owner.pass->BeginTracked(command,clear,error),error);program->Bind(command,sets);
            const VkBuffer vb=owner.vertices->Handle();
            for(unsigned value=0;value<width;++value) {
                Vertex vertices[3]{};const float xy[3][2]={{-1,-1},{-1,3},{3,-1}};
                for(unsigned i=0;i<3;++i){for(unsigned c=0;c<2;++c)vertices[i].position[c]=xy[i][c];vertices[i].position[3]=1;
                    const unsigned a[4]={value,255-value,(value*73)%256,(value*19+1)%256};const unsigned b[4]={(value*7+3)%256,(value*13)%256,255-value,value};
                    for(unsigned c=0;c<4;++c){vertices[i].raw[c]=a[c];vertices[i].other[c]=b[c];}}
                const VkDeviceSize offset=(test*width+value)*vertexBytes;Check(owner.vertices->Write(offset,vertices,sizeof(vertices),error),error);
                const VkViewport viewport{float(value),1,1,-1,0,1};const VkRect2D scissor{{int(value),0},{1,1}};
                vkCmdSetViewport(command,0,1,&viewport);vkCmdSetScissor(command,0,1,&scissor);vkCmdBindVertexBuffers(command,0,1,&vb,&offset);vkCmdDraw(command,3,1,0,0);
            }
            Check(owner.pass->EndTracked(command,error),error);owner.programs.push_back(std::move(program));
            VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};barrier.image=owner.color->Handle();barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;barrier.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};barrier.oldLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;barrier.newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;barrier.srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
            vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&barrier);
            VkBufferImageCopy copy{};copy.bufferOffset=test*width*16;copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};copy.imageExtent={width,1,1};vkCmdCopyImageToBuffer(command,owner.color->Handle(),VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,owner.readback->Handle(),1,&copy);
            barrier.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;barrier.newLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;barrier.srcAccessMask=VK_ACCESS_TRANSFER_READ_BIT;barrier.dstAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,0,0,nullptr,0,nullptr,1,&barrier);
        }
        VkMemoryBarrier host{VK_STRUCTURE_TYPE_MEMORY_BARRIER};host.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;host.dstAccessMask=VK_ACCESS_HOST_READ_BIT;vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&host,0,nullptr,0,nullptr);
        Check(tracking.Validate(error),error);Vk(vkEndCommandBuffer(command),"ubyte end");VkFenceCreateInfo fi{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};Vk(vkCreateFence(device,&fi,nullptr,&owner.fence),"ubyte fence");VkQueue queue{};vkGetDeviceQueue(device,family,0,&queue);VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&command;Vk(vkQueueSubmit(queue,1,&submit,owner.fence),"ubyte submit");owner.pending=true;tracking.Commit();Vk(vkWaitForFences(device,1,&owner.fence,VK_TRUE,10000000000ull),"ubyte actual fence");owner.pending=false;
        std::vector<float> pixels(cases*width*4);Check(owner.readback->Read(0,pixels.data(),pixels.size()*4,error),error);
        for(unsigned test=0;test<cases;++test) {
            for(unsigned value=0;value<width;++value){const unsigned a[4]={value,255-value,(value*73)%256,(value*19+1)%256};const unsigned b[4]={(value*7+3)%256,(value*13)%256,255-value,value};
                for(unsigned c=0;c<4;++c){const float expected=test==1?-float(a[3-c]):test==2?float(a[c])+float(b[c])/255:test==3?float(a[c]+b[c]):test==4?float(a[0]):test==5?float(a[c])/255:float(a[c]);const float actual=pixels[(test*width+value)*4+c];
                    if(!std::isfinite(actual)||((test==0||test==1||test==3||test==4)?actual!=expected:std::fabs(actual-expected)>0.0001f)){std::fprintf(stderr,"UBYTE mismatch case=%u value=%u channel=%u actual=%.9g expected=%.9g\n",test,value,c,actual,expected);throw std::runtime_error("UBYTE GPU value mismatch");}}}
            std::printf("native_ubyte_input=PASS case=%u byte_values=256 channels=4 fence_completed=1\n",test);
        }
        return true;
    }catch(const std::exception& e){std::fprintf(stderr,"native_ubyte_input=FAIL %s\n",e.what());return false;}
}
