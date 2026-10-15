#include "shader_translate.h"
#include "graphics_program.h"
#include "pipeline_key.h"
#include "resources.h"
#include "attachment_pass.h"
#include "image_usage.h"
#include "texture_bindings.h"
#include <cstdio>
#include <cmath>
#include <memory>
#include <stdexcept>

namespace {
using namespace bo1::vulkan;
void Check(bool ok,const std::string& message){if(!ok)throw std::runtime_error(message);}
void Vk(VkResult result,const char* call){if(result!=VK_SUCCESS)throw std::runtime_error(std::string(call)+": "+std::to_string(int(result)));}
struct Owners {
    VkDevice device;VkCommandPool commands{};VkFence fence{};VkDescriptorPool descriptors{};bool submitted{};
    std::shared_ptr<Texture2D> depth,color;
    std::unique_ptr<Buffer> vertices,readback;
    std::unique_ptr<texture_bindings::Sampler> compare,raw,linear;
    std::unique_ptr<AttachmentPass> depthPass,colorPass;
    std::vector<std::unique_ptr<GraphicsProgram>> programs;
    ~Owners(){
        if(submitted)vkDeviceWaitIdle(device);
        programs.clear();depthPass.reset();colorPass.reset();
        if(descriptors)vkDestroyDescriptorPool(device,descriptors,nullptr);
        if(fence)vkDestroyFence(device,fence,nullptr);
        if(commands)vkDestroyCommandPool(device,commands,nullptr);
    }
};
const unsigned vertexCode[]={0xfffe0300,
    0x0200001f,0x80000000,0x900f0000,0x0200001f,0x80000005,0x900f0001,
    0x0200001f,0x80000000,0xe00f0000,0x0200001f,0x80000005,0xe00f0001,
    0x02000001,0xe00f0000,0x90e40000,0x02000001,0xe00f0001,0x90e40001,0x0000ffff};
const unsigned depthPixel[]={0xffff0300,0x05000051,0xa00f0000,0,0,0,0,
    0x02000001,0x800f0800,0xa0e40000,0x0000ffff};
struct Vertex{float position[4],coordinate[4];};
void Fill(Vertex* v,float depth,float reference,float w){
    const float xy[3][2]={{-1,-1},{-1,3},{3,-1}},uv[3][2]={{0,1},{0,-1},{2,1}};
    for(unsigned i=0;i<3;++i){
        v[i]={{xy[i][0],xy[i][1],depth,1},{uv[i][0]*w,uv[i][1]*w,reference,w}};
    }
}
}

