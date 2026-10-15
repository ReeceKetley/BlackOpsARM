#pragma once
#include "image_assets.h"
struct GfxRenderTarget;
namespace bo1::vulkan::render_targets {
struct Record {std::shared_ptr<const image_assets::Record> color,depth;};
bool Assign(const GfxRenderTarget*,const GfxImage*,bool depth,std::string&);
std::shared_ptr<const Record> Acquire(const GfxRenderTarget*);
void Release(const GfxRenderTarget*);
bool Share(const GfxRenderTarget* source,const GfxRenderTarget* destination,std::string&);
bool AttachDepth(const GfxRenderTarget*,unsigned width,unsigned height,std::string&);
bool ShareDepth(const GfxRenderTarget* source,const GfxRenderTarget* destination,std::string&);
bool AttachAuxiliary(const GfxRenderTarget*,unsigned width,unsigned height,unsigned format,bool depth,std::string&);
bool References(const GfxImage*);
void Clear();
}
