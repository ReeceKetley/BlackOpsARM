#include "server_profiler_sampler.h"
#include <atomic>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <ucontext.h>
#include <unistd.h>

namespace {
constexpr unsigned slots = 4096;
static_assert(std::atomic<std::uintptr_t>::is_always_lock_free);
static_assert(std::atomic<unsigned>::is_always_lock_free);
static_assert(std::atomic<int>::is_always_lock_free);
[[noreturn]] void failed(const char *operation) { std::perror(operation); std::abort(); }
int signalNumber() { return SIGRTMIN + 6; }
struct AtomicSample {
    std::atomic<std::uintptr_t> pc{0}, position{0};
    std::atomic<int> category{0};
};
struct Sampler {
    timer_t timer{};
    bool running = false;
    std::atomic<unsigned> count{0}, dropped{0};
    std::atomic<int> mode{0}, category{0}, scriptCategory{0};
    std::atomic<std::uintptr_t> position{0};
    AtomicSample samples[slots];
    ~Sampler();
};
thread_local Sampler sampler;
void capture(int, siginfo_t *info, void *context) {
    const int savedError = errno;
    if (info && info->si_code == SI_TIMER) {
        auto *state = static_cast<Sampler *>(info->si_value.sival_ptr);
        const int mode = state->mode.load(std::memory_order_relaxed);
        const int cat = state->category.load(std::memory_order_relaxed);
        if (mode && (mode == 2 || cat == state->scriptCategory.load(std::memory_order_relaxed))) {
            const unsigned index = state->count.load(std::memory_order_relaxed);
            if (index < slots) {
                state->samples[index].pc.store(static_cast<ucontext_t *>(context)->uc_mcontext.pc, std::memory_order_relaxed);
                state->samples[index].position.store(state->position.load(std::memory_order_relaxed), std::memory_order_relaxed);
                state->samples[index].category.store(cat, std::memory_order_relaxed);
                state->count.store(index + 1, std::memory_order_release);
            } else state->dropped.fetch_add(1, std::memory_order_relaxed);
            if (info->si_overrun > 0) state->dropped.fetch_add(static_cast<unsigned>(info->si_overrun), std::memory_order_relaxed);
        }
    }
    errno = savedError;
}
void install() {
    static std::once_flag once;
    std::call_once(once, [] {
        if (signalNumber() > SIGRTMAX) { errno = EINVAL; failed("Server sampler signal range"); }
        struct sigaction previous{};
        if (::sigaction(signalNumber(), nullptr, &previous)) failed("Server sampler inspect signal");
        if (previous.sa_handler != SIG_DFL) { errno = EBUSY; failed("Server sampler signal already owned"); }
        struct sigaction action{};
        action.sa_sigaction = capture;
        action.sa_flags = SA_SIGINFO | SA_RESTART;
        ::sigemptyset(&action.sa_mask);
        if (::sigaction(signalNumber(), &action, nullptr)) failed("Server sampler install signal");
    });
}
struct BlockSignal {
    sigset_t set{}, old{};
    BlockSignal() {
        ::sigemptyset(&set); ::sigaddset(&set, signalNumber());
        const int result = ::pthread_sigmask(SIG_BLOCK, &set, &old);
        if (result) { errno = result; failed("Server sampler block signal"); }
    }
    ~BlockSignal() {
        const int result = ::pthread_sigmask(SIG_SETMASK, &old, nullptr);
        if (result) { errno = result; failed("Server sampler restore signal mask"); }
    }
};
Sampler::~Sampler() {
    if (running) {
        bo1::android::ServerProfileSample discarded;
        unsigned lost;
        bo1::android::EndServerSamples(&discarded, 1, &lost);
    }
}
}
void bo1::android::BeginServerSamples(int mode, int scriptCategory) {
    install();
    BlockSignal blocked;
    if (sampler.running || (mode != 1 && mode != 2) || ::sigismember(&blocked.old, signalNumber())) {
        errno = EINVAL; failed("Server sampler begin state");
    }
    sampler.count.store(0); sampler.dropped.store(0);
    sampler.category.store(0); sampler.scriptCategory.store(scriptCategory);
    struct sigevent event{};
    event.sigev_notify = SIGEV_THREAD_ID;
    event.sigev_signo = signalNumber();
    event.sigev_notify_thread_id = ::gettid();
    event.sigev_value.sival_ptr = &sampler;
    if (::timer_create(CLOCK_MONOTONIC, &event, &sampler.timer)) failed("Server sampler create timer");
    sampler.running = true; sampler.mode.store(mode);
    itimerspec interval{};
    interval.it_value.tv_nsec = interval.it_interval.tv_nsec = 1000000;
    if (::timer_settime(sampler.timer, 0, &interval, nullptr)) failed("Server sampler arm timer");
}
void bo1::android::SetServerSampleCategory(int category) { sampler.category.store(category); }
void bo1::android::PublishServerScriptPosition(int instance, const char *position) {
    if (instance == 0) sampler.position.store(reinterpret_cast<std::uintptr_t>(position));
}
std::size_t bo1::android::EndServerSamples(ServerProfileSample *output, std::size_t capacity, unsigned *dropped) {
    BlockSignal blocked;
    if (!sampler.running || !output || !dropped) { errno = EINVAL; failed("Server sampler end state"); }
    sampler.mode.store(0);
    if (::timer_delete(sampler.timer)) failed("Server sampler delete timer");
    sampler.running = false;
    timespec immediate{};
    while (::sigtimedwait(&blocked.set, nullptr, &immediate) >= 0 || errno == EINTR) {}
    if (errno != EAGAIN) failed("Server sampler drain signal");
    const unsigned total = sampler.count.load(std::memory_order_acquire);
    const std::size_t copied = total < capacity ? total : capacity;
    for (std::size_t i = 0; i < copied; ++i) {
        output[i] = {sampler.samples[i].pc.load(), reinterpret_cast<const char *>(sampler.samples[i].position.load()), sampler.samples[i].category.load()};
    }
    *dropped = sampler.dropped.load() + total - static_cast<unsigned>(copied);
    return copied;
}
