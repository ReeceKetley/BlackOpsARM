#include <platform/critical_section_services.h>
#include <platform/android/script_string_services.h>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <vector>

static void require(bool condition, const char *why) {
    if (!condition) { std::fprintf(stderr,"FAIL %s\n",why); std::exit(1); }
}
static bool otherTry(CriticalSection section) {
    bool result = false;
    std::thread worker([&] { result=Sys_TryEnterCriticalSection(section); if(result) Sys_LeaveCriticalSection(section); });
    worker.join(); return result;
}
int main(int argc, char **argv) {
#if defined(_WIN32)
    // Expected abort controls must exit, without Windows fault-reporting UI.
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif
    // The process bounds its own lifetime even if a lock regression deadlocks.
    std::thread([] { std::this_thread::sleep_for(std::chrono::seconds(20)); std::fputs("FAIL lock watchdog\n",stderr); std::_Exit(2); }).detach();
    if(argc==3) {
        const auto section=static_cast<CriticalSection>(std::atoi(argv[2]));
        if(!std::strcmp(argv[1],"--invalid-enter")) Sys_EnterCriticalSection(section);
        else if(!std::strcmp(argv[1],"--invalid-try")) (void)Sys_TryEnterCriticalSection(section);
        else if(!std::strcmp(argv[1],"--invalid-leave")) Sys_LeaveCriticalSection(section);
        else return 4;
        std::fputs("FAIL invalid identifier returned\n",stderr); return 1;
    }
    require(argc==1,"unexpected arguments");
    std::atomic<bool> start{};
    unsigned initialized=0;
    std::vector<std::thread> workers;
    for(unsigned i=0;i<12;++i) workers.emplace_back([&] {
        while(!start.load(std::memory_order_acquire)) std::this_thread::yield();
        Sys_InitializeCriticalSections();
        Sys_EnterCriticalSection(CRITSECT_CBUF); ++initialized; Sys_LeaveCriticalSection(CRITSECT_CBUF);
    });
    start.store(true,std::memory_order_release);
    for(auto &worker:workers) worker.join();
    require(initialized==12,"concurrent first initialization and mutual exclusion");
    unsigned sections=0;
    for(int i=0;i<CRITSECT_COUNT;++i) {
        const auto section=static_cast<CriticalSection>(i);
        Sys_EnterCriticalSection(section); Sys_EnterCriticalSection(section);
        require(Sys_TryEnterCriticalSection(section),"same-thread recursive try");
        Sys_InitializeCriticalSections();
        require(!otherTry(section),"other-thread busy at depth3 after reinit");
        const auto independent=static_cast<CriticalSection>((i+1)%CRITSECT_COUNT);
        require(otherTry(independent),"independent section not blocked");
        Sys_LeaveCriticalSection(section);
        require(!otherTry(section),"depth2 retains lock");
        Sys_LeaveCriticalSection(section);
        require(!otherTry(section),"depth1 retains lock");
        Sys_LeaveCriticalSection(section);
        require(otherTry(section),"final release permits other thread");
        unsigned counter=0;
        workers.clear();
        for(unsigned j=0;j<4;++j) workers.emplace_back([&] {
            for(unsigned k=0;k<100;++k) {
                Sys_EnterCriticalSection(section); const auto value=counter;
                std::this_thread::yield(); counter=value+1; Sys_LeaveCriticalSection(section);
            }
        });
        for(auto &worker:workers) worker.join();
        require(counter==400,"protected concurrent counter"); ++sections;
    }
    const CriticalSection enums[]={CRITSECT_SCRIPT_STRING,CRITSECT_MEMORY_TREE,CRITSECT_DBHASH};
    const bo1::script::PoolLock pools[]={bo1::script::PoolLock::Strings,bo1::script::PoolLock::MemoryTree,bo1::script::PoolLock::Database};
    for(unsigned i=0;i<3;++i) {
        bo1::script::Lock(pools[i]);
        require(Sys_TryEnterCriticalSection(enums[i]),"cross-interface same-thread recursion");
        Sys_LeaveCriticalSection(enums[i]);
        require(!otherTry(enums[i]),"PoolLock blocks engine interface");
        bo1::script::Unlock(pools[i]);
        Sys_EnterCriticalSection(enums[i]); bool acquired=false;
        std::thread worker([&] { acquired=bo1::script::TryLock(pools[i]); if(acquired) bo1::script::Unlock(pools[i]); });
        worker.join(); require(!acquired,"engine interface blocks PoolLock");
        Sys_LeaveCriticalSection(enums[i]);
        require(otherTry(enums[i]),"cross-interface final release");
    }
    require(!bo1::script::IsServerThread(),"lock initialization does not publish server role");
    std::printf("{\"native_critical_sections\":{\"pointer_bytes\":%zu,\"sections\":%u,\"initializers\":12,\"protected_increments\":30000,\"shared_pool_locks\":3,\"passed\":true}}\n",sizeof(void*),sections);
}
