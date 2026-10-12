#include <platform/startup_services_api.h>
#include <clientscript/cscr_stringlist.h>
#include <clientscript/cscr_debugger.h>
#include <clientscript/cscr_memorytree.h>
#include <universal/thread_data.h>
#include <array>
#include <chrono>
#include <thread>
#if defined(_WIN32)
#include <crtdbg.h>
#endif

// These are application globals, initially null as in common.cpp. Any test
// settings below point to actual registered Dvars, never manufactured objects.
const dvar_t *com_sv_running;
const dvar_t *com_logfile;
const dvar_t *bo1_zombies;
const dvar_t *useFastFile;

[[noreturn]] static void unavailable(const char *name) {
    std::fprintf(stderr,"Startup service dependency unavailable: %s\n",name);
    std::fflush(stderr);std::abort();
}
static void print(const char *fmt, va_list arguments) {std::vfprintf(stderr,fmt,arguments);}
#define LOG(name) void name(int,const char *fmt,...){va_list a;va_start(a,fmt);print(fmt,a);va_end(a);}
LOG(Com_Printf) LOG(Com_DPrintf) LOG(Com_PrintError) LOG(Com_PrintWarning)
void Com_PrintMessage(int,char *msg,int){std::fputs(msg,stderr);}
void Com_Error(errorParm_t,const char *fmt,...){va_list a;va_start(a,fmt);print(fmt,a);va_end(a);unavailable("Com_Error recovery");}
bool Assert_MyHandler(const char *file,int line,int,const char *fmt,...){std::fprintf(stderr,"Startup assertion %s:%d ",file,line);va_list a;va_start(a,fmt);print(fmt,a);va_end(a);std::abort();}
const char *Com_PlayerConfigName(){unavailable("player configuration name");}
void Com_PlayerConfigPath(char*,int){unavailable("player configuration path");}
void Scr_DumpScriptThreads(scriptInstance_t){unavailable("VM thread dump");}
void Scr_DumpScriptVariablesDefault(scriptInstance_t){unavailable("VM variable dump");}
void Scr_TerminalError(const char*,scriptInstance_t){unavailable("VM terminal recovery");}
unsigned int Sys_Milliseconds(){unavailable("engine millisecond clock");}
void CL_ForwardCommandToServer(int,const char*){unavailable("client command forwarding");}
int SV_GameCommand(){unavailable("server game command");}
void SV_WaitServer(){unavailable("server coordination");}
int SV_Cmd_Argc(){unavailable("server arguments");}
const char *SV_Cmd_Argv(int){unavailable("server arguments");}
void Con_Restricted_ExecuteBuf(int,int,char*){unavailable("restricted console");}
bool DB_IsMinimumFastFileLoaded(){unavailable("whole database initialization");}
XAssetHeader DB_FindXAssetHeader(XAssetType,char*,bool,int){unavailable("whole database asset-header wrapper");}
const char *SEH_StringEd_GetString(char*){unavailable("localization token");}
char *SEH_LocalizeTextMessage(const char*,const char*,msgLocErrType_t){unavailable("localized message");}
char *GetExpressionResultStringCompile(int,itemDef_s*,const char**,void*,int){unavailable("command expression integration");}
int FS_ReadFile(const char*,void**){unavailable("filesystem read");}
void FS_FreeFile(void*){unavailable("filesystem ownership");}
void FS_Printf(int,char*,...){unavailable("filesystem write");}
const char **FS_ListFiles(const char*,char*,FsListBehavior_e,int*){unavailable("filesystem enumeration");}

static void require(bool value,const char *why){if(!value){std::fprintf(stderr,"Integrated startup services failed: %s\n",why);std::abort();}}
void InitializeEntryOwnerInput();
void InitializeRegistryAddInputs();
int RunNativeStartupZoneChecks(int argc,char **argv);
int main(int argc,char **argv){
    std::setvbuf(stdout,nullptr,_IONBF,0);
    std::thread([]{std::this_thread::sleep_for(std::chrono::seconds(20));std::fputs("Integrated startup service test watchdog expired\n",stderr);std::_Exit(2);}).detach();
#if defined(_WIN32)
    _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
#endif
    require(argc==2,"genuine retail MP zone required");
    Sys_InitializeCriticalSections();Sys_InitMainThread();Com_InitParse();Dvar_Init();
    require(dvarCount==2,"actual initial two Dvars");unsigned commands=0;for(auto*p=cmd_functions;p;p=p->next)++commands;require(commands==18,"actual18earlycommandnodes");
    SL_Init(SCRIPTINSTANCE_SERVER);SL_Init(SCRIPTINSTANCE_CLIENT);Cbuf_Init();Cmd_Init();
    commands=0;for(auto*p=cmd_functions;p;p=p->next)++commands;require(commands==23,"actual23commandnodes");
    useFastFile=_Dvar_RegisterBool("useFastFile",true,0x10,"Enables loading data from fast files.");InitializeRegistryAddInputs();InitializeEntryOwnerInput();
    require(dvarCount==5&&useFastFile==Dvar_FindVar("useFastFile")&&useFastFile->type==DVAR_TYPE_BOOL&&useFastFile->flags==0x10&&useFastFile->current.enabled,"actual requiredFastFilesetting");
    require(db_xassetdebug==Dvar_FindVar("db_xassetdebug")&&db_xassetdebug->type==DVAR_TYPE_BOOL&&!db_xassetdebug->current.enabled&&db_xassetdebug->flags==0,"actual database boolsetting");
    require(db_xassetdebugtype==Dvar_FindVar("db_xassetdebugtype")&&db_xassetdebugtype->type==DVAR_TYPE_INT&&db_xassetdebugtype->current.integer==-1&&db_xassetdebugtype->domain.integer.min==-2&&db_xassetdebugtype->domain.integer.max==43&&db_xassetdebugtype->flags==0,"actual database typesetting");
    require(Sys_GetThreadContext()==0&&Sys_GetValue(1)==&va_info[0]&&Sys_GetValue(2)==g_com_error[0]&&Sys_GetValue(3)==&g_traceThreadInfo[0]&&Sys_GetValue(4)==g_cmd_args,"actualcanonicalmainTLS");
    require(!gScrStringDebugGlob[0]->totalRefCount&&!gScrStringDebugGlob[1]->totalRefCount,"SLbaselinebeforezone");
    std::printf("{\"startup_services_integration\":{\"pointer_bytes\":%zu,\"thread_context\":0,\"registered_dvars\":5,\"commands\":23,\"sl_instances\":2,\"actual_parse\":true,\"actual_dvar_init\":true,\"actual_cbuf_cmd_init\":true,\"actual_registered_loader_dvars\":3,\"com_init_executed\":false}}\n",sizeof(void*));
    require(RunNativeStartupZoneChecks(argc,argv)==0,"allgenuineMP71cycles");
    require(dvarCount==5&&useFastFile==Dvar_FindVar("useFastFile")&&!gScrStringDebugGlob[0]->totalRefCount&&!gScrStringDebugGlob[1]->totalRefCount,"genuinezonepreservesregisteredsettingsandSLownership");
    Dvar_Shutdown();Cmd_Shutdown();SL_CheckLeaks(SCRIPTINSTANCE_SERVER);SL_CheckLeaks(SCRIPTINSTANCE_CLIENT);SL_Shutdown(SCRIPTINSTANCE_CLIENT);SL_Shutdown(SCRIPTINSTANCE_SERVER);
    require(!dvarCount&&!cmd_functions,"actualpostzoneservicecleanup");
}
