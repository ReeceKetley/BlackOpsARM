#include <universal/narrow32.h>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <type_traits>
#include <vector>
#if defined(__ANDROID__)
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

static void Check(bool condition, int line, const char *expression)
{
    if (!condition) { std::fprintf(stderr, "FAIL %d: %s\n", line, expression); std::abort(); }
}
#define CHECK(c) Check((c), __LINE__, #c)
static_assert(sizeof(BO1_NARROW(char)) == 4);
static_assert(alignof(BO1_NARROW(char)) == 4);
#if BO1_NATIVE_64BIT
static_assert(std::is_trivially_copyable_v<Narrow32<char>>);
#endif

int main()
{
    CHECK(BO1_NARROW_ENCODE(nullptr) == 0);
    CHECK(BO1_NARROW_DECODE(char, 0) == nullptr);
    char bytes[32]{};
    for (unsigned int i = 0; i < sizeof(bytes); ++i)
    {
        BO1_NARROW(char) slot{};
        slot = &bytes[i];
        CHECK(static_cast<char *>(slot) == &bytes[i]);
        unsigned int raw = Bo1_NarrowBits(slot);
        BO1_NARROW(char) copied{};
        std::memcpy(&copied, &raw, 4);
        CHECK(static_cast<char *>(copied) == &bytes[i]);
        Bo1_NarrowSetBits(copied, 0);
        CHECK(static_cast<char *>(copied) == nullptr);
    }
#if BO1_NATIVE_64BIT
    // Fabricated addresses are encoded/decoded but never dereferenced. This
    // proves bit preservation independently of an allocator's address choices.
    for (uintptr_t address : {uintptr_t(0x100000000ULL), uintptr_t(0x12ABCDEF01ULL), uintptr_t(0x7F123456789AULL)})
    {
        void *p = reinterpret_cast<void *>(address);
        CHECK(Bo1_NarrowDecode(Bo1_NarrowEncode(p)) == p);
    }
    constexpr unsigned int regions = 32;
    std::atomic<unsigned int> published[regions]{};
    std::atomic<bool> start{false};
    std::vector<std::thread> threads;
    for (unsigned int t = 0; t < 8; ++t)
        threads.emplace_back([&, t] {
            while (!start.load(std::memory_order_acquire)) std::this_thread::yield();
            for (unsigned int k = 0; k < 20000; ++k)
            {
                const unsigned int r = (k + t) % regions;
                const uintptr_t address = uintptr_t(0x4000000000ULL) + uintptr_t(r) * 0x1000000 + 0xABC;
                void *p = reinterpret_cast<void *>(address);
                const unsigned int handle = Bo1_NarrowEncode(p);
                CHECK(Bo1_NarrowDecode(handle) == p);
                unsigned int expected = 0;
                if (!published[r].compare_exchange_strong(expected, handle, std::memory_order_release, std::memory_order_acquire))
                    CHECK(expected == handle);
                CHECK(Bo1_NarrowDecode(published[r].load(std::memory_order_acquire)) == p);
            }
        });
    start.store(true, std::memory_order_release);
    for (auto &thread : threads) thread.join();
#if defined(__ANDROID__)
    // A full region table must terminate explicitly, never manufacture null.
    const pid_t child = fork();
    CHECK(child >= 0);
    if (child == 0)
    {
        for (unsigned int i = 0; i < 256; ++i)
            Bo1_NarrowEncode(reinterpret_cast<void *>(uintptr_t(0x6000000000ULL) + uintptr_t(i) * 0x1000000));
        _exit(99);
    }
    int status = 0;
    CHECK(waitpid(child, &status, 0) == child);
    CHECK(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT);
#endif
#endif
    std::printf("PASS: pointer=%zu narrow=%zu align=%zu; null/roundtrip/raw-slot/high-address/concurrency/exhaustion checks as applicable\n",
                sizeof(void *), sizeof(BO1_NARROW(char)), alignof(BO1_NARROW(char)));
}
