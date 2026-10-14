#pragma once

// mod: anim info pool. Retail uses 4096 entries; bo1_mod_scriptvars 1 (startup) uses XANIM_INFO_MAX.
// Info indices are uint16 in XAnimInfo / XAnimTree_s and short in the anim command code, so at most 32768.
#define XANIM_INFO_MAX 16384
extern int g_xAnimInfoCount;

