#include <platform/startup_services_api.h>
#include <clientscript/cscr_stringlist.h>
#include <clientscript/cscr_debugger.h>
#include <clientscript/cscr_memorytree.h>
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
XAssetHeader DB_FindXAssetHeader(XAssetType,char*,bool,int){unavailable("database asset lookup");}
const char *SEH_StringEd_GetString(char*){unavailable("localization token");}
char *SEH_LocalizeTextMessage(const char*,const char*,msgLocErrType_t){unavailable("localized message");}
char *GetExpressionResultStringCompile(int,itemDef_s*,const char**,void*,int){unavailable("command expression integration");}
int FS_ReadFile(const char*,void**){unavailable("filesystem read");}
void FS_FreeFile(void*){unavailable("filesystem ownership");}
void FS_Printf(int,char*,...){unavailable("filesystem write");}
const char **FS_ListFiles(const char*,char*,FsListBehavior_e,int*){unavailable("filesystem enumeration");}

static void require(bool value,const char *why){if(!value){std::fprintf(stderr,"Startup service test failed: %s\n",why);std::abort();}}
static unsigned commands(){unsigned n=0;for(auto*p=cmd_functions;p;p=p->next){require(++n<128,"finite commands");}return n;}
static unsigned directCommands;
static void execute(const char *text){++directCommands;char buffer[4096];I_strncpyz(buffer,text,sizeof(buffer));Cmd_ExecuteSingleCommand(0,0,buffer);}
struct BindingExpected {const char *name;void (*function)();};
static const BindingExpected bindings[]={
    {"wait",Cmd_Wait_f},{"vstr",Cmd_Vstr_f},{"exec_addtext",Cmd_ExecAddText_f},{"exec",Cmd_Exec_f},{"cmdlist",Cmd_List_f},
    {"restoreDvars",Dvar_RestoreDvars},{"setu",Dvar_SetU_f},{"dvar_color",Dvar_RegisterColor_f},{"dvar_float",Dvar_RegisterFloat_f},
    {"dvar_int",Dvar_RegisterInt_f},{"dvar_bool",Dvar_RegisterBool_f},{"dvardump",Dvar_Dump_f},{"dvarlist",Dvar_List_f},
    {"reset",Dvar_Reset_f},{"setdvartotime",Dvar_SetToTime_f},{"setfromlocString",Dvar_SetFromLocalizedStr_f},
    {"setfromdvar",Dvar_SetFromDvar_f},{"setadmindvar",Dvar_SetAdmin_f},{"seta",Dvar_SetA_f},{"sets",Dvar_SetS_f},
    {"set",Dvar_Set_f},{"togglep",Dvar_TogglePrint_f},{"toggle",Dvar_Toggle_f}
};
static void validateBindings(unsigned first){
    unsigned i=first;
    for(auto *node=cmd_functions;node;node=node->next,++i){
        require(i<23&&!std::strcmp(node->name,bindings[i].name)&&node->function==bindings[i].function,"actual command node callback and order");
        require(Cmd_FindCommand(bindings[i].name)==node,"actual command lookup identity");
    }
    require(i==23,"all expected command nodes");
}
static std::uint32_t bits(float f){std::uint32_t value;std::memcpy(&value,&f,4);return value;}
static void vectorTrace(unsigned index,const dvar_s *dvar){
    std::printf("{\"startup_vector\":{\"index\":%u,\"type\":%u,\"current\":[%u,%u,%u],\"latched\":[%u,%u,%u],\"reset\":[%u,%u,%u],\"current_tail\":%u,\"latched_tail\":%u}}\n",index,static_cast<unsigned>(dvar->type),bits(dvar->current.vector[0]),bits(dvar->current.vector[1]),bits(dvar->current.vector[2]),bits(dvar->latched.vector[0]),bits(dvar->latched.vector[1]),bits(dvar->latched.vector[2]),bits(dvar->reset.vector[0]),bits(dvar->reset.vector[1]),bits(dvar->reset.vector[2]),bits(dvar->current.vector[3]),bits(dvar->latched.vector[3]));
}
static void ownership(unsigned stage){
    std::printf("{\"startup_ownership\":{\"stage\":%u,\"server_refs\":%u,\"client_refs\":%u,\"server_allocations\":%d,\"server_buckets\":%d,\"client_allocations\":%d,\"client_buckets\":%d}}\n",stage,static_cast<unsigned>(gScrStringDebugGlob[0]->totalRefCount),static_cast<unsigned>(gScrStringDebugGlob[1]->totalRefCount),gScrMemTreeGlob[0].totalAlloc,gScrMemTreeGlob[0].totalAllocBuckets,gScrMemTreeGlob[1].totalAlloc,gScrMemTreeGlob[1].totalAllocBuckets);
}
int main(int argc,char **argv){
    std::setvbuf(stdout,nullptr,_IONBF,0);
    std::thread([]{std::this_thread::sleep_for(std::chrono::seconds(20));std::fputs("Startup service test watchdog expired\n",stderr);std::_Exit(2);}).detach();
#if defined(_WIN32)
    _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
#endif
    require(!Com_LogFileOpen(),"actual zero logfile state");logfile=17;require(Com_LogFileOpen(),"actual nonzero logfile state");logfile=0;
    Sys_InitializeCriticalSections();Sys_InitMainThread();Com_InitParse();Dvar_Init();
    require(dvarCount==2&&commands()==18,"original pre-Com_Init registration order");validateBindings(5);
    require(dvar_cheats->type==DVAR_TYPE_BOOL&&dvar_cheats->current.enabled&&dvar_cheats->latched.enabled&&dvar_cheats->reset.enabled&&dvar_cheats->flags==0x18&&dvar_cheats->hash==-1413893531,"actual initial cheats");
    require(!std::strcmp(dvar_cheats->name,"sv_cheats")&&!std::strcmp(dvar_cheats->description,"External Dvar")&&CanKeepStringPointer(const_cast<char *>(dvar_cheats->name)),"initial borrowed cheats name/description");
    require(dvar_restoreDvarsOnLive->type==DVAR_TYPE_BOOL&&dvar_restoreDvarsOnLive->current.enabled&&dvar_restoreDvarsOnLive->latched.enabled&&dvar_restoreDvarsOnLive->reset.enabled&&dvar_restoreDvarsOnLive->flags==0&&dvar_restoreDvarsOnLive->hash==1679256369,"actual restoreDvars");
    require(!std::strcmp(dvar_restoreDvarsOnLive->name,"sv_restoreDvars")&&!std::strcmp(dvar_restoreDvarsOnLive->description,"Enable to restore Dvars on entering the Xbox Live menu")&&CanKeepStringPointer(const_cast<char *>(dvar_restoreDvarsOnLive->name)),"initial borrowed restoreDvars name/description");
    for(unsigned i=0;i<4;++i)require(g_parse[i].parseInfo[0].lines==1&&g_parse[i].parseInfo[0].spaceDelimited,"actual parse initialization");
    SL_Init(SCRIPTINSTANCE_SERVER);SL_Init(SCRIPTINSTANCE_CLIENT);Cbuf_Init();
    require(gScrStringDebugGlob[0]->totalRefCount==0&&gScrStringDebugGlob[1]->totalRefCount==0,"empty SL baseline");
    require(!SL_FindString("sv_cheats",SCRIPTINSTANCE_SERVER)&&!SL_FindString("sv_restoreDvars",SCRIPTINSTANCE_SERVER),"initialbool borrowednames not interned");ownership(0);
    for(auto *args:{&g_cmd_args[0],&g_cmd_args[1],&sv_cmd_args}){args->nesting=7;args->totalUsedArgvPool=19;args->totalUsedTextPool=23;args->usedTextPool[2]=31;args->textPool[8191]='Q';}
    Cmd_Init();validateBindings(0);
    for(auto *args:{&g_cmd_args[0],&g_cmd_args[1],&sv_cmd_args})require(args->nesting==-1&&args->totalUsedArgvPool==0&&args->totalUsedTextPool==0&&args->usedTextPool[2]==31&&args->textPool[8191]=='Q',"actual3 argument resets and preserved unrelated storage");
    unsigned nodeIndex=0;for(auto *node=cmd_functions;node;node=node->next)std::printf("{\"startup_command_node\":{\"index\":%u,\"name\":\"%s\",\"callback_match\":true}}\n",nodeIndex++,node->name);
    if(argc==2){
        if(!std::strcmp(argv[1],"expression"))execute("set probe (1+2)");
        else if(!std::strcmp(argv[1],"localization"))execute("set probe @MISSING_KEY");
        else if(!std::strcmp(argv[1],"exec"))execute("exec unavailable.cfg");
        else if(!std::strcmp(argv[1],"time"))execute("setdvartotime probe");
        else if(!std::strcmp(argv[1],"server"))execute("unregistered_engine_command");
        else return 2;
        return 2;
    }
    require(argc==1,"known invocation");
    require(commands()==23&&g_cmd_args[0].nesting==-1&&g_cmd_args[1].nesting==-1&&sv_cmd_args.nesting==-1,"actual Cmd_Init preserves pre-existing nodes and resets3args");
    useFastFile=_Dvar_RegisterBool("useFastFile",true,0x10,"Enables loading data from fast files.");
    const auto *integer=_Dvar_RegisterInt("arm_probe_int",3,-5,9,0,"test-controlled driver registration");
    const auto *string=_Dvar_RegisterString("arm_probe_string","initial",0,"test-controlled driver registration");
    const auto *vector=_Dvar_RegisterVec3("arm_probe_vec3",1,2,3,-10,10,0,"test-controlled driver registration");
    execute("set arm_probe_int 7");require(integer->current.integer==7,"real set command");
    execute("toggle arm_probe_int");require(integer->current.integer==0,"real toggle integer");
    execute("toggle arm_probe_int");require(integer->current.integer==1,"real second toggle");
    execute("set arm_probe_int 999");require(integer->current.integer==1,"actual out-of-domain rejection preserves value");
    execute("set arm_probe_string \"genuine command words\"");require(!std::strcmp(string->current.string,"genuine command words"),"real quoted tokenizer and SL-owned string");
    execute("reset arm_probe_int");require(integer->current.integer==3,"real reset command");
    require(vector->current.vector[0]==1&&vector->current.vector[1]==2&&vector->current.vector[2]==3,"real vec3 byte copy");
    for(unsigned i=0;i<3;++i){
        static char names[3][32]={"arm_vector_float3","arm_vector_linear","arm_vector_xyz"};
        const dvarType_t type=i==0?DVAR_TYPE_FLOAT_3:(i==1?DVAR_TYPE_LINEAR_COLOR_RGB:DVAR_TYPE_COLOR_XYZ);
        DvarValue value{};value.vector[0]=-1.25f;value.vector[1]=2.5f;value.vector[2]=-3.75f;value.vector[3]=99;
        DvarLimits domain{};domain.vector.min=-10;domain.vector.max=10;
        auto *item=const_cast<dvar_s *>(Dvar_RegisterVariant(names[i],type,0,value,domain,"test-controlled vector registration"));
        require(!std::memcmp(item->current.vector,value.vector,12)&&!std::memcmp(item->latched.vector,value.vector,12),"actual initial12 vector bytes");
        const std::uint32_t currentTail=0x67452301u+i,latchedTail=0xefcdab89u+i;
        std::memcpy(&item->current.vector[3],&currentTail,4);std::memcpy(&item->latched.vector[3],&latchedTail,4);
        vectorTrace(i*3,item);
        value.vector[0]=4.125f;value.vector[1]=-5.5f;value.vector[2]=6.75f;value.vector[3]=-77;
        Dvar_UpdateValue(item,value);
        require(!std::memcmp(item->current.vector,value.vector,12)&&!std::memcmp(item->latched.vector,value.vector,12),"actual update12 vector bytes");vectorTrace(i*3+1,item);
        require(bits(item->current.vector[3])==currentTail&&bits(item->latched.vector[3])==latchedTail,"update preserves neighboring4 bytes");
        Dvar_Reset(item,DVAR_SOURCE_INTERNAL);require(bits(item->current.vector[3])==currentTail&&bits(item->latched.vector[3])==latchedTail,"reset preserves neighboring4 bytes");vectorTrace(i*3+2,item);
    }
    execute("set arm_dynamic_name alpha");auto *dynamic=const_cast<dvar_s *>(Dvar_FindVar("arm_dynamic_name"));
    require(dynamic&&dynamic->type==DVAR_TYPE_STRING&&(dynamic->flags&0x4000),"actual dynamic name policy");
    const unsigned nameId=SL_FindString("arm_dynamic_name",SCRIPTINSTANCE_SERVER);
    require(nameId&&dynamic->name==SL_ConvertToString(nameId,SCRIPTINSTANCE_SERVER),"actual interned dynamic name identity");
    require(dynamic->current.string==dynamic->latched.string&&dynamic->current.string==dynamic->reset.string,"actual initial value aliases");ownership(1);
    const auto refs=gScrStringDebugGlob[0]->totalRefCount;const auto allocations=gScrMemTreeGlob[0].totalAlloc;
    const char *same=dynamic->current.string;execute("set arm_dynamic_name alpha");
    require(dynamic->current.string==same&&gScrStringDebugGlob[0]->totalRefCount==refs&&gScrMemTreeGlob[0].totalAlloc==allocations,"equal value preserves actual ownership");ownership(2);
    execute("set arm_dynamic_name beta");require(!std::strcmp(dynamic->current.string,"beta")&&!std::strcmp(dynamic->reset.string,"alpha"),"actual replacement retains reset owner");ownership(3);
    execute("reset arm_dynamic_name");require(dynamic->current.string==dynamic->reset.string&&dynamic->current.string==dynamic->latched.string,"actual reset alias ownership");ownership(4);
    const auto *latch=_Dvar_RegisterString("arm_latched_string","start",0x20,"test-controlled latch");
    execute("set arm_latched_string pending");require(!std::strcmp(latch->current.string,"start")&&!std::strcmp(latch->latched.string,"pending"),"actual external latch");ownership(5);
    Dvar_MakeLatchedValueCurrent(const_cast<dvar_s *>(latch));require(!std::strcmp(latch->current.string,"pending")&&latch->current.string==latch->latched.string,"actual latch publication");ownership(6);
    Dvar_Reset(const_cast<dvar_s *>(latch),DVAR_SOURCE_INTERNAL);require(latch->current.string==latch->reset.string,"actual latched reset");ownership(7);
    const auto *equal=_Dvar_RegisterString("arm_equal_name_value","arm_equal_name_value",0x4000,"test-controlled equal name/value ownership");
    const unsigned equalId=SL_FindString("arm_equal_name_value",SCRIPTINSTANCE_SERVER);
    require(equalId&&equal->name==equal->current.string&&equal->current.string==SL_ConvertToString(equalId,SCRIPTINSTANCE_SERVER)&&equal->current.string==equal->latched.string&&equal->current.string==equal->reset.string&&gScrStringDebugGlob[0]->refCount[equalId]==2&&SL_GetUser(equalId,SCRIPTINSTANCE_SERVER)==0,"equal name/value realSL identity has two independent user0 references");
    auto *cheat=const_cast<dvar_s *>(_Dvar_RegisterInt("arm_cheat",1,0,9,0x80,"test-controlled cheat"));
    Dvar_SetBoolFromSource(const_cast<dvar_s *>(dvar_cheats),false,DVAR_SOURCE_INTERNAL);execute("set arm_cheat 8");require(cheat->current.integer==1,"actual cheat rejection");
    Dvar_SetBoolFromSource(const_cast<dvar_s *>(dvar_cheats),true,DVAR_SOURCE_INTERNAL);execute("set arm_cheat 8");require(cheat->current.integer==8,"actual enabled cheat update");
    Dvar_SetCheatState();require(cheat->current.integer==1,"actual cheat reset");
    _Dvar_RegisterString("arm_macro","set arm_probe_int 4",0,"test-controlled command string");
    Cbuf_AddText(0,"vstr arm_macro;wait 1;set arm_probe_int 5\n");Cbuf_ExecuteInternal(0,0);require(integer->current.integer==4&&cmd_textArray[0].cmdsize>0,"actual vstr/wait buffer progress");
    Cbuf_ExecuteInternal(0,0);require(integer->current.integer==5&&cmd_textArray[0].cmdsize==0,"actual deferred command progress");
    require(!gScrDebuggerGlob[0].abort&&!gScrDebuggerGlob[1].abort,"ordinary commands do not abort debugger");
    Scr_MonitorCommand("map_restart",SCRIPTINSTANCE_SERVER);require(gScrDebuggerGlob[0].abort,"actual monitor restart");gScrDebuggerGlob[0].abort=false;
    Scr_MonitorCommand("map_restart_suffix",SCRIPTINSTANCE_SERVER);require(!gScrDebuggerGlob[0].abort,"actual monitor command boundary");
#if defined(__ANDROID__) || defined(BO1_STARTUP_SERVICES_NATIVE_HOST_TEST)
    DvarDumpInfo dump{};dump.channel=0;dump.match="arm_probe_int";Com_DvarDumpSingle(integer,&dump);require(dump.count==1,"actual typed dump match metadata");
#endif

    require(useFastFile==Dvar_FindVar("useFastFile")&&useFastFile->current.enabled,"actual registered loader Dvar");
    std::printf("{\"startup_services\":{\"pointer_bytes\":%zu,\"initial_dvars\":2,\"commands_before_cmd_init\":18,\"commands_after_cmd_init\":23,\"actual_parse\":true,\"actual_sl_instances\":2,\"actual_command_operations\":%u,\"actual_loader_dvar\":true,\"com_init_executed\":false}}\n",sizeof(void*),directCommands);
    Dvar_Shutdown();Cmd_Shutdown();ownership(8);require(gScrStringDebugGlob[0]->totalRefCount==0&&gScrStringDebugGlob[1]->totalRefCount==0&&gScrMemTreeGlob[0].totalAlloc==0&&gScrMemTreeGlob[0].totalAllocBuckets==0&&gScrMemTreeGlob[1].totalAlloc==0&&gScrMemTreeGlob[1].totalAllocBuckets==0,"real SL/MT ownership baseline restored");SL_CheckLeaks(SCRIPTINSTANCE_SERVER);SL_CheckLeaks(SCRIPTINSTANCE_CLIENT);SL_Shutdown(SCRIPTINSTANCE_CLIENT);SL_Shutdown(SCRIPTINSTANCE_SERVER);
    require(dvarCount==0&&commands()==0,"actual service cleanup");
}
