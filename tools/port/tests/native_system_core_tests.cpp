// Actual Sys_Init, Dvar/Cmd/GPad and installed console producer. The filesystem
// adapter owns a genuine isolated temporary directory; unexecuted engine paths
// fail explicitly. Fault probes deliberately return from Com_Error.
#include <win32/win_main.cpp>
#include <platform/platform.hpp>
#include <platform/android/console_input.h>
#include <platform/android/system_initialization.h>
#include <platform/android/thread_services.h>
#include <platform/android/zone_heap.h>
#include <platform/critical_section_services.h>
#include <clientscript/cscr_stringlist.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <thread>
#include <vector>
#include <cstdarg>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <atomic>

static unsigned checks;
static bool fixtureFiles,returnFatal,failUname;
static int *fatalMarker;
static char fixtureRoot[128];
static std::atomic<unsigned long long> entered{},exited{};
extern "C" void __cyg_profile_func_enter(void*,void*){entered.fetch_add(1,std::memory_order_relaxed);}
extern "C" void __cyg_profile_func_exit(void*,void*){exited.fetch_add(1,std::memory_order_relaxed);}
const dvar_t *com_sv_running,*com_logfile,*bo1_zombies,*useFastFile;
static void check(bool ok,const char *name){if(!ok){fprintf(stderr,"FAIL %s\n",name);exit(1);}++checks;printf("PASS %s\n",name);fflush(stdout);}
[[noreturn]] static void unsupported(const char *name){fprintf(stderr,"UNEXECUTED dependency reached: %s\n",name);abort();}
static void print(const char *fmt,va_list a){vfprintf(stderr,fmt,a);}
#define LOG(name) void name(int,const char *fmt,...){va_list a;va_start(a,fmt);print(fmt,a);va_end(a);}
LOG(Com_Printf) LOG(Com_DPrintf) LOG(Com_PrintError) LOG(Com_PrintWarning)
void Com_PrintMessage(int,char *msg,int){fputs(msg,stderr);}
bool Com_LogFileOpen(){return false;} // this fixture logger owns stderr, no engine logfile
void Scr_MonitorCommand(const char*,scriptInstance_t){unsupported("script command monitor");}
void Com_Error(errorParm_t,const char *fmt,...){va_list a;va_start(a,fmt);print(fmt,a);va_end(a);if(returnFatal){*fatalMarker=1;return;}unsupported("Com_Error recovery");}
bool Assert_MyHandler(const char *file,int line,int,const char *fmt,...){fprintf(stderr,"assert %s:%d ",file,line);va_list a;va_start(a,fmt);print(fmt,a);va_end(a);abort();}
const char *Com_PlayerConfigName(){unsupported("player configuration");}
void Com_PlayerConfigPath(char*,int){unsupported("player configuration path");}
void Scr_DumpScriptThreads(scriptInstance_t){unsupported("script dump");}
void Scr_DumpScriptVariablesDefault(scriptInstance_t){unsupported("script variable dump");}
void Scr_TerminalError(const char*,scriptInstance_t){unsupported("script terminal error");}
void CL_ForwardCommandToServer(int,const char*){unsupported("client forwarding");}
int SV_GameCommand(){unsupported("server command");}
void SV_WaitServer(){unsupported("server wait");}
int SV_Cmd_Argc(){unsupported("server arguments");}
const char *SV_Cmd_Argv(int){unsupported("server arguments");}
void Con_Restricted_ExecuteBuf(int,int,char*){unsupported("restricted console");}
bool DB_IsMinimumFastFileLoaded(){unsupported("database readiness");}
XAssetHeader DB_FindXAssetHeader(XAssetType,char*,bool,int){unsupported("database asset");}
const char *SEH_StringEd_GetString(char*){unsupported("localization");}
char *SEH_LocalizeTextMessage(const char*,const char*,msgLocErrType_t){unsupported("localization");}
char *GetExpressionResultStringCompile(int,itemDef_s*,const char**,void*,int){unsupported("expression");}
void FS_Printf(int,char*,...){unsupported("configuration write");}
const char **FS_ListFiles(const char*,char*,FsListBehavior_e,int*){unsupported("configuration enumeration");}
void Sys_Listen_f(){unsupported("network listen callback execution");}
void Sys_Connect_f(){unsupported("network connect callback execution");}
void NET_Restart(){unsupported("network restart callback execution");}
void mjpeg_init(){unsupported("movie startup callback execution");}
void mjpeg_create(const char*,int,int,unsigned){unsupported("movie capture callback execution");}
void mjpeg_close(){unsupported("movie close callback execution");}
const dvar_t *r_clipFPS,*r_clipSize;
vidConfig_t vidConfig{};
void Com_Quit_f(){unsupported("engine quit callback execution");}
extern "C" int __real_uname(utsname *);
extern "C" int __wrap_uname(utsname *out){if(failUname){errno=EIO;return -1;}return __real_uname(out);}
bool FS_Initialized(){return fixtureFiles;}
static void filePath(const char *name,char *out){check(fixtureFiles&&!strchr(name,'/'),"fixture filesystem accepts bounded local file name");snprintf(out,256,"%s/%s",fixtureRoot,name);}
int FS_WriteFile(char *name,char *data,unsigned size){char path[256];filePath(name,path);FILE *f=fopen(path,"wb");if(!f)return -1;size_t n=fwrite(data,1,size,f);fclose(f);return n==size?static_cast<int>(size):-1;}
int FS_ReadFile(const char *name,void **out){char path[256];filePath(name,path);FILE *f=fopen(path,"rb");if(!f){*out=nullptr;return -1;}fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);*out=malloc(n+1);check(*out!=nullptr,"real FS read buffer allocated");if(fread(*out,1,n,f)!=static_cast<size_t>(n)){free(*out);*out=nullptr;fclose(f);return -1;}static_cast<char *>(*out)[n]=0;fclose(f);return n;}
void FS_FreeFile(void *buffer){free(buffer);}
static bool rejects(void(*action)()){*fatalMarker=0;pid_t pid=fork();if(!pid){returnFatal=true;action();_Exit(99);}check(pid>0,"fork real invalid prerequisite probe");int status;check(waitpid(pid,&status,0)==pid,"join invalid prerequisite probe");return WIFSIGNALED(status)&&WTERMSIG(status)==SIGABRT&&*fatalMarker==1;}
static void noFilesystem(){fixtureFiles=false;bo1::android::BootstrapNativePlatform();}
static void badOS(){failUname=true;bo1::android::InitializeSystemOS();}
static void wrongOwner(){std::thread other([]{IN_Frame_Native();});other.join();}
static void missingRendered(){IN_Shutdown();s_headless=false;IN_Init();}
void Sys_FindInfo();
int main(){
    fatalMarker=static_cast<int *>(mmap(nullptr,4096,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0));check(fatalMarker!=MAP_FAILED,"shared failure marker created");
    Sys_InitMainThread();Sys_InitializeCriticalSections();Sys_InitializeHeadlessPolicy("+set bo1_headless 1");SL_Init(SCRIPTINSTANCE_SERVER);SL_Init(SCRIPTINSTANCE_CLIENT);Cmd_Init();Dvar_Init();
    check(!Platform_IsReady()&&!Platform(),"actual original registry starts empty");
    check(rejects(noFilesystem)&&!Platform()&&!FileSystem()&&!InputBackend(),"returning fatal FS callback cannot publish descriptors");
    check(rejects(badOS),"returning fatal OS callback cannot continue");
    strcpy(fixtureRoot,"/data/local/tmp/blackopsarm-core-files-XXXXXX");check(mkdtemp(fixtureRoot)!=nullptr,"initialize genuine isolated filesystem directory");fixtureFiles=true;
    int pipes[2];check(pipe(pipes)==0,"create real console pipe");bo1::android::InstallConsoleInput(pipes[0]);close(pipes[0]);write(pipes[1],"core_init\n",10);
    Sys_FindInfo();Sys_Init();
    struct Expected{const char *name;void(*callback)();};Expected expected[]={{"in_restart",Sys_In_Restart_f},{"net_restart",Sys_Net_Restart_f},{"movie_start",Sys_Mjpeg},{"movie_stop",Sys_MjpegClose},{"net_listen",Sys_Listen_f},{"net_connect",Sys_Connect_f}};
    for(auto item:expected){auto *command=Cmd_FindCommand(item.name);check(command&&command->function==item.callback,"actual Sys_Init command binds real core callback");}
    check(Dvar_FindVar("in_mouse")&&Dvar_FindVar("gpad_present")&&!Dvar_FindVar("gpad_present")->current.enabled,"actual Dvar/GPad headless initialization registers real state");
    sysEvent_t event{};Sys_GetEvent(&event);check(event.evType==SE_CONSOLE&&!strcmp(static_cast<char *>(event.evPtr),"core_init"),"actual IN_Init polls installed producer into canonical event queue");Z_Free(static_cast<char *>(event.evPtr),11);
    check(Platform()&&FileSystem()&&InputBackend()&&!AudioBackend()&&!RenderBackend()&&!PlatformServices()&&!VideoBackend()&&!Platform_IsReady(),"only three actual descriptors published and completeness remains false");
    const char text[]="core_fs";check(FileSystem()->WriteFile("fixture",text,sizeof(text))==sizeof(text),"registered FS adapter writes actual isolated file");void *read=nullptr;check(FileSystem()->ReadFile("fixture",&read)==sizeof(text)&&!strcmp(static_cast<char *>(read),text),"registered FS adapter reads owned actual file");FileSystem()->FreeFile(read);
    Sys_In_Restart_f();write(pipes[1],"core_restart\n",13);InputBackend()->Frame();Sys_GetEvent(&event);check(event.evType==SE_CONSOLE&&!strcmp(static_cast<char *>(event.evPtr),"core_restart"),"actual restart retains real input producer and owner");Z_Free(static_cast<char *>(event.evPtr),11);
    check(rejects(wrongOwner)&&rejects(missingRendered),"returning fatal owner/rendered callbacks cannot publish input state");
    InputBackend()->Shutdown();close(pipes[1]);bo1::android::RetireConsoleInput();printf("SYSTEM_CORE PASS checks=%u\n",checks);
}
