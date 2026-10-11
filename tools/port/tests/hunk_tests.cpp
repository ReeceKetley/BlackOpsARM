#include <universal/mem_userhunk.h>
#include <universal/memory_services.h>
#include "hunk_test_checks.h"
#include <algorithm>
#include <cstring>
#include <thread>
#include <vector>
#if defined(__ANDROID__)
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
template<class F> static void dies(F fn, int signal)
{
    const pid_t child = fork();
    CHECK(child >= 0);
    if (child == 0) { fn(); _exit(99); }
    int status = 0;
    CHECK(waitpid(child, &status, 0) == child);
    CHECK(WIFSIGNALED(status) && WTERMSIG(status) == signal);
}
#endif
int main()
{
    CHECK(HunkTestReservationCount() == 0);
    auto *user = Hunk_UserCreate(65536, HU_SCHEME_DEFAULT, 0, nullptr, "allocator test", 7);
    CHECK(HunkTestReservationCount() == 1);
    CHECK(user->type == 7 && std::strcmp(user->name, "allocator test") == 0);
    struct Payload { unsigned char *ptr; std::size_t size; unsigned char byte; };
    std::vector<Payload> payloads;
    for (unsigned i = 0; i < 2000; ++i) {
        const int alignment = 1 << (i % 13);
        const int size = 1 + static_cast<int>((i * 17) % 8192);
        auto *p = static_cast<unsigned char *>(Hunk_UserAlloc(user, size, alignment, nullptr));
        CHECK(reinterpret_cast<std::uintptr_t>(p) % alignment == 0);
        const auto byte = static_cast<unsigned char>(i);
        std::memset(p, byte, size);
        payloads.push_back({p, static_cast<std::size_t>(size), byte});
    }
    for (auto p : payloads) for (std::size_t i = 0; i < p.size; ++i) CHECK(p.ptr[i] == p.byte);
    CHECK(HunkTestReservationCount() > 1);
    Hunk_UserFree(user, payloads[0].ptr); // Arena free intentionally retains storage.
    CHECK(payloads[0].ptr[0] == payloads[0].byte);
    Hunk_UserReset(user);
    CHECK(HunkTestReservationCount() == 1);
    auto *hunk = reinterpret_cast<HunkUserDefault *>(user);
    CHECK(hunk->current == hunk && hunk->next == nullptr);
    auto *p = static_cast<unsigned char *>(Hunk_UserAlloc(user, 12000, 32, nullptr));
    for (unsigned i = 0; i < 12000; ++i) CHECK(p[i] == 0);
    CHECK(std::strcmp(Hunk_CopyString(user, "native arena"), "native arena") == 0);
    Hunk_UserReset(user);
    Hunk_UserReset(user);
    Hunk_UserDestroy(user);
    CHECK(HunkTestReservationCount() == 0);
    // Caller owns this buffer before and after reset/destroy; no virtual release.
    alignas(4096) static unsigned char buffer[65536];
    auto *external = Hunk_UserCreateFromBuffer(buffer, sizeof(buffer), HU_SCHEME_DEFAULT, HF_FIXEDSIZE, nullptr, "external", 7);
    auto *e = static_cast<unsigned char *>(Hunk_UserAlloc(external, 96, 32, nullptr));
    CHECK(e >= buffer && e + 96 <= buffer + sizeof(buffer));
    std::memset(e, 0x5a, 96);
    Hunk_UserSetPos(external, e);
    CHECK(Hunk_UserAlloc(external, 96, 32, nullptr) == e);
    Hunk_UserReset(external);
    Hunk_UserDestroy(external);
    CHECK(e[0] == 0x5a && HunkTestReservationCount() == 0);
    buffer[0] = 0x72;
    CHECK(buffer[0] == 0x72);
    // Independent arenas can use the shared OS registry concurrently. A single
    // default hunk remains owner-thread confined, matching the engine contract.
    std::vector<std::thread> threads;
    for (unsigned t = 0; t < 8; ++t) threads.emplace_back([] {
        for (unsigned i = 0; i < 100; ++i) {
            auto *u = Hunk_UserCreate(65536, HU_SCHEME_DEFAULT, 0, nullptr, "thread arena", 7);
            std::memset(Hunk_UserAlloc(u, 7000, 64, nullptr), 0x23, 7000);
            Hunk_UserDestroy(u);
        }
    });
    for (auto &t : threads) t.join();
    CHECK(HunkTestReservationCount() == 0);
#if defined(__ANDROID__)
    const int page = static_cast<int>(bo1::android::VirtualPageSize());
    char *mapping = static_cast<char *>(Z_VirtualReserve(page * 3));
    CHECK(reinterpret_cast<std::uintptr_t>(mapping) % page == 0);
    dies([&] { volatile char value = mapping[page]; (void)value; }, SIGSEGV);
    Z_VirtualCommit(mapping + page, page, 7);
    std::memset(mapping + page, 0x4f, page);
    Z_VirtualDecommit(mapping + page, page, 7);
    dies([&] { volatile char value = mapping[page]; (void)value; }, SIGSEGV);
    Z_VirtualCommit(mapping + page, page, 7);
    for (int i = 0; i < page; ++i) CHECK(mapping[page + i] == 0);
    dies([&] { Z_VirtualCommit(mapping + page * 3, 1, 7); }, SIGABRT);
    dies([&] { Z_VirtualFree(mapping + page, 7); }, SIGABRT);
    dies([&] { Z_VirtualCommit(mapping, page, 8); }, SIGABRT);
    Z_VirtualFree(mapping, 7);
    // Exact-end allocation used to request one page past the reservation.
    auto *full = Hunk_UserCreate(65536, HU_SCHEME_DEFAULT, HF_FIXEDSIZE, nullptr, "full", 7);
    auto *f = reinterpret_cast<HunkUserDefault *>(full);
    const int remaining = static_cast<int>(f->end - f->pos);
    auto *all = static_cast<char *>(Hunk_UserAlloc(full, remaining, 1, nullptr));
    std::memset(all, 0x39, remaining);
    CHECK(f->pos == f->end && f->locked == f->end);
    dies([&] { Hunk_UserAlloc(full, 1, 1, nullptr); }, SIGABRT);
    Hunk_UserDestroy(full);
    dies([] { Hunk_UserCreate(65536, HU_SCHEME_DEFAULT, 8, nullptr, "tracking", 7); }, SIGABRT);
    dies([] { Hunk_UserCreate(65536, HU_SCHEME_DEBUG, 0, nullptr, "debug", 7); }, SIGABRT);
    auto *valid = Hunk_UserCreate(65536, HU_SCHEME_DEFAULT, 0, nullptr, "invalid alloc", 7);
    dies([&] { Hunk_UserAlloc(valid, -1, 4, nullptr); }, SIGABRT);
    dies([&] { Hunk_UserAlloc(valid, 4, 0, nullptr); }, SIGABRT);
    dies([&] { Hunk_UserAlloc(valid, 4, 3, nullptr); }, SIGABRT);
    Hunk_UserDestroy(valid);
    std::printf("Native page size: %d; protection/discard/bounds/failures verified\n", page);
#endif
    CHECK(HunkTestReservationCount() == 0);
    std::puts("PASS actual HunkUser: 2000 payloads, alignments 1..4096, growth/reset, caller ownership, 800 independent threaded arenas");
}
