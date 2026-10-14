#include <platform/android/renderer_window.h>
#include <platform/android/window_lifecycle.h>
#include <platform/android/controller_service.h>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <vector>
using namespace bo1::android;
static void Check(bool value,const char *message) { if(!value) { std::fprintf(stderr,"FAIL: %s (%s)\n",message,SDL_GetError()); std::abort(); } }
extern "C" void *__wrap_SDL_AndroidGetJNIEnv() { std::abort(); }
extern "C" void *__wrap_SDL_AndroidGetActivity() { std::abort(); }
void Sys_SetQuitRequested() { std::abort(); } // No quit event is part of this event-owner fixture.
std::atomic<int> eventCount{};
void Consume(const SDL_Event &event,void *) { if(event.type==SDL_USEREVENT && event.user.code==731) ++eventCount; }
struct Work { std::atomic<int> *count; std::thread::id owner; unsigned delay; };
bool WorkTask(void *opaque) { auto &work=*static_cast<Work *>(opaque); Check(std::this_thread::get_id()==work.owner,"actual SDL event owner executes request"); if(work.delay) std::this_thread::sleep_for(std::chrono::milliseconds(work.delay)); ++*work.count; return true; }
bool Reentrant(void *opaque) { return RunRendererOwnerTask(WorkTask,opaque); }
bool Failed(void *) { SDL_SetError("fixture actual owner rejection"); return false; }
#undef main
int main() {
    SDL_SetMainReady(); // Actual standalone SDL entry contract; no Java/video publication.
    std::atomic<int> executions{};
    Work work{&executions,std::this_thread::get_id(),0};
    Check(!RunRendererOwnerTask(WorkTask,&work,20),"missing event owner times out");
    Check(executions==0,"timed-out pending task never executes");
    InstallSDLEventOwner(Consume,nullptr);
    Check(PumpRendererOwnerRequestsIfOwner(),"real owner pump");
    Check(executions==0,"cancelled request removed before owner starts");
    Check(!SDLJavaSetupComplete(),"actual Java setup absent in shell");
    Check(!PrepareRendererVideo(),"video honestly rejects absent Java before unsafe JNI getters");
    Check(!(SDL_WasInit(SDL_INIT_VIDEO)&SDL_INIT_VIDEO),"rejection does not initialize fake video");
    Check(RunRendererOwnerTask(Reentrant,&work),"reentrant owner dispatch without transport lock");
    Check(executions==1,"reentrant task executes once");
    std::atomic<int> finished{}; std::atomic<bool> ok{true};
    std::vector<std::thread> producers;
    for(int n=0;n<4;++n) producers.emplace_back([&] { for(int i=0;i<30;++i) if(!RunRendererOwnerTask(WorkTask,&work,2000)) ok=false; ++finished; });
    while(finished!=4) { Check(PumpRendererOwnerRequestsIfOwner(),"owner remains real"); std::this_thread::yield(); }
    for(auto &thread:producers) thread.join();
    Check(ok && executions==121,"concurrent dispatch preserves exact calls");
    finished=0; work.delay=80;
    auto start=std::chrono::steady_clock::now();
    std::thread started([&] { Check(RunRendererOwnerTask(WorkTask,&work,20),"started request must finish even beyond pending timeout"); ++finished; });
    std::this_thread::sleep_for(std::chrono::milliseconds(3));
    Check(PumpRendererOwnerRequestsIfOwner(),"pump executes started request"); started.join();
    Check(finished==1 && executions==122 && std::chrono::steady_clock::now()-start>=std::chrono::milliseconds(80),"started task keeps caller context alive");
    work.delay=0; finished=0;
    std::thread rejected([&] { Check(!RunRendererOwnerTask(Failed,nullptr,1000),"actual callback failure propagates"); Check(std::string(SDL_GetError())=="fixture actual owner rejection","owner error copied to caller thread"); ++finished; });
    while(!finished) { PumpRendererOwnerRequestsIfOwner(); std::this_thread::yield(); } rejected.join();
    SDL_Event event{}; event.type=SDL_USEREVENT; event.user.code=731;
    Check(SDL_PushEvent(&event)==1,"real SDL event publication"); PumpSDLEvents(); Check(eventCount==1,"actual consumer receives event");
    Check(RetireRendererVideo(),"no-resource retirement is idempotent without Java");
    RetireSDLEventOwner();
    Check(!PumpRendererOwnerRequestsIfOwner(),"retired event owner cannot execute requests");
    Check(!RunRendererOwnerTask(WorkTask,&work,20) && executions==122,"post-retirement queued work cancels");
    std::puts("PASS real SDL event-owner dispatch: cancellation, started lifetime, concurrent/reentrant calls, failure TLS, absent-Java video guard, event consumer and retirement; no GPU proof");
    return 0;
}