bool NativeVulkanDepthCompareDraw(VkPhysicalDevice physical,VkDevice device,unsigned family,const char* directory){
    using namespace bo1::vulkan;
    try {
        Owners owner{device};std::string error;
        VkFormat format=VK_FORMAT_UNDEFINED;VkFormatProperties features{};
        for(auto candidate:{VK_FORMAT_D24_UNORM_S8_UINT,VK_FORMAT_D32_SFLOAT}){
            vkGetPhysicalDeviceFormatProperties(physical,candidate,&features);
            const auto needed=VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
            if((features.optimalTilingFeatures&needed)==needed){format=candidate;break;}
        }
        Check(format!=VK_FORMAT_UNDEFINED,"No actual sampled depth attachment format");
        std::printf("depth_compare_device_format=%u linear_filter=%u\n",unsigned(format),!!(features.optimalTilingFeatures&VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT));
        TextureDescription description;description.extent={4,4,1};description.format=format;description.linearSampling=false;
        description.aspects=VK_IMAGE_ASPECT_DEPTH_BIT|(format==VK_FORMAT_D24_UNORM_S8_UINT?VK_IMAGE_ASPECT_STENCIL_BIT:0);
        description.usage=VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT;
        owner.depth=std::make_shared<Texture2D>(physical,device);Check(owner.depth->Create(description,error),error);
        description.format=VK_FORMAT_R32G32B32A32_SFLOAT;description.aspects=VK_IMAGE_ASPECT_COLOR_BIT;
        description.usage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        owner.color=std::make_shared<Texture2D>(physical,device);Check(owner.color->Create(description,error),error);
        owner.depthPass=std::make_unique<AttachmentPass>(device);Check(owner.depthPass->Create(nullptr,owner.depth,true,error),error);
        owner.colorPass=std::make_unique<AttachmentPass>(device);Check(owner.colorPass->Create(owner.color,nullptr,false,error),error);
        constexpr unsigned cases=9;constexpr VkDeviceSize verticesBytes=sizeof(Vertex)*3;
        owner.vertices=std::make_unique<Buffer>(physical,device);
        Check(owner.vertices->Create(verticesBytes*(cases+2),VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,error),error);
        owner.readback=std::make_unique<Buffer>(physical,device);
        Check(owner.readback->Create(cases*4*4*4*sizeof(float),VK_BUFFER_USAGE_TRANSFER_DST_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,error),error);
        owner.compare=std::make_unique<texture_bindings::Sampler>(device);owner.raw=std::make_unique<texture_bindings::Sampler>(device);
        const unsigned decoded=(3u<<20)|(3u<<22)|(3u<<24)|(1u<<8)|(1u<<12);
        Check(owner.compare->Create(decoded,1,0,error,true)&&owner.compare->Comparison(),error);
        Check(owner.raw->Create(decoded,1,0,error)&&!owner.raw->Comparison(),error);
        Check((features.optimalTilingFeatures&VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)!=0,"Linear depth comparison unavailable on test device");
        owner.linear=std::make_unique<texture_bindings::Sampler>(device);
        Check(owner.linear->Create((3u<<20)|(3u<<22)|(3u<<24)|(2u<<8)|(2u<<12),1,0,error,true),error);
        const VkDescriptorPoolSize sizes[]={{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,cases*2},{VK_DESCRIPTOR_TYPE_SAMPLER,cases*2}};
        VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};pool.maxSets=2*(cases+1);pool.poolSizeCount=2;pool.pPoolSizes=sizes;
        Vk(vkCreateDescriptorPool(device,&pool,nullptr,&owner.descriptors),"descriptor pool");
        VkCommandPoolCreateInfo commands{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};commands.queueFamilyIndex=family;
        Vk(vkCreateCommandPool(device,&commands,nullptr,&owner.commands),"command pool");
        VkCommandBufferAllocateInfo allocate{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};allocate.commandPool=owner.commands;allocate.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;allocate.commandBufferCount=1;
        VkCommandBuffer command{};Vk(vkAllocateCommandBuffers(device,&allocate,&command),"command allocation");
        VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};begin.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        Vk(vkBeginCommandBuffer(command,&begin),"command begin");
        image_usage::Tracker tracker;image_usage::RecordingScope tracking(tracker,command);
        const VkVertexInputBindingDescription binding{0,sizeof(Vertex),VK_VERTEX_INPUT_RATE_VERTEX};
        const VkVertexInputAttributeDescription attributes[]={{0,0,VK_FORMAT_R32G32B32A32_SFLOAT,0},{1,0,VK_FORMAT_R32G32B32A32_SFLOAT,16}};
        ProgramDescription program;program.vertexBytecode=vertexCode;program.vertexBytes=sizeof(vertexCode);
        program.pixelBytecode=depthPixel;program.pixelBytes=sizeof(depthPixel);program.renderPass=owner.depthPass->RenderPass();
        program.vertexBindings=&binding;program.vertexBindingCount=1;program.vertexAttributes=attributes;program.vertexAttributeCount=2;
        program.colorAttachment=false;program.depthTest=true;program.depthWrite=true;
        auto make=[&](const ProgramDescription& d){auto p=std::make_unique<GraphicsProgram>(device);Check(p->Create(d,VK_NULL_HANDLE,error),error);owner.programs.push_back(std::move(p));return owner.programs.back().get();};
        auto depthProgram=make(program);std::array<VkDescriptorSet,2> sets{};
        Check(depthProgram->BindResources(owner.descriptors,{}, {},sets,error),error);
        const VkClearColorValue clear{{0,0,0,0}};Check(owner.depthPass->BeginTracked(command,clear,error),error);
        depthProgram->Bind(command,sets);const VkBuffer vb=owner.vertices->Handle();Vertex vertices[3];
        for(unsigned side=0;side<2;++side){
            Fill(vertices,side?0.75f:0.25f,0,1);const VkDeviceSize offset=side*verticesBytes;
            Check(owner.vertices->Write(offset,vertices,sizeof(vertices),error),error);
            VkViewport viewport{float(side*2),4,2,-4,0,1};VkRect2D scissor{{int(side*2),0},{2,4}};
            vkCmdSetViewport(command,0,1,&viewport);vkCmdSetScissor(command,0,1,&scissor);
            vkCmdBindVertexBuffers(command,0,1,&vb,&offset);vkCmdDraw(command,3,1,0,0);
        }
        Check(owner.depthPass->EndTracked(command,error),error);VkImageLayout depthLayout{};
        Check(image_usage::Sample(owner.depth,depthLayout,error),error);
        for(unsigned test=0;test<cases;++test){
            // s3 deliberately tests a nonzero comparison-mask/register bit.
            std::vector<unsigned> pixel={0xffff0300,0x0200001f,0x80000005,0x900f0000,
                0x0200001f,0x90000000,0xa00f0803,0x03000042,0x800f0000,0x90e40000,0xa0e40803,
                0x02000001,0x800f0800,0x80000000,0x0000ffff};
            if(test==4) {
                pixel.insert(pixel.begin()+7,{0x0200001f,0x90000000,0xa00f0800});
                pixel.insert(pixel.begin()+14,{0x03000042,0x800f0001,0x90e40000,0xa0e40800,
                    0x03000005,0x800f0000,0x80000000,0x80000001});
            }
            if(test==0) {
                ShaderTranslation rejected;
                Check(!TranslateShader(vertexCode,sizeof(vertexCode),rejected,nullptr,nullptr,8)&&rejected.spirv.empty(),"Vertex comparison mask accepted");
                Check(!TranslateShader(pixel.data(),pixel.size()*4,rejected,nullptr,nullptr,0x10000)&&rejected.spirv.empty(),"Out-of-range comparison mask accepted");
                auto cube=pixel;cube[5]=0x98000000;
                Check(!TranslateShader(cube.data(),cube.size()*4,rejected,nullptr,nullptr,8)&&rejected.spirv.empty(),"Cube comparison silently accepted");
                std::puts("native_depth_negative_masks=PASS stage=vertex slots=range cube=explicit_rejection");
            }
            if(test==2)pixel[7]|=0x10000; // texldp: projected XY and reference Z.
            if(test==6)pixel[7]|=0x20000; // texldb: bias comes from W, reference from Z.
            if(test==7)pixel[7]=0x0300005f; // texldl: explicit LOD comes from W.
            if(test==8) { // texldd: retain both supplied XY gradients.
                pixel[7]=0x0500005d;
                pixel.insert(pixel.begin()+11,{0x90e40000,0x90e40000});
            }
            const bool comparison=test!=3;
            program.renderPass=owner.colorPass->RenderPass();program.colorAttachment=true;program.depthTest=program.depthWrite=false;
            program.pixelBytecode=pixel.data();program.pixelBytes=pixel.size()*4;program.pixelDepthSamplerMask=comparison?8:0;
            PipelineKey compared,ordinary;Check(PipelineKey::Create(program,compared,error),error);
            auto other=program;other.pixelDepthSamplerMask=comparison?0:8;Check(PipelineKey::Create(other,ordinary,error)&&compared!=ordinary,"Depth mask missing from cache identity");
            ShaderTranslation translated;Check(TranslateShader(pixel.data(),pixel.size()*4,translated,nullptr,nullptr,program.pixelDepthSamplerMask),translated.diagnostics);
            if(directory){auto path=std::string(directory)+"/depth-case"+std::to_string(test)+".spv";auto f=std::fopen(path.c_str(),"wb");Check(f!=nullptr,"SPIR-V dump open");const bool written=std::fwrite(translated.spirv.data(),4,translated.spirv.size(),f)==translated.spirv.size();const bool closed=std::fclose(f)==0;Check(written&&closed,"SPIR-V dump write");}
            auto draw=make(program);
            ProgramResource texture{};texture.kind=ResourceKind::Texture;texture.registerIndex=3;texture.image={VK_NULL_HANDLE,owner.depth->View(),depthLayout};
            ProgramResource sampler{};sampler.kind=ResourceKind::Sampler;sampler.registerIndex=3;sampler.image={test==5?owner.linear->Handle():comparison?owner.compare->Handle():owner.raw->Handle(),VK_NULL_HANDLE,VK_IMAGE_LAYOUT_UNDEFINED};
            std::vector<ProgramResource> resources{texture,sampler};
            if(test==4){auto rawTexture=texture;rawTexture.registerIndex=0;auto rawSampler=sampler;rawSampler.registerIndex=0;rawSampler.image.sampler=owner.raw->Handle();resources.push_back(rawTexture);resources.push_back(rawSampler);}
            Check(draw->BindResources(owner.descriptors,{},resources,sets,error),error);
            Fill(vertices,0,test==1?0.875f:test==2?1.0f:0.5f,test==2?2.0f:1.0f);
            if(test==5)for(auto& v:vertices){v.coordinate[0]=0.5f;v.coordinate[1]=0.5f;}
            const VkDeviceSize offset=(test+2)*verticesBytes;Check(owner.vertices->Write(offset,vertices,sizeof(vertices),error),error);
            Check(owner.colorPass->BeginTracked(command,clear,error),error);draw->Bind(command,sets);
            VkViewport viewport{0,4,4,-4,0,1};VkRect2D scissor{{0,0},{4,4}};
            vkCmdSetViewport(command,0,1,&viewport);vkCmdSetScissor(command,0,1,&scissor);vkCmdBindVertexBuffers(command,0,1,&vb,&offset);vkCmdDraw(command,3,1,0,0);
            Check(owner.colorPass->EndTracked(command,error),error);
            VkImageMemoryBarrier transfer{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};transfer.image=owner.color->Handle();transfer.srcQueueFamilyIndex=transfer.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
            transfer.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};transfer.oldLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;transfer.newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            transfer.srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;transfer.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
            vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&transfer);
            VkBufferImageCopy copy{};copy.bufferOffset=test*256;copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};copy.imageExtent={4,4,1};
            vkCmdCopyImageToBuffer(command,owner.color->Handle(),VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,owner.readback->Handle(),1,&copy);
            transfer.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;transfer.newLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;transfer.srcAccessMask=VK_ACCESS_TRANSFER_READ_BIT;transfer.dstAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,0,0,nullptr,0,nullptr,1,&transfer);
        }
        VkMemoryBarrier host{VK_STRUCTURE_TYPE_MEMORY_BARRIER};host.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;host.dstAccessMask=VK_ACCESS_HOST_READ_BIT;
        vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&host,0,nullptr,0,nullptr);
        Check(tracking.Validate(error),error);Vk(vkEndCommandBuffer(command),"command end");
        VkFenceCreateInfo fence{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};Vk(vkCreateFence(device,&fence,nullptr,&owner.fence),"fence");
        VkQueue queue{};vkGetDeviceQueue(device,family,0,&queue);VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&command;
        Vk(vkQueueSubmit(queue,1,&submit,owner.fence),"submit");owner.submitted=true;tracking.Commit();
        Vk(vkWaitForFences(device,1,&owner.fence,VK_TRUE,10000000000ull),"actual GPU fence");owner.submitted=false;
        float pixels[cases*64]{};Check(owner.readback->Read(0,pixels,sizeof(pixels),error),error);bool all=true;
        for(unsigned test=0;test<cases;++test){bool match=true;for(unsigned i=0;i<64;++i){const unsigned x=(i/4)%4;const float expected=test==5?0.5f:test==4?(x<2?0.0f:0.75f):test==3?(x<2?0.25f:0.75f):test==1?0.0f:x<2?0.0f:1.0f;match=match&&std::isfinite(pixels[test*64+i])&&std::fabs(pixels[test*64+i]-expected)<0.00001f;}
            std::printf("native_depth_compare=%s case=%u left=%.7g right=%.7g real_depth_draw=1 fence_completed=1\n",match?"PASS":"FAIL",test,pixels[test*64],pixels[test*64+8]);all=all&&match;}
        return all;
    }catch(const std::exception& e){std::fprintf(stderr,"native_depth_compare=FAIL %s\n",e.what());return false;}
}

