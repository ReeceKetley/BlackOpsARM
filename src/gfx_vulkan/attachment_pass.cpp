#include "attachment_pass.h"
#include "image_usage.h"
#include "render_pass_compatibility.h"
#include <vector>
#include <algorithm>
namespace bo1::vulkan {
namespace {
bool Check(VkResult r,const char* label,std::string& error) {if(r==VK_SUCCESS)return true;error=std::string(label)+": "+std::to_string(int(r));return false;}
}
SharedAttachmentRenderPass::~SharedAttachmentRenderPass(){if(handle)vkDestroyRenderPass(device,handle,nullptr);}
SharedFramebuffer::~SharedFramebuffer(){if(handle)vkDestroyFramebuffer(device,handle,nullptr);}
std::shared_ptr<SharedFramebuffer> AttachmentPassCache::Framebuffer(VkRenderPass pass,const std::shared_ptr<Texture2D>& color,
    const std::shared_ptr<Texture2D>& depth,const VkFramebufferCreateInfo& info,std::string& error) {
    // Periodically drop entries whose attachment images were destroyed; their
    // framebuffers survive in any pass that still records or retains them.
    if(++lookups_>=1024) {
        lookups_=0;
        framebuffers_.erase(std::remove_if(framebuffers_.begin(),framebuffers_.end(),[](const FramebufferEntry& e){
            return (e.colorKey&&e.color.expired())||(e.depthKey&&e.depth.expired());}),framebuffers_.end());
    }
    for(const auto& e:framebuffers_)
        if(e.pass==pass&&e.colorKey==color.get()&&e.depthKey==depth.get()
            &&(!e.colorKey||e.color.lock()==color)&&(!e.depthKey||e.depth.lock()==depth))return e.framebuffer;
    auto framebuffer=std::make_shared<SharedFramebuffer>();framebuffer->device=device_;
    if(!Check(vkCreateFramebuffer(device_,&info,nullptr,&framebuffer->handle),"cached attachment framebuffer",error))return {};
    framebuffers_.push_back({pass,color,depth,color.get(),depth.get(),framebuffer});return framebuffer;
}
std::shared_ptr<SharedAttachmentRenderPass> AttachmentPassCache::Acquire(VkDevice device,const std::array<unsigned,8>& key,const VkRenderPassCreateInfo& info,std::string& error) {
    if(!device||device!=device_) {error="Attachment render-pass cache device mismatch";return {};}
    for(const auto& entry:entries_)if(entry.key==key)return entry.pass;
    // No live identity eviction: ProgramCache keys refer to these handles.
    if(entries_.size()>=128) {error="Attachment render-pass definition budget exceeded";return {};}
    auto pass=std::make_shared<SharedAttachmentRenderPass>();pass->device=device_;
    if(!Check(vkCreateRenderPass(device_,&info,nullptr,&pass->handle),"cached attachment render pass",error))return {};
    std::vector<std::uint32_t> compatibility;
    if(RenderPassCompatibility(info,compatibility)) {
        for(const auto& entry:entries_)if(entry.compatibility==compatibility) {
            pass->pipelinePass=entry.pass->pipelinePass?entry.pass->pipelinePass:entry.pass;break;
        }
    }
    entries_.push_back({key,pass,std::move(compatibility)});return pass;
}
AttachmentPass::~AttachmentPass(){if(framebuffer_&&!sharedFramebuffer_)vkDestroyFramebuffer(device_,framebuffer_,nullptr);if(pass_&&!sharedPass_)vkDestroyRenderPass(device_,pass_,nullptr);}
bool AttachmentPass::Create(std::shared_ptr<Texture2D> color,std::shared_ptr<Texture2D> depth,bool preserveDepth,std::string& error,bool loadExisting,int depthLoad) {
    const bool loadDepth=depthLoad<0?loadExisting:depthLoad!=0;
    if(depthLoad>1||depthLoad<-1) {error="Invalid independent depth load policy";return false;}
    if(pass_||(!color&&!depth)) {error="Invalid attachment pass owner";return false;}
    const auto& size=(color?color:depth)->Description();extent_={size.extent.width,size.extent.height};
    for(const auto& image:{color,depth})if(image) {
        const auto& d=image->Description();
        if(image->Device()!=device_||!image->AttachmentView()||d.viewType!=VK_IMAGE_VIEW_TYPE_2D||d.mipLevels!=1
            ||d.extent.width!=extent_.width||d.extent.height!=extent_.height) {error="Attachment shape/device mismatch";return false;}
    }
    if(color&&(color->Description().aspects!=VK_IMAGE_ASPECT_COLOR_BIT||!(color->Description().usage&VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT))) {error="Invalid color attachment role";return false;}
    if(depth&&(!(depth->Description().aspects&VK_IMAGE_ASPECT_DEPTH_BIT)||!(depth->Description().usage&VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
        ||(preserveDepth&&(depth->Description().usage&VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT))
        ||(!preserveDepth&&(depth->Description().usage&VK_IMAGE_USAGE_SAMPLED_BIT)))) {error="Depth preservation conflicts with image role";return false;}
    if(loadDepth&&depth&&(depth->Description().usage&VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT)) {error="Transient depth cannot load prior-pass contents";return false;}
    if(color&&(color->Description().usage&VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT)) {error="Stored color cannot be transient";return false;}
    std::vector<VkAttachmentDescription> attachments;std::vector<VkImageView> views;
    VkAttachmentReference colorRef{},depthRef{};
    if(color) {
        VkAttachmentDescription a{};a.format=color->Description().format;a.samples=VK_SAMPLE_COUNT_1_BIT;
        a.loadOp=loadExisting?VK_ATTACHMENT_LOAD_OP_LOAD:VK_ATTACHMENT_LOAD_OP_CLEAR;a.storeOp=VK_ATTACHMENT_STORE_OP_STORE;a.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;a.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
        a.finalLayout=(color->Description().usage&VK_IMAGE_USAGE_SAMPLED_BIT)?VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        a.initialLayout=loadExisting?a.finalLayout:VK_IMAGE_LAYOUT_UNDEFINED;
        colorRef={unsigned(attachments.size()),VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};attachments.push_back(a);views.push_back(color->AttachmentView());
    }
    if(depth) {
        VkAttachmentDescription a{};a.format=depth->Description().format;a.samples=VK_SAMPLE_COUNT_1_BIT;
        a.loadOp=loadDepth?VK_ATTACHMENT_LOAD_OP_LOAD:VK_ATTACHMENT_LOAD_OP_CLEAR;a.storeOp=preserveDepth?VK_ATTACHMENT_STORE_OP_STORE:VK_ATTACHMENT_STORE_OP_DONT_CARE;
        a.stencilLoadOp=loadDepth?VK_ATTACHMENT_LOAD_OP_LOAD:VK_ATTACHMENT_LOAD_OP_CLEAR;a.stencilStoreOp=a.storeOp;
        a.finalLayout=(depth->Description().usage&VK_IMAGE_USAGE_SAMPLED_BIT)?VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        a.initialLayout=loadDepth?a.finalLayout:VK_IMAGE_LAYOUT_UNDEFINED;
        depthRef={unsigned(attachments.size()),VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};attachments.push_back(a);views.push_back(depth->AttachmentView());
    }
    VkSubpassDescription sub{};sub.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;
    if(color){sub.colorAttachmentCount=1;sub.pColorAttachments=&colorRef;}if(depth)sub.pDepthStencilAttachment=&depthRef;
    const VkPipelineStageFlags attachmentStages=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    const VkPipelineStageFlags shaderStages=VK_PIPELINE_STAGE_VERTEX_SHADER_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    const VkAccessFlags attachmentAccess=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    VkSubpassDependency dependencies[2]{};dependencies[0]={VK_SUBPASS_EXTERNAL,0,shaderStages|attachmentStages|VK_PIPELINE_STAGE_TRANSFER_BIT,attachmentStages,
        VK_ACCESS_SHADER_READ_BIT|attachmentAccess|VK_ACCESS_TRANSFER_READ_BIT|VK_ACCESS_TRANSFER_WRITE_BIT,attachmentAccess|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,VK_DEPENDENCY_BY_REGION_BIT};
    dependencies[1]={0,VK_SUBPASS_EXTERNAL,attachmentStages,shaderStages|VK_PIPELINE_STAGE_TRANSFER_BIT,
        attachmentAccess,VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_TRANSFER_READ_BIT,VK_DEPENDENCY_BY_REGION_BIT};
    VkRenderPassCreateInfo create{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};create.attachmentCount=unsigned(attachments.size());create.pAttachments=attachments.data();create.subpassCount=1;create.pSubpasses=&sub;create.dependencyCount=2;create.pDependencies=dependencies;
    if(cache_) {
        const std::array<unsigned,8> key{{color?unsigned(color->Description().format):0,depth?unsigned(depth->Description().format):0,
            color?unsigned(color->Description().usage&VK_IMAGE_USAGE_SAMPLED_BIT):0,
            depth?unsigned(depth->Description().usage&VK_IMAGE_USAGE_SAMPLED_BIT):0,
            unsigned(loadExisting),unsigned(loadDepth),unsigned(preserveDepth),depth?unsigned(depth->Description().aspects):0}};
        sharedPass_=cache_->Acquire(device_,key,create,error);if(!sharedPass_)return false;pass_=sharedPass_->handle;
    } else if(!Check(vkCreateRenderPass(device_,&create,nullptr,&pass_),"attachment render pass",error))return false;
    VkFramebufferCreateInfo framebuffer{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};framebuffer.renderPass=pass_;framebuffer.attachmentCount=unsigned(views.size());framebuffer.pAttachments=views.data();framebuffer.width=extent_.width;framebuffer.height=extent_.height;framebuffer.layers=1;
    if(cache_) {
        sharedFramebuffer_=cache_->Framebuffer(pass_,color,depth,framebuffer,error);if(!sharedFramebuffer_)return false;
        framebuffer_=sharedFramebuffer_->handle;
    } else if(!Check(vkCreateFramebuffer(device_,&framebuffer,nullptr,&framebuffer_),"attachment framebuffer",error))return false;
    loadExisting_=loadExisting;loadDepth_=loadDepth;preserveDepth_=preserveDepth;
    color_=std::move(color);depth_=std::move(depth);return true;
}
bool AttachmentPass::BeginTracked(VkCommandBuffer command,const VkClearColorValue& color,std::string& error,float depth,unsigned stencil)const {
    if(!pass_||!image_usage::Begin(command,color_,depth_,loadExisting_,loadDepth_,error))return false;
    Begin(command,color,depth,stencil);return true;
}
bool AttachmentPass::EndTracked(VkCommandBuffer command,std::string& error)const {
    if(!image_usage::End(command,color_,depth_,preserveDepth_,error))return false;
    End(command);return true;
}
void AttachmentPass::Begin(VkCommandBuffer command,const VkClearColorValue& color,float depth,unsigned stencil)const {
    VkClearValue values[2]{};unsigned count=0;if(color_)values[count++].color=color;if(depth_)values[count++].depthStencil={depth,stencil};
    VkRenderPassBeginInfo begin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};begin.renderPass=pass_;begin.framebuffer=framebuffer_;begin.renderArea={{0,0},extent_};begin.clearValueCount=count;begin.pClearValues=values;
    vkCmdBeginRenderPass(command,&begin,VK_SUBPASS_CONTENTS_INLINE);
}
}
