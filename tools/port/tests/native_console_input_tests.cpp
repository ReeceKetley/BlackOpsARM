#define SDL_MAIN_HANDLED
#include <platform/android/console_input.h>
#include <platform/android/headless_policy.h>
#include <platform/critical_section_services.h>
#include <platform/android/zone_heap.h>
#include <universal/memory_services.h>
#include <win32/win_local.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
sysEvent_t *Sys_GetEvent(sysEvent_t *);
void Com_Printf(int,const char *,...){}
void Com_Quit_f(){std::abort();}
static void check(bool ok,const char *name){if(!ok){std::fprintf(stderr,"FAIL %s\n",name);std::exit(1);}std::printf("PASS %s\n",name);}
static void receive(const char *text){sysEvent_t event;Sys_GetEvent(&event);check(event.evType==SE_CONSOLE && event.evPtrLength==static_cast<int>(strlen(text)+1) && !strcmp(static_cast<char *>(event.evPtr),text),"actual console producer reaches owned engine event");Z_Free(static_cast<char *>(event.evPtr),11);}
int main(){
    Sys_InitializeCriticalSections();Sys_InitializeHeadlessPolicy("+set bo1_headless 1");
    check(Sys_IsHeadless(),"real headless policy selected");
    int pipefd[2];check(!pipe(pipefd),"actual input pipe created");
    int flags=fcntl(pipefd[0],F_GETFL);bo1::android::InstallConsoleInput(pipefd[0]);
    check(fcntl(pipefd[0],F_GETFL)==flags,"console ownership does not change caller file flags");
    close(pipefd[0]);auto baseline=bo1::android::ZoneAllocationCount();
    write(pipefd[1],"sta",3);Platform_InputFrame();sysEvent_t event;Sys_GetEvent(&event);check(event.evType==SE_NONE,"partial line retained without fabricated command");
    write(pipefd[1],"tus\r",4);Platform_InputFrame();receive("status");
    write(pipefd[1],"\nmap test\n",10);Platform_InputFrame();receive("map test");Sys_GetEvent(&event);check(event.evType==SE_NONE,"CRLF publishes once");
    char oversized[1100];memset(oversized,'x',sizeof(oversized));write(pipefd[1],oversized,sizeof(oversized));write(pipefd[1],"\nok\n",4);Platform_InputFrame();receive("ok");
    const char invalid[]={'b',0,'d','\n'};write(pipefd[1],invalid,sizeof(invalid));write(pipefd[1],"tail",4);close(pipefd[1]);Platform_InputFrame();receive("tail");
    Platform_InputFrame();Sys_GetEvent(&event);check(event.evType==SE_NONE,"EOF flushes partial line once and retires stream");
    bo1::android::RetireConsoleInput();check(bo1::android::ZoneAllocationCount()==baseline,"console event payload ownership returns to baseline");
}
