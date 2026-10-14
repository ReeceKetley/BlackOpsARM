#define SDL_MAIN_HANDLED
#include <platform/android/window_lifecycle.h>
#include <platform/android/quit_services.h>
#include <cstdio>
#include <cstdlib>
#include <thread>
using namespace bo1::android;
static unsigned keys,background,foreground,quits,terminations;
static std::thread::id owner;
static void check(bool ok,const char *name){if(!ok){std::fprintf(stderr,"FAIL %s (%s)\n",name,SDL_GetError());std::exit(1);}std::printf("PASS %s\n",name);}
static void input(const SDL_Event &event,void *context){
    check(std::this_thread::get_id()==owner && context==&keys,"consumer runs on actual event owner");
    if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_a)++keys;
    if(event.type==SDL_APP_DIDENTERBACKGROUND)++background;
    if(event.type==SDL_APP_DIDENTERFOREGROUND)++foreground;
    if(event.type==SDL_QUIT)++quits;
    if(event.type==SDL_APP_TERMINATING)++terminations;
}
int main(){
    SDL_SetMainReady();owner=std::this_thread::get_id();InstallSDLEventOwner(input,&keys);
    SDL_version version;SDL_GetVersion(&version);std::printf("SDL %u.%u.%u\n",version.major,version.minor,version.patch);
    SDL_Event event{};event.type=SDL_KEYDOWN;event.key.keysym.sym=SDLK_a;
    check(SDL_PushEvent(&event)==1,"real SDL input queue accepts key");
    event={};event.type=SDL_APP_DIDENTERBACKGROUND;check(SDL_PushEvent(&event)==1,"real SDL queue accepts background event");
    event={};event.type=SDL_APP_DIDENTERFOREGROUND;check(SDL_PushEvent(&event)==1,"real SDL queue accepts foreground event");
    event={};event.type=SDL_APP_TERMINATING;check(SDL_PushEvent(&event)==1,"real SDL queue accepts termination event");
    std::thread producer([]{check(PushSDLQuit(),"cross-thread actual SDL quit publication");});producer.join();
    check(PumpSDLEventsIfOwner(),"owner performs actual SDL event pump");
    bool otherPumped=true;std::thread reader([&]{otherPumped=PumpSDLEventsIfOwner();});reader.join();
    check(!otherPumped,"non-owner cannot pump SDL video events");
    check(!PushSDLFocus(nullptr,false),"absent window cannot accept focus diagnostic");
    check(keys==1 && background==1 && foreground==1 && quits==1 && terminations==1,"all queued input and lifecycle events delivered exactly once");
    check(Sys_IsQuitRequested(),"SDL quit reaches actual native engine quit state");
    WindowLifecycleState snapshot;snapshot.width=123;
    check(!ReadSDLWindow(nullptr,&snapshot) && snapshot.width==123,"absent window rejected without fabricated readiness");
    RetireSDLEventOwner();check(!(SDL_WasInit(SDL_INIT_EVENTS)&SDL_INIT_EVENTS),"owned event subsystem reference retired");
}
