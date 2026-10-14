#include <platform/android/render_worker.h>
#include <atomic>
#include <thread>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

void *Sys_RendererSleep();
bool Sys_RendererReady();
void Sys_RenderCompleted();
void Sys_WakeRenderer(void *);
void Sys_StopRenderer();
void Sys_StartRenderer();
bool Sys_WaitBackendEvent(unsigned);

static void check(bool value) { if(!value) std::abort(); }
struct Packet { unsigned sequence; unsigned words[64]; };

int main() {
    alarm(20);
    Sys_CreateEvent(true,true,&renderCompletedEvent);
    Sys_CreateEvent(false,false,&backendEvent[0]);
    Sys_CreateEvent(false,false,&backendEvent[1]);
    Sys_CreateEvent(true,true,&rendererRunningEvent);
    Sys_CreateEvent(false,false,&renderPausedEvent);
    check(!Sys_RendererReady());
    check(Sys_RendererSleep()==nullptr);
    check(!Sys_WaitBackendEvent(0));
    Sys_StopRenderer();
    check(!Sys_WaitForSingleObjectTimeout(&rendererRunningEvent,0));
    check(Sys_WaitForSingleObjectTimeout(&renderPausedEvent,0));
    check(!Sys_WaitForSingleObjectTimeout(&renderPausedEvent,0));
    Sys_StartRenderer();
    check(Sys_WaitForSingleObjectTimeout(&rendererRunningEvent,0));
    std::atomic<unsigned> consumed{};
    Packet packet{};
    std::thread worker([&] {
        for(unsigned sequence=1;sequence<=512;++sequence) {
            check(Sys_WaitBackendEvent(5000));
            auto *received=static_cast<Packet *>(Sys_RendererSleep());
            check(received==&packet && received->sequence==sequence);
            for(unsigned i=0;i<64;++i) check(received->words[i]==(sequence ^ (i*31337)));
            check(!Sys_RendererReady());
            check(Sys_RendererSleep()==nullptr);
            consumed.store(sequence,std::memory_order_relaxed);
            Sys_RenderCompleted();
        }
    });
    for(unsigned sequence=1;sequence<=512;++sequence) {
        packet.sequence=sequence;
        for(unsigned i=0;i<64;++i) packet.words[i]=sequence ^ (i*31337);
        Sys_WakeRenderer(&packet);
        check(Sys_WaitForSingleObjectTimeout(&renderCompletedEvent,5000));
        check(consumed.load(std::memory_order_relaxed)==sequence);
        check(Sys_WaitForSingleObjectTimeout(&backendEvent[0],0));
    }
    worker.join();
    check(!Sys_RendererReady());
    check(!Sys_WaitBackendEvent(0));
    Sys_DestroyEvent(&renderCompletedEvent);
    Sys_DestroyEvent(&backendEvent[0]);
    Sys_DestroyEvent(&backendEvent[1]);
    Sys_DestroyEvent(&rendererRunningEvent);
    Sys_DestroyEvent(&renderPausedEvent);
    std::puts("native renderer handoff: 512 real event/publication cycles passed");
}
