#pragma once
#include <cstdint>
#include <cstdio>
using HRESULT = std::int32_t;
using DWORD = std::uint32_t;
constexpr HRESULT S_OK=0, E_FAIL=static_cast<HRESULT>(0x80004005u), E_POINTER=static_cast<HRESULT>(0x80004003u), D3DERR_NOTFOUND=static_cast<HRESULT>(0x88760866u);
constexpr DWORD FALSE=0;
enum D3DRENDERSTATETYPE {D3DRS_ZENABLE, D3DRS_ZWRITEENABLE, D3DRS_STENCILENABLE};
inline int calls{}, releases{}, restores{}, unbinds{};
inline int failState=-1;
inline HRESULT getResult{}, unbindResult{}, restoreResult{};
struct IDirect3DSurface9 { int refs=1; void Release(){--refs;++releases;std::fprintf(stderr,"TRACE Release refs=%d\n",refs);} };
struct IDirect3DDevice9 {
 DWORD states[3]{};
 IDirect3DSurface9* bound{};
 HRESULT GetRenderState(D3DRENDERSTATETYPE state,DWORD* value){++calls;*value=states[state];return state==failState ? E_FAIL:S_OK;}
 HRESULT GetDepthStencilSurface(IDirect3DSurface9** value){++calls;*value=bound;if(bound)++bound->refs;return getResult ? getResult:(bound?S_OK:D3DERR_NOTFOUND);}
 HRESULT SetDepthStencilSurface(IDirect3DSurface9* value){++calls;if(value){++restores;std::fprintf(stderr,"TRACE restore\n");if(restoreResult)return restoreResult;}else{++unbinds;std::fprintf(stderr,"TRACE unbind\n");if(unbindResult)return unbindResult;}bound=value;return S_OK;}
};
