#include "shader_translate.h"
#include "graphics_program.h"
#include "platform/android/post_fusion_trial_bytecode.h"
#include <memory>
#include <algorithm>
#include <vulkan/vulkan.h>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <vector>
#include <stdexcept>

namespace {
void Require(VkResult status,const char* call) {
    if(status!=VK_SUCCESS) { std::fprintf(stderr,"FAIL %s result=%d\n",call,int(status));throw std::runtime_error(call); }
}
struct DrawResources {
    VkDevice d;VkImage image{};VkDeviceMemory imageMemory{},bufferMemory{};VkBuffer buffer{};
    VkImageView view{};VkRenderPass pass{};VkFramebuffer framebuffer{};VkShaderModule vs{},ps{};
    VkPipelineLayout layout{};VkPipeline pipeline{};VkCommandPool pool{};VkFence fence{};
    bool submitted{};
    std::unique_ptr<bo1::vulkan::GraphicsProgram> program;
    VkDescriptorPool descriptors{};VkImage texture{};VkDeviceMemory textureMemory{};VkImageView textureView{};VkSampler sampler{};
    ~DrawResources() {
        // Failure recovery only. Normal execution retires the single owner fence.
        if(submitted)vkDeviceWaitIdle(d);
        program.reset();
        if(descriptors)vkDestroyDescriptorPool(d,descriptors,nullptr);
        if(sampler)vkDestroySampler(d,sampler,nullptr);if(textureView)vkDestroyImageView(d,textureView,nullptr);
        if(texture)vkDestroyImage(d,texture,nullptr);if(textureMemory)vkFreeMemory(d,textureMemory,nullptr);
        if(fence)vkDestroyFence(d,fence,nullptr);if(pool)vkDestroyCommandPool(d,pool,nullptr);
        if(pipeline)vkDestroyPipeline(d,pipeline,nullptr);if(layout)vkDestroyPipelineLayout(d,layout,nullptr);
        if(vs)vkDestroyShaderModule(d,vs,nullptr);if(ps)vkDestroyShaderModule(d,ps,nullptr);
        if(framebuffer)vkDestroyFramebuffer(d,framebuffer,nullptr);if(pass)vkDestroyRenderPass(d,pass,nullptr);
        if(view)vkDestroyImageView(d,view,nullptr);if(image)vkDestroyImage(d,image,nullptr);
        if(imageMemory)vkFreeMemory(d,imageMemory,nullptr);if(buffer)vkDestroyBuffer(d,buffer,nullptr);
        if(bufferMemory)vkFreeMemory(d,bufferMemory,nullptr);
    }
};
unsigned Memory(VkPhysicalDevice p,unsigned bits,VkMemoryPropertyFlags flags) {
    VkPhysicalDeviceMemoryProperties m{};vkGetPhysicalDeviceMemoryProperties(p,&m);
    for(unsigned i=0;i<m.memoryTypeCount;++i)if((bits&(1u<<i))&&(m.memoryTypes[i].propertyFlags&flags)==flags)return i;
    throw std::runtime_error("No suitable memory type");
}
}

