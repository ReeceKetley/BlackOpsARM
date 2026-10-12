#include <database/db_material_graph_load.h>
#include <universal/assertive.h>
#include <universal/dvar.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#if defined(_WIN32)
#include <crtdbg.h>
#endif
static void unavailable(){
#if defined(_WIN32)
_set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
#endif
std::fputs("Material graph fixture: renderer-enabled GPU operation unavailable\n",stderr);std::fflush(stderr);std::abort();}
#if defined(_WIN32)
using HRESULT=long;
#else
using HRESULT=int;
#endif
#include <universal/com_errors.h>
void Com_Error(errorParm_t,const char*,...);
struct UnavailableDevice {
 HRESULT CreatePixelShader(DWORD*,IDirect3DPixelShader9**){unavailable();return -1;}
 HRESULT CreateVertexShader(DWORD*,IDirect3DVertexShader9**){unavailable();return -1;}
 unsigned GetAvailableTextureMem(){unavailable();return 0;}
};
static struct {UnavailableDevice*device;} dx;
static int g_hr,mem,g_disableRendering;
static bool skipShaderCreationHack;
dvar_s* r_loadForRenderer;
void SetGraphRenderer(bool enabled,bool absent){static dvar_s load{};load.current.enabled=enabled;r_loadForRenderer=absent?nullptr:&load;}
static dvar_s* r_logFile;
struct UnavailableStreamSource {unsigned char stream,offset,type;};
static const UnavailableStreamSource s_streamSourceInfo[18][10]{};
static void ProfLoad_Begin(const char*){unavailable();}
static void ProfLoad_End(){unavailable();}
static bool Sys_IsRenderThread(){unavailable();return false;}
static void RB_Resource_CreatePixelShader(unsigned*,IDirect3DPixelShader9**){unavailable();}
static void RB_Resource_CreateVertexShader(unsigned*,IDirect3DVertexShader9**){unavailable();}
static void RB_Resource_Flush(){unavailable();}
static void DB_LoadedExternalData(unsigned){unavailable();}
static void RB_LogPrint(const char*){unavailable();}
static const char* R_ErrorDescription(int){unavailable();return nullptr;}
static IDirect3DVertexDeclaration9* Material_BuildVertexDecl(const MaterialStreamRouting*,int,const UnavailableStreamSource*){unavailable();return nullptr;}
#include <gfx_d3d/r_material_shader_load.inl>
