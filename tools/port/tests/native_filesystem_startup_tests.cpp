// Executes the original filesystem initializer, not a replacement Com_Init.
#include <platform/startup_services_api.h>
#include <platform/android/command_line_services.h>
#include <platform/android/engine_paths.h>
#include <platform/android/headless_policy.h>
#include <platform/android/zone_heap.h>
#include <platform/android/virtual_memory.h>
#include <universal/com_files.h>
#include <universal/com_memory.h>
#include <stringed/stringed_hooks.h>
#include <stringed/stringed_ingame.h>
#include <clientscript/cscr_stringlist.h>
#include <unistd.h>
#include <chrono>
#include <thread>
extern "C" void __cyg_profile_func_enter(void *, void *) {}
extern "C" void __cyg_profile_func_exit(void *, void *) {}
extern CStringEdPackage *TheStringPackage;
extern languageInfo_t g_languages[13];
extern const dvar_t *useFastFile;
void Com_ParseCommandLine(char *);
void check(bool result,const char *name) {
    if(!result){std::fprintf(stderr,"FAIL %s\n",name);std::exit(1);}
    std::printf("PASS %s\n",name);
}
int main(int argc,char **argv) {
    if(argc!=3)return 2;
    std::setvbuf(stdout,nullptr,_IONBF,0);
    std::thread([] {std::this_thread::sleep_for(std::chrono::seconds(45));
        std::fputs("FS startup watchdog expired\n",stderr);std::_Exit(3);}).detach();
    char commandLine[]="+set bo1_headless 1 +set dedicated 1";
    bo1::android::InstallEngineCommandLine(argv[0],commandLine);
    Sys_InitializeHeadlessPolicy(commandLine);
    bo1::android::InstallEngineDataRoot(argv[1]);
    bo1::android::InstallEngineUserRoot(argv[2]);
    check(chdir(argv[1])==0,"actual game-root working directory");
    Sys_InitializeCriticalSections();Sys_InitMainThread();Com_InitParse();
    Dvar_Init();SL_Init(SCRIPTINSTANCE_SERVER);SL_Init(SCRIPTINSTANCE_CLIENT);
    Cbuf_Init();Cmd_Init();Com_ParseCommandLine(commandLine);
    // Actual configuration prerequisites, with Com_Init's original defaults.
    // This remains a filesystem integration test, not full engine boot evidence.
    useFastFile=_Dvar_RegisterBool("useFastFile",1,0x10u,"Enables loading data from fast files.");
    const auto heapBaseline=bo1::android::ZoneAllocationCount();
    const auto vmBaseline=bo1::android::VirtualReservationCount();
    std::puts("FS STARTUP calling original FS_InitFilesystem");
    FS_InitFilesystem(false);
    check(FS_Initialized(),"original filesystem initializer returned with real search paths");
    void *buffer=nullptr;const int length=FS_ReadFile("fileSysCheck.cfg",&buffer);
    check(length>0 && buffer && static_cast<char *>(buffer)[length]==0,"genuine retail filesystem marker read through mounted assets");
    FS_FreeFile(buffer);
    check(TheStringPackage && !TheStringPackage->m_StringEntries.empty(),"original localization parser populated real string package");
    check(g_languages[0].bPresent && !std::strcmp(SEH_GetLanguageName(SEH_GetCurrentLanguage()),"english"),"actual localized asset discovery and selected language");
    auto *home=Dvar_FindVar("fs_h");
    check(home && !std::strcmp(home->current.string,Sys_EngineUserRoot()),"writable user root remains separate from original assets");
    std::printf("FS STARTUP localized strings %zu marker bytes %d\n",TheStringPackage->m_StringEntries.size(),length);
    SEH_Shutdown_StringEd();FS_Shutdown();
    check(!TheStringPackage && !FS_Initialized(),"original localization and filesystem shutdown");
    check(bo1::android::VirtualReservationCount()==vmBaseline,"filesystem virtual mappings released");
    std::printf("FS STARTUP heap baseline %zu final %zu\n",heapBaseline,bo1::android::ZoneAllocationCount());
    Dvar_Shutdown();SL_Shutdown(SCRIPTINSTANCE_CLIENT);SL_Shutdown(SCRIPTINSTANCE_SERVER);
}
