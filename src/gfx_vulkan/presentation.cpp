#include "presentation.h"
#if defined(__ANDROID__)
#include <platform/android/vulkan_window.h>
#include <vulkan/vulkan_android.h>
#endif
#include <SDL.h>
#include <SDL_vulkan.h>
#include <algorithm>
#include <cstring>

namespace bo1::vulkan {
VkExtent2D Presentation::DisplayExtent()const {
    int width=0,height=0;SDL_Vulkan_GetDrawableSize(window_,&width,&height);
    return {unsigned(std::max(0,width)),unsigned(std::max(0,height))};
}
void Presentation::PreRotateProjection(std::array<float,16>& matrix)const {
    auto x=matrix,y=matrix;
    const bool mirror=transform_==VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_BIT_KHR
        ||transform_==VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90_BIT_KHR
        ||transform_==VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180_BIT_KHR
        ||transform_==VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270_BIT_KHR;
    if(mirror)for(unsigned i=0;i<4;++i)x[i]=-x[i];
    for(unsigned i=0;i<4;++i) {
        if(transform_==VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR||transform_==VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90_BIT_KHR) {matrix[i]=y[i+4];matrix[i+4]=-x[i];}
        else if(transform_==VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR||transform_==VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180_BIT_KHR) {matrix[i]=-x[i];matrix[i+4]=-y[i+4];}
        else if(transform_==VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR||transform_==VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270_BIT_KHR) {matrix[i]=-y[i+4];matrix[i+4]=x[i];}
        else {matrix[i]=x[i];matrix[i+4]=y[i+4];}
    }
}
namespace {
bool Ok(VkResult v,const char* operation,std::string& error) {
    if(v==VK_SUCCESS)return true;error=std::string(operation)+": VkResult="+std::to_string(int(v));return false;
}
}
Presentation::~Presentation() {
    recording_.reset();
    if(device_)vkDeviceWaitIdle(device_);
    ReleaseSwapchain();
    if(device_) {
        for(auto& slot:slots_) {if(slot.acquired)vkDestroySemaphore(device_,slot.acquired,nullptr);if(slot.fence)vkDestroyFence(device_,slot.fence,nullptr);if(slot.pool)vkDestroyCommandPool(device_,slot.pool,nullptr);}
        vkDestroyDevice(device_,nullptr);
    }
    ReleaseSurface();if(instance_)vkDestroyInstance(instance_,nullptr);
}
void Presentation::ReleaseSurface() {
    if(surface_)vkDestroySurfaceKHR(instance_,surface_,nullptr);surface_={};
#if defined(__ANDROID__)
    if(nativeWindow_)ANativeWindow_release(nativeWindow_);nativeWindow_=nullptr;
#endif
}
bool Presentation::CreateSurface(std::string& error) {
#if defined(__ANDROID__)
    // Keep the acquired native window alive for the entire Vulkan surface lifetime.
    auto* window=bo1::android::AcquireVulkanWindow(error);
    if(!window)return false;
    auto create=reinterpret_cast<PFN_vkCreateAndroidSurfaceKHR>(vkGetInstanceProcAddr(instance_,"vkCreateAndroidSurfaceKHR"));
    if(!create) {ANativeWindow_release(window);error="Android Vulkan surface extension unavailable";return false;}
    VkAndroidSurfaceCreateInfoKHR info{VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR};info.window=window;
    const VkResult result=create(instance_,&info,nullptr,&surface_);
    if(result!=VK_SUCCESS) {
        ANativeWindow_release(window);
        if(result!=VK_ERROR_SURFACE_LOST_KHR)Ok(result,"Android surface",error);
        return false;
    }
    nativeWindow_=window;
    return true;
#else
    if(!SDL_Vulkan_CreateSurface(window_,instance_,&surface_)) {error=SDL_GetError();return false;}
    return true;
#endif
}
void Presentation::ReleaseSwapchain() {
    for(auto frame:frames_)if(frame)vkDestroyFramebuffer(device_,frame,nullptr);frames_.clear();
    for(auto ready:ready_)if(ready)vkDestroySemaphore(device_,ready,nullptr);ready_.clear();
    for(auto view:views_)if(view)vkDestroyImageView(device_,view,nullptr);views_.clear();
    if(renderPass_)vkDestroyRenderPass(device_,renderPass_,nullptr);renderPass_={};
    if(swapchain_)vkDestroySwapchainKHR(device_,swapchain_,nullptr);swapchain_={};extent_={};
}
bool Presentation::Open(SDL_Window* window,bool preferMailbox,std::string& error) {
    error.clear();if(!window||instance_) {error="Invalid SDL window or presentation already opened";return false;}
    ownerThread_=SDL_ThreadID();window_=window;mailbox_=preferMailbox;
    unsigned count{};if(!SDL_Vulkan_GetInstanceExtensions(window,&count,nullptr)) {error=SDL_GetError();return false;}
    std::vector<const char*> extensions(count);
    if(!SDL_Vulkan_GetInstanceExtensions(window,&count,extensions.data())) {error=SDL_GetError();return false;}
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};app.pApplicationName="BlackOpsARM native Vulkan";app.apiVersion=VK_API_VERSION_1_1;
    VkInstanceCreateInfo instance{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};instance.pApplicationInfo=&app;
    instance.enabledExtensionCount=count;instance.ppEnabledExtensionNames=extensions.data();
    if(!Ok(vkCreateInstance(&instance,nullptr,&instance_),"instance",error))return false;
    if(!CreateSurface(error)) {if(error.empty())error="Initial Android surface is not ready";return false;}
    if(!Ok(vkEnumeratePhysicalDevices(instance_,&count,nullptr),"physical devices",error))return false;
    std::vector<VkPhysicalDevice> devices(count);
    if(!Ok(vkEnumeratePhysicalDevices(instance_,&count,devices.data()),"physical devices",error))return false;
    for(auto p:devices) {
        VkPhysicalDeviceProperties properties{};vkGetPhysicalDeviceProperties(p,&properties);if(properties.apiVersion<VK_API_VERSION_1_1)continue;
        unsigned n{};vkEnumerateDeviceExtensionProperties(p,nullptr,&n,nullptr);std::vector<VkExtensionProperties> available(n);
        if(vkEnumerateDeviceExtensionProperties(p,nullptr,&n,available.data())!=VK_SUCCESS)continue;
        bool swap=false;for(const auto& ext:available)swap=swap||!std::strcmp(ext.extensionName,VK_KHR_SWAPCHAIN_EXTENSION_NAME);if(!swap)continue;
        vkGetPhysicalDeviceQueueFamilyProperties(p,&n,nullptr);std::vector<VkQueueFamilyProperties> queues(n);vkGetPhysicalDeviceQueueFamilyProperties(p,&n,queues.data());
        for(unsigned i=0;i<n;++i) {
            VkBool32 present{};
            if(queues[i].queueCount&&(queues[i].queueFlags&VK_QUEUE_GRAPHICS_BIT)
                &&vkGetPhysicalDeviceSurfaceSupportKHR(p,i,surface_,&present)==VK_SUCCESS&&present) {physical_=p;family_=i;break;}
        }
        if(physical_)break;
    }
    if(!physical_) {error="No Vulkan1.1 graphics/present queue with swapchain support";return false;}
    float priority=1;VkDeviceQueueCreateInfo queue{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};queue.queueFamilyIndex=family_;queue.queueCount=1;queue.pQueuePriorities=&priority;
    const char* deviceExtension=VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    VkDeviceCreateInfo device{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};device.queueCreateInfoCount=1;device.pQueueCreateInfos=&queue;
    VkPhysicalDeviceFeatures available{},enabled{};vkGetPhysicalDeviceFeatures(physical_,&available);
    enabled.occlusionQueryPrecise=available.occlusionQueryPrecise;device.pEnabledFeatures=&enabled;
    preciseOcclusion_=enabled.occlusionQueryPrecise==VK_TRUE;
    // Original DXT assets sample directly when the device supports BC formats.
    enabled.textureCompressionBC=available.textureCompressionBC;blockCompression_=enabled.textureCompressionBC==VK_TRUE;
    device.enabledExtensionCount=1;device.ppEnabledExtensionNames=&deviceExtension;
    if(!Ok(vkCreateDevice(physical_,&device,nullptr,&device_),"device",error))return false;
    vkGetDeviceQueue(device_,family_,0,&queue_);
    VkCommandPoolCreateInfo pool{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};pool.queueFamilyIndex=family_;pool.flags=VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    for(auto& slot:slots_) {
        if(!Ok(vkCreateCommandPool(device_,&pool,nullptr,&slot.pool),"frame pool",error))return false;
        VkCommandBufferAllocateInfo allocation{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};allocation.commandPool=slot.pool;allocation.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;allocation.commandBufferCount=1;
        VkSemaphoreCreateInfo semaphore{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};VkFenceCreateInfo fence{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};fence.flags=VK_FENCE_CREATE_SIGNALED_BIT;
        if(!Ok(vkAllocateCommandBuffers(device_,&allocation,&slot.command),"frame command",error)
            ||!Ok(vkCreateSemaphore(device_,&semaphore,nullptr,&slot.acquired),"acquire semaphore",error)
            ||!Ok(vkCreateFence(device_,&fence,nullptr,&slot.fence),"frame fence",error))return false;
    }
    return Rebuild(error);
}
bool Presentation::Rebuild(std::string& error) {
    if(surfaceLost_) {
        if(!Ok(vkDeviceWaitIdle(device_),"surface retirement",error))return false;
        ReleaseSwapchain();
        ReleaseSurface();
        if(!CreateSurface(error)) {
            // SurfaceHolder may be between destroyed and created callbacks.
            // Keep recovery pending and defer presentation; never send null to Vulkan.
            resize_=true;
            return error.empty();
        }
        VkBool32 supported{};
        if(!Ok(vkGetPhysicalDeviceSurfaceSupportKHR(physical_,family_,surface_,&supported),"replacement surface support",error))return false;
        if(!supported) {error="Replacement surface requires another present queue";return false;}
        surfaceLost_=false;
    }
    VkSurfaceCapabilitiesKHR caps{};
    const VkResult capabilities=vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_,surface_,&caps);
    if(capabilities==VK_ERROR_SURFACE_LOST_KHR) {surfaceLost_=resize_=true;return true;}
    if(!Ok(capabilities,"surface capabilities",error))return false;
    VkExtent2D size=caps.currentExtent;
    if(size.width==~0u) {
        int width{},height{};SDL_Vulkan_GetDrawableSize(window_,&width,&height);
        if(width<=0||height<=0) {resize_=true;return true;}
        size={std::clamp(unsigned(width),caps.minImageExtent.width,caps.maxImageExtent.width),
              std::clamp(unsigned(height),caps.minImageExtent.height,caps.maxImageExtent.height)};
    }
    if(!size.width||!size.height) {resize_=true;return true;}
    if(!(caps.supportedUsageFlags&VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)) {error="Surface cannot render color attachments";return false;}
    unsigned n{};
    if(!Ok(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_,surface_,&n,nullptr),"surface formats",error)||!n)return false;
    std::vector<VkSurfaceFormatKHR> formats(n);if(!Ok(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_,surface_,&n,formats.data()),"surface formats",error))return false;
    auto chosen=formats[0];
    for(auto format:formats)if(format.format==VK_FORMAT_B8G8R8A8_UNORM&&format.colorSpace==VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {chosen=format;break;}
    if(chosen.format==VK_FORMAT_UNDEFINED)chosen.format=VK_FORMAT_B8G8R8A8_UNORM;
    if(!Ok(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_,surface_,&n,nullptr),"present modes",error))return false;
    std::vector<VkPresentModeKHR> modes(n);if(!Ok(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_,surface_,&n,modes.data()),"present modes",error))return false;
    VkPresentModeKHR mode=VK_PRESENT_MODE_FIFO_KHR;
    if(mailbox_&&std::find(modes.begin(),modes.end(),VK_PRESENT_MODE_MAILBOX_KHR)!=modes.end())mode=VK_PRESENT_MODE_MAILBOX_KHR;
    VkCompositeAlphaFlagBitsKHR alpha=VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    for(auto candidate:{VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR})
        if(caps.supportedCompositeAlpha&candidate) {alpha=candidate;break;}
    // Recreation/teardown is a rare lifecycle drain; steady frames use fences.
    if(!Ok(vkDeviceWaitIdle(device_),"resize drain",error))return false;
    ReleaseSwapchain();
    unsigned images=caps.minImageCount+1;if(caps.maxImageCount)images=std::min(images,caps.maxImageCount);
    VkSwapchainCreateInfoKHR swap{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};swap.surface=surface_;swap.minImageCount=images;
    swap.imageFormat=chosen.format;swap.imageColorSpace=chosen.colorSpace;swap.imageExtent=size;swap.imageArrayLayers=1;
    swap.imageUsage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;swap.imageSharingMode=VK_SHARING_MODE_EXCLUSIVE;
    swap.preTransform=caps.currentTransform;swap.compositeAlpha=alpha;swap.presentMode=mode;swap.clipped=VK_TRUE;
    if(!Ok(vkCreateSwapchainKHR(device_,&swap,nullptr,&swapchain_),"swapchain",error))return false;
    VkAttachmentDescription attachment{};attachment.format=chosen.format;attachment.samples=VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;attachment.storeOp=VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;attachment.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;attachment.finalLayout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    VkAttachmentReference color{0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};VkSubpassDescription subpass{};subpass.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;subpass.colorAttachmentCount=1;subpass.pColorAttachments=&color;
    VkSubpassDependency dependency{VK_SUBPASS_EXTERNAL,0,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,0,VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,0};
    VkRenderPassCreateInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};pass.attachmentCount=1;pass.pAttachments=&attachment;pass.subpassCount=1;pass.pSubpasses=&subpass;pass.dependencyCount=1;pass.pDependencies=&dependency;
    if(!Ok(vkCreateRenderPass(device_,&pass,nullptr,&renderPass_),"swapchain renderpass",error))return false;
    if(!Ok(vkGetSwapchainImagesKHR(device_,swapchain_,&n,nullptr),"swapchain images",error))return false;
    std::vector<VkImage> targets(n);if(!Ok(vkGetSwapchainImagesKHR(device_,swapchain_,&n,targets.data()),"swapchain images",error))return false;
    views_.resize(n);frames_.resize(n);ready_.resize(n);
    for(unsigned i=0;i<n;++i) {
        VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};view.image=targets[i];view.viewType=VK_IMAGE_VIEW_TYPE_2D;view.format=chosen.format;view.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
        VkFramebufferCreateInfo frame{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};frame.renderPass=renderPass_;frame.attachmentCount=1;frame.pAttachments=&views_[i];frame.width=size.width;frame.height=size.height;frame.layers=1;
        VkSemaphoreCreateInfo semaphore{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        if(!Ok(vkCreateImageView(device_,&view,nullptr,&views_[i]),"swapchain view",error)
            ||!Ok(vkCreateFramebuffer(device_,&frame,nullptr,&frames_[i]),"swapchain framebuffer",error)
            ||!Ok(vkCreateSemaphore(device_,&semaphore,nullptr,&ready_[i]),"present semaphore",error))return false;
    }
    extent_=size;transform_=caps.currentTransform;resize_=false;++generation_;return true;
}
bool Presentation::Owner(std::string& error)const {
    if(ownerThread_!=SDL_ThreadID()) {error="Native frame transaction requires its SDL/queue owner thread";return false;}
    if(submitting_) {error="Native frame submission callback cannot reenter presentation";return false;}
    if(failed_) {error="Native frame transaction failed; device teardown is required";return false;}
    return true;
}
FrameResult Presentation::BeginFrame(FrameContext& context,std::string& error) {return Begin(true,context,error);}
FrameResult Presentation::BeginWork(FrameContext& context,std::string& error) {return Begin(false,context,error);}
bool Presentation::WaitFrame(unsigned slot,std::string& error) {
    error.clear();if(!Owner(error))return false;
    if(recording_||slot>=slots_.size()) {error="Native wait needs a submitted frame slot outside recording";return false;}
    return Ok(vkWaitForFences(device_,1,&slots_[slot].fence,VK_TRUE,1000000000ull),"synchronous frame completion",error);
}
FrameResult Presentation::Begin(bool present,FrameContext& context,std::string& error) {
    error.clear();context={};
    if(!Owner(error))return FrameResult::Failed;
    if(recording_) {error="Native frame transaction is already recording";return FrameResult::Failed;}
    if(!device_) {error="Presentation device unavailable";return FrameResult::Failed;}
    if(present&&(resize_||!swapchain_)) {if(!Rebuild(error))return FrameResult::Failed;if(resize_||!swapchain_)return FrameResult::Deferred;}
    auto& slot=slots_[next_];
    VkResult result=vkWaitForFences(device_,1,&slot.fence,VK_TRUE,1000000000ull);
    if(result==VK_TIMEOUT)return FrameResult::Deferred;if(!Ok(result,"frame retirement",error))return FrameResult::Failed;
    unsigned image=~0u;if(present) {result=vkAcquireNextImageKHR(device_,swapchain_,1000000000ull,slot.acquired,VK_NULL_HANDLE,&image);
    if(result==VK_ERROR_SURFACE_LOST_KHR) {surfaceLost_=resize_=true;return FrameResult::Deferred;}
    if(result==VK_ERROR_OUT_OF_DATE_KHR) {resize_=true;return FrameResult::Deferred;}
    if(result==VK_TIMEOUT||result==VK_NOT_READY)return FrameResult::Deferred;
    if(result!=VK_SUCCESS&&result!=VK_SUBOPTIMAL_KHR) {Ok(result,"acquire",error);return FrameResult::Failed;}
    }
    // After acquiring, a recording/submission error is terminal: never reuse an
    // acquire semaphore whose signal has not been consumed by a queue wait.
    if(!Ok(vkResetCommandPool(device_,slot.pool,0),"frame reset",error)) {failed_=true;return FrameResult::Failed;}
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};begin.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if(!Ok(vkBeginCommandBuffer(slot.command,&begin),"frame begin",error)) {failed_=true;return FrameResult::Failed;}
    current_={slot.command,extent_,next_,image,generation_};
    recording_=std::make_unique<image_usage::RecordingScope>(imageStates_,slot.command);
    windowPass_=windowRecorded_=false;context=current_;return FrameResult::Recording;
}
bool Presentation::BeginWindowPass(const VkClearColorValue& clear,std::string& error) {
    error.clear();if(!Owner(error))return false;
    if(!recording_||current_.image==~0u||windowPass_||windowRecorded_) {error="Native window pass requires one open frame and may be recorded once";return false;}
    // Do not nest the window pass inside an unfinished generated-image pass.
    if(!recording_->Validate(error))return false;
    VkClearValue value{};value.color=clear;
    VkRenderPassBeginInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};pass.renderPass=renderPass_;pass.framebuffer=frames_[current_.image];pass.renderArea.extent=extent_;pass.clearValueCount=1;pass.pClearValues=&value;
    vkCmdBeginRenderPass(current_.command,&pass,VK_SUBPASS_CONTENTS_INLINE);
    windowPass_=windowRecorded_=true;return true;
}
bool Presentation::EndWindowPass(std::string& error) {
    error.clear();if(!Owner(error))return false;
    if(!recording_||!windowPass_) {error="Native window pass is not active";return false;}
    vkCmdEndRenderPass(current_.command);windowPass_=false;return true;
}
FrameResult Presentation::SubmitFrame(std::string& error,SubmittedFrame submitted,void* user) {
    error.clear();if(!Owner(error))return FrameResult::Failed;
    if(!recording_||windowPass_||(current_.image!=~0u&&!windowRecorded_)) {error="Native submit requires a recorded and ended window pass";return FrameResult::Failed;}
    if(!recording_->Validate(error))return FrameResult::Failed;
    auto& slot=slots_[current_.slot];const unsigned image=current_.image;
    if(!Ok(vkEndCommandBuffer(slot.command),"frame end",error)||!Ok(vkResetFences(device_,1,&slot.fence),"frame fence reset",error)) {failed_=true;return FrameResult::Failed;}
    const VkPipelineStageFlags stage=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.waitSemaphoreCount=image==~0u?0:1;submit.pWaitSemaphores=&slot.acquired;submit.pWaitDstStageMask=&stage;
    submit.commandBufferCount=1;submit.pCommandBuffers=&slot.command;submit.signalSemaphoreCount=image==~0u?0:1;submit.pSignalSemaphores=image==~0u?nullptr:&ready_[image];
    if(!Ok(vkQueueSubmit(queue_,1,&submit,slot.fence),"frame submit",error)) {failed_=true;return FrameResult::Failed;}
    recording_->Commit(); // Publish only successfully submitted, ordered work.
    recording_.reset();
    // Callbacks may release recording scopes and publish actual submission
    // fences, but may not start another frame on this queue during this call.
    const unsigned submittedSlot=current_.slot;
    submitting_=true;
    if(submitted)submitted(user,submittedSlot,slot.fence);
    submitting_=false;
    if(image==~0u) {next_=(next_+1)%slots_.size();return FrameResult::Presented;}
    VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};present.waitSemaphoreCount=1;present.pWaitSemaphores=&ready_[image];present.swapchainCount=1;present.pSwapchains=&swapchain_;present.pImageIndices=&image;
    VkResult result=vkQueuePresentKHR(queue_,&present);next_=(next_+1)%slots_.size();
    if(result==VK_ERROR_SURFACE_LOST_KHR) {surfaceLost_=resize_=true;return FrameResult::Deferred;}
    if(result==VK_ERROR_OUT_OF_DATE_KHR) {resize_=true;return FrameResult::Deferred;}
    if(result!=VK_SUCCESS&&result!=VK_SUBOPTIMAL_KHR) {Ok(result,"present",error);failed_=true;return FrameResult::Failed;}
    return FrameResult::Presented;
}
FrameResult Presentation::Render(const VkClearColorValue& clear,DrawFrame draw,void* user,std::string& error,DrawFrame prepare,SubmittedFrame submitted) {
    FrameContext context{};const auto begun=BeginFrame(context,error);
    if(begun!=FrameResult::Recording)return begun;
    if(prepare)prepare(user,context);
    if(!BeginWindowPass(clear,error))return FrameResult::Failed;
    if(draw)draw(user,context);
    if(!EndWindowPass(error))return FrameResult::Failed;
    return SubmitFrame(error,submitted,user);
}
}
