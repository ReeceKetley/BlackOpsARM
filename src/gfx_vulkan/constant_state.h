#pragma once
#include "shader_translate.h"
#include <array>
#include <memory>
struct GfxCmdBufState;
namespace bo1::vulkan::constant_state {
using Bank=std::array<std::array<float,4>,256>;
struct Snapshot {std::array<Bank,2> floats{};std::uint64_t revision{};};
bool Set(const GfxCmdBufState*,ShaderStage,unsigned first,unsigned rows,const float*,std::string&);
std::shared_ptr<const Snapshot> Acquire(const GfxCmdBufState*);
void Release(const GfxCmdBufState*);
void Clone(const GfxCmdBufState*,const GfxCmdBufState*);
}
