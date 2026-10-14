#include <universal/mem_largelocal.h>
#include <platform/android/thread_services.h>
#include <platform/android/script_string_services.h>
#include <atomic>
#include <thread>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstddef>
#include <cstdint>
extern int g_largeLocalPos, g_largeLocalRightPos;
static void require(bool condition, const char *message) {
    if (!condition) { std::fprintf(stderr, "FAIL %s\n", message); std::abort(); }
}
int main() {
    Sys_InitMainThread();
    require(Sys_IsMainThread(), "real main role");
    LargeLocalReset();
    {
        LargeLocal first(1); auto *a = first.GetBuf(); a[0] = 0x5a;
        require(!(reinterpret_cast<std::uintptr_t>(a) % alignof(std::max_align_t)), "aligned first allocation");
        const int outer = g_largeLocalPos;
        {
            LargeLocal second(17); auto *b = second.GetBuf(); std::memset(b, 0x3c, 17);
            require(!(reinterpret_cast<std::uintptr_t>(b) % alignof(std::max_align_t)), "aligned nested odd allocation");
            require(b >= a + 1 && a[0] == 0x5a, "nonoverlap and preserved outer storage");
            const int middle = g_largeLocalPos;
            { LargeLocal third(33); std::memset(third.GetBuf(), 0x7f, 33); }
            require(g_largeLocalPos == middle && b[16] == 0x3c, "inner destructor rollback");
        }
        require(g_largeLocalPos == outer && a[0] == 0x5a, "nested rollback preserves parent");
    }
    require(g_largeLocalPos == 0 && g_largeLocalRightPos == 524288, "whole scope reclaimed");
    { LargeLocal full(524288); auto *buffer = full.GetBuf(); buffer[0] = 1; buffer[524287] = 2;
      require(buffer[0] == 1 && buffer[524287] == 2 && g_largeLocalPos == 524288, "full arena boundary"); }
    require(g_largeLocalPos == 0 && g_largeLocalRightPos == 524288, "full arena rollback");
    { LargeLocal zero(0); require(zero.GetBuf() != nullptr && g_largeLocalPos == 0, "zero allocation reference behavior"); }
    std::atomic<unsigned> phase{0};
    std::thread server([&] {
        bo1::script::RegisterServerThread();
        require(bo1::script::IsServerThread() && !Sys_IsMainThread(), "actual registered server identity");
        {
            LargeLocal right(1041); auto *data = right.GetBuf();
            require(!(reinterpret_cast<std::uintptr_t>(data) % alignof(std::max_align_t)), "right alignment");
            std::memset(data, 0x6d, 1041);
            { LargeLocal nested(23); require(nested.GetBuf() + 23 <= data, "nested right nonoverlap"); }
            phase.store(1, std::memory_order_release);
            while (phase.load(std::memory_order_acquire) != 2) std::this_thread::yield();
            for (unsigned i = 0; i < 1041; ++i) require(data[i] == 0x6d, "right content preserved during main allocations");
        }
        phase.store(3, std::memory_order_release);
        while (phase.load(std::memory_order_acquire) != 4) std::this_thread::yield();
    });
    while (phase.load(std::memory_order_acquire) != 1) std::this_thread::yield();
    for (unsigned cycle = 0; cycle < 64; ++cycle) {
        LargeLocal left(1023); auto *data = left.GetBuf(); std::memset(data, 0x3f, 1023);
        { LargeLocal nested(129); std::memset(nested.GetBuf(), 0x1a, 129); }
        for (unsigned i = 0; i < 1023; ++i) require(data[i] == 0x3f, "left parent content preserved");
    }
    phase.store(2, std::memory_order_release);
    while (phase.load(std::memory_order_acquire) != 3) std::this_thread::yield();
    require(g_largeLocalPos == 0 && g_largeLocalRightPos == 524288, "both ends reclaimed");
    phase.store(4, std::memory_order_release); server.join();
    // No server identity queries after join: registration is process lifetime.
    LargeLocalReset();
    std::puts("PASS original LargeLocal real main/server identities, nested alignment, nonoverlap, full arena and rollback");
}
