#include "render_pass_compatibility.h"
#include <array>
#include <cstdio>
#include <stdexcept>
#if defined(BO1_TEST_VULKAN_DRIVER)
#include "attachment_pass.h"
#endif
using namespace bo1::vulkan;
namespace {
unsigned checks;
void Check(bool value){++checks;if(!value)throw std::runtime_error("render pass compatibility invariant failed");}
struct Fixture {
    std::array<VkAttachmentDescription,2> attachments{};
    VkAttachmentReference color{0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkAttachmentReference depth{1,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    VkSubpassDescription subpass{};
    VkSubpassDependency dependency{};
    Fixture() {
        attachments[0].format=VK_FORMAT_R8G8B8A8_UNORM;
        attachments[1].format=VK_FORMAT_D24_UNORM_S8_UINT;
        for(auto& a:attachments){a.samples=VK_SAMPLE_COUNT_1_BIT;a.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;a.storeOp=VK_ATTACHMENT_STORE_OP_STORE;}
        subpass.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;
        dependency.srcSubpass=VK_SUBPASS_EXTERNAL;dependency.dstSubpass=0;
        dependency.srcStageMask=dependency.dstStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    }
    VkRenderPassCreateInfo Info() {
        subpass.colorAttachmentCount=1;subpass.pColorAttachments=&color;subpass.pDepthStencilAttachment=&depth;
        VkRenderPassCreateInfo i{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
        i.attachmentCount=2;i.pAttachments=attachments.data();i.subpassCount=1;i.pSubpasses=&subpass;
        i.dependencyCount=1;i.pDependencies=&dependency;return i;
    }
};
#if defined(BO1_TEST_VULKAN_DRIVER)
void DriverCache() {
    struct Context {
        VkInstance instance{};VkDevice device{};
        ~Context(){if(device)vkDestroyDevice(device,nullptr);if(instance)vkDestroyInstance(instance,nullptr);}
    } context;
    VkApplicationInfo application{VK_STRUCTURE_TYPE_APPLICATION_INFO};application.apiVersion=VK_API_VERSION_1_1;
    VkInstanceCreateInfo instance{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};instance.pApplicationInfo=&application;
    Check(vkCreateInstance(&instance,nullptr,&context.instance)==VK_SUCCESS);
    unsigned count=0;Check(vkEnumeratePhysicalDevices(context.instance,&count,nullptr)==VK_SUCCESS&&count);
    std::vector<VkPhysicalDevice> physical(count);Check(vkEnumeratePhysicalDevices(context.instance,&count,physical.data())==VK_SUCCESS);
    unsigned queueCount=0;vkGetPhysicalDeviceQueueFamilyProperties(physical[0],&queueCount,nullptr);
    std::vector<VkQueueFamilyProperties> queues(queueCount);vkGetPhysicalDeviceQueueFamilyProperties(physical[0],&queueCount,queues.data());
    unsigned family=0;while(family<queueCount&&!(queues[family].queueFlags&VK_QUEUE_GRAPHICS_BIT))++family;Check(family<queueCount);
    float priority=1;VkDeviceQueueCreateInfo queue{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};queue.queueFamilyIndex=family;queue.queueCount=1;queue.pQueuePriorities=&priority;
    VkDeviceCreateInfo device{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};device.queueCreateInfoCount=1;device.pQueueCreateInfos=&queue;
    Check(vkCreateDevice(physical[0],&device,nullptr,&context.device)==VK_SUCCESS);
    AttachmentPassCache cache(context.device);std::string error;
    Fixture fixture;auto info=fixture.Info();
    fixture.attachments[0].finalLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    fixture.attachments[1].finalLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    std::array<unsigned,8> exact{};
    auto clear=cache.Acquire(context.device,exact,info,error);Check(bool(clear));
    std::weak_ptr<SharedAttachmentRenderPass> weakClear=clear;
    for(auto& a:fixture.attachments){a.loadOp=VK_ATTACHMENT_LOAD_OP_LOAD;a.initialLayout=a.finalLayout;}
    exact[4]=1;
    auto load=cache.Acquire(context.device,exact,info,error);
    Check(load&&load->handle!=clear->handle&&load->PipelineHandle()==clear->handle);
    Check(cache.Acquire(context.device,exact,info,error)==load);
    fixture.attachments[0].format=VK_FORMAT_B8G8R8A8_UNORM;exact[0]=1;
    auto incompatible=cache.Acquire(context.device,exact,info,error);
    Check(incompatible&&incompatible->PipelineHandle()==incompatible->handle&&incompatible->PipelineHandle()!=clear->handle);
    cache.Clear();clear.reset();
    Check(!weakClear.expired()); // compatible pass owns the canonical handle
    load.reset();Check(weakClear.expired()); // no shared-owner cycle
}
#endif
}
int main(){try {
    Fixture base;auto info=base.Info();std::vector<std::uint32_t> expected,key;
    Check(RenderPassCompatibility(info,expected));
    Fixture other;auto changed=other.Info();
    // The exact differences used by clear/resume passes do share a pipeline.
    for(auto& a:other.attachments) {
        a.loadOp=a.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_LOAD;
        a.storeOp=a.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
        a.initialLayout=VK_IMAGE_LAYOUT_GENERAL;a.finalLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    other.color.layout=other.depth.layout=VK_IMAGE_LAYOUT_GENERAL;
    Check(RenderPassCompatibility(changed,key)&&key==expected);
    const auto different=[&](auto mutate){Fixture f;auto i=f.Info();mutate(f,i);Check(RenderPassCompatibility(i,key)&&key!=expected);};
    different([](auto& f,auto&){f.attachments[0].format=VK_FORMAT_B8G8R8A8_UNORM;});
    different([](auto& f,auto&){f.attachments[1].format=VK_FORMAT_D16_UNORM;});
    different([](auto& f,auto&){f.attachments[0].samples=VK_SAMPLE_COUNT_4_BIT;});
    different([](auto& f,auto&){f.attachments[0].flags=VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT;});
    different([](auto& f,auto&){f.color.attachment=VK_ATTACHMENT_UNUSED;});
    different([](auto& f,auto&){f.subpass.pDepthStencilAttachment=nullptr;});
    different([](auto& f,auto&){f.subpass.inputAttachmentCount=1;f.subpass.pInputAttachments=&f.depth;});
    different([](auto& f,auto&){f.subpass.pResolveAttachments=&f.color;});
    different([](auto& f,auto&){f.subpass.preserveAttachmentCount=1;f.subpass.pPreserveAttachments=&f.depth.attachment;});
    different([](auto& f,auto&){f.dependency.dependencyFlags=VK_DEPENDENCY_BY_REGION_BIT;});
    different([](auto& f,auto&){f.dependency.dstAccessMask=VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;});
    different([](auto& f,auto&){f.dependency.srcStageMask=VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;});
    different([](auto&,auto& i){i.dependencyCount=0;});
    std::array<VkSubpassDescription,2> subpasses{{base.subpass,base.subpass}};
    info.subpassCount=2;info.pSubpasses=subpasses.data();
    Check(RenderPassCompatibility(info,key)&&key!=expected);
    info=base.Info();VkRenderPassMultiviewCreateInfo multiview{VK_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_CREATE_INFO};info.pNext=&multiview;
    Check(!RenderPassCompatibility(info,key)&&key.empty());
    info=base.Info();info.pAttachments=nullptr;Check(!RenderPassCompatibility(info,key)&&key.empty());
    info=base.Info();base.color.attachment=2;Check(!RenderPassCompatibility(info,key)&&key.empty());
#if defined(BO1_TEST_VULKAN_DRIVER)
    DriverCache();
#endif
    std::printf("render-pass compatibility PASS checks=%u\n",checks);return 0;
}catch(const std::exception& e){std::fprintf(stderr,"%s after %u checks\n",e.what(),checks);return 1;}}
