#pragma once
#include <vulkan/vulkan.h>
#include "image_usage.h"
#include <array>
#include <vector>
#include <string>
struct SDL_Window;
namespace bo1::vulkan {
enum class FrameResult { Presented, Deferred, Failed, Recording };
struct FrameContext { VkCommandBuffer command;VkExtent2D extent;unsigned slot,image,generation; };
using SubmittedFrame=void(*)(void*,unsigned,VkFence);
using DrawFrame=void(*)(void*,const FrameContext&);
// SDL supplies the platform surface, while this owner manages native Vulkan
// device, swapchain and fence-retired frame commands. One thread owns calls.
class Presentation {
public:
    // Two frames of upload, scene and window submissions, all fence retired.
    static constexpr unsigned FrameSlots=6;
    Presentation()=default;~Presentation();
    Presentation(const Presentation&)=delete;Presentation& operator=(const Presentation&)=delete;
    bool Open(SDL_Window*,bool preferMailbox,std::string& error);
    FrameResult Render(const VkClearColorValue&,DrawFrame,void*,std::string& error,DrawFrame prepare=nullptr,SubmittedFrame submitted=nullptr);
    // Explicit engine transaction. Recording remains on the SDL/queue owner.
    FrameResult BeginFrame(FrameContext&,std::string& error);
    FrameResult BeginWork(FrameContext&,std::string& error);
    bool WaitFrame(unsigned slot,std::string& error);
    bool BeginWindowPass(const VkClearColorValue&,std::string& error);
    bool EndWindowPass(std::string& error);
    FrameResult SubmitFrame(std::string& error,SubmittedFrame submitted=nullptr,void* user=nullptr);
    bool FrameRecording()const{return recording_!=nullptr;}
    void Resize(){resize_=true;}
    bool PreciseOcclusionEnabled()const{return preciseOcclusion_;}
    bool BlockCompressionEnabled()const{return blockCompression_;}
    bool SwapchainReady()const{return swapchain_&&renderPass_&&!frames_.empty()&&extent_.width&&extent_.height;}
    unsigned QueueFamily()const{return family_;}
    VkQueue Queue()const{return queue_;}
    VkDevice Device()const{return device_;}VkPhysicalDevice Physical()const{return physical_;}
    VkRenderPass RenderPass()const{return renderPass_;}unsigned Generation()const{return generation_;}
    VkExtent2D Extent()const{return extent_;}
    VkExtent2D DisplayExtent()const;
    VkSurfaceTransformFlagBitsKHR Transform()const{return transform_;}
    // Clip rows used by T5's dp4 projection. Pre-rotate rendering to match the
    // swapchain transform so the compositor presents the logical orientation.
    void PreRotateProjection(std::array<float,16>&)const;
private:
    bool Rebuild(std::string&);void ReleaseSwapchain();
    bool Owner(std::string&)const;
    FrameResult Begin(bool present,FrameContext&,std::string&);
    struct Slot { VkCommandBuffer command{};VkSemaphore acquired{};VkFence fence{}; };
    image_usage::Tracker imageStates_;
    std::unique_ptr<image_usage::RecordingScope> recording_;
    FrameContext current_{};
    unsigned long ownerThread_{};
    bool windowPass_{},windowRecorded_{},failed_{},submitting_{};
    SDL_Window* window_{};VkInstance instance_{};VkSurfaceKHR surface_{};VkPhysicalDevice physical_{};
    VkDevice device_{};VkQueue queue_{};unsigned family_{};VkCommandPool pool_{};
    VkSwapchainKHR swapchain_{};VkRenderPass renderPass_{};VkExtent2D extent_{};
    std::vector<VkImageView> views_;std::vector<VkFramebuffer> frames_;
    // A semaphore belongs to its swapchain image, not to a CPU frame slot.
    // Reacquiring that image proves its previous present wait has consumed it.
    std::vector<VkSemaphore> ready_;
    std::array<Slot,FrameSlots> slots_{};unsigned next_{},generation_{};
    bool preciseOcclusion_{},blockCompression_{};
    bool resize_{},mailbox_{},surfaceLost_{};
    VkSurfaceTransformFlagBitsKHR transform_=VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
};
}
