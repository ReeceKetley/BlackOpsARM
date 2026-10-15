#pragma once
#include <SDL.h>
#include <vector>
#include <string>
#include "gfx_vulkan/hardware_info.h"
namespace bo1::android::native_display {
struct Snapshot {int index{};SDL_Rect bounds{};SDL_DisplayMode current{};std::vector<SDL_DisplayMode> modes;};
// Caller runs on the real SDL event/video owner. Pixel format is SDL display
// metadata, independent of the Vulkan surface's eventual attachment format.
bool Read(int display,Snapshot&,std::string&);
// Physical query only: does not create/select a logical device or publish
// resource readiness. Device creation re-queries the surface-selected GPU.
bool ProbeHardware(bo1::vulkan::HardwareInfo&,std::string&);
}
