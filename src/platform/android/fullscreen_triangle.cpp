#include "fullscreen_triangle.h"
#if defined(BO1_NATIVE_FULLSCREEN_TRIANGLE) && BO1_NATIVE_FULLSCREEN_TRIANGLE
#include "post_fusion_trial_bytecode.h"
#include "gfx_d3d/rb_backend.h"
#include "database/db_material_graph_types.h"
#include "database/db_techset_types.h"
#include "universal/dvar.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
namespace bo1::android::fullscreen_triangle {
namespace {
thread_local unsigned active{};
const dvar_s* control{};
bool reported[2]{};
template<class T> struct Ref {
    T* p{}; ~Ref(){reset();} Ref()=default; Ref(const Ref&)=delete;
    void reset(){if(p)p->Release();p=nullptr;} T** put(){reset();return &p;}
};
void Restore(HRESULT hr){if(hr!=S_OK){std::fprintf(stderr,"BO1_NATIVE_FULLSCREEN_TRIANGLE restore_failure=0x%08x\n",unsigned(hr));std::fflush(stderr);std::abort();}}
struct State {
    IDirect3DDevice9* d; Ref<IDirect3DStateBlock9> block; Ref<IDirect3DSurface9> targets[4],depth;
    unsigned count{}; bool valid{};
    explicit State(IDirect3DDevice9* dev):d(dev){
        D3DCAPS9 caps{};if(d->GetDeviceCaps(&caps)!=S_OK||!caps.NumSimultaneousRTs||caps.NumSimultaneousRTs>4)return;
        count=caps.NumSimultaneousRTs;
        if(d->CreateStateBlock(D3DSBT_ALL,block.put())!=S_OK||block.p->Capture()!=S_OK)return;
        for(unsigned i=0;i<count;++i){HRESULT h=d->GetRenderTarget(i,targets[i].put());if(h!=S_OK&&h!=D3DERR_NOTFOUND)return;}
        HRESULT h=d->GetDepthStencilSurface(depth.put());if(h!=S_OK&&h!=D3DERR_NOTFOUND)return;
        valid=targets[0].p!=nullptr;
    }
    ~State(){if(!valid)return;Restore(d->SetDepthStencilSurface(nullptr));for(unsigned i=1;i<count;++i)Restore(d->SetRenderTarget(i,nullptr));
        Restore(d->SetRenderTarget(0,targets[0].p));for(unsigned i=1;i<count;++i)Restore(d->SetRenderTarget(i,targets[i].p));
        Restore(d->SetDepthStencilSurface(depth.p));Restore(block.p->Apply());}
};
constexpr D3DSAMPLERSTATETYPE samplerTypes[]={D3DSAMP_ADDRESSU,D3DSAMP_ADDRESSV,D3DSAMP_ADDRESSW,D3DSAMP_BORDERCOLOR,D3DSAMP_MAGFILTER,D3DSAMP_MINFILTER,D3DSAMP_MIPFILTER,D3DSAMP_MIPMAPLODBIAS,D3DSAMP_MAXMIPLEVEL,D3DSAMP_MAXANISOTROPY,D3DSAMP_SRGBTEXTURE,D3DSAMP_ELEMENTINDEX,D3DSAMP_DMAPOFFSET};
struct Key {
    IDirect3DDevice9* device{};const Material* material{};unsigned width{},height{};D3DFORMAT format{};
    DWORD sampler[2][13]{};D3DSURFACE_DESC texture[2]{};
};
struct Case {Key key{};unsigned pairs{},lastFrame{};bool occupied{},failed{},hasFrame{};Ref<IDirect3DSurface9> quad,triangle,quadRead,triangleRead;};
std::array<Case,4> cases;
bool Same(const Key& a,const Key& b){return a.device==b.device&&a.material==b.material&&a.width==b.width&&a.height==b.height&&a.format==b.format
    &&!std::memcmp(a.sampler,b.sampler,sizeof(a.sampler))&&!std::memcmp(a.texture,b.texture,sizeof(a.texture));}
template<class T,size_t N> bool Program(T* shader,const std::uint32_t(&expected)[N]){
    UINT size{};std::array<std::uint32_t,N> code{};return shader&&shader->GetFunction(nullptr,&size)==S_OK&&size==sizeof(expected)
        &&shader->GetFunction(code.data(),&size)==S_OK&&size==sizeof(expected)&&!std::memcmp(code.data(),expected,sizeof(expected));
}
bool Gate(IDirect3DDevice9* d,const Material* m,unsigned pass,Key& key,const char*& reason){
    if(!d||!m||!m->info.name||pass||!m->techniqueSet||gfxCmdBufState.stateOverride)return (reason="context_or_pass",false);
    bool bloom=!std::strcmp(m->info.name,"bloom_apply_ldr"),color=!std::strcmp(m->info.name,"postfx_color");if(!bloom&&!color)return (reason="material_name",false);
    auto* technique=m->techniqueSet->techniques[4];if(!technique||technique->passCount!=1)return (reason="technique_pass_count",false);
    Ref<IDirect3DVertexShader9> vs;Ref<IDirect3DPixelShader9> ps;
    if(d->GetVertexShader(vs.put())!=S_OK||d->GetPixelShader(ps.put())!=S_OK||!Program(vs.p,post_fusion_trial::bytecode::SharedVS)
        ||(bloom?!Program(ps.p,post_fusion_trial::bytecode::BloomPS):!Program(ps.p,post_fusion_trial::bytecode::ColorPS)))return (reason="actual_shader_bytecode",false);
    if(tess.vertexCount!=4||tess.indexCount!=6)return (reason="tess_counts",false);
    Ref<IDirect3DVertexBuffer9> originalStream;UINT streamOffset{},streamStride{},streamFrequency{};
    if(d->GetStreamSource(0,originalStream.put(),&streamOffset,&streamStride)!=S_OK||!originalStream.p
        ||streamStride!=sizeof(GfxVertex)||d->GetStreamSourceFreq(0,&streamFrequency)!=S_OK||streamFrequency!=1)return (reason="stream_stride_frequency",false);
    const unsigned short indices[]={3,0,2,2,0,1};if(std::memcmp(indices,tess.indices,sizeof(indices)))return (reason="tess_indices",false);
    Ref<IDirect3DSurface9> target;D3DSURFACE_DESC desc{};
    if(d->GetRenderTarget(0,target.put())!=S_OK||target.p->GetDesc(&desc)!=S_OK||!desc.Width||!desc.Height
        ||desc.MultiSampleType!=D3DMULTISAMPLE_NONE||(desc.Format!=D3DFMT_A8R8G8B8&&desc.Format!=D3DFMT_A8B8G8R8))return (reason="target_format_size_msaa",false);
    D3DCAPS9 caps{};if(d->GetDeviceCaps(&caps)!=S_OK||!caps.NumSimultaneousRTs||caps.NumSimultaneousRTs>4)return (reason="mrt_caps",false);
    for(unsigned i=1;i<caps.NumSimultaneousRTs;++i){Ref<IDirect3DSurface9> extra;HRESULT h=d->GetRenderTarget(i,extra.put());if(h!=D3DERR_NOTFOUND&&(h!=S_OK||extra.p))return (reason="extra_mrt",false);}
    const D3DRENDERSTATETYPE off[]={D3DRS_ZENABLE,D3DRS_ZWRITEENABLE,D3DRS_STENCILENABLE,D3DRS_ALPHABLENDENABLE,D3DRS_ALPHATESTENABLE,D3DRS_SRGBWRITEENABLE};
    for(auto s:off){DWORD value{};if(d->GetRenderState(s,&value)!=S_OK||value)return (reason="enabled_render_state",false);}
    DWORD mask{};if(d->GetRenderState(D3DRS_COLORWRITEENABLE,&mask)!=S_OK||mask!=15)return (reason="color_write_mask",false);
    D3DVIEWPORT9 vp{};if(d->GetViewport(&vp)!=S_OK||vp.X||vp.Y||vp.Width!=desc.Width||vp.Height!=desc.Height||vp.MinZ||vp.MaxZ!=1)return (reason="viewport",false);
    DWORD scissors{};RECT rect{};if(d->GetRenderState(D3DRS_SCISSORTESTENABLE,&scissors)!=S_OK||d->GetScissorRect(&rect)!=S_OK)return (reason="scissor_query",false);
    if(scissors&&(rect.left||rect.top||rect.right!=LONG(desc.Width)||rect.bottom!=LONG(desc.Height)))return (reason="scissor_bounds",false);
    const float iw=float(1.0/double(desc.Width)),ih=float(1.0/double(desc.Height));
    const float expected[16]={2*iw,0,0,-1-iw,0,-2*ih,0,1+ih,0,0,0,1,0,0,0,1};float matrix[16]{};
    if(d->GetVertexShaderConstantF(0,matrix,4)!=S_OK||std::memcmp(matrix,expected,sizeof(matrix)))return (reason="vertex_matrix",false);
    const float xy[4][2]={{0,0},{float(desc.Width),0},{float(desc.Width),float(desc.Height)},{0,float(desc.Height)}};
    const float uv[4][2]={{0,0},{1,0},{1,1},{0,1}};
    for(unsigned i=0;i<4;++i){const auto& v=tess.verts[i];if(v.xyzw[0]!=xy[i][0]||v.xyzw[1]!=xy[i][1]||v.xyzw[2]||v.xyzw[3]!=1
        ||std::memcmp(v.texCoord,uv[i],sizeof(uv[i]))||std::memcmp(&v.color,&tess.verts[0].color,sizeof(v.color))||std::memcmp(&v.normal,&tess.verts[0].normal,sizeof(v.normal)))return (reason="tess_geometry_uv_uniform",false);}
    Ref<IDirect3DVertexDeclaration9> declaration;D3DVERTEXELEMENT9 elements[65]{};UINT count=65;
    if(d->GetVertexDeclaration(declaration.put())!=S_OK||!declaration.p||declaration.p->GetDeclaration(elements,&count)!=S_OK)return (reason="declaration_query",false);
    bool position=false,texcoord=false;
    for(UINT i=0;i<count&&elements[i].Stream!=0xff;++i){const auto& e=elements[i];if(e.Stream||e.Method!=D3DDECLMETHOD_DEFAULT||e.UsageIndex)return (reason="declaration_stream_method_usage_index",false);
        if(e.Usage==D3DDECLUSAGE_POSITION&&e.Offset==0&&(e.Type==D3DDECLTYPE_FLOAT4||e.Type==D3DDECLTYPE_FLOAT3))position=true;
        else if(e.Usage==D3DDECLUSAGE_TEXCOORD&&e.Offset==20&&e.Type==D3DDECLTYPE_FLOAT2)texcoord=true;
        else if(e.Usage==D3DDECLUSAGE_COLOR&&e.Offset==16&&e.Type==D3DDECLTYPE_D3DCOLOR){}
        else if(e.Usage==D3DDECLUSAGE_NORMAL&&e.Offset==28&&e.Type==D3DDECLTYPE_UBYTE4N){}
        else return (reason="declaration_element",false);}
    if(!position||!texcoord)return (reason="declaration_position_texcoord",false);
    key.device=d;key.material=m;key.width=desc.Width;key.height=desc.Height;key.format=desc.Format;
    for(unsigned i=0;i<(bloom?2u:1u);++i){for(unsigned j=0;j<13;++j)if(d->GetSamplerState(i,samplerTypes[j],&key.sampler[i][j])!=S_OK)return (reason="sampler_query",false);
        if(key.sampler[i][10])return (reason="sampler_srgb",false);Ref<IDirect3DBaseTexture9> base;if(d->GetTexture(i,base.put())!=S_OK||!base.p||base.p->GetType()!=D3DRTYPE_TEXTURE)return (reason="texture_binding",false);
        auto* texture=static_cast<IDirect3DTexture9*>(base.p);if(texture->GetLevelDesc(0,&key.texture[i])!=S_OK)return (reason="texture_desc",false);
        Ref<IDirect3DSurface9> source;if(texture->GetSurfaceLevel(0,source.put())!=S_OK||source.p==target.p)return (reason="texture_target_alias",false);}
    return true;
}

void Diagnostic(IDirect3DDevice9* d,const Material* m,unsigned pass,const char* reason){
    std::fprintf(stderr,"BO1_NATIVE_FULLSCREEN_TRIANGLE material=%s decision=ineligible reason=%s pass=%u scope=%u vertices=%d indices=%d override=%u original_fallback=1\n",
        m->info.name,reason,pass,active,tess.vertexCount,tess.indexCount,unsigned(gfxCmdBufState.stateOverride));
    if(!d)return;
    Ref<IDirect3DSurface9> target;D3DSURFACE_DESC desc{};D3DVIEWPORT9 vp{};
    if(d->GetRenderTarget(0,target.put())==S_OK&&target.p&&target.p->GetDesc(&desc)==S_OK)
        std::fprintf(stderr,"BO1_NATIVE_FULLSCREEN_TRIANGLE material=%s target_width=%u target_height=%u format=%u msaa=%u quality=%u\n",m->info.name,desc.Width,desc.Height,unsigned(desc.Format),unsigned(desc.MultiSampleType),desc.MultiSampleQuality);
    if(d->GetViewport(&vp)==S_OK)
        std::fprintf(stderr,"BO1_NATIVE_FULLSCREEN_TRIANGLE material=%s viewport=%u,%u,%u,%u z=%.9g,%.9g\n",m->info.name,vp.X,vp.Y,vp.Width,vp.Height,vp.MinZ,vp.MaxZ);
    if(!std::strncmp(reason,"declaration_",12)){
        Ref<IDirect3DVertexDeclaration9> declaration;D3DVERTEXELEMENT9 elements[65]{};UINT count=65;
        if(d->GetVertexDeclaration(declaration.put())==S_OK&&declaration.p&&declaration.p->GetDeclaration(elements,&count)==S_OK)
            for(UINT i=0;i<count&&i<65;++i){const auto& e=elements[i];
                std::fprintf(stderr,"BO1_NATIVE_FULLSCREEN_TRIANGLE material=%s declaration_entry=%u stream=%u offset=%u type=%u method=%u usage=%u usage_index=%u\n",m->info.name,i,unsigned(e.Stream),unsigned(e.Offset),unsigned(e.Type),unsigned(e.Method),unsigned(e.Usage),unsigned(e.UsageIndex));
                if(e.Stream==0xff)break;}
    }
    if(!std::strcmp(reason,"enabled_render_state")||!std::strcmp(reason,"color_write_mask")){
        const D3DRENDERSTATETYPE states[]={D3DRS_ZENABLE,D3DRS_ZWRITEENABLE,D3DRS_STENCILENABLE,D3DRS_ALPHABLENDENABLE,D3DRS_SEPARATEALPHABLENDENABLE,D3DRS_ALPHATESTENABLE,D3DRS_SRGBWRITEENABLE,D3DRS_COLORWRITEENABLE};
        for(auto state:states){DWORD value{};HRESULT hr=d->GetRenderState(state,&value);
            std::fprintf(stderr,"BO1_NATIVE_FULLSCREEN_TRIANGLE material=%s state=%u value=%u hr=0x%08x\n",m->info.name,unsigned(state),unsigned(value),unsigned(hr));}
    }
    if(!std::strcmp(reason,"stream_stride_frequency")){Ref<IDirect3DVertexBuffer9> stream;UINT offset{},stride{},frequency{};
        HRESULT hr=d->GetStreamSource(0,stream.put(),&offset,&stride);HRESULT freq=d->GetStreamSourceFreq(0,&frequency);
        std::fprintf(stderr,"BO1_NATIVE_FULLSCREEN_TRIANGLE material=%s stream_present=%u offset=%u stride=%u frequency=%u hr=0x%08x frequency_hr=0x%08x\n",m->info.name,stream.p?1u:0u,offset,stride,frequency,unsigned(hr),unsigned(freq));}
    if(!std::strcmp(reason,"vertex_matrix")){float matrix[16]{};if(d->GetVertexShaderConstantF(0,matrix,4)==S_OK)
        for(unsigned i=0;i<4;++i)std::fprintf(stderr,"BO1_NATIVE_FULLSCREEN_TRIANGLE material=%s matrix_row=%u values=%.9g,%.9g,%.9g,%.9g\n",m->info.name,i,matrix[i*4],matrix[i*4+1],matrix[i*4+2],matrix[i*4+3]);}
    if(!std::strcmp(reason,"tess_geometry_uv_uniform"))for(unsigned i=0;i<4;++i){const auto& v=tess.verts[i];
        std::fprintf(stderr,"BO1_NATIVE_FULLSCREEN_TRIANGLE material=%s vertex=%u xyzw=%.9g,%.9g,%.9g,%.9g uv=%.9g,%.9g color_uniform=%u normal_uniform=%u\n",m->info.name,i,v.xyzw[0],v.xyzw[1],v.xyzw[2],v.xyzw[3],v.texCoord[0],v.texCoord[1],!std::memcmp(&v.color,&tess.verts[0].color,sizeof(v.color)),!std::memcmp(&v.normal,&tess.verts[0].normal,sizeof(v.normal)));}
}

bool Draw(IDirect3DDevice9* d,const Key& key,bool triangle){
    if(!triangle)return d->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST,0,4,2,tess.indices,D3DFMT_INDEX16,tess.verts,sizeof(GfxVertex))==S_OK;
    GfxVertex v[3]={tess.verts[0],tess.verts[0],tess.verts[0]};
    v[0].xyzw[1]=2*float(key.height);v[0].texCoord[1]=2;
    v[2].xyzw[0]=2*float(key.width);v[2].texCoord[0]=2;
    return d->DrawPrimitiveUP(D3DPT_TRIANGLELIST,1,v,sizeof(GfxVertex))==S_OK;
}
bool Resources(Case& c){auto* d=c.key.device;auto w=c.key.width,h=c.key.height;auto f=c.key.format;
    return (c.quad.p||d->CreateRenderTarget(w,h,f,D3DMULTISAMPLE_NONE,0,FALSE,c.quad.put(),nullptr)==S_OK)
        &&(c.triangle.p||d->CreateRenderTarget(w,h,f,D3DMULTISAMPLE_NONE,0,FALSE,c.triangle.put(),nullptr)==S_OK)
        &&(c.quadRead.p||d->CreateOffscreenPlainSurface(w,h,f,D3DPOOL_SYSTEMMEM,c.quadRead.put(),nullptr)==S_OK)
        &&(c.triangleRead.p||d->CreateOffscreenPlainSurface(w,h,f,D3DPOOL_SYSTEMMEM,c.triangleRead.put(),nullptr)==S_OK);
}
bool Compare(Case& c,unsigned long long& mismatch){auto* d=c.key.device;State state(d);if(!state.valid||!Resources(c))return false;
    if(d->SetDepthStencilSurface(nullptr)!=S_OK)return false;for(unsigned i=1;i<state.count;++i)if(d->SetRenderTarget(i,nullptr)!=S_OK)return false;
    D3DVIEWPORT9 vp{0,0,c.key.width,c.key.height,0,1};
    if(d->SetRenderTarget(0,c.quad.p)!=S_OK||d->SetViewport(&vp)!=S_OK||!Draw(d,c.key,false)
        ||d->SetRenderTarget(0,c.triangle.p)!=S_OK||d->SetViewport(&vp)!=S_OK||!Draw(d,c.key,true)
        ||d->GetRenderTargetData(c.quad.p,c.quadRead.p)!=S_OK||d->GetRenderTargetData(c.triangle.p,c.triangleRead.p)!=S_OK)return false;
    D3DLOCKED_RECT a{},b{};if(c.quadRead.p->LockRect(&a,nullptr,D3DLOCK_READONLY)!=S_OK)return false;
    if(c.triangleRead.p->LockRect(&b,nullptr,D3DLOCK_READONLY)!=S_OK){Restore(c.quadRead.p->UnlockRect());return false;}
    bool valid=a.pBits&&b.pBits&&a.Pitch>=int(c.key.width*4)&&b.Pitch>=int(c.key.width*4);
    if(valid)for(unsigned y=0;y<c.key.height;++y)for(unsigned x=0;x<c.key.width;++x)
        mismatch+=std::memcmp(static_cast<const char*>(a.pBits)+size_t(y)*a.Pitch+x*4,static_cast<const char*>(b.pBits)+size_t(y)*b.Pitch+x*4,4)!=0;
    Restore(c.triangleRead.p->UnlockRect());Restore(c.quadRead.p->UnlockRect());return valid;
}
}
Scope::Scope(){++active;}Scope::~Scope(){--active;}
void RegisterControls(){control=_Dvar_RegisterBool("r_nativeFullscreenTriangle",false,0,"Validated full-screen triangle; false restores canonical quads");
    std::fprintf(stderr,"BO1_NATIVE_FULLSCREEN_TRIANGLE control_registered=%u default=0 current=%u\n",control?1u:0u,control&&control->current.enabled?1u:0u);}
