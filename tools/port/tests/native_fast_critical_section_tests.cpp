// Controlled admission callers and diagnostics are test-only. The linked
// library executes the actual production writer and reader/writer releases.
#include <platform/fast_critical_section.h>
#include <atomic>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <vector>
#if defined(_WIN32)
#include <crtdbg.h>
#endif

bool __cdecl Assert_MyHandler(const char *file, int line, int, const char *format, ...) {
    std::fprintf(stderr, "FastCriticalSection assertion %s:%d: ", file, line);
    va_list arguments;
    va_start(arguments, format);
    std::vfprintf(stderr, format, arguments);
    va_end(arguments);
    std::fputc('\n', stderr);
    std::fflush(stderr);
    std::abort();
}
namespace {
void check(bool condition) {
    if (!condition) { std::fputs("FastCriticalSection test assertion failed\n", stderr); std::abort(); }
}
struct Guarded {
    std::uint32_t before = 0x12345678u;
    FastCriticalSection section{};
    std::uint32_t after = 0x90abcdefu;
};
void canaries(const Guarded &g) { check(g.before == 0x12345678u && g.after == 0x90abcdefu); }
// Original inline reader admission contract, using the same native primitive
// helpers as the seven production Dvar sites. No canonical Sys_LockRead exists.
void read(Guarded &g) {
    bo1::fcs::AddRead(&g.section);
    while (bo1::fcs::LoadWrite(&g.section)) bo1::fcs::YieldThread();
}
void zero(const Guarded &g) {
    canaries(g);
    check(bo1::fcs::LoadRead(&g.section) == 0 && bo1::fcs::LoadWrite(&g.section) == 0);
}
}
int main(int argc, char **argv) {
    std::thread([] {
        std::this_thread::sleep_for(std::chrono::seconds(20));
        std::fputs("FastCriticalSection test watchdog expired\n", stderr);
        std::fflush(stderr);
        std::_Exit(2);
    }).detach();
#if defined(_WIN32)
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
#endif
    Guarded g;
    if (argc == 2) {
        if (!std::strcmp(argv[1], "read_zero")) Sys_UnlockRead(&g.section);
        else if (!std::strcmp(argv[1], "write_zero")) Sys_UnlockWrite(&g.section);
        else if (!std::strcmp(argv[1], "read_negative_one")) { bo1::atomic32::ExchangeAdd32(&g.section.readCount, UINT32_MAX); Sys_UnlockRead(&g.section); }
        else if (!std::strcmp(argv[1], "write_negative_one")) { bo1::atomic32::ExchangeAdd32(&g.section.writeCount, UINT32_MAX); Sys_UnlockWrite(&g.section); }
        else if (!std::strcmp(argv[1], "read_negative_min")) { bo1::atomic32::ExchangeAdd32(&g.section.readCount, 0x80000000u); Sys_UnlockRead(&g.section); }
        else if (!std::strcmp(argv[1], "write_negative_min")) { bo1::atomic32::ExchangeAdd32(&g.section.writeCount, 0x80000000u); Sys_UnlockWrite(&g.section); }
        else return 2;
        return 2;
    }
    check(argc == 1);
    for (unsigned i=0; i<3; ++i) read(g);
    check(bo1::fcs::LoadRead(&g.section) == 3);
    for (unsigned i=0; i<3; ++i) Sys_UnlockRead(&g.section);
    zero(g);

    std::atomic<unsigned> ready{0};
    std::atomic<bool> releaseFirst{false}, releaseSecond{false}, firstDone{false}, writerStarted{false}, writerEntered{false};
    std::thread first([&] { read(g); ready.fetch_add(1); while(!releaseFirst) bo1::fcs::YieldThread(); Sys_UnlockRead(&g.section); firstDone=true; });
    std::thread second([&] { read(g); ready.fetch_add(1); while(!releaseSecond) bo1::fcs::YieldThread(); Sys_UnlockRead(&g.section); });
    while(ready.load()!=2) bo1::fcs::YieldThread();
    check(bo1::fcs::LoadRead(&g.section)==2);
    std::thread blockedWriter([&] { writerStarted=true; Sys_LockWrite(&g.section); writerEntered=true; Sys_UnlockWrite(&g.section); });
    while(!writerStarted) bo1::fcs::YieldThread();
    for(unsigned i=0;i<1000;++i) { check(!writerEntered); bo1::fcs::YieldThread(); }
    releaseFirst=true;
    while(!firstDone) bo1::fcs::YieldThread();
    check(bo1::fcs::LoadRead(&g.section)==1&&!writerEntered);
    releaseSecond=true;
    first.join(); second.join(); blockedWriter.join(); check(writerEntered); zero(g);

    Sys_LockWrite(&g.section);
    std::atomic<bool> readerEntered{false};
    std::thread queuedReader([&] { read(g); readerEntered=true; Sys_UnlockRead(&g.section); });
    while(bo1::fcs::LoadRead(&g.section)!=1) bo1::fcs::YieldThread();
    check(!readerEntered&&bo1::fcs::LoadWrite(&g.section)==1);
    Sys_UnlockWrite(&g.section);
    queuedReader.join(); check(readerEntered); zero(g);

    Guarded independent;
    Sys_LockWrite(&g.section);
    std::thread other([&] { Sys_LockWrite(&independent.section); Sys_UnlockWrite(&independent.section); });
    other.join(); Sys_UnlockWrite(&g.section); zero(g); zero(independent);

    unsigned generation=0, complement=UINT32_MAX;
    constexpr unsigned writers=4, readers=6, writesPerWriter=2500, readsPerReader=2000;
    std::atomic<bool> start{false};
    std::atomic<unsigned> checks{0}, activeWriters{0};
    std::vector<std::thread> threads;
    for(unsigned i=0;i<writers;++i) threads.emplace_back([&] {
        while(!start) bo1::fcs::YieldThread();
        for(unsigned j=0;j<writesPerWriter;++j) {
            Sys_LockWrite(&g.section);
            check(activeWriters.fetch_add(1)==0);
            ++generation; bo1::fcs::YieldThread(); complement=~generation;
            check(activeWriters.fetch_sub(1)==1);
            Sys_UnlockWrite(&g.section);
        }
    });
    for(unsigned i=0;i<readers;++i) threads.emplace_back([&] {
        while(!start) bo1::fcs::YieldThread();
        for(unsigned j=0;j<readsPerReader;++j) {
            read(g); check(complement==~generation); Sys_UnlockRead(&g.section); checks.fetch_add(1);
        }
    });
    start=true;
    for(auto &thread:threads) thread.join();
    check(generation==writers*writesPerWriter&&complement==~generation&&checks==readers*readsPerReader);
    check(activeWriters.load()==0);
    zero(g); zero(independent);
    check(!bo1::fcs::SignedPositive(0)&&bo1::fcs::SignedPositive(1)&&bo1::fcs::SignedPositive(INT32_MAX)&&!bo1::fcs::SignedPositive(0x80000000u)&&!bo1::fcs::SignedPositive(UINT32_MAX));
    std::printf("{\"fast_critical_section\":{\"pointer_bytes\":%zu,\"size\":%zu,\"alignment\":%zu,\"read_offset\":%zu,\"write_offset\":%zu,\"nested_reads\":3,\"simultaneous_readers\":2,\"blocked_writer\":true,\"queued_reader\":true,\"independent_sections\":2,\"writer_threads\":%u,\"reader_threads\":%u,\"total_writes\":%u,\"reader_checks\":%u,\"canaries\":true,\"final_read_count\":%u,\"final_write_count\":%u,\"passed\":true}}\n",sizeof(void*),sizeof(FastCriticalSection),alignof(FastCriticalSection),offsetof(FastCriticalSection,readCount),offsetof(FastCriticalSection,writeCount),writers,readers,generation,checks.load(),bo1::fcs::LoadRead(&g.section),bo1::fcs::LoadWrite(&g.section));
}
