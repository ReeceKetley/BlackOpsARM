#include "performance_overlay.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <thread>
#include <system_error>
#include <time.h>
#include <cerrno>

namespace bo1::android::performance {
namespace {
bool Clock(clockid_t id, std::uint64_t &ns) {
    const int saved = errno;
    timespec value{};
    const bool ok = clock_gettime(id, &value) == 0 && value.tv_sec >= 0
        && value.tv_nsec >= 0 && value.tv_nsec < 1000000000;
    errno = saved;
    if (ok) ns = static_cast<std::uint64_t>(value.tv_sec) * 1000000000ULL + value.tv_nsec;
    return ok;
}
struct Cache {
    std::mutex mutex;
    std::condition_variable wake;
    std::thread sampler;
    std::atomic<bool> enabled{};
    bool stop{};
    unsigned generation{};
    Snapshot snapshot;
    std::uint64_t renderStamp{}, mainStamp{}, processStamp{}, rssStamp{}, gpuStamp{};
    std::uint64_t lastPresent{}, lastRenderCpu{}, renderSpan{}, renderCpuSum{}, mainCpuSum{}, mainWindow{};
    unsigned intervals{}, renderCpuSamples{}, mainSamples{};
    std::thread::id renderOwner;
    ~Cache() {
        { std::lock_guard<std::mutex> lock(mutex); stop = true; }
        wake.notify_all();
        if (sampler.joinable()) sampler.join();
    }
    void Reset() {
        snapshot = {}; renderStamp = mainStamp = processStamp = rssStamp = gpuStamp = 0;
        lastPresent = lastRenderCpu = renderSpan = renderCpuSum = mainCpuSum = mainWindow = 0;
        intervals = renderCpuSamples = mainSamples = 0; renderOwner = {};
    }
};
Cache cache;
thread_local unsigned mainGeneration;
double Rss() {
    const int saved = errno;
    FILE *status = std::fopen("/proc/self/status", "r");
    double result = -1;
    if (status) {
        char line[256];
        while (std::fgets(line, sizeof(line), status)) {
            if (std::strncmp(line, "VmRSS:", 6) != 0) continue;
            unsigned long long kib = 0; char unit[8]{};
            if (std::sscanf(line + 6, "%llu %7s", &kib, unit) == 2 && std::strcmp(unit, "kB") == 0)
                result = static_cast<double>(kib) / 1024.0;
            break;
        }
        if (std::ferror(status)) result = -1;
        std::fclose(status);
    }
    errno = saved;
    return result;
}
void Sample() {
    std::unique_lock<std::mutex> lock(cache.mutex);
    while (!cache.stop) {
        cache.wake.wait(lock, [] { return cache.stop || cache.enabled.load(); });
        if (cache.stop) break;
        const unsigned generation = cache.generation;
        std::uint64_t previousWall = 0, previousCpu = 0, lastRss = 0;
        bool previousValid = false;
        while (!cache.stop && cache.enabled.load() && generation == cache.generation) {
            lock.unlock();
            std::uint64_t now = 0, cpu = 0;
            const bool valid = Clock(CLOCK_MONOTONIC, now) && Clock(CLOCK_PROCESS_CPUTIME_ID, cpu);
            double percent = -1;
            if (valid && previousValid && now > previousWall && cpu >= previousCpu)
                percent = 100.0 * static_cast<double>(cpu - previousCpu) / static_cast<double>(now - previousWall);
            const bool sampleRss = valid && (!lastRss || now - lastRss >= 1000000000ULL);
            const double rss = sampleRss ? Rss() : -1;
            lock.lock();
            if (cache.enabled.load() && generation == cache.generation) {
                cache.snapshot.processCpuPercent = percent;
                cache.processStamp = percent >= 0 ? now : 0;
                if (sampleRss) { cache.snapshot.rssMiB = rss; cache.rssStamp = rss >= 0 ? now : 0; lastRss = now; }
            }
            previousValid = valid; previousWall = now; previousCpu = cpu;
            cache.wake.wait_for(lock, std::chrono::milliseconds(500), [generation] {
                return cache.stop || !cache.enabled.load() || generation != cache.generation;
            });
        }
    }
}
double Age(std::uint64_t now, std::uint64_t stamp) {
    return stamp && now >= stamp ? static_cast<double>(now - stamp) / 1e9 : -1;
}
}
bool SetEnabled(bool enabled) {
    std::lock_guard<std::mutex> lock(cache.mutex);
    if (enabled && !cache.sampler.joinable()) {
        try { cache.sampler = std::thread(Sample); }
        catch (const std::system_error &) { return false; }
    }
    if (enabled != cache.enabled.load()) {
        cache.enabled.store(enabled); ++cache.generation; cache.Reset(); cache.wake.notify_all();
    }
    return true;
}
bool Enabled() { return cache.enabled.load(std::memory_order_relaxed); }
unsigned Epoch() {
    if (!Enabled()) return 0;
    std::lock_guard<std::mutex> lock(cache.mutex);
    return cache.enabled.load() ? cache.generation : 0;
}
void GpuObservation(unsigned epoch, GpuStatus status, double ms, std::uint64_t stamp) {
    std::uint64_t now = 0;
    const bool valid = Clock(CLOCK_MONOTONIC, now);
    std::lock_guard<std::mutex> lock(cache.mutex);
    if (!epoch || !cache.enabled.load() || epoch != cache.generation) return;
    // Pending queries do not erase a resolved sample; its real age still grows.
    if (status == GpuStatus::Pending && (cache.snapshot.gpuStatus == GpuStatus::Valid
        || cache.snapshot.gpuStatus == GpuStatus::Pending)) return;
    if (status == GpuStatus::Valid && (!valid || !stamp || now < stamp || ms < 0))
        status = GpuStatus::Failed;
    cache.snapshot.gpuStatus = status;
    cache.snapshot.gpuMs = status == GpuStatus::Valid ? ms : -1;
    cache.gpuStamp = valid ? (stamp && now >= stamp ? stamp : now) : 0;
}
void Present(bool active) {
    if (!Enabled()) return;
    std::uint64_t now = 0, cpu = 0;
    const bool wallValid = Clock(CLOCK_MONOTONIC, now);
    const bool cpuValid = Clock(CLOCK_THREAD_CPUTIME_ID, cpu);
    std::lock_guard<std::mutex> lock(cache.mutex);
    if (!cache.enabled.load()) return;
    if (!active || !wallValid) {
        cache.lastPresent = cache.lastRenderCpu = cache.renderSpan = cache.renderCpuSum = 0;
        cache.intervals = cache.renderCpuSamples = 0;
        cache.snapshot.renderFps = cache.snapshot.intervalMs = cache.snapshot.renderCpuMs = -1;
        cache.renderStamp = 0;
        return;
    }
    const auto owner = std::this_thread::get_id();
    if (cache.lastPresent && now > cache.lastPresent && now - cache.lastPresent <= 1500000000ULL
        && cache.renderOwner == owner) {
        cache.renderSpan += now - cache.lastPresent; ++cache.intervals;
        if (cpuValid && cache.lastRenderCpu && cpu >= cache.lastRenderCpu) {
            cache.renderCpuSum += cpu - cache.lastRenderCpu; ++cache.renderCpuSamples;
        }
        if (cache.renderSpan >= 500000000ULL) {
            cache.snapshot.renderFps = static_cast<double>(cache.intervals) * 1e9 / cache.renderSpan;
            cache.snapshot.intervalMs = static_cast<double>(cache.renderSpan) / cache.intervals / 1e6;
            cache.snapshot.renderCpuMs = cache.renderCpuSamples == cache.intervals
                ? static_cast<double>(cache.renderCpuSum) / cache.renderCpuSamples / 1e6 : -1;
            cache.renderStamp = now;
            cache.renderSpan = cache.renderCpuSum = 0; cache.intervals = cache.renderCpuSamples = 0;
        }
    } else {
        cache.renderSpan = cache.renderCpuSum = 0; cache.intervals = cache.renderCpuSamples = 0;
        cache.snapshot.renderFps = cache.snapshot.intervalMs = cache.snapshot.renderCpuMs = -1;
        cache.renderStamp = 0;
    }
    cache.lastPresent = now; cache.lastRenderCpu = cpuValid ? cpu : 0; cache.renderOwner = owner;
}
std::uint64_t MainBegin() {
    std::uint64_t cpu = 0;
    if (!Enabled() || !Clock(CLOCK_THREAD_CPUTIME_ID, cpu)) return 0;
    std::lock_guard<std::mutex> lock(cache.mutex);
    mainGeneration = cache.generation;
    return cache.enabled.load() ? cpu : 0;
}
void MainEnd(std::uint64_t begin) {
    if (!begin || !Enabled()) return;
    std::uint64_t now = 0, cpu = 0;
    if (!Clock(CLOCK_MONOTONIC, now) || !Clock(CLOCK_THREAD_CPUTIME_ID, cpu) || cpu < begin) return;
    std::lock_guard<std::mutex> lock(cache.mutex);
    // A main Com_Frame CPU sample belongs to the client loop, never to server FPS.
    if (!cache.enabled.load() || mainGeneration != cache.generation || !cache.lastPresent || now < cache.lastPresent
        || now - cache.lastPresent > 1500000000ULL) return;
    if (!cache.mainWindow) cache.mainWindow = now;
    cache.mainCpuSum += cpu - begin; ++cache.mainSamples;
    if (now - cache.mainWindow >= 500000000ULL) {
        cache.snapshot.mainCpuMs = static_cast<double>(cache.mainCpuSum) / cache.mainSamples / 1e6;
        cache.mainStamp = now; cache.mainWindow = now; cache.mainCpuSum = 0; cache.mainSamples = 0;
    }
}
Snapshot Read() {
    std::uint64_t now = 0;
    const bool valid = Clock(CLOCK_MONOTONIC, now);
    std::lock_guard<std::mutex> lock(cache.mutex);
    Snapshot result = cache.snapshot;
    if (!cache.enabled.load() || !valid) return {};
    result.renderAgeSeconds = Age(now, cache.renderStamp);
    result.mainAgeSeconds = Age(now, cache.mainStamp);
    result.processAgeSeconds = Age(now, cache.processStamp);
    result.rssAgeSeconds = Age(now, cache.rssStamp);
    result.gpuAgeSeconds = Age(now, cache.gpuStamp);
    return result;
}
}
