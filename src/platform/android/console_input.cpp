#include "console_input.h"
#include "headless_policy.h"
#include "window_lifecycle.h"
#include <win32/win_local.h>
#include <universal/memory_services.h>
#include <array>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <thread>
#include <unistd.h>
void Sys_QueEvent(unsigned,sysEventType_t,int,int,int,void *);
namespace bo1::android {
namespace {
int input=-1;
std::thread::id owner;
std::array<char,1024> line;
unsigned length;
bool discard,afterCR;
[[noreturn]] void fail(const char *what){std::fprintf(stderr,"Native console input: %s (%d)\n",what,errno);std::abort();}
void requireOwner(){if(owner!=std::this_thread::get_id())fail("input polling requires installed owner");}
void emit(){
    if(!discard && length){
        auto *payload=Z_Malloc(length+1,"native console input",11);
        memcpy(payload,line.data(),length);reinterpret_cast<char *>(payload)[length]=0;
        Sys_QueEvent(0,SE_CONSOLE,0,0,length+1,payload);
    }
    length=0;discard=false;
}
}
void InstallConsoleInput(int descriptor){
    if(owner!=std::thread::id{} || descriptor<0)fail("invalid console installation");
    input=fcntl(descriptor,F_DUPFD_CLOEXEC,3);if(input<0)fail("cannot own console descriptor");
    owner=std::this_thread::get_id();length=0;discard=afterCR=false;
}
void PollConsoleInput(){
    requireOwner();if(input<0)return; // Actual EOF: no more input on this stream.
    // Bound per-frame work so a continuously writing console cannot starve simulation.
    for(unsigned processed=0;processed<4096;++processed){
        pollfd ready{input,POLLIN,0};int result;
        do{result=poll(&ready,1,0);}while(result<0 && errno==EINTR);
        if(result<0)fail("poll failed");if(!result)return;
        if(ready.revents&(POLLERR|POLLNVAL))fail("console descriptor error");
        if(!(ready.revents&(POLLIN|POLLHUP)))return;
        char value;ssize_t size;
        do{size=read(input,&value,1);}while(size<0 && errno==EINTR);
        if(size<0 && (errno==EAGAIN || errno==EWOULDBLOCK))return;
        if(size<0)fail("read failed");
        if(!size){emit();close(input);input=-1;return;}
        if(value=='\n' && afterCR){afterCR=false;continue;}
        afterCR=value=='\r';
        if(value=='\r' || value=='\n'){emit();continue;}
        if(discard)continue;
        if(!value){discard=true;length=0;std::fputs("Native console discarded line containing NUL\n",stderr);continue;}
        if(length==line.size()-1){discard=true;length=0;std::fputs("Native console discarded oversized line\n",stderr);continue;}
        line[length++]=value;
    }
}
bool ConsoleInputOwnedByCaller(){return owner==std::this_thread::get_id();}
void RetireConsoleInput(){requireOwner();if(input>=0)close(input);input=-1;owner={};length=0;discard=afterCR=false;}
}
#if defined(BO1_NATIVE_RENDER_OWNER_DISPATCH)
#include "renderer_window.h"
#endif
#if defined(BO1_NATIVE_SYSTEM_CORE)
#include "ui_platform.h"
#endif
void Platform_InputFrame(){
#if defined(BO1_NATIVE_RENDER_OWNER_DISPATCH)
    bo1::android::PumpRendererOwnerRequestsIfOwner();
#endif
    if(Sys_IsHeadless()){bo1::android::PollConsoleInput();return;}
#if defined(BO1_NATIVE_SYSTEM_CORE)
    bo1::android::PumpAndroidConsoleCommandsIfOwner();
#endif
    if(!bo1::android::PumpSDLEventsIfOwner()){
        std::fputs("Native input requires the real installed SDL event owner\n",stderr);std::abort();
    }
#if defined(BO1_NATIVE_RENDER_OWNER_DISPATCH)
    bo1::android::PumpRendererOwnerRequestsIfOwner();
#endif
}