// Focused production pipeline bias check; no historical shader cases are run.
bool NativeVulkanDepthBiasDraw(VkPhysicalDevice physical,VkDevice device,unsigned family) {
    using namespace bo1::vulkan;
    try {
        for(const auto format:{VK_FORMAT_D16_UNORM,VK_FORMAT_D24_UNORM_S8_UINT}) {
            Owners owner{device};std::string error;
            TextureDescription d;d.extent={4,4,1};d.format=format;d.linearSampling=false;
            d.aspects=VK_IMAGE_ASPECT_DEPTH_BIT|(format==VK_FORMAT_D24_UNORM_S8_UINT?VK_IMAGE_ASPECT_STENCIL_BIT:0);
            d.usage=VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT;
            owner.depth=std::make_shared<Texture2D>(physical,device);Check(owner.depth->Create(d,error),error);
            d.format=VK_FORMAT_R32G32B32A32_SFLOAT;d.aspects=VK_IMAGE_ASPECT_COLOR_BIT;
            d.usage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
            owner.color=std::make_shared<Texture2D>(physical,device);Check(owner.color->Create(d,error),error);
            owner.depthPass=std::make_unique<AttachmentPass>(device);Check(owner.depthPass->Create(nullptr,owner.depth,true,error),error);
            owner.colorPass=std::make_unique<AttachmentPass>(device);Check(owner.colorPass->Create(owner.color,nullptr,false,error),error);
            constexpr unsigned cases=5;constexpr VkDeviceSize vertexBytes=sizeof(Vertex)*3;
            owner.vertices=std::make_unique<Buffer>(physical,device);
            Check(owner.vertices->Create(vertexBytes*cases*2,VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,error),error);
            owner.readback=std::make_unique<Buffer>(physical,device);
            Check(owner.readback->Create(cases*256,VK_BUFFER_USAGE_TRANSFER_DST_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,error),error);
            owner.raw=std::make_unique<texture_bindings::Sampler>(device);
            Check(owner.raw->Create((3u<<20)|(3u<<22)|(3u<<24)|(1u<<8)|(1u<<12),1,0,error),error);
            const VkDescriptorPoolSize sizes[]={{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,cases},{VK_DESCRIPTOR_TYPE_SAMPLER,cases}};
            VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};pool.maxSets=cases*4;pool.poolSizeCount=2;pool.pPoolSizes=sizes;
            Vk(vkCreateDescriptorPool(device,&pool,nullptr,&owner.descriptors),"bias descriptors");
            VkCommandPoolCreateInfo ci{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};ci.queueFamilyIndex=family;
            Vk(vkCreateCommandPool(device,&ci,nullptr,&owner.commands),"bias command pool");
            VkCommandBufferAllocateInfo ai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};ai.commandPool=owner.commands;ai.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;ai.commandBufferCount=1;
            VkCommandBuffer command{};Vk(vkAllocateCommandBuffers(device,&ai,&command),"bias command allocation");
            VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};begin.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;Vk(vkBeginCommandBuffer(command,&begin),"bias begin");
            image_usage::Tracker tracker;image_usage::RecordingScope tracking(tracker,command);
            const VkVertexInputBindingDescription binding{0,sizeof(Vertex),VK_VERTEX_INPUT_RATE_VERTEX};
            const VkVertexInputAttributeDescription attributes[]={{0,0,VK_FORMAT_R32G32B32A32_SFLOAT,0},{1,0,VK_FORMAT_R32G32B32A32_SFLOAT,16}};
            const unsigned pixel[]={0xffff0300,0x0200001f,0x80000005,0x900f0000,0x0200001f,0x90000000,0xa00f0803,
                0x03000042,0x800f0000,0x90e40000,0xa0e40803,0x02000001,0x800f0800,0x80000000,0x0000ffff};
            auto make=[&](const ProgramDescription& description){auto p=std::make_unique<GraphicsProgram>(device);Check(p->Create(description,VK_NULL_HANDLE,error),error);auto result=p.get();owner.programs.push_back(std::move(p));return result;};
            ProgramDescription description;description.vertexBytecode=vertexCode;description.vertexBytes=sizeof(vertexCode);description.vertexBindings=&binding;description.vertexBindingCount=1;description.vertexAttributes=attributes;description.vertexAttributeCount=2;
            auto unresolved=description;unresolved.depthBias={true,false,0,0};PipelineKey bad;
            Check(!PipelineKey::Create(unresolved,bad,error),"Unresolved bias must not enter cache");
            Check(!ResolveDepthBias(unresolved,VK_FORMAT_UNDEFINED,1,0,error),"Absent depth format must reject bias");
            Check(!ResolveDepthBias(unresolved,VK_FORMAT_D32_SFLOAT,1,0,error),"Floating constant bias needs explicit policy");
            const VkBuffer vb=owner.vertices->Handle();VkClearColorValue clear{{0,0,0,0}};
            for(unsigned test=0;test<cases;++test) {
                description.renderPass=owner.depthPass->RenderPass();description.colorAttachment=false;description.depthTest=description.depthWrite=true;
                description.pixelBytecode=depthPixel;description.pixelBytes=sizeof(depthPixel);description.depthCompare=VK_COMPARE_OP_ALWAYS;
                description.depthBias={test!=0,test==0,0,0};
                const float normalized=test==1?1.0f/1024:test==2?-1.0f/1024:0;
                const float slope=test==3?1.0f:test==4?-1.0f:0;
                Check(ResolveDepthBias(description,format,normalized,slope,error),error);
                PipelineKey key,changed;Check(PipelineKey::Create(description,key,error),error);
                auto other=description;other.depthBias.enabled=true;other.depthBias.constantFactor+=1;
                Check(PipelineKey::Create(other,changed,error)&&key!=changed,"Bias factors missing from pipeline identity");
                other=description;other.depthBias.enabled=true;other.depthBias.slopeFactor+=1;
                Check(PipelineKey::Create(other,changed,error)&&key!=changed,"Slope missing from pipeline identity");
                auto depthProgram=make(description);std::array<VkDescriptorSet,2> sets{};Check(depthProgram->BindResources(owner.descriptors,{}, {},sets,error),error);
                Vertex vertices[3];Fill(vertices,0.5f,0,1);
                if(test>=3){vertices[0].position[2]=0.25f;vertices[1].position[2]=0.25f;vertices[2].position[2]=0.75f;}
                VkDeviceSize offset=test*2*vertexBytes;Check(owner.vertices->Write(offset,vertices,sizeof(vertices),error),error);
                Check(owner.depthPass->BeginTracked(command,clear,error),error);depthProgram->Bind(command,sets);
                const VkViewport viewport{0,4,4,-4,0,1};const VkRect2D scissor{{0,0},{4,4}};
                vkCmdSetViewport(command,0,1,&viewport);vkCmdSetScissor(command,0,1,&scissor);vkCmdBindVertexBuffers(command,0,1,&vb,&offset);vkCmdDraw(command,3,1,0,0);
                Check(owner.depthPass->EndTracked(command,error),error);VkImageLayout layout{};Check(image_usage::Sample(owner.depth,layout,error),error);
                description.renderPass=owner.colorPass->RenderPass();description.colorAttachment=true;description.depthTest=description.depthWrite=false;description.depthBias={};description.pixelBytecode=pixel;description.pixelBytes=sizeof(pixel);
                auto colorProgram=make(description);ProgramResource texture{};texture.kind=ResourceKind::Texture;texture.registerIndex=3;texture.image={VK_NULL_HANDLE,owner.depth->View(),layout};
                ProgramResource sampler{};sampler.kind=ResourceKind::Sampler;sampler.registerIndex=3;sampler.image={owner.raw->Handle(),VK_NULL_HANDLE,VK_IMAGE_LAYOUT_UNDEFINED};
                Check(colorProgram->BindResources(owner.descriptors,{}, {texture,sampler},sets,error),error);
                Fill(vertices,0,0,1);offset+=vertexBytes;Check(owner.vertices->Write(offset,vertices,sizeof(vertices),error),error);
                Check(owner.colorPass->BeginTracked(command,clear,error),error);colorProgram->Bind(command,sets);vkCmdSetViewport(command,0,1,&viewport);vkCmdSetScissor(command,0,1,&scissor);vkCmdBindVertexBuffers(command,0,1,&vb,&offset);vkCmdDraw(command,3,1,0,0);Check(owner.colorPass->EndTracked(command,error),error);
                VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};barrier.image=owner.color->Handle();barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;barrier.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};barrier.oldLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;barrier.newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;barrier.srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
                vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&barrier);
                VkBufferImageCopy copy{};copy.bufferOffset=test*256;copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};copy.imageExtent={4,4,1};vkCmdCopyImageToBuffer(command,owner.color->Handle(),VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,owner.readback->Handle(),1,&copy);
                barrier.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;barrier.newLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;barrier.srcAccessMask=VK_ACCESS_TRANSFER_READ_BIT;barrier.dstAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,0,0,nullptr,0,nullptr,1,&barrier);
            }
            VkMemoryBarrier host{VK_STRUCTURE_TYPE_MEMORY_BARRIER};host.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;host.dstAccessMask=VK_ACCESS_HOST_READ_BIT;
            vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&host,0,nullptr,0,nullptr);Check(tracking.Validate(error),error);Vk(vkEndCommandBuffer(command),"bias end");
            VkFenceCreateInfo fi{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};Vk(vkCreateFence(device,&fi,nullptr,&owner.fence),"bias fence");VkQueue queue{};vkGetDeviceQueue(device,family,0,&queue);VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&command;
            Vk(vkQueueSubmit(queue,1,&submit,owner.fence),"bias submit");owner.submitted=true;tracking.Commit();Vk(vkWaitForFences(device,1,&owner.fence,VK_TRUE,10000000000ull),"bias real GPU fence");owner.submitted=false;
            float pixels[cases*64]{};Check(owner.readback->Read(0,pixels,sizeof(pixels),error),error);
            for(unsigned test=0;test<cases;++test) {
                for(unsigned i=0;i<64;++i) {
                    const float value=pixels[test*64+i];const auto x=(i/4)%4;const float base=test>=3?0.25f+0.0625f*(x+0.5f):0.5f;
                    if(test==1||test==2){const float delta=(value-base)*(test==1?1:-1);Check(std::isfinite(value)&&delta>=1.0f/1024-0.00005f&&delta<=2.0f/1024+0.00005f,"Fixed depth constant bias outside Vulkan bounds");}
                    else {const float expected=base+(test==3?0.0625f:test==4?-0.0625f:0);Check(std::isfinite(value)&&std::fabs(value-expected)<0.00005f,"Actual slope/disabled bias pixel mismatch");}
                }
                std::printf("native_depth_bias=PASS format=%u case=%u first=%.9g last=%.9g fence_completed=1\n",unsigned(format),test,pixels[test*64],pixels[test*64+12]);
            }
        }
        return true;
    }catch(const std::exception& e){std::fprintf(stderr,"native_depth_bias=FAIL %s\n",e.what());return false;}
}
