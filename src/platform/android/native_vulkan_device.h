#pragma once
#include "gfx_vulkan/presentation.h"
#include "gfx_vulkan/hardware_info.h"
namespace bo1::vulkan {class EngineFrames;}
namespace bo1::android::native_device {
// Backend choice is configuration, not initialization readiness. The creation
// thread creates/closes the device; renderer users must retire before closing it.
// In the real engine this is the render thread, separate from SDL event ownership.
bool Configure(bool native,std::string& error);
bool Requested();
bool Open(SDL_Window*,bo1::vulkan::HardwareInfo&,std::string& error);
bo1::vulkan::Presentation* Renderer();
bool AttachWindow(SDL_Window*,int width,int height,std::string& error);
bool WindowAttached();
bo1::vulkan::EngineFrames* Frames(std::string& error);
bool Close(std::string& error);
}

#if defined(__ANDROID__)
void R_DetachNativeWindow(SDL_Window*);
#endif
