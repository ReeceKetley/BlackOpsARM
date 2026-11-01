#include "native_vulkan_device.h"
#include "gfx_vulkan/engine_frames.h"
#include "gfx_vulkan/image_assets.h"
#include "gfx_vulkan/image_uploads.h"
#include "gfx_vulkan/query_assets.h"
#include "gfx_vulkan/event_fences.h"
#include "gfx_vulkan/buffer_assets.h"
#include "gfx_vulkan/shader_assets.h"
#include "gfx_vulkan/geometry_bindings.h"
#include "gfx_vulkan/texture_bindings.h"
#include "gfx_vulkan/material_bindings.h"
#include "gfx_vulkan/constant_state.h"
#include "gfx_vulkan/target_bindings.h"
#include <SDL.h>
#include <SDL_vulkan.h>
#include <atomic>
#include <cstdlib>
#include <cstdio>
#include <memory>
namespace bo1::android::native_device {
namespace {std::atomic<bool> requested{};std::unique_ptr<bo1::vulkan::Presentation> renderer;std::unique_ptr<bo1::vulkan::EngineFrames> frames;SDL_threadID owner{};SDL_Window* ownedWindow{};unsigned windowId{};bool windowAttached{};}
bool Requested(){return requested.load();}
bool Configure(bool native,std::string& error) {
    if(renderer){error="Cannot change native backend with a live device";return false;}
    requested.store(native);return true;
}
bool Open(SDL_Window* window,bo1::vulkan::HardwareInfo& hardware,std::string& error) {
    using namespace bo1::vulkan;
    if(!Requested()||renderer||!window||!(SDL_GetWindowFlags(window)&SDL_WINDOW_VULKAN)) {
        error="Invalid native device/window creation state";return false;
    }
    if(shader_assets::Active()||!image_assets::Empty()||!buffer_assets::Empty()||!query_assets::Empty()||!event_fences::Empty()) {
        error="Native device creation requires drained resource owners";return false;
    }
    auto candidate=std::make_unique<Presentation>();
    if(!candidate->Open(window,true,error))return false;
    HardwareInfo actual;if(!QueryHardware(candidate->Physical(),actual,error))return false;
    if(!image_assets::Attach(candidate->Physical(),candidate->Device(),error,candidate->BlockCompressionEnabled()))return false;
    if(!shader_assets::SelectNative(true)) {
        std::string rollback;if(!image_assets::Attach(VK_NULL_HANDLE,VK_NULL_HANDLE,rollback))std::abort();
        error="Native shader backend selection rejected device ownership";return false;
    }
    if(!query_assets::Attach(candidate->Device(),candidate->PreciseOcclusionEnabled(),error)) {
        std::string rollback;if(!shader_assets::SelectNative(false)||!image_assets::Attach(VK_NULL_HANDLE,VK_NULL_HANDLE,rollback))std::abort();return false;
    }
    if(!event_fences::Attach(candidate->Device(),candidate->Queue(),error)) {
        std::string rollback;query_assets::Attach(VK_NULL_HANDLE,false,rollback);
        if(!shader_assets::SelectNative(false)||!image_assets::Attach(VK_NULL_HANDLE,VK_NULL_HANDLE,rollback))std::abort();return false;
    }
    hardware=actual;owner=SDL_ThreadID();ownedWindow=window;windowId=SDL_GetWindowID(window);windowAttached=false;renderer=std::move(candidate);return true;
}
bo1::vulkan::Presentation* Renderer(){return renderer.get();}
bool WindowAttached(){return windowAttached;}
bo1::vulkan::EngineFrames* Frames(std::string& error) {
    if(!renderer||SDL_ThreadID()!=owner) {error="Native engine frames need their live device owner";return nullptr;}
    if(!frames) {
        auto candidate=std::make_unique<bo1::vulkan::EngineFrames>(*renderer);
        if(!candidate->Initialize(error))return nullptr;frames=std::move(candidate);
    }
    return frames.get();
}
bool AttachWindow(SDL_Window* window,int width,int height,std::string& error) {
    if(!renderer||SDL_ThreadID()!=owner||windowAttached||!window||window!=ownedWindow||!windowId||SDL_GetWindowID(window)!=windowId
        ||!(SDL_GetWindowFlags(window)&SDL_WINDOW_VULKAN)||!renderer->SwapchainReady()) {error="Native attachment needs its actual live SDL window and swapchain";return false;}
    int drawableWidth{},drawableHeight{};SDL_Vulkan_GetDrawableSize(window,&drawableWidth,&drawableHeight);
    if(width<=0||height<=0||drawableWidth!=width||drawableHeight!=height) {error="Native attachment dimensions do not match the actual drawable";return false;}
    windowAttached=true;return true;
}
bool Close(std::string& error) {
    using namespace bo1::vulkan;
    if(!renderer)return true;
    if(SDL_ThreadID()!=owner){error="Native device close must run on its creation thread";return false;}
    std::fprintf(stderr,"Native device close: draining frames\n");
    if(frames) {if(!frames->Drain(error))return false;frames.reset();}
    // Frame drain has retired every submitted snapshot. Command registries
    // (including temporary parallel lane states) also retain bindings between
    // draws; release those CPU borrowers before testing asset-owner emptiness.
    const auto bindings=geometry_bindings::RetireAll()+texture_bindings::RetireAll()
        +material_bindings::RetireAll()+constant_state::RetireAll()+target_bindings::RetireAll();
    std::fprintf(stderr,"Native device close: retired %zu command binding states\n",bindings);
    std::fprintf(stderr,"Native device close: clearing uploads\n");
    if(!image_uploads::Clear(error))return false;
    const bool imagesEmpty=image_assets::Empty(),buffersEmpty=buffer_assets::Empty(),queriesEmpty=query_assets::Empty(),eventsEmpty=event_fences::Empty();
    std::fprintf(stderr,"Native device close: empty images=%i buffers=%i queries=%i events=%i\n",imagesEmpty,buffersEmpty,queriesEmpty,eventsEmpty);
    if(!imagesEmpty||!buffersEmpty||!queriesEmpty||!eventsEmpty) {error="Native device still has GPU resource owners or frame borrowers";return false;}
    if(!shader_assets::SelectNative(false)) {error="Native device still has shader or declaration owners";return false;}
    if(!image_assets::Attach(VK_NULL_HANDLE,VK_NULL_HANDLE,error))return false;
    if(!buffer_assets::Attach(VK_NULL_HANDLE,VK_NULL_HANDLE,error))return false;
    if(!query_assets::Attach(VK_NULL_HANDLE,false,error))return false;
    // Presentation drains its queues and releases swapchain/device/surface in
    // dependency order. The caller then destroys the actual SDL window.
    if(!event_fences::Attach(VK_NULL_HANDLE,VK_NULL_HANDLE,error))return false;
    R_DetachNativeWindow(ownedWindow);
    renderer.reset();owner=0;ownedWindow=nullptr;windowId=0;windowAttached=false;return true;
}
}
