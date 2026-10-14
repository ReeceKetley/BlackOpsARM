#pragma once
#include <cstdint>
using HRESULT=int; using BOOL=int; using UINT64=std::uint64_t;
constexpr int S_OK=0,S_FALSE=1,TRUE=1;
enum {D3DQUERYTYPE_TIMESTAMPDISJOINT=1,D3DQUERYTYPE_TIMESTAMPFREQ=2,D3DQUERYTYPE_TIMESTAMP=3,D3DISSUE_BEGIN=1,D3DISSUE_END=2};
struct IDirect3DQuery9 { virtual HRESULT Issue(unsigned)=0; virtual HRESULT GetData(void*,unsigned,unsigned)=0; virtual unsigned Release()=0; virtual ~IDirect3DQuery9()=default; };
struct IDirect3DDevice9 { virtual HRESULT CreateQuery(unsigned,IDirect3DQuery9**)=0; virtual ~IDirect3DDevice9()=default; };