static bool DrawShader(VkPhysicalDevice physical,VkDevice device,unsigned family,bool colour,const char* directory,const char* texturePath,int mathCase=-1) {
    // Original D3D lrp convention, including extrapolation outside [0,1].
    const std::uint32_t vertex[]={0xfffe0300,0x0200001f,0x80000000,0x900f0000,
        0x0200001f,0x80000000,0xe00f0000,0x02000001,0xe00f0000,0x90e40000,0x0000ffff};
    const std::uint32_t fragment[]={0xffff0300,
        0x05000051,0xa00f0000,0xbf800000,0x3e800000,0x3fc00000,0x3f800000,
        0x05000051,0xa00f0001,0x3f000000,0x3f000000,0x3e800000,0x00000000,
        0x05000051,0xa00f0002,0x3e800000,0x3f400000,0x3f800000,0x3f000000,
        0x04000012,0x800f0000,0xa0e40000,0xa0e40001,0xa0e40002,
        0x02000001,0x800f0800,0x80e40000,0x0000ffff};
    std::vector<std::uint32_t> mathShader={0xffff0300,
        0x05000051,0xa00f0000,0x40400000,0x40800000,0,0x40200000,
        0x05000051,0xa00f0001,0x3e800000,0x3e800000,0x3e800000,0x3e800000,
        0x02000001,0x800f0000,0xa0e40001,
        0x02000024,0x800f0000,0xa0e40000,
        0x02000001,0x800f0800,0x80e40000,0x0000ffff};
    if(mathCase==1) {mathShader[3]=mathShader[4]=mathShader[5]=0;mathShader[6]=0x3f800000;}
    if(mathCase==2)mathShader[17]=0x80080000;
    if(mathCase==3)mathShader[18]=0xa0c60000;
    if(mathCase==4)mathShader[18]=0xa1e40000;
    if(mathCase==5) {
        mathShader[3]=0xbf000000;mathShader[9]=0x40000000;
        mathShader.erase(mathShader.begin()+16,mathShader.begin()+19);
        mathShader.insert(mathShader.begin()+16,{0x03000020,0x800f0000,0xa0000000,0xa0000001});
    }
    if(mathCase==6) {
        mathShader.erase(mathShader.begin()+16,mathShader.begin()+19);
        mathShader.insert(mathShader.begin()+16,{0x0400005a,0x800f0000,0xa0e40000,0xa0e40001,0xa0ff0001});
    }
    const std::uint32_t varyingVertex[]={0xfffe0300,
        0x0200001f,0x80000000,0x900f0000,0x0200001f,0x80000000,0xe00f0000,
        0x0200001f,0x80000005,0xe00f0001,
        0x05000051,0xa00f0000,0x3e800000,0x3f000000,0x3f400000,0x3f800000,
        0x02000001,0xe00f0000,0x90e40000,0x02000001,0xe00f0001,0xa0e40000,0x0000ffff};
    if(mathCase>=7&&mathCase<=9) {
        mathShader={0xffff0300,0x0200001f,0x80000000,mathCase==9?0x90021000u:0x90031000u,
            0x05000051,0xa00f0000,0,0,0,0x3f800000,
            0x02000001,0x800f0000,0xa0e40000,
            0x02000001,mathCase==9?0x80020000u:0x80030000u,mathCase==9?0x90551000u:0x90e41000u,
            0x02000001,0x800f0800,0x80e40000,0x0000ffff};
        if(mathCase==8) {
            mathShader.insert(mathShader.begin()+1,{0x0200001f,0x80000005,0x900f0000});
            mathShader[15]=0x90e40000; // mov r0,v0 while vPos occupies its own namespace.
        }
    }
    if(mathCase>=10) {
        mathShader={0xffff0300,0x0200001f,0x80000000,0x90011001,
            0x05000051,0xa00f0000,0,0x3f800000,0xbf800000,0,
            0x02000001,0x800f0000,0xa0000000,
            0x02010029,0x90001001,0xa0000000,
            0x02000001,0x80010000,0xa0550000,
            0x0000002a,0x02000001,0x80010000,0xa0aa0000,0x0000002b,
            0x02000001,0x800f0800,0x80e40000,0x0000ffff};
    }
    if(mathCase>=12) {
        mathShader={0xffff0300,0x0200001f,0x80000005,0x900c0000,
            0x05000051,0xa00f0000,0x3e800000,0x3e800000,0x3e800000,0x3e800000,
            0x02000001,0x800f0000,0xa0e40000,
            0x02000001,0x800c0000,0x90e40000,
            0x02000001,0x800f0800,0x80e40000,0x0000ffff};
        if(mathCase==13) {
            mathShader[13]=0x03000002;
            mathShader.insert(mathShader.begin()+16,0xa0e40000);
        }
    }
    if(mathCase>=14) {
        const std::uint32_t repeatCount=mathCase==15?0:mathCase==16?0xffffffffu:mathCase==21?5:mathCase==23?255:2;
        mathShader={0xffff0300,
            0x05000051,0xa00f0000,0x3e800000,0x3e800000,0x3e800000,0x3e800000,
            0x05000051,0xa00f0001,0,0,0,0,
            0x05000030,0xf00f0000,repeatCount,0,0,0,
            0x05000030,0xf00f0001,3,0,0,0,
            0x02000001,0x800f0000,0xa0e40001,
            0x01000026,0xf0e40000};
        if(mathCase==17||mathCase==20||mathCase==22)mathShader.insert(mathShader.end(),{0x01000026,mathCase==22?0xf0e40000u:0xf0e40001u});
        mathShader.insert(mathShader.end(),{0x03000002,0x800f0000,0x80e40000,0xa0e40000});
        if(mathCase==18||mathCase==20)mathShader.push_back(0x0000002c);
        if(mathCase==21)mathShader.insert(mathShader.end(),{0x0201002d,0x80000000,0xa0000000});
        if(mathCase==17||mathCase==20||mathCase==22)mathShader.push_back(0x00000027);
        mathShader.insert(mathShader.end(),{0x00000027,0x02000001,0x800f0800,0x80e40000,0x0000ffff});
        if(mathCase==19)mathShader.erase(mathShader.begin()+13,mathShader.begin()+19);
    }
    bo1::vulkan::ShaderTranslation v,p;
    using namespace bo1::android::post_fusion_trial::bytecode;
    const void* vertexCode=(mathCase==8||mathCase==12||mathCase==13)?static_cast<const void*>(varyingVertex):colour?static_cast<const void*>(SharedVS):vertex;
    const auto vertexBytes=(mathCase==8||mathCase==12||mathCase==13)?sizeof(varyingVertex):colour?sizeof(SharedVS):sizeof(vertex);
    const void* pixelCode=mathCase>=0?static_cast<const void*>(mathShader.data()):colour?static_cast<const void*>(ColorPS):fragment;
    const auto pixelBytes=mathCase>=0?mathShader.size()*4:colour?sizeof(ColorPS):sizeof(fragment);
    if(!bo1::vulkan::TranslateShaderPair(vertexCode,vertexBytes,pixelCode,pixelBytes,v,p)) {
        std::fprintf(stderr,"FAIL lrp translate %s %s\n",v.diagnostics.c_str(),p.diagnostics.c_str());return false;
    }
    if(mathCase==12) {
        bo1::vulkan::ShaderAlphaTest alpha;
        if(!bo1::vulkan::TranslateShaderPair(vertexCode,vertexBytes,pixelCode,pixelBytes,v,p,&alpha))return false;
        auto dump=[](const char* path,const bo1::vulkan::ShaderTranslation& shader){if(auto f=std::fopen(path,"wb")){std::fwrite(shader.spirv.data(),4,shader.spirv.size(),f);std::fclose(f);}};
        dump("/data/local/tmp/blackopsarm-native-raster45/partial-vs.spv",v);
        dump("/data/local/tmp/blackopsarm-native-raster45/partial-ps.spv",p);
    }
    if(mathCase>=14) {
        char path[160];std::snprintf(path,sizeof(path),"/data/local/tmp/blackopsarm-native-repeat46/rep-%i-ps.spv",mathCase);
        if(auto file=std::fopen(path,"wb")){std::fwrite(p.spirv.data(),4,p.spirv.size(),file);std::fclose(file);}
    }
    try {
        DrawResources r{device};
        const unsigned width=colour?64:2,height=colour?40:2;
        const VkDeviceSize readBytes=width*height*16;
        VkPhysicalDeviceProperties props{};vkGetPhysicalDeviceProperties(physical,&props);
        const auto align=props.limits.minUniformBufferOffsetAlignment;
        const VkDeviceSize vertexOffset=readBytes,uniformOffset=((vertexOffset+72+align-1)/align)*align;
        const VkDeviceSize pixelOffset=uniformOffset+4096,textureOffset=pixelOffset+4096;
        std::vector<unsigned char> pixels(width*height*4);
        for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x) {
            const unsigned i=(y*width+x)*4;pixels[i]=x*255/(width-1);pixels[i+1]=y*255/(height-1);
            pixels[i+2]=((x/8+y/8)&1)?32:192;pixels[i+3]=255;
        }
        if(colour&&texturePath) {
            FILE* f=std::fopen(texturePath,"rb");if(!f)throw std::runtime_error("Input texture unavailable");
            const bool read=std::fread(pixels.data(),1,pixels.size(),f)==pixels.size();
            const bool exact=std::fgetc(f)==EOF;const bool closed=std::fclose(f)==0;
            if(!read||!exact||!closed)throw std::runtime_error("Input texture size/read failure");
        }
        const auto format=VK_FORMAT_R32G32B32A32_SFLOAT;
        VkFormatProperties fp{};vkGetPhysicalDeviceFormatProperties(physical,format,&fp);
        if(!(fp.optimalTilingFeatures&VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT))throw std::runtime_error("RGBA32F unsupported");
        VkImageCreateInfo image{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};image.imageType=VK_IMAGE_TYPE_2D;
        image.format=format;image.extent={width,height,1};image.mipLevels=1;image.arrayLayers=1;image.samples=VK_SAMPLE_COUNT_1_BIT;
        image.tiling=VK_IMAGE_TILING_OPTIMAL;image.usage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        Require(vkCreateImage(device,&image,nullptr,&r.image),"create image");
        VkMemoryRequirements requirements{};vkGetImageMemoryRequirements(device,r.image,&requirements);
        VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};allocation.allocationSize=requirements.size;
        allocation.memoryTypeIndex=Memory(physical,requirements.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        Require(vkAllocateMemory(device,&allocation,nullptr,&r.imageMemory),"image memory");
        Require(vkBindImageMemory(device,r.image,r.imageMemory,0),"bind image");
        VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};view.image=r.image;view.viewType=VK_IMAGE_VIEW_TYPE_2D;
        view.format=format;view.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
        Require(vkCreateImageView(device,&view,nullptr,&r.view),"create view");
        // Readback uses bytes0..63, vertex data64..111, with a legal16-byte alignment.
        VkBufferCreateInfo buffer{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};buffer.size=textureOffset+pixels.size();
        buffer.usage=VK_BUFFER_USAGE_VERTEX_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT|VK_BUFFER_USAGE_TRANSFER_SRC_BIT|VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        Require(vkCreateBuffer(device,&buffer,nullptr,&r.buffer),"create buffer");
        vkGetBufferMemoryRequirements(device,r.buffer,&requirements);allocation.allocationSize=requirements.size;
        allocation.memoryTypeIndex=Memory(physical,requirements.memoryTypeBits,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        Require(vkAllocateMemory(device,&allocation,nullptr,&r.bufferMemory),"buffer memory");
        Require(vkBindBufferMemory(device,r.buffer,r.bufferMemory,0),"bind buffer");
        void* mapped{};Require(vkMapMemory(device,r.bufferMemory,0,VK_WHOLE_SIZE,0,&mapped),"map upload");
        const float positions[]={-1,-1,0,1, 3,-1,0,1, -1,3,0,1};
        std::memset(mapped,0,std::size_t(buffer.size));
        const float fullscreen[]={0,float(height*2),0,1,0,2, 0,0,0,1,0,0, float(width*2),0,0,1,2,0};
        std::memcpy(static_cast<char*>(mapped)+vertexOffset,colour?fullscreen:positions,colour?sizeof(fullscreen):sizeof(positions));
        const float matrix[]={2.0f/width,0,0,-1, 0,-2.0f/height,0,1, 0,0,1,0, 0,0,0,1};
        std::memcpy(static_cast<char*>(mapped)+uniformOffset,matrix,sizeof(matrix));
        float constants[256][4]{};
        constants[5][0]=constants[8][0]=constants[11][0]=0.75f;
        constants[6][1]=constants[7][2]=constants[9][1]=constants[10][2]=constants[20][1]=constants[21][2]=1;
        std::memcpy(static_cast<char*>(mapped)+pixelOffset,constants,sizeof(constants));
        if(mathCase==19) {const int count[4]={3,0,0,0};std::memcpy(static_cast<char*>(mapped)+pixelOffset,count,sizeof(count));}
        std::memcpy(static_cast<char*>(mapped)+textureOffset,pixels.data(),pixels.size());vkUnmapMemory(device,r.bufferMemory);
        if(colour) {
            image.format=VK_FORMAT_R8G8B8A8_UNORM;image.usage=VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT;
            Require(vkCreateImage(device,&image,nullptr,&r.texture),"texture image");
            vkGetImageMemoryRequirements(device,r.texture,&requirements);allocation.allocationSize=requirements.size;
            allocation.memoryTypeIndex=Memory(physical,requirements.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            Require(vkAllocateMemory(device,&allocation,nullptr,&r.textureMemory),"texture memory");
            Require(vkBindImageMemory(device,r.texture,r.textureMemory,0),"bind texture");
            view.image=r.texture;view.format=image.format;Require(vkCreateImageView(device,&view,nullptr,&r.textureView),"texture view");
            VkSamplerCreateInfo sampler{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};sampler.magFilter=sampler.minFilter=VK_FILTER_LINEAR;
            sampler.mipmapMode=VK_SAMPLER_MIPMAP_MODE_NEAREST;sampler.addressModeU=sampler.addressModeV=sampler.addressModeW=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            Require(vkCreateSampler(device,&sampler,nullptr,&r.sampler),"texture sampler");
        }
        VkAttachmentDescription attachment{};attachment.format=format;attachment.samples=VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;attachment.storeOp=VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;attachment.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;attachment.finalLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        VkAttachmentReference color{0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};VkSubpassDescription subpass{};
        subpass.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;subpass.colorAttachmentCount=1;subpass.pColorAttachments=&color;
        VkSubpassDependency dependency{0,VK_SUBPASS_EXTERNAL,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,VK_ACCESS_TRANSFER_READ_BIT,0};
        VkRenderPassCreateInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};pass.attachmentCount=1;pass.pAttachments=&attachment;
        pass.subpassCount=1;pass.pSubpasses=&subpass;pass.dependencyCount=1;pass.pDependencies=&dependency;
        Require(vkCreateRenderPass(device,&pass,nullptr,&r.pass),"render pass");
        VkFramebufferCreateInfo framebuffer{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};framebuffer.renderPass=r.pass;
        framebuffer.attachmentCount=1;framebuffer.pAttachments=&r.view;framebuffer.width=width;framebuffer.height=height;framebuffer.layers=1;
        Require(vkCreateFramebuffer(device,&framebuffer,nullptr,&r.framebuffer),"framebuffer");
        r.program=std::make_unique<bo1::vulkan::GraphicsProgram>(device);
        VkVertexInputBindingDescription binding{0,colour?24u:16u,VK_VERTEX_INPUT_RATE_VERTEX};
        VkVertexInputAttributeDescription attributes[]={{0,0,VK_FORMAT_R32G32B32A32_SFLOAT,0},{1,0,VK_FORMAT_R32G32_SFLOAT,16}};
        bo1::vulkan::ProgramDescription description;
        description.vertexBytecode=vertexCode;description.vertexBytes=vertexBytes;description.pixelBytecode=pixelCode;description.pixelBytes=pixelBytes;
        if(mathCase==10)description.frontFace=VK_FRONT_FACE_COUNTER_CLOCKWISE;
        description.renderPass=r.pass;description.vertexBindings=&binding;description.vertexBindingCount=1;
        description.vertexAttributes=attributes;description.vertexAttributeCount=colour?2:1;
        std::string error;if(!r.program->Create(description,VK_NULL_HANDLE,error))throw std::runtime_error(error);
        VkDescriptorPoolSize sizes[]={{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,2},{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,1},{VK_DESCRIPTOR_TYPE_SAMPLER,1}};
        VkDescriptorPoolCreateInfo descriptorPool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};descriptorPool.maxSets=2;
        descriptorPool.poolSizeCount=3;descriptorPool.pPoolSizes=sizes;
        Require(vkCreateDescriptorPool(device,&descriptorPool,nullptr,&r.descriptors),"frame descriptor pool");
        std::vector<bo1::vulkan::ProgramResource> vertexResources,pixelResources;
        if(colour) {
            using Kind=bo1::vulkan::ResourceKind;
            vertexResources.push_back({Kind::Constants,0,{r.buffer,uniformOffset,4096},{}});
            pixelResources.push_back({Kind::Constants,0,{r.buffer,pixelOffset,4096},{}});
            pixelResources.push_back({Kind::Texture,0,{}, {VK_NULL_HANDLE,r.textureView,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}});
            pixelResources.push_back({Kind::Sampler,0,{}, {r.sampler,VK_NULL_HANDLE,VK_IMAGE_LAYOUT_UNDEFINED}});
        }
        if(mathCase==19)for(const auto& binding:p.bindings)if(binding.kind==bo1::vulkan::ResourceKind::Constants)
            pixelResources.push_back({bo1::vulkan::ResourceKind::Constants,binding.registerIndex,{r.buffer,pixelOffset,4096},{}});
        std::array<VkDescriptorSet,2> sets{};
        if(!r.program->BindResources(r.descriptors,vertexResources,pixelResources,sets,error))throw std::runtime_error(error);
        VkViewport vp{0,float(height),float(width),-float(height),0,1};VkRect2D scissor{{0,0},{width,height}};
        VkCommandPoolCreateInfo pool{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};pool.queueFamilyIndex=family;
        Require(vkCreateCommandPool(device,&pool,nullptr,&r.pool),"command pool");
        VkCommandBufferAllocateInfo command{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};command.commandPool=r.pool;command.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;command.commandBufferCount=1;
        VkCommandBuffer cmd{};Require(vkAllocateCommandBuffers(device,&command,&cmd),"command buffer");
        VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};begin.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        Require(vkBeginCommandBuffer(cmd,&begin),"begin command");
        if(colour) {
            VkImageMemoryBarrier transition{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};transition.image=r.texture;
            transition.srcQueueFamilyIndex=transition.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
            transition.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};transition.oldLayout=VK_IMAGE_LAYOUT_UNDEFINED;
            transition.newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;transition.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
            vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&transition);
            VkBufferImageCopy upload{};upload.bufferOffset=textureOffset;upload.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};upload.imageExtent={width,height,1};
            vkCmdCopyBufferToImage(cmd,r.buffer,r.texture,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&upload);
            transition.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;transition.newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            transition.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;transition.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
            vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,0,0,nullptr,0,nullptr,1,&transition);
        }
        VkClearValue clear{};VkRenderPassBeginInfo draw{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};draw.renderPass=r.pass;draw.framebuffer=r.framebuffer;
        draw.renderArea=scissor;draw.clearValueCount=1;draw.pClearValues=&clear;
        vkCmdBeginRenderPass(cmd,&draw,VK_SUBPASS_CONTENTS_INLINE);r.program->Bind(cmd,sets);vkCmdSetViewport(cmd,0,1,&vp);vkCmdSetScissor(cmd,0,1,&scissor);
        VkDeviceSize offset=vertexOffset;vkCmdBindVertexBuffers(cmd,0,1,&r.buffer,&offset);vkCmdDraw(cmd,3,1,0,0);vkCmdEndRenderPass(cmd);
        VkBufferImageCopy copy{};copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};copy.imageExtent={width,height,1};
        vkCmdCopyImageToBuffer(cmd,r.image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,r.buffer,1,&copy);
        VkBufferMemoryBarrier barrier{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};barrier.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_HOST_READ_BIT;
        barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;barrier.buffer=r.buffer;barrier.size=readBytes;
        vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,0,nullptr,1,&barrier,0,nullptr);
        Require(vkEndCommandBuffer(cmd),"end command");VkFenceCreateInfo fence{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        Require(vkCreateFence(device,&fence,nullptr,&r.fence),"fence");VkQueue queue{};vkGetDeviceQueue(device,family,0,&queue);
        VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&cmd;
        Require(vkQueueSubmit(queue,1,&submit,r.fence),"submit");r.submitted=true;
        Require(vkWaitForFences(device,1,&r.fence,VK_TRUE,10000000000ull),"owner fence");r.submitted=false;
        Require(vkMapMemory(device,r.bufferMemory,0,VK_WHOLE_SIZE,0,&mapped),"map readback");
        std::vector<float> result(width*height*4);std::memcpy(result.data(),mapped,std::size_t(readBytes));vkUnmapMemory(device,r.bufferMemory);
        const float expected[4]={0,0.6875f,-0.125f,0};bool matches=true;
        const float mathExpected[7][4]={{0.6f,0.8f,0,0.5f},{0,0,0,0x1.fffffep+127f},
            {0.25f,0.25f,0.25f,0.5f},{0,0.8f,0.6f,0.5f},{-0.6f,-0.8f,0,-0.5f},{0.25f,0.25f,0.25f,0.25f},{2,2,2,2}};
        double maximumError=0;
        for(unsigned i=0;i<result.size();++i) {
            const float oracle=mathCase>=14?(mathCase==15||mathCase==16?0.0f:mathCase==17?1.5f:mathCase==18?0.25f:mathCase==19?0.75f:mathCase==22?1.0f:mathCase==23?63.75f:0.5f)
                :mathCase>=12?(i%4<2?0.25f:i%4==2?(mathCase==12?0.75f:1.0f):(mathCase==12?1.0f:1.25f))
                :mathCase>=10?(i%4==0?(mathCase==10?1.0f:-1.0f):0.0f)
                :mathCase>=7?(i%4==0?(mathCase==9?0.0f:float((i/4)%width)):i%4==1?float((i/4)/width):i%4==2?(mathCase==8?0.75f:0.0f):1.0f)
                :mathCase>=0?mathExpected[mathCase][i%4]:colour?(pixels[i]/255.0f)*(i%4==0?0.75f:1.0f):expected[i%4];
            const double error=std::fabs(result[i]-oracle);maximumError=std::max(maximumError,error);
            matches=matches&&std::isfinite(result[i])&&error<0.00001*std::max(1.0,double(std::fabs(oracle)));
        }
        if(mathCase>=0) {
            std::printf("sm3_math_gpu=%s operation=%s case=%i rgba=%.9g,%.9g,%.9g,%.9g\n",matches?"PASS":"FAIL",mathCase>=14?"rep":mathCase>=12?"partial_input":mathCase>=10?"vface":mathCase>=7?"vpos":mathCase==5?"pow_abs":mathCase==6?"dp2add":"nrm",mathCase,result[0],result[1],result[2],result[3]);return matches;
        }
        if(colour) {
            std::printf("t5_colour_native=%s pixels=%u max_error=%.9g texture=%s full_game_frame=0\n",matches?"PASS":"FAIL",width*height,maximumError,texturePath?"captured_game":"gradient");
            if(directory) {
                const std::string path=std::string(directory)+"/t5-colour-native.pam";FILE* f=std::fopen(path.c_str(),"wb");
                if(!f)throw std::runtime_error("Output PAM unavailable");
                std::fprintf(f,"P7\nWIDTH %u\nHEIGHT %u\nDEPTH 4\nMAXVAL 255\nTUPLTYPE RGB_ALPHA\nENDHDR\n",width,height);
                std::vector<unsigned char> output(pixels.size());for(unsigned i=0;i<output.size();++i)output[i]=static_cast<unsigned char>(std::lround(std::fmin(1.0f,std::fmax(0.0f,result[i]))*255));
                const bool written=std::fwrite(output.data(),1,output.size(),f)==output.size();const bool closed=std::fclose(f)==0;
                if(!written||!closed)throw std::runtime_error("Output PAM write failure");
            }
            return matches;
        }
        std::printf("lrp_gpu=%s rgba=%.9g,%.9g,%.9g,%.9g expected=0,0.6875,-0.125,0 pixels=4\n",
            matches?"PASS":"FAIL",result[0],result[1],result[2],result[3]);return matches;
    } catch(const std::exception& e) { std::fprintf(stderr,"FAIL lrp GPU: %s\n",e.what());return false; }
}

bool NativeVulkanLrpDraw(VkPhysicalDevice p,VkDevice d,unsigned family) {
    if(!DrawShader(p,d,family,false,nullptr,nullptr))return false;
    for(int i=0;i<24;++i)if(!DrawShader(p,d,family,false,nullptr,nullptr,i))return false;
    return true;
}
bool NativeVulkanColourDraw(VkPhysicalDevice p,VkDevice d,unsigned family,const char* directory,const char* texture) {
    return DrawShader(p,d,family,true,directory,texture);
}