void Release(){reported[0]=reported[1]=false;for(auto& c:cases){c.quad.reset();c.triangle.reset();c.quadRead.reset();c.triangleRead.reset();c.key={};c.pairs=0;c.failed=false;c.occupied=false;c.hasFrame=false;c.lastFrame=0;}}
bool TryDraw(IDirect3DDevice9* d,const Material* m,unsigned pass){
    if(!active||!control||!control->current.enabled)return false;
    if(!m||!m->info.name)return false;
    int candidate=!std::strcmp(m->info.name,"bloom_apply_ldr")?0:!std::strcmp(m->info.name,"postfx_color")?1:-1;
    if(candidate<0)return false;
    Key key{};const char* reason="none";if(!Gate(d,m,pass,key,reason)){
        if(!reported[candidate]){reported[candidate]=true;Diagnostic(d,m,pass,reason);}return false;
    }
    Case* selected{};for(auto& c:cases)if(c.occupied&&Same(c.key,key)){selected=&c;break;}
    if(!selected)for(auto& c:cases)if(!c.occupied){c.occupied=true;c.key=key;selected=&c;break;}
    if(!selected||selected->failed)return false;auto& c=*selected;
    if(c.pairs<3){if(!backEndData||(c.hasFrame&&c.lastFrame==backEndData->frameCount))return false;
        c.lastFrame=backEndData->frameCount;c.hasFrame=true;
        unsigned long long mismatch{};bool complete=Compare(c,mismatch);c.failed=!complete||mismatch;c.pairs+=complete?1:0;
        std::fprintf(stderr,"BO1_NATIVE_FULLSCREEN_TRIANGLE material=%s width=%u height=%u pairs=%u pixel_mismatches=%llu decision=%s original_fallback=1\n",m->info.name,key.width,key.height,c.pairs,mismatch,c.failed?(complete?"rejected":"incomplete"):(c.pairs==3?"accepted":"pending"));return false;}
    // DrawPrimitiveUP clears stream0. It does not change targets, shaders,
    // constants or raster state; preserve only these UP binding side effects.
    Ref<IDirect3DVertexBuffer9> stream;Ref<IDirect3DIndexBuffer9> indices;UINT offset{},stride{};
    if(d->GetStreamSource(0,stream.put(),&offset,&stride)!=S_OK||d->GetIndices(indices.put())!=S_OK)return false;
    const bool drawn=Draw(d,key,true);
    Restore(d->SetStreamSource(0,stream.p,offset,stride));Restore(d->SetIndices(indices.p));
    if(!drawn){c.failed=true;std::fprintf(stderr,"BO1_NATIVE_FULLSCREEN_TRIANGLE material=%s decision=draw_error original_fallback=1\n",m->info.name);return false;}
    return true;
}
}
#else
namespace bo1::android::fullscreen_triangle {void RegisterControls(){} Scope::Scope(){} Scope::~Scope(){} bool TryDraw(IDirect3DDevice9*,const Material*,unsigned){return false;} void Release(){}}
#endif
