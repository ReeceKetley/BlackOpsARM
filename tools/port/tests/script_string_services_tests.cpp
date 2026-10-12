// Real platform prerequisites only. This does not certify the SL/MT pool.
#include <platform/atomic32.h>
#include <platform/android/script_string_services.h>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <vector>
#if defined(_WIN32)
#include <crtdbg.h>
#endif
using namespace bo1::atomic32;
using namespace bo1::script;
static void require(bool condition, const char *message) {
    if (!condition) { std::fprintf(stderr, "FAIL %s\n", message); std::abort(); }
}
static void wait(const std::atomic<bool> &ready) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(20);
    while (!ready.load(std::memory_order_acquire)) {
        require(std::chrono::steady_clock::now() < deadline, "thread synchronization timeout");
        std::this_thread::yield();
    }
}
int main(int argc, char **argv) {
#if defined(_WIN32)
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif
    if (argc == 2 && !std::strcmp(argv[1], "--conflicting-server")) {
        RegisterServerThread();
        std::thread conflicting([] { RegisterServerThread(); });
        conflicting.join();
        require(false, "conflicting registration must terminate");
    }
    if (argc == 2 && !std::strcmp(argv[1], "--invalid-lock")) {
        Lock(static_cast<PoolLock>(3));
        require(false, "invalid section must terminate");
    }
    require(argc == 1, "unknown arguments");
    struct Guarded { std::uint32_t before = 0x11223344; volatile std::uint32_t value = 0xffffffff; std::uint32_t after = 0x55667788; } guarded;
    require(AtomicLoad32(&guarded.value) == 0xffffffff, "atomic load full unsigned width");
    require(Increment32(&guarded.value) == 0, "increment new/wrap");
    require(Decrement32(&guarded.value) == 0xffffffff, "decrement new/wrap");
    require(ExchangeAdd32(&guarded.value, 2) == 0xffffffff && AtomicLoad32(&guarded.value) == 1, "exchange-add old/wrap");
    require(CmpExchange32(&guarded.value, 7, 2) == 1 && AtomicLoad32(&guarded.value) == 1, "compare-exchange failure old");
    require(CmpExchange32(&guarded.value, 0xaa040001, 1) == 1, "compare-exchange success old");
    require(Decrement32(&guarded.value) == 0xaa040000 && (AtomicLoad32(&guarded.value) << 16) == 0, "packed reference zero preserves user/length");
    require(Increment32(&guarded.value) == 0xaa040001 && (AtomicLoad32(&guarded.value) << 16) != 0, "packed reference nonzero");
    ExchangeAdd32(&guarded.value, std::uint32_t(0) - AtomicLoad32(&guarded.value));
    std::vector<std::thread> workers;
    for (unsigned i = 0; i < 4; ++i) workers.emplace_back([&] { for (unsigned n = 0; n < 20000; ++n) Increment32(&guarded.value); });
    for (auto &worker : workers) worker.join();
    require(AtomicLoad32(&guarded.value) == 80000, "80000 atomic updates");
    require(guarded.before == 0x11223344 && guarded.after == 0x55667788, "atomic neighbor guards");
    InitializeServices(); InitializeServices();
    for (PoolLock lock : {PoolLock::Strings, PoolLock::MemoryTree}) {
        Lock(lock); Lock(lock); require(TryLock(lock), "recursive try acquisition");
        Unlock(lock); Unlock(lock); Unlock(lock);
    }
    Lock(PoolLock::Strings);
    std::atomic<bool> attempting{false}, acquired{false};
    std::thread contender([&] {
        require(!TryLock(PoolLock::Strings), "try section held by other thread");
        require(TryLock(PoolLock::MemoryTree), "two independent sections");
        Unlock(PoolLock::MemoryTree);
        attempting.store(true, std::memory_order_release);
        Lock(PoolLock::Strings);
        acquired.store(true, std::memory_order_release);
        Unlock(PoolLock::Strings);
    });
    wait(attempting); require(!acquired.load(std::memory_order_acquire), "contending lock blocks until release");
    Unlock(PoolLock::Strings); wait(acquired); contender.join();
    unsigned lockedUpdates = 0;
    workers.clear();
    for (unsigned i = 0; i < 4; ++i) workers.emplace_back([&] {
        for (unsigned n = 0; n < 20000; ++n) { Lock(PoolLock::MemoryTree); ++lockedUpdates; Unlock(PoolLock::MemoryTree); }
    });
    for (auto &worker : workers) worker.join();
    require(lockedUpdates == 80000, "80000 mutually exclusive updates");
    require(!IsServerThread(), "unset server is false");
    std::atomic<bool> serverReady{false}, releaseServer{false};
    std::thread server([&] {
        require(!IsServerThread(), "worker before registration false");
        RegisterServerThread(); RegisterServerThread();
        require(IsServerThread(), "actual registered server true");
        serverReady.store(true, std::memory_order_release);
        wait(releaseServer);
    });
    wait(serverReady);
    require(!IsServerThread(), "main does not acquire another thread role");
    releaseServer.store(true, std::memory_order_release);
    server.join(); // No identity query after the registered thread exits.
    std::printf("{\"schema\":\"BlackOpsARM.script-services.v1\",\"pointer_bytes\":%zu,\"atomic_bytes\":%zu,\"atomic_updates\":80000,\"locked_updates\":80000,\"recursive_sections\":2,\"independent_sections\":true,\"contention_pass\":true,\"server_identity_pass\":true,\"pool_certified\":false}\n", sizeof(void *), sizeof(guarded.value));
}
