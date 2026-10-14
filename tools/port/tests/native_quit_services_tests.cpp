#include <platform/android/quit_services.h>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
namespace { void check(bool ok,const char *what){if(!ok){std::fprintf(stderr,"FAIL %s\n",what);std::exit(1);}std::printf("PASS %s\n",what);} }
int main(){
    check(!Sys_IsQuitRequested(),"initial quit state is clear");
    std::atomic<bool> seen{};
    std::thread waiter([&]{while(!Sys_IsQuitRequested())std::this_thread::yield();seen=true;});
    std::thread producer([]{Sys_SetQuitRequested();});producer.join();waiter.join();
    check(seen && Sys_IsQuitRequested(),"real cross-thread quit request publication");
    Sys_SetQuitRequested();check(Sys_IsQuitRequested(),"repeated request remains latched");
    auto child=fork();check(child>=0,"emergency-exit subprocess created");
    if(!child){atexit([]{_Exit(99);});Sys_QuitWithLostDevice(2200);}
    int status=0;check(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==0,
        "lost-device exit terminates without running atexit handlers");
}
