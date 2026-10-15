#include "native_vulkan_entry.h"
#include "native_vulkan_device.h"
#include "native_vulkan_display.h"
#include "gfx_d3d/r_init.h"
#include "gfx_d3d/r_state.h"
#include "gfx_vulkan/engine_frames.h"
#include "gfx_vulkan/target_bindings.h"
#include "gfx_vulkan/image_assets.h"
#include "gfx_vulkan/image_uploads.h"
#include <cstdlib>
#include "performance_overlay.h"
#include "gfx_vulkan/presentation.h"
#include "native_vulkan_material_preview.h"
#include <SDL.h>
#include <SDL_vulkan.h>
#include <android/log.h>
#include <cstdio>
namespace bo1::android {
int RunNativeVulkanPresentation(const char* userRoot) {
    __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan","PRESENTATION_ONLY start Com_Init_called=0");
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS)!=0) {
        __android_log_print(ANDROID_LOG_ERROR,"BO1NativeVulkan","SDL init failed: %s",SDL_GetError());return 1;
    }
    native_display::Snapshot display;bo1::vulkan::HardwareInfo physicalProbe;std::string probeError;
    if(!native_display::Read(0,display,probeError)||!native_display::ProbeHardware(physicalProbe,probeError)) {
        __android_log_print(ANDROID_LOG_ERROR,"BO1NativeVulkan","native_pre_window_probe_failed=%s",probeError.c_str());SDL_Quit();return 1;
    }
    __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan","native_pre_window_probe=1 device_created=0 gpu=%s modes=%zu current=%ix%i refresh=%i SDLformat=%s",physicalProbe.properties.deviceName,display.modes.size(),display.current.w,display.current.h,display.current.refresh_rate,SDL_GetPixelFormatName(display.current.format));
    SDL_Window* window=SDL_CreateWindow("BlackOpsARM native Vulkan",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,
        1920,1200,SDL_WINDOW_VULKAN|SDL_WINDOW_SHOWN|SDL_WINDOW_RESIZABLE|SDL_WINDOW_FULLSCREEN_DESKTOP);
    if(!window) {__android_log_print(ANDROID_LOG_ERROR,"BO1NativeVulkan","window failed: %s",SDL_GetError());SDL_Quit();return 1;}
    int status=0;
    {
        std::string error;GfxWindowParms parms{};parms.hwnd=reinterpret_cast<HWND__*>(window);parms.aaSamples=1;
        SDL_Vulkan_GetDrawableSize(window,&parms.displayWidth,&parms.displayHeight);parms.sceneWidth=parms.displayWidth;parms.sceneHeight=parms.displayHeight;
        if(!native_device::Configure(true,error)||!R_CreateDevice(&parms)) {__android_log_print(ANDROID_LOG_ERROR,"BO1NativeVulkan","native engine device creation failed: %s",error.c_str());status=1;}
        else {
            R_FinishAttachingToWindow(&parms);
            if(!native_device::WindowAttached()||dx.windowCount!=1||dx.windows[0].swapChain) {
                __android_log_print(ANDROID_LOG_ERROR,"BO1NativeVulkan","canonical native window did not attach");std::abort();
            }
            auto& presentation=*native_device::Renderer();
            // Exercise engine-owned startup work before acquiring any frame
            // image. Real attachment clear/readback and precise query result.
            {
                using namespace bo1::vulkan;
                GfxImage image{};image.mapType=3;image.width=image.height=4;image.depth=1;
                GfxRenderTarget target{};target.width=target.height=4;
                GfxCmdBufState state{};state.depthRangeFar=1;
                IDirect3DQuery9* query=nullptr;
                auto frames=native_device::Frames(error);
                auto require=[&](bool ok){if(!ok){__android_log_print(ANDROID_LOG_ERROR,"BO1NativeVulkan","native_engine_startup_work_failed=%s",error.c_str());std::abort();}};
                require(frames&&image_assets::Create(&image,1,0x20000,D3DFMT_A8B8G8R8,error));
                require(render_targets::Assign(&target,&image,false,error));
                require(render_targets::AttachDepth(&target,4,4,error));
                require(target_bindings::Select(&state,&target,error));
                RB_HW_AllocOcclusionQueryFromOwner(&query);
                Buffer readback(presentation.Physical(),presentation.Device());
                require(readback.Create(4,VK_BUFFER_USAGE_TRANSFER_DST_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,error));
                auto copyToReadback=[&](VkCommandBuffer command) {
                auto texture=image_assets::Acquire(&image)->texture;
                VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};barrier.image=texture->Handle();
                barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
                barrier.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
                barrier.oldLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;barrier.newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT|VK_ACCESS_TRANSFER_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
                vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&barrier);
                VkBufferImageCopy copy{};copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};copy.imageExtent={1,1,1};
                vkCmdCopyImageToBuffer(command,texture->Handle(),VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,readback.Handle(),1,&copy);
                barrier.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;barrier.newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                barrier.srcAccessMask=VK_ACCESS_TRANSFER_READ_BIT;barrier.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
                vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,0,0,nullptr,0,nullptr,1,&barrier);
                };
                unsigned char pixels[64];for(unsigned i=0;i<64;i+=4) {pixels[i]=80;pixels[i+1]=160;pixels[i+2]=240;pixels[i+3]=255;}
                require(image_assets::QueueUpload(&image,D3DFMT_A8B8G8R8,0,0,pixels,error));
                auto uploaded=image_assets::Acquire(&image);require(!image_uploads::Ready(uploaded));
                require(image_uploads::Flush(error));require(image_uploads::Ready(uploaded));
                FrameContext copyFrame{};require(presentation.BeginWork(copyFrame,error)==FrameResult::Recording);
                copyToReadback(copyFrame.command);
                require(presentation.SubmitFrame(error)==FrameResult::Presented&&presentation.WaitFrame(copyFrame.slot,error));
                unsigned char uploadReadback[4]{};require(readback.Read(0,uploadReadback,4,error));
                require(uploadReadback[0]==80&&uploadReadback[1]==160&&uploadReadback[2]==240&&uploadReadback[3]==255);
                __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan","native_image_upload_readback=pass rgba=80,160,240,255 readiness_after_actual_fence=1");
                require(frames->Begin(&state,error)==FrameResult::Recording);
                require(frames->Context().image==~0u);
                const float color[4]={0.25f,0.5f,0.75f,1};
                R_ClearScreenInternal(nullptr,3,color,0.5f,0,nullptr);
                require(RB_HW_IssueOcclusionQueryFromOwner(&query,2)==S_OK);
                require(RB_HW_IssueOcclusionQueryFromOwner(&query,1)==S_OK);
                unsigned samples=777;
                require(RB_HW_ReadOcclusionQueryFromOwner(&query,&samples,0)==S_FALSE&&samples==777);
                require(pass_recording::Current()->Finish(error));
                copyToReadback(frames->Context().command);
                require(frames->Submit(true,error));
                require(RB_HW_ReadOcclusionQueryFromOwner(&query,&samples,0)==S_OK&&samples==0);
                unsigned char rgba[4]{};require(readback.Read(0,rgba,4,error));
                require(rgba[0]==64&&rgba[1]==128&&rgba[2]==191&&rgba[3]==255);
                RB_HW_ReleaseOcclusionQueryFromOwner(&query);
                target_bindings::Release(&state);render_targets::Release(&target);image_assets::Release(&image);
                require(frames->Drain(error));
                __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan","native_engine_startup_work=pass swapchain_acquire=0 gpu_rgba=64,128,191,255 real_empty_query_samples=0 unsubmitted_result_pending=1 synchronous_frame_fence=1");
            }
            NativeMaterialPreview material(presentation,userRoot);
            bool running=true,paused=false;unsigned frames=0;
            while(running) {
                SDL_Event event;
                while(SDL_PollEvent(&event)) {
                    if(event.type==SDL_QUIT||(event.type==SDL_KEYDOWN&&event.key.keysym.sym==SDLK_ESCAPE))running=false;
                    if(event.type==SDL_APP_WILLENTERBACKGROUND||event.type==SDL_APP_DIDENTERBACKGROUND)paused=true;
                    if(event.type==SDL_APP_DIDENTERFOREGROUND) {paused=false;presentation.Resize();}
                    if(event.type==SDL_WINDOWEVENT&&(event.window.event==SDL_WINDOWEVENT_SIZE_CHANGED||event.window.event==SDL_WINDOWEVENT_RESIZED))presentation.Resize();
                }
                if(!running)break;if(paused) {SDL_Delay(16);continue;}
                VkClearColorValue clear{{0.03f,0.17f,0.25f,1}};
                bo1::vulkan::FrameContext frame{};
                auto result=presentation.BeginFrame(frame,error);
                if(result==bo1::vulkan::FrameResult::Recording) {
                    if(frames==0) {
                        std::string rejected;bo1::vulkan::FrameContext duplicate{};
                        if(presentation.BeginFrame(duplicate,rejected)!=bo1::vulkan::FrameResult::Failed
                            ||presentation.SubmitFrame(rejected)!=bo1::vulkan::FrameResult::Failed
                            ||presentation.EndWindowPass(rejected)||!presentation.FrameRecording())std::abort();
                        __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan","native_frame_transaction=1 duplicate_begin_rejected=1 premature_submit_rejected=1 inactive_end_rejected=1");
                    }
                    NativeMaterialPreview::Prepare(&material,frame);
                    if(!material.Error().empty()) {status=1;break;}
                    if(!presentation.BeginWindowPass(clear,error))result=bo1::vulkan::FrameResult::Failed;
                    else {
                        if(frames==0) {
                            std::string rejected;
                            if(presentation.BeginWindowPass(clear,rejected)
                                ||presentation.SubmitFrame(rejected)!=bo1::vulkan::FrameResult::Failed)std::abort();
                            __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan","native_frame_active_pass_submit_rejected=1 nested_window_pass_rejected=1");
                        }
                        NativeMaterialPreview::Draw(&material,frame);
                        if(!material.Error().empty()) {status=1;break;}
                        if(!presentation.EndWindowPass(error))result=bo1::vulkan::FrameResult::Failed;
                        else result=presentation.SubmitFrame(error,NativeMaterialPreview::Submitted,&material);
                    }
                }
                if(!material.Error().empty()) {__android_log_print(ANDROID_LOG_ERROR,"BO1NativeVulkan","material failed: %s",material.Error().c_str());status=1;break;}
                if(result==bo1::vulkan::FrameResult::Failed) {__android_log_print(ANDROID_LOG_ERROR,"BO1NativeVulkan","frame failed: %s",error.c_str());status=1;break;}
                if(result==bo1::vulkan::FrameResult::Deferred) {SDL_Delay(16);continue;}
                performance::Present(true);++frames;
                if(frames==1||frames%120==0) __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan",
                    "PRESENTATION_ONLY frames=%u extent=%ux%u generation=%u clear_only=0 native_material=1 Com_Init_called=0",frames,
                    presentation.Extent().width,presentation.Extent().height,presentation.Generation());
                // Diagnostic pacing only; capture preview is not a game benchmark.
                SDL_Delay(16);
            }
            __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan","PRESENTATION_ONLY exit frames=%u status=%d",frames,status);
        }
    }
    std::string closeError;
    if(!native_device::Close(closeError)||!native_device::Configure(false,closeError)) {
        __android_log_print(ANDROID_LOG_ERROR,"BO1NativeVulkan","native_device_retirement_failed=%s",closeError.c_str());std::abort();
    }
    performance::Present(false);SDL_DestroyWindow(window);SDL_Quit();
    __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan","PRESENTATION_ONLY resources_released status=%d",status);return status;
}
}
