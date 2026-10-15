#pragma once
#include <cstdint>
#include <vector>
namespace bo1::vulkan::shader_dump {
// Diagnostic disassembly of material shaders, written once per material and
// technique to <user root>/shader-dump when <user root>/dump-shaders exists.
bool Enabled();
void Write(const char* material,const char* technique,const std::vector<std::uint32_t>& vertex,
    const std::vector<std::uint32_t>& pixel,const float (*vertexConstants)[4],const float (*pixelConstants)[4]);
}
