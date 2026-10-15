#pragma once
#include "shader_assets.h"
#include "gfx_d3d/r_init.h"
#include "qcommon/common.h"
#include "platform/win32_compat/win32_time.h"
#include <cstdlib>
namespace bo1::vulkan::engine_queries {
inline bool Active(){return shader_assets::Active();}
inline void Issue(IDirect3DQuery9* const* owner,unsigned flags) {
 if(RB_HW_IssueOcclusionQueryFromOwner(owner,flags)!=S_OK){Com_Error(ERR_FATAL,"Native engine query issue failed");std::abort();}
}
inline unsigned Read(IDirect3DQuery9* const* owner) {
 unsigned samples{};
 for(;;){auto result=RB_HW_ReadOcclusionQueryFromOwner(owner,&samples,D3DGETDATA_FLUSH);
  if(result==S_OK)return samples;
  if(result!=S_FALSE){Com_Error(ERR_FATAL,"Native engine query read requires a successfully submitted frame");std::abort();}
  bo1::win32::Sleep(0);
 }
}
}
