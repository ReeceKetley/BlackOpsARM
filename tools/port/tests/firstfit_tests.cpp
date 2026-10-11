#include <universal/mem_firstfit.h>
#include "hunk_test_checks.h"
#include "hunk_main_thread_test.h"
#include <cstring>
#include <thread>
#include <vector>
#if defined(__ANDROID__)
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
template<class F> static void aborts(F fn) {
    pid_t p = fork(); CHECK(p >= 0);
    if (!p) { fn(); _exit(99); }
    int status; CHECK(waitpid(p, &status, 0) == p);
    CHECK(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT);
}
#endif
static void inspect(FirstFitHunkUser *heap)
{
    for (auto *n = heap->free_blocks; n; n = n->next) {
#if BO1_NATIVE_64BIT
        CHECK(reinterpret_cast<std::uintptr_t>(n) % alignof(_firstfit_heapnode) == 0);
#endif
        CHECK(n->size > 0);
        CHECK(!n->next || reinterpret_cast<std::uintptr_t>(n->next) > reinterpret_cast<std::uintptr_t>(n));
    }
}
int main()
{
    RegisterHunkTestMainThread();
    CHECK(IsHunkTestMainThread());
    bool workerIsMain = true;
    std::thread identity([&] { workerIsMain = IsHunkTestMainThread(); });
    identity.join(); CHECK(!workerIsMain);
    alignas(4096) static unsigned char memory[1 << 20];
    auto *u = Hunk_UserCreateFromBuffer(memory, sizeof(memory), HU_SCHEME_FIRSTFIT, 0, nullptr, "first fit", 7);
    auto *heap = reinterpret_cast<FirstFitHunkUser *>(u);
    struct Payload { unsigned char *p; int size; unsigned char value; };
    std::vector<Payload> payloads;
    for (unsigned i = 0; i < 400; ++i) {
        int alignment = 1 << (i % 13);
        int size = 1 + (i * 11) % 127; // Odd sizes expose native heap-header alignment.
        auto *p = static_cast<unsigned char *>(Hunk_UserAlloc(u, size, alignment, nullptr));
        CHECK(p && reinterpret_cast<std::uintptr_t>(p) % alignment == 0);
        unsigned char value = static_cast<unsigned char>(i);
        std::memset(p, value, size); payloads.push_back({p, size, value});
        inspect(heap);
    }
    for (auto v : payloads) for (int j = 0; j < v.size; ++j) CHECK(v.p[j] == v.value);
    // Free in three interleaved passes, exercising insertion and both coalesces.
    for (unsigned offset = 0; offset < 3; ++offset) for (unsigned i = offset; i < payloads.size(); i += 3) {
        Hunk_UserFree(u, payloads[i].p); inspect(heap);
    }
    CHECK(heap->used == sizeof(FirstFitHunkUser));
    CHECK(heap->free_blocks && !heap->free_blocks->next);
    CHECK(heap->free_blocks->size == sizeof(memory) - sizeof(FirstFitHunkUser));
    CHECK(Hunk_UserAlloc(u, sizeof(memory) - 1024, 8, nullptr) == nullptr); // Reserved-tail exhaustion is preserved.
    // First-fit lock allows concurrent use of this shared pool.
    std::vector<std::thread> threads;
    for (unsigned t = 0; t < 8; ++t) threads.emplace_back([&, t] {
        for (unsigned i = 0; i < 1000; ++i) {
            auto *p = static_cast<unsigned char *>(Hunk_UserAlloc(u, 17 + t, 16, nullptr));
            CHECK(p); std::memset(p, static_cast<unsigned char>(t), 17 + t);
            for (unsigned j = 0; j < 17 + t; ++j) CHECK(p[j] == t);
            Hunk_UserFree(u, p);
        }
    });
    for (auto &t : threads) t.join();
    CHECK(heap->used == sizeof(FirstFitHunkUser)); inspect(heap);
#if defined(__ANDROID__)
    auto *badFree = static_cast<char *>(Hunk_UserAlloc(u, 32, 8, nullptr));
    aborts([&] { Hunk_UserFree(u, badFree + 1); });
    Hunk_UserFree(u, badFree);
    aborts([&] { Hunk_UserFree(u, badFree); });
#endif
    Hunk_UserReset(u); Hunk_UserDestroy(u);
    CHECK(HunkTestReservationCount() == 0);
    memory[0] = 0x71; CHECK(memory[0] == 0x71);
    auto *debug = Hunk_UserCreate(65536, HU_SCHEME_DEBUG, 0, nullptr, "debug heap", 7);
    CHECK(HunkTestReservationCount() == 1);
    auto *p = static_cast<char *>(Hunk_UserAlloc(debug, 29, 32, nullptr));
    CHECK(reinterpret_cast<std::uintptr_t>(p) % 32 == 0);
    std::memset(p, 0x6b, 29); Hunk_UserFree(debug, p);
    Hunk_UserReset(debug);
    CHECK(reinterpret_cast<HunkUserDebug *>(debug)->firstFit != nullptr);
    Hunk_UserDestroy(debug); CHECK(HunkTestReservationCount() == 0);
#if defined(__ANDROID__)
    aborts([] { std::thread other([] { bo1::android::RegisterHunkMainThread(); }); other.join(); });
    auto *d = Hunk_UserCreate(65536, HU_SCHEME_DEBUG, 0, nullptr, "wrong thread", 7);
    aborts([&] { std::thread other([&] { Hunk_UserAlloc(d, 32, 8, nullptr); }); other.join(); });
    Hunk_UserDestroy(d);
#endif
    std::puts("PASS actual first-fit/debug HunkUser: 400 odd payloads, native headers, alignments 1..4096, coalescing/exhaustion, 8000 shared-pool operations, thread identity/ownership");
}
