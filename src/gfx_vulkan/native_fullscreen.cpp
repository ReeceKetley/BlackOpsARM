#include "native_fullscreen.h"
#include <cstdint>
namespace bo1::vulkan::native_fullscreen {
namespace {
const std::uint32_t FullscreenVertex[]={
#include "fullscreen.vert.spv.inc"
};
const std::uint32_t DofDownsampleFragment[]={
#include "dof_downsample.frag.spv.inc"
};
const std::uint32_t PostComposeFragment[]={
#include "post_compose.frag.spv.inc"
};
bool Check(VkResult result,const char* action,std::string& error) {
    if(result==VK_SUCCESS)return true;error=std::string("Native fullscreen ")+action+": "+std::to_string(int(result));return false;
}
bool Module(VkDevice device,const std::uint32_t* code,std::size_t bytes,VkShaderModule& module,std::string& error) {
    VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};info.codeSize=bytes;info.pCode=code;
    return Check(vkCreateShaderModule(device,&info,nullptr,&module),"shader module",error);
}
}
unsigned ImageCount(Program program){return program==Program::DofDownsample?2:5;}
Library::~Library() {
    for(auto& entry:programs_) {
        for(auto& pipeline:entry.pipelines)vkDestroyPipeline(device_,pipeline.second,nullptr);
        if(entry.layout)vkDestroyPipelineLayout(device_,entry.layout,nullptr);
        if(entry.set)vkDestroyDescriptorSetLayout(device_,entry.set,nullptr);
        if(entry.fragment)vkDestroyShaderModule(device_,entry.fragment,nullptr);
    }
    if(vertex_)vkDestroyShaderModule(device_,vertex_,nullptr);
    if(linear_)vkDestroySampler(device_,linear_,nullptr);
    if(point_)vkDestroySampler(device_,point_,nullptr);
}
bool Library::Initialize(std::string& error) {
    if(!device_||vertex_) {error="Invalid native fullscreen library initialization";return false;}
    if(!Module(device_,FullscreenVertex,sizeof(FullscreenVertex),vertex_,error))return false;
    const std::uint32_t* fragments[]={DofDownsampleFragment,PostComposeFragment};
    const std::size_t sizes[]={sizeof(DofDownsampleFragment),sizeof(PostComposeFragment)};
    for(unsigned i=0;i<unsigned(Program::Count);++i) {
        auto& entry=programs_[i];
        if(!Module(device_,fragments[i],sizes[i],entry.fragment,error))return false;
        VkDescriptorSetLayoutBinding bindings[9]{};unsigned count=0;
        for(unsigned image=0;image<ImageCount(Program(i));++image)
            bindings[count++]={image,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr};
        bindings[count++]={8,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr};
        VkDescriptorSetLayoutCreateInfo set{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};set.bindingCount=count;set.pBindings=bindings;
        if(!Check(vkCreateDescriptorSetLayout(device_,&set,nullptr,&entry.set),"set layout",error))return false;
        VkPipelineLayoutCreateInfo layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};layout.setLayoutCount=1;layout.pSetLayouts=&entry.set;
        if(!Check(vkCreatePipelineLayout(device_,&layout,nullptr,&entry.layout),"pipeline layout",error))return false;
    }
    VkSamplerCreateInfo sampler{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    sampler.addressModeU=sampler.addressModeV=sampler.addressModeW=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler.magFilter=sampler.minFilter=VK_FILTER_LINEAR;sampler.mipmapMode=VK_SAMPLER_MIPMAP_MODE_NEAREST;
    if(!Check(vkCreateSampler(device_,&sampler,nullptr,&linear_),"linear sampler",error))return false;
    sampler.magFilter=sampler.minFilter=VK_FILTER_NEAREST;
    return Check(vkCreateSampler(device_,&sampler,nullptr,&point_),"point sampler",error);
}
bool Library::Acquire(Program program,VkRenderPass pass,VkPipeline& pipeline,VkPipelineLayout& layout,VkDescriptorSetLayout& set,std::string& error) {
    if(unsigned(program)>=unsigned(Program::Count)||!pass||!vertex_) {error="Native fullscreen program/pass unavailable";return false;}
    auto& entry=programs_[unsigned(program)];layout=entry.layout;set=entry.set;
    if(auto found=entry.pipelines.find(pass);found!=entry.pipelines.end()) {pipeline=found->second;return true;}
    VkPipelineShaderStageCreateInfo stages[2]={{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO},{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO}};
    stages[0].stage=VK_SHADER_STAGE_VERTEX_BIT;stages[0].module=vertex_;stages[0].pName="main";
    stages[1].stage=VK_SHADER_STAGE_FRAGMENT_BIT;stages[1].module=entry.fragment;stages[1].pName="main";
    VkPipelineVertexInputStateCreateInfo input{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};assembly.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};viewport.viewportCount=viewport.scissorCount=1;
    VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    raster.polygonMode=VK_POLYGON_MODE_FILL;raster.cullMode=VK_CULL_MODE_NONE;raster.lineWidth=1;
    VkPipelineMultisampleStateCreateInfo samples{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};samples.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
    VkPipelineDepthStencilStateCreateInfo depth{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    VkPipelineColorBlendAttachmentState attachment{};attachment.colorWriteMask=0xf;
    VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};blend.attachmentCount=1;blend.pAttachments=&attachment;
    const VkDynamicState dynamic[]={VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};dynamicState.dynamicStateCount=2;dynamicState.pDynamicStates=dynamic;
    VkGraphicsPipelineCreateInfo info{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    info.stageCount=2;info.pStages=stages;info.pVertexInputState=&input;info.pInputAssemblyState=&assembly;info.pViewportState=&viewport;
    info.pRasterizationState=&raster;info.pMultisampleState=&samples;info.pDepthStencilState=&depth;info.pColorBlendState=&blend;
    info.pDynamicState=&dynamicState;info.layout=entry.layout;info.renderPass=pass;
    if(!Check(vkCreateGraphicsPipelines(device_,VK_NULL_HANDLE,1,&info,nullptr,&pipeline),"pipeline",error))return false;
    entry.pipelines.emplace(pass,pipeline);return true;
}
}
