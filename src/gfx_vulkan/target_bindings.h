#pragma once
#include "render_targets.h"
struct GfxCmdBufState;struct GfxCmdBufPrimState;struct GfxViewport;
namespace bo1::vulkan::target_bindings {
bool Select(GfxCmdBufState*,const GfxRenderTarget*,std::string&);
bool Viewport(GfxCmdBufState*,const GfxViewport&,std::string&);
const GfxRenderTarget* Acquire(const GfxCmdBufState*);
const GfxCmdBufState* Owner(const GfxCmdBufPrimState*);
void Release(const GfxCmdBufState*);void Clone(const GfxCmdBufState*,const GfxCmdBufState*);
}
