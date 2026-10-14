#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <string>
using HRESULT=int;
constexpr int S_OK=0,D3DERR_NOTFOUND=-1,D3DSBT_ALL=1;
struct D3DCAPS9 {unsigned NumSimultaneousRTs;};
struct IDirect3DSurface9 {int refs=1; void AddRef(){++refs;} void Release(){--refs;assert(refs>=1);}};
struct IDirect3DStateBlock9 {bool fail{};int *destroyed; std::vector<std::string>* events;
 HRESULT Capture(){return fail?-2:0;} HRESULT Apply(){events->push_back("apply");return 0;}
 void Release(){++*destroyed;delete this;}};
struct IDirect3DDevice9 {
 IDirect3DSurface9 surfaces[4],depth;std::vector<std::string> events;int destroyed{},failGet=-1;bool failCapture{},failRestore{};
 HRESULT GetDeviceCaps(D3DCAPS9*p){p->NumSimultaneousRTs=4;return 0;}
 HRESULT CreateStateBlock(int,IDirect3DStateBlock9**p){*p=new IDirect3DStateBlock9{failCapture,&destroyed,&events};return 0;}
 HRESULT GetRenderTarget(unsigned i,IDirect3DSurface9**p){if(static_cast<int>(i)==failGet)return -2;*p=&surfaces[i];(*p)->AddRef();return 0;}
 HRESULT GetDepthStencilSurface(IDirect3DSurface9**p){*p=&depth;(*p)->AddRef();return 0;}
 HRESULT SetDepthStencilSurface(IDirect3DSurface9*p){events.push_back(p?"depth":"depth_null");return 0;}
 HRESULT SetRenderTarget(unsigned i,IDirect3DSurface9*p){events.push_back("rt"+std::to_string(i)+(p?"":"_null"));return failRestore&&i==0?-2:0;}
};
void RestoreCheck(const char* op,HRESULT hr){if(hr){std::fprintf(stderr,"restore failure %s\n",op);std::abort();}}
template<class T> class Ref {
public:
    T* p{};
    Ref() = default;
    ~Ref() { reset(); }
    Ref(const Ref&) = delete;
    Ref& operator=(const Ref&) = delete;
    void reset() { if (p) p->Release(); p = nullptr; }
    T** put() { reset(); return &p; }
};
class StateScope {
    IDirect3DDevice9* d;
    Ref<IDirect3DStateBlock9> block;
    Ref<IDirect3DSurface9> targets[4], depth;
    unsigned count{};
    bool captured{};
public:
    explicit StateScope(IDirect3DDevice9* device):d(device) {
        D3DCAPS9 caps{};
        if (d->GetDeviceCaps(&caps)!=S_OK || caps.NumSimultaneousRTs>4 || !caps.NumSimultaneousRTs) return;
        count=caps.NumSimultaneousRTs;
        if (d->CreateStateBlock(D3DSBT_ALL,block.put())!=S_OK || block.p->Capture()!=S_OK) return;
        for (unsigned i=0;i<count;++i) {
            HRESULT hr=d->GetRenderTarget(i,targets[i].put());
            if (hr!=S_OK && hr!=D3DERR_NOTFOUND) return;
        }
        if (!targets[0].p) return;
        HRESULT hr=d->GetDepthStencilSurface(depth.put());
        if (hr!=S_OK && hr!=D3DERR_NOTFOUND) return;
        captured=true;
    }
    bool ready() const { return captured; }
    ~StateScope() {
        if (!captured) return;
        RestoreCheck("depth_null",d->SetDepthStencilSurface(nullptr));
        for (unsigned i=1;i<count;++i) RestoreCheck("mrt_null",d->SetRenderTarget(i,nullptr));
        RestoreCheck("target0",d->SetRenderTarget(0,targets[0].p));
        for (unsigned i=1;i<count;++i) RestoreCheck("mrt",d->SetRenderTarget(i,targets[i].p));
        RestoreCheck("depth",d->SetDepthStencilSurface(depth.p));
        RestoreCheck("stateblock",block.p->Apply());
    }
};

int main(int argc,char**) {
 _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
 {IDirect3DSurface9 v;{Ref<IDirect3DSurface9> ref;v.AddRef();ref.p=&v;assert(v.refs==2);ref.reset();assert(v.refs==1);v.AddRef();*ref.put()=&v;}assert(v.refs==1);}
 {IDirect3DDevice9 d;{StateScope scope(&d);assert(scope.ready());for(auto &s:d.surfaces)assert(s.refs==2);assert(d.depth.refs==2);d.failRestore=argc>1;}
  assert(d.destroyed==1);for(auto&s:d.surfaces)assert(s.refs==1);assert(d.depth.refs==1);
  const std::vector<std::string> expected={"depth_null","rt1_null","rt2_null","rt3_null","rt0","rt1","rt2","rt3","depth","apply"};assert(d.events==expected);}
 {IDirect3DDevice9 d;d.failCapture=true;{StateScope scope(&d);assert(!scope.ready());}assert(d.events.empty());assert(d.destroyed==1);}
 for(int i=0;i<4;++i){IDirect3DDevice9 d;d.failGet=i;{StateScope scope(&d);assert(!scope.ready());}assert(d.events.empty());assert(d.destroyed==1);for(auto&s:d.surfaces)assert(s.refs==1);}
 std::puts("production Ref/StateScope ownership/restoration ordering PASS");
}
