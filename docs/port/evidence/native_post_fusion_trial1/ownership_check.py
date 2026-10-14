from pathlib import Path
import subprocess,json
root=Path.cwd();out=Path(__file__).parent;s=(root/'src/platform/android/post_fusion_trial.cpp').read_text()
ref=s[s.index('template<class T> class Ref'):s.index('const dvar_s* control')]
scope=s[s.index('class StateScope {'):s.index('bool DrawTrial()')]
text=r'''#include <cassert>
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
'''+ref+scope+r'''
int main(int argc,char**) {
 {IDirect3DSurface9 v;{Ref<IDirect3DSurface9> ref;v.AddRef();ref.p=&v;assert(v.refs==2);ref.reset();assert(v.refs==1);v.AddRef();*ref.put()=&v;}assert(v.refs==1);}
 {IDirect3DDevice9 d;{StateScope scope(&d);assert(scope.ready());for(auto &s:d.surfaces)assert(s.refs==2);assert(d.depth.refs==2);d.failRestore=argc>1;}
  assert(d.destroyed==1);for(auto&s:d.surfaces)assert(s.refs==1);assert(d.depth.refs==1);
  const std::vector<std::string> expected={"depth_null","rt1_null","rt2_null","rt3_null","rt0","rt1","rt2","rt3","depth","apply"};assert(d.events==expected);}
 {IDirect3DDevice9 d;d.failCapture=true;{StateScope scope(&d);assert(!scope.ready());}assert(d.events.empty());assert(d.destroyed==1);}
 for(int i=0;i<4;++i){IDirect3DDevice9 d;d.failGet=i;{StateScope scope(&d);assert(!scope.ready());}assert(d.events.empty());assert(d.destroyed==1);for(auto&s:d.surfaces)assert(s.refs==1);}
 std::puts("production Ref/StateScope ownership/restoration ordering PASS");
}
'''
(out/'ownership.cpp').write_text(text,encoding='utf8')
exe=out/'ownership.exe';cmd=f'"C:/BuildTools/VC/Auxiliary/Build/vcvarsall.bat" x64 >nul && cl /nologo /W4 /WX /std:c++20 /EHsc /MTd "{(out/"ownership.cpp").resolve()}" /Fo"{(out/"ownership.obj").resolve()}" /Fe"{exe.resolve()}"'
(out/'ownership-command.txt').write_text(cmd,encoding='utf8')
with (out/'ownership-compile.log').open('wb') as f: code=subprocess.run('cmd.exe /d /s /c "'+cmd+'"',stdout=f,stderr=subprocess.STDOUT).returncode
result={'compile':code}
if code==0:
 for name,args in [('pass',[]),('restore_failure',['fail'])]:
  with (out/f'ownership-{name}.log').open('wb') as f:result[name]=subprocess.run([str(exe.resolve()),*args],stdout=f,stderr=subprocess.STDOUT).returncode
(out/'ownership-results.json').write_text(json.dumps(result),encoding='utf8');print(result)
assert code==0 and result['pass']==0 and result['restore_failure']!=0
