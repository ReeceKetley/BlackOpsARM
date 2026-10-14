#include <platform/android/server_profiler_sampler.h>
#include <cstdio>
#include <atomic>
#include <cstdlib>
#include <thread>
#include <time.h>

static void require(bool condition, const char *message) {
    if (!condition) { std::fprintf(stderr, "FAIL %s\n", message); std::abort(); }
}
static unsigned long long now() {
    timespec value{};
    require(::clock_gettime(CLOCK_MONOTONIC, &value) == 0, "clock");
    return static_cast<unsigned long long>(value.tv_sec) * 1000000000ULL + value.tv_nsec;
}
static __attribute__((noinline)) void busy(unsigned milliseconds) {
    const auto until = now() + milliseconds * 1000000ULL;
    while (now() < until) std::atomic_signal_fence(std::memory_order_seq_cst);
}
int main() {
    static bo1::android::ServerProfileSample samples[4096];
    static char position[4];
    unsigned dropped = 0;
    bo1::android::PublishServerScriptPosition(0, position);
    bo1::android::BeginServerSamples(2, 6);
    bo1::android::SetServerSampleCategory(4); busy(30);
    bo1::android::SetServerSampleCategory(6); busy(30);
    const auto count = bo1::android::EndServerSamples(samples, 4096, &dropped);
    require(count > 5 && dropped == 0, "real periodic samples without overflow");
    unsigned categories[2]{};
    for (std::size_t i = 0; i < count; ++i) {
        require(samples[i].pc > 0xffffffffULL, "native ARM64 instruction address retained");
        require(samples[i].scriptPosition == position, "published script address retained");
        require(samples[i].category == 4 || samples[i].category == 6, "published category");
        ++categories[samples[i].category == 6];
    }
    require(categories[0] && categories[1], "both category intervals sampled");
    bo1::android::BeginServerSamples(1, 6);
    bo1::android::SetServerSampleCategory(4); busy(20);
    bo1::android::SetServerSampleCategory(6); busy(25);
    const auto scriptCount = bo1::android::EndServerSamples(samples, 4096, &dropped);
    require(scriptCount > 2 && dropped == 0, "script-only samples");
    for (std::size_t i = 0; i < scriptCount; ++i) require(samples[i].category == 6, "script filter");
    for (int cycle = 0; cycle < 3; ++cycle) {
        bo1::android::BeginServerSamples(2, 6); busy(15);
        require(bo1::android::EndServerSamples(samples, 1, &dropped) == 1 && dropped > 0, "bounded copy reports loss");
    }
    std::thread exiting([] { bo1::android::BeginServerSamples(2, 6); busy(12); });
    exiting.join(); // Destructor must stop delivery and destroy the live timer.
    std::thread reuse([] {
        bo1::android::ServerProfileSample local[64]; unsigned lost;
        bo1::android::BeginServerSamples(2, 6); busy(15);
        require(bo1::android::EndServerSamples(local, 64, &lost) > 0 && lost == 0, "new thread owns its samples");
    });
    reuse.join();
    std::printf("PASS native server timer samples=%zu script=%zu; PC width, publication, bounded loss, restart and thread teardown\n", count, scriptCount);
}
