#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3dx9.h>
#include <intrin.h>
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <string>
#include <vector>
using _BYTE=unsigned char;
// PRODUCTION_TYPES
// PRODUCTION_DECLARATIONS
static void require(bool condition,const char *why) {if(!condition){fprintf(stderr,"FAIL: %s\n",why);exit(1);}}
bool Assert_MyHandler(const char*,int,int,const char*,...) {require(false,"production assertion");return true;}
struct Allocation {unsigned char *pointer;size_t size;};
std::vector<Allocation> allocations;
unsigned char *Material_Alloc(unsigned size) {
    static uintptr_t next=sizeof(void*)==8?uintptr_t(0x180000000ull):0;
    size_t aligned=(size+15)&~size_t(15),pages=(aligned+4095)&~size_t(4095);
    auto *base=static_cast<unsigned char*>(VirtualAlloc(reinterpret_cast<void*>(next),pages+8192,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
    require(base!=nullptr,"guarded native allocation");if(sizeof(void*)==8){require(uintptr_t(base)>0x100000000ull,"objects above4GB");next+=0x100000;}
    DWORD old;require(VirtualProtect(base,4096,PAGE_NOACCESS,&old)!=0,"front guard");require(VirtualProtect(base+4096+pages,4096,PAGE_NOACCESS,&old)!=0,"end guard");
    auto *p=base+4096+pages-aligned;memset(p,0,aligned);memset(p+size,0xa5,aligned-size);allocations.push_back({p,size});return p;
}
void checkGuards() {for(auto a:allocations)for(size_t i=a.size;i<((a.size+15)&~size_t(15));++i)require(a.pointer[i]==0xa5,"allocation end canary");}
void *temporary;unsigned temporaryFrees;
void *Hunk_AllocateTempMemory(int size,const char*) {require(!temporary,"single temporary allocation");temporary=Material_Alloc(size);return temporary;}
void Hunk_FreeTempMemory(void *p) {require(p==temporary,"matching temporary release");temporary=nullptr;++temporaryFrees;}
int Com_sprintf(char *dest,unsigned size,const char *format,...) {va_list ap;va_start(ap,format);int n=vsnprintf(dest,size,format,ap);va_end(ap);return n;}
std::vector<std::string> errors,warnings;
void message(std::vector<std::string>&messages,const char *format,va_list ap) {char text[8192];vsnprintf(text,sizeof(text),format,ap);messages.emplace_back(text);}
void Com_ScriptError(const char *format,...) {va_list ap;va_start(ap,format);message(errors,format,ap);va_end(ap);}
void Com_ScriptWarning(const char *format,...) {va_list ap;va_start(ap,format);message(warnings,format,ap);va_end(ap);}
const char *errorPrefix="original error",*warningPrefix="original warning";
const char *Com_GetScriptErrorPrefix() {return errorPrefix;}
const char *Com_GetScriptWarningPrefix() {return warningPrefix;}
void Com_SetScriptErrorPrefix(const char *p) {errorPrefix=p;}
void Com_SetScriptWarningPrefix(const char *p) {warningPrefix=p;}
const char *I_stristr(const char *text,const char *pattern) {for(auto *p=text;*p;++p)if(!_strnicmp(p,pattern,strlen(pattern)))return p;return nullptr;}
const char *R_ErrorDescription(HRESULT) {return "controlled HRESULT";}
std::string mode;
unsigned generated,compilerCalls,cacheWrites,cacheReads;
DWORD observedFlags;bool diagnosticsRequested,constantsRequested;
struct Reference {std::string kind;IUnknown *object;};
std::vector<Reference> refs;
void retain(const char *kind,IUnknown *object) {
    if(!object)return;
    if(sizeof(void*)==8)require(uintptr_t(object)>0x100000000ull,"real native COM pointer above4GB");
    object->AddRef();refs.push_back({kind,object});
}
ULONG referenceCount(IUnknown *object) {ULONG count=object->AddRef();object->Release();return count-1;}
void references(unsigned shaderOwners) {
    for(auto &ref:refs) {
        unsigned expected=(ref.kind=="shader"?shaderOwners:0)+1;
        require(referenceCount(ref.object)==expected,("COM reference balance: "+ref.kind).c_str());
    }
}
void releaseRetained() {for(auto &ref:refs)ref.object->Release();refs.clear();}
std::vector<unsigned char> compiledProgram;
HRESULT WINAPI Observed_D3DXCompileShader(LPCSTR text,UINT length,const D3DXMACRO *defines,LPD3DXINCLUDE include,
    LPCSTR entry,LPCSTR target,DWORD flags,LPD3DXBUFFER *shader,LPD3DXBUFFER *diagnostics,LPD3DXCONSTANTTABLE *constants) {
    ++compilerCalls;observedFlags=flags;diagnosticsRequested=diagnostics!=nullptr;constantsRequested=constants!=nullptr;
    HRESULT result=D3DXCompileShader(text,length,defines,include,entry,target,flags,shader,diagnostics,constants);
    if(mode=="warning_output" && diagnostics && !*diagnostics) {
        const char text[]="fixture(1,1): warning X3206: controlled diagnostic fixture";
        require(SUCCEEDED(D3DXCreateBuffer(sizeof(text),diagnostics)),"real COM diagnostic buffer");
        memcpy((*diagnostics)->GetBufferPointer(),text,sizeof(text));
    }
    retain("shader",*shader);if(diagnostics)retain("diagnostics",*diagnostics);if(constants)retain("constants",*constants);
    if(*shader){auto *p=static_cast<unsigned char*>((*shader)->GetBufferPointer());compiledProgram.assign(p,p+(*shader)->GetBufferSize());}
    if(mode=="partial_failure")return E_FAIL;
    if(mode=="null_constants" && constants && *constants){(*constants)->Release();*constants=nullptr;}
    if(mode=="null_shader" && *shader){(*shader)->Release();*shader=nullptr;}
    return result;
}
unsigned Material_GenerateShaderString(GfxAssembledShaderText *prog,const char*,MaterialShaderType type,char *out,unsigned capacity) {
    ++generated;memset(prog,0,sizeof(*prog));prog->string=out;prog->fileCount=1;prog->files[0].srcLine=1;prog->files[0].destLine=1;strcpy_s(prog->files[0].fileName,"fixture.hlsl");
    if(mode=="empty")return 0;
    std::string source=type==MTL_VERTEX_SHADER?"float4 tint : register(c0); float4 vs_main(float4 pos:POSITION):POSITION { ":
        "float4 tint : register(c0); float4 ps_main(float2 uv:TEXCOORD0):COLOR0 { ";
    if(mode=="compile_error")source+="return missing_value; }";
    else if(mode=="gpu" && type==MTL_PIXEL_SHADER)source+="return tint; }";
    else source+=type==MTL_VERTEX_SHADER?"return pos + tint; }":"return tint + float4(uv,0,0); }";
    require(source.size()<capacity,"source capacity");memcpy(out,source.data(),source.size());return unsigned(source.size());
}
char Material_FindCachedShaderDX(const char*,unsigned,const char*,const char*,ID3DXBuffer **shader,const char*,bool) {
    ++cacheReads;if(mode!="cache" && mode!="vertex_cache" && mode!="pixel_cache")return 0;
    require(SUCCEEDED(D3DXCreateBuffer(16,shader)),"real cached buffer");const DWORD code[]={0x12345678,0x89abcdef,0x55667788,0};
    memcpy((*shader)->GetBufferPointer(),code,sizeof(code));retain("shader",*shader);compiledProgram.assign(reinterpret_cast<const unsigned char*>(code),reinterpret_cast<const unsigned char*>(code)+sizeof(code));return 1;
}
void Material_CacheShaderDX(const char*,unsigned,const char*,const char*,ID3DXBuffer *shader,bool) {require(shader!=nullptr,"cached compiled shader");++cacheWrites;}
void Material_GetShaderTargetString(char *target,unsigned size,const char *prefix) {sprintf_s(target,size,"%s_3_0",prefix);}
struct TestDevice {
    IDirect3DVertexShader9 *vs=nullptr;IDirect3DPixelShader9 *ps=nullptr;unsigned calls=0;
#ifdef SHADER_GPU
    IDirect3DDevice9 *real=nullptr;
#endif
    HRESULT create(const DWORD *program,void **output) {
        ++calls;require(!compiledProgram.empty(),"observed compiled bytecode");require(memcmp(program,compiledProgram.data(),compiledProgram.size())==0,"exact shader bytecode supplied to device");
        if(mode=="vertex_failure" || mode=="pixel_failure")return E_FAIL;
        *output=Material_Alloc(16);return S_OK;
    }
    HRESULT CreateVertexShader(const DWORD *program,IDirect3DVertexShader9 **output) {
#ifdef SHADER_GPU
        ++calls;require(memcmp(program,compiledProgram.data(),compiledProgram.size())==0,"real vertex creation bytecode");auto hr=real->CreateVertexShader(program,output);
#else
        auto hr=create(program,reinterpret_cast<void**>(output));
#endif
        vs=*output;return hr;
    }
    HRESULT CreatePixelShader(const DWORD *program,IDirect3DPixelShader9 **output) {
#ifdef SHADER_GPU
        ++calls;require(memcmp(program,compiledProgram.data(),compiledProgram.size())==0,"real pixel creation bytecode");auto hr=real->CreatePixelShader(program,output);
#else
        auto hr=create(program,reinterpret_cast<void**>(output));
#endif
        ps=*output;return hr;
    }
} device;
struct {TestDevice *device;} dx{&device};
#define D3DXCompileShader Observed_D3DXCompileShader
// PRODUCTION_FUNCTIONS
#undef D3DXCompileShader
void compileCase() {
    char name[64]="shader_fixture",entry[]="vs_main",target[]="vs_3_0";
    if(mode=="sw4")strcpy_s(name,"pimp_shader_sw4");
    auto *shader=Material_CompileShader(name,MTL_VERTEX_SHADER,entry,target);
    const bool success=mode=="success" || mode=="warning_output" || mode=="cache" || mode=="sw4";
    require((shader!=nullptr)==success,"compiler success/failure result");
    require(temporary==nullptr && temporaryFrees==1,"temporary allocation always released");
    require(!strcmp(errorPrefix,"original error") && !strcmp(warningPrefix,"original warning"),"diagnostic prefixes restored");
    if(mode=="cache")require(compilerCalls==0 && cacheWrites==0 && cacheReads==1,"cached shader avoids compiler/cache rewrite");
    else if(mode=="empty")require(compilerCalls==0 && cacheReads==0 && refs.empty(),"empty source avoids COM");
    else {
        require(compilerCalls==1 && diagnosticsRequested && constantsRequested,"typed real compiler outputs requested");
        require(observedFlags==(mode=="sw4"?2u:0x10002u),"original shader flags");
        require(cacheWrites==(success?1u:0u),"only successful compile cached");
    }
    if(mode=="warning_output")require(!warnings.empty() && warnings[0].find("compiler message(s)")!=std::string::npos,"controlled warning reported through real COM buffer");
    if(mode=="compile_error")require(errors.size()>=2 && errors[0].find("compiler message(s)")!=std::string::npos,"real error reported");
    unsigned owners=shader?1:0;references(owners);
    if(shader)shader->Release();references(0);releaseRetained();checkGuards();
}
template<class Shader> void checkLoaded(Shader *shader,const char *name) {
    require(shader && !strcmp(shader->name,name),"owned native shader name");
    require(shader->prog.loadDef.program==reinterpret_cast<DWORD*>(shader+1),"bytecode follows native shader header");
    require(shader->name==reinterpret_cast<const char*>(shader+1)+compiledProgram.size(),"name follows bytecode");
    require(shader->prog.loadDef.programSize*4==compiledProgram.size(),"stored shader word count");
    require(memcmp(shader->prog.loadDef.program,compiledProgram.data(),compiledProgram.size())==0,"exact stored bytecode");
}
void loaderCase(bool vertex) {
    char name[]="shader_fixture";
    bool fail=mode=="vertex_failure" || mode=="pixel_failure";
    if(vertex) {auto *shader=Material_LoadVertexShader(name);if(fail)require(!shader,"vertex creation failure result");else {checkLoaded(shader,name);require(shader->prog.vs==device.vs,"full native vertex shader pointer");}}
    else {auto *shader=Material_LoadPixelShader(name);if(fail)require(!shader,"pixel creation failure result");else {checkLoaded(shader,name);require(shader->prog.ps==device.ps,"full native pixel shader pointer");}}
    require(device.calls==1,"exactly one device creation");require(temporary==nullptr && temporaryFrees==1,"compiler temporary freed");
    if(fail)require(!errors.empty(),"device error reported");references(0);releaseRetained();checkGuards();
}
#ifdef SHADER_GPU
void gpuCase() {
    WNDCLASSA windowClass{};windowClass.lpfnWndProc=DefWindowProcA;windowClass.hInstance=GetModuleHandleA(nullptr);windowClass.lpszClassName="BO1ShaderCheck";
    require(RegisterClassA(&windowClass)!=0,"native test window class");
    HWND window=CreateWindowA(windowClass.lpszClassName,"BO1 Shader Check",WS_OVERLAPPEDWINDOW,60,60,96,96,nullptr,nullptr,windowClass.hInstance,nullptr);
    require(window!=nullptr,"hidden native test window");
    auto *d3d=Direct3DCreate9(D3D_SDK_VERSION);require(d3d!=nullptr,"actual D3D9 interface");
    D3DPRESENT_PARAMETERS present{};present.Windowed=TRUE;present.hDeviceWindow=window;present.BackBufferWidth=32;present.BackBufferHeight=32;
    present.BackBufferFormat=D3DFMT_UNKNOWN;present.SwapEffect=D3DSWAPEFFECT_DISCARD;present.PresentationInterval=D3DPRESENT_INTERVAL_IMMEDIATE;
    HRESULT created=d3d->CreateDevice(D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,window,D3DCREATE_HARDWARE_VERTEXPROCESSING|D3DCREATE_FPU_PRESERVE,&present,&device.real);
    if(FAILED(created))fprintf(stderr,"CreateDevice HRESULT=0x%08lx\n",static_cast<unsigned long>(created));
    require(SUCCEEDED(created),"actual native device creation");
    char name[]="shader_fixture";auto *vs=Material_LoadVertexShader(name);require(vs!=nullptr,"actual compiled vertex creation");checkLoaded(vs,name);
    auto *ps=Material_LoadPixelShader(name);require(ps!=nullptr,"actual compiled pixel creation");checkLoaded(ps,name);
    require(device.calls==2 && compilerCalls==2 && cacheWrites==2 && temporaryFrees==2 && !temporary,"actual compile/create counts and cleanup");
    references(0);releaseRetained();checkGuards();
    auto *gpu=device.real;
    require(SUCCEEDED(gpu->SetVertexShader(vs->prog.vs)) && SUCCEEDED(gpu->SetPixelShader(ps->prog.ps)),"actual shader binding");
    float vertexConstant[]={0,0,0,0},pixelConstant[]={.125f,.25f,.5f,1};
    require(SUCCEEDED(gpu->SetVertexShaderConstantF(0,vertexConstant,1)) && SUCCEEDED(gpu->SetPixelShaderConstantF(0,pixelConstant,1)),"actual constant binding");
    require(SUCCEEDED(gpu->SetFVF(D3DFVF_XYZ|D3DFVF_TEX1)),"actual vertex declaration");
    gpu->SetRenderState(D3DRS_ZENABLE,FALSE);gpu->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);
    gpu->SetRenderState(D3DRS_LIGHTING,FALSE);gpu->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);
    struct Vertex {float x,y,z,u,v;};const Vertex vertices[]={{-1,-1,.5f,0,0},{-1,1,.5f,0,1},{1,1,.5f,1,1},{1,-1,.5f,1,0}};
    require(SUCCEEDED(gpu->Clear(0,nullptr,D3DCLEAR_TARGET,0,1,0)) && SUCCEEDED(gpu->BeginScene()),"actual scene begin");
    require(SUCCEEDED(gpu->DrawPrimitiveUP(D3DPT_TRIANGLEFAN,2,vertices,sizeof(Vertex))),"actual native shader draw");
    require(SUCCEEDED(gpu->EndScene()),"actual scene end");
    IDirect3DSurface9 *target=nullptr,*readback=nullptr;
    require(SUCCEEDED(gpu->GetRenderTarget(0,&target)),"actual render target");D3DSURFACE_DESC desc{};target->GetDesc(&desc);
    require(SUCCEEDED(gpu->CreateOffscreenPlainSurface(desc.Width,desc.Height,desc.Format,D3DPOOL_SYSTEMMEM,&readback,nullptr)),"actual readback surface");
    require(SUCCEEDED(gpu->GetRenderTargetData(target,readback)),"actual GPU pixel readback");D3DLOCKED_RECT locked{};
    require(SUCCEEDED(readback->LockRect(&locked,nullptr,D3DLOCK_READONLY)),"readback lock");
    auto *pixel=static_cast<unsigned char*>(locked.pBits)+16*locked.Pitch+16*4;
    printf("GPU RGB %u %u %u\n",pixel[2],pixel[1],pixel[0]);
    require(abs(int(pixel[2])-32)<=1 && abs(int(pixel[1])-64)<=1 && abs(int(pixel[0])-128)<=1,"compiled native shaders produce expected pixels");
    for(auto *module:{"d3d9.dll","d3dx9_43.dll","vulkan-1.dll"}) {char path[MAX_PATH];auto handle=GetModuleHandleA(module);if(handle && GetModuleFileNameA(handle,path,sizeof(path)))printf("MODULE %s %s\n",module,path);}
    readback->UnlockRect();readback->Release();target->Release();gpu->SetVertexShader(nullptr);gpu->SetPixelShader(nullptr);
    vs->prog.vs->Release();ps->prog.ps->Release();gpu->Release();d3d->Release();DestroyWindow(window);UnregisterClassA(windowClass.lpszClassName,windowClass.hInstance);
}
#endif
int main(int argc,char **argv) {
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);require(argc==2,"case argument");mode=argv[1];
#ifdef SHADER_GPU
    require(mode=="gpu","GPU case argument");gpuCase();
#else
    if(mode.rfind("vertex_",0)==0)loaderCase(true);else if(mode.rfind("pixel_",0)==0)loaderCase(false);else compileCase();
#endif
    printf("PASS %s: production shader loading/compiler, real D3DX COM, pointer width%zu\n",mode.c_str(),sizeof(void*));
}
