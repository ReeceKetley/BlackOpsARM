#include "window_lifecycle.h"
#include "quit_services.h"
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <thread>
namespace bo1::android {
namespace {
std::mutex mutex;
std::thread::id owner;
SDLEventConsumer consumer;
void *consumerContext;
SDL_Window *window;
WindowLifecycleState state;
std::atomic<int> cursorRequest{-1};
[[noreturn]] void fail(const char *reason){std::fprintf(stderr,"Native SDL lifecycle: %s (%s)\n",reason,SDL_GetError());std::abort();}
void requireOwner(){if(owner!=std::this_thread::get_id())fail("event/video operation from non-owner");}
void refreshWindow(){
    if(!window)return;
    state.flags=SDL_GetWindowFlags(window);
    SDL_GetWindowSize(window,&state.width,&state.height);
}
}
void InstallSDLEventOwner(SDLEventConsumer input,void *context){
    std::lock_guard<std::mutex> guard(mutex);
    if(owner!=std::thread::id{} || !input)fail("invalid event owner installation");
    if(SDL_InitSubSystem(SDL_INIT_EVENTS))fail("SDL event subsystem initialization failed");
    owner=std::this_thread::get_id();consumer=input;consumerContext=context;
    state={}; // Foreground is established by real window flags/app events.
}
void AttachSDLWindow(SDL_Window *value){
    std::lock_guard<std::mutex> guard(mutex);requireOwner();
    if(window || !value || !(SDL_WasInit(SDL_INIT_VIDEO)&SDL_INIT_VIDEO))fail("invalid live video window attachment");
    const auto id=SDL_GetWindowID(value);if(!id)fail("invalid SDL window identity");
    window=value;state={};state.windowId=id;refreshWindow();
    state.foreground=(state.flags&SDL_WINDOW_SHOWN) && !(state.flags&SDL_WINDOW_MINIMIZED);
    state.focused=(state.flags&SDL_WINDOW_INPUT_FOCUS)!=0;
}
void DetachSDLWindow(SDL_Window *value){
    std::lock_guard<std::mutex> guard(mutex);requireOwner();
    if(!window || window!=value)fail("detach requires exact live window owner");
    window=nullptr;state={};
}
void PumpSDLEvents(){
    SDLEventConsumer input;void *context;
    {std::lock_guard<std::mutex> guard(mutex);requireOwner();input=consumer;context=consumerContext;}
    SDL_Event event;
    while(SDL_PollEvent(&event)){
        bool quit=false;
        {
            std::lock_guard<std::mutex> guard(mutex);
            if(event.type==SDL_QUIT || event.type==SDL_APP_TERMINATING){state.closing=true;quit=true;}
            else if(event.type==SDL_APP_WILLENTERBACKGROUND || event.type==SDL_APP_DIDENTERBACKGROUND)state.foreground=false;
            else if(event.type==SDL_APP_DIDENTERFOREGROUND)state.foreground=true;
            else if(event.type==SDL_WINDOWEVENT && event.window.windowID==state.windowId && window){
                switch(event.window.event){
                case SDL_WINDOWEVENT_FOCUS_GAINED:state.focused=true;break;
                case SDL_WINDOWEVENT_FOCUS_LOST:state.focused=false;break;
                case SDL_WINDOWEVENT_MINIMIZED:case SDL_WINDOWEVENT_HIDDEN:state.foreground=false;break;
                case SDL_WINDOWEVENT_RESTORED:case SDL_WINDOWEVENT_SHOWN:state.foreground=true;break;
                case SDL_WINDOWEVENT_CLOSE:state.closing=true;quit=true;break;
                }
                refreshWindow();
            }
        }
        if(quit)Sys_SetQuitRequested();
        // Deliver all events, including lifecycle, to the actual engine consumer.
        // Never call it while holding the snapshot lock.
        input(event,context);
    }
    const int cursor=cursorRequest.exchange(-1,std::memory_order_acq_rel);
    if(cursor>=0 && SDL_ShowCursor(cursor?SDL_ENABLE:SDL_DISABLE)<0)fail("SDL cursor request failed");
    std::lock_guard<std::mutex> guard(mutex);refreshWindow();
}
bool ReadSDLWindow(SDL_Window *value,WindowLifecycleState *out){
    if(!out)return false;
    std::lock_guard<std::mutex> guard(mutex);
    if(owner==std::thread::id{} || !value || value!=window)return false;
    *out=state;return true;
}
bool HasAttachedSDLWindow(){std::lock_guard<std::mutex> guard(mutex);return window!=nullptr;}
bool IsSDLEventOwner(){
    std::lock_guard<std::mutex> guard(mutex);
    return owner==std::this_thread::get_id() && consumer!=nullptr;
}
bool PumpSDLEventsIfOwner(){
    {std::lock_guard<std::mutex> guard(mutex);if(owner!=std::this_thread::get_id())return false;}
    PumpSDLEvents();return true;
}
bool PushSDLFocus(SDL_Window *value,bool focused){
    SDL_Event event{};
    {std::lock_guard<std::mutex> guard(mutex);
        if(!value || value!=window)return false;
        event.type=SDL_WINDOWEVENT;event.window.windowID=state.windowId;
        event.window.event=focused?SDL_WINDOWEVENT_FOCUS_GAINED:SDL_WINDOWEVENT_FOCUS_LOST;}
    return SDL_PushEvent(&event)==1;
}
void RequestSDLCursor(bool visible){cursorRequest.store(visible?1:0,std::memory_order_release);}
bool PushSDLQuit(){SDL_Event event{};event.type=SDL_QUIT;return SDL_PushEvent(&event)==1;}
void RetireSDLEventOwner(){
    std::lock_guard<std::mutex> guard(mutex);requireOwner();
    if(window)fail("live window must detach before event owner retirement");
    SDL_QuitSubSystem(SDL_INIT_EVENTS);owner={};consumer=nullptr;consumerContext=nullptr;state={};cursorRequest=-1;
}
}
