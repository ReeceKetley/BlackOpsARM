#pragma once
#if defined(__ANDROID__)
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <time.h>

// Measurement only. Every scope belongs to the actual calling OS thread.
// Inclusive scopes overlap: their totals must not be added as independent work.
namespace bo1::android::frame_timing {
#define BO1_FRAME_CLOCK_FN __attribute__((no_instrument_function))
enum Kind {
    MainFrame, PacingSleep, MainSVFrame, MainServerWait, CompletionPoll,
    AssistWork, StaleHandshake, Yield, SyncThreads, SyncRenderer, SyncWorkers,
    ServerIdle, ServerWork, ServerPre, ServerRun, GameRun, ServerSave, ServerPost,
    NetworkLoop, NetworkWait, WakeReset, PostDebugger, PostTimeouts, PostAuth,
    SnapshotSend, SnapshotFarCheck, SnapshotAnimCollect, SnapshotBuildClients,
    SnapshotTransmitClients, SnapshotBuild, SnapshotWrite, SnapshotTransmit,
    ArchiveSerialize, ArchiveCommit, Count
};
inline constexpr const char *names[Count] = {
    "main_frame", "pacing_sleep", "sv_frame_main", "server_sync_main", "completion_poll",
    "assist_work", "stale_handshake", "yield", "com_sync", "render_sync", "worker_sync",
    "server_idle", "server_work", "server_pre", "server_run", "game_run", "server_save",
    "server_post", "network_loop", "network_wait", "wake_reset", "post_debugger", "post_timeouts",
    "post_auth", "snapshot_send", "snapshot_far_check", "snapshot_anim_collect", "snapshot_build_clients",
    "snapshot_transmit_clients", "snapshot_build", "snapshot_write", "snapshot_transmit",
    "archive_serialize", "archive_commit"
};
struct Sample { std::uint64_t wall{}, cpu{}; bool wallValid{}, cpuValid{}; };
BO1_FRAME_CLOCK_FN inline bool Clock(clockid_t id, std::uint64_t &ns) {
    timespec value{};
    const int savedErrno = errno;
    const bool ok = clock_gettime(id, &value) == 0 && value.tv_sec >= 0
        && value.tv_nsec >= 0 && value.tv_nsec < 1000000000;
    errno = savedErrno;
    if (ok) ns = static_cast<std::uint64_t>(value.tv_sec) * 1000000000ULL + value.tv_nsec;
    return ok;
}
BO1_FRAME_CLOCK_FN inline Sample Now() {
    Sample sample;
    sample.wallValid = Clock(CLOCK_MONOTONIC, sample.wall);
    sample.cpuValid = Clock(CLOCK_THREAD_CPUTIME_ID, sample.cpu);
    return sample;
}
struct Aggregate {
    std::uint64_t calls{}, wallSamples{}, cpuSamples{}, wall{}, cpu{}, wallMax{}, cpuMax{};
};
struct ThreadData {
    Aggregate values[Count]{};
    bool enabled{};
    unsigned wakes{}, reportedWakes{};
    int frameMsec{};
    Sample windowStart{};
    std::uint64_t previousMainWall{}, previousMainCpu{}, previousSVWall{};
    unsigned previousBatchWakes{};
    bool previousValid{}, previousCpuValid{}, previousSVValid{};
};
inline thread_local ThreadData data;
BO1_FRAME_CLOCK_FN inline void Add(Kind kind, const Sample &start, const Sample &end) {
    Aggregate &item = data.values[kind];
    ++item.calls;
    if (start.wallValid && end.wallValid && end.wall >= start.wall) {
        const std::uint64_t elapsed = end.wall - start.wall;
        ++item.wallSamples; item.wall += elapsed;
        if (elapsed > item.wallMax) item.wallMax = elapsed;
    }
    if (kind == MainSVFrame) {
        data.previousSVValid = start.wallValid && end.wallValid && end.wall >= start.wall;
        data.previousSVWall = data.previousSVValid ? end.wall - start.wall : 0;
    }
    if (start.cpuValid && end.cpuValid && end.cpu >= start.cpu) {
        const std::uint64_t elapsed = end.cpu - start.cpu;
        ++item.cpuSamples; item.cpu += elapsed;
        if (elapsed > item.cpuMax) item.cpuMax = elapsed;
    }
}
class Scope {
    Kind kind; Sample start{}; bool active;
public:
    BO1_FRAME_CLOCK_FN explicit Scope(Kind value) : kind(value), active(data.enabled) {
        if (active) start = Now();
    }
    BO1_FRAME_CLOCK_FN void Stop() {
        if (active) { Add(kind, start, Now()); active = false; }
    }
    BO1_FRAME_CLOCK_FN ~Scope() { Stop(); }
    Scope(const Scope &) = delete;
    Scope &operator=(const Scope &) = delete;
};
BO1_FRAME_CLOCK_FN inline double Milliseconds(std::uint64_t ns) { return ns / 1000000.0; }
BO1_FRAME_CLOCK_FN inline void Dump(const char *owner, unsigned index, int simTime, int tickMsec) {
    char line[4096];
    const Sample end = Now();
    const bool spanValid = data.windowStart.wallValid && end.wallValid && end.wall >= data.windowStart.wall;
    int used = std::snprintf(line, sizeof(line),
        "BO1_NATIVE_TIMING owner=%s index=%u sim_time=%d tick_msec=%d window_wall_valid=%d window_wall_ms=%.3f units=ms inclusive=1",
        owner, index, simTime, tickMsec, spanValid,
        spanValid ? Milliseconds(end.wall - data.windowStart.wall) : -1.0);
    for (int k = 0; k < Count && used > 0 && used < static_cast<int>(sizeof(line)) - 2; ++k) {
        const Aggregate &a = data.values[k];
        if (!a.calls) continue;
        const int added = std::snprintf(line + used, sizeof(line) - used,
            " %s={n:%llu,w_valid:%llu,c_valid:%llu,w:%.3f,c:%.3f,w_max:%.3f,c_max:%.3f}", names[k],
            static_cast<unsigned long long>(a.calls), static_cast<unsigned long long>(a.wallSamples),
            static_cast<unsigned long long>(a.cpuSamples), a.wallSamples ? Milliseconds(a.wall) : -1.0,
            a.cpuSamples ? Milliseconds(a.cpu) : -1.0, a.wallSamples ? Milliseconds(a.wallMax) : -1.0,
            a.cpuSamples ? Milliseconds(a.cpuMax) : -1.0);
        if (added < 0 || added >= static_cast<int>(sizeof(line)) - used) { used = -1; break; }
        used += added;
    }
    if (used > 0 && used < static_cast<int>(sizeof(line)) - 1) {
        line[used++] = '\n';
        std::fwrite(line, 1, static_cast<std::size_t>(used), stderr);
    } else std::fputs("BO1_NATIVE_TIMING diagnostic_buffer_exhausted\n", stderr);
    for (auto &value : data.values) value = {};
    data.windowStart = Now(); // Excludes diagnostic output from the following window.
}
BO1_FRAME_CLOCK_FN inline void MainBegin(bool enabled) {
    if (enabled && !data.enabled) {
        for (auto &value : data.values) value = {};
        data.windowStart = Now();
    }
    data.enabled = enabled;
}
BO1_FRAME_CLOCK_FN inline void MainEnd(unsigned startWakes, const Sample &start) {
    if (!data.enabled) return;
    const Sample end = Now();
    data.previousValid = start.wallValid && end.wallValid && end.wall >= start.wall;
    data.previousMainWall = data.previousValid ? end.wall - start.wall : 0;
    data.previousCpuValid = start.cpuValid && end.cpuValid && end.cpu >= start.cpu;
    data.previousMainCpu = data.previousCpuValid ? end.cpu - start.cpu : 0;
    data.previousBatchWakes = data.wakes - startWakes;
    if (data.wakes != data.reportedWakes
        && (data.wakes <= 3 || data.wakes / 100 != data.reportedWakes / 100)) {
        Dump("main", data.wakes, -1, data.frameMsec);
        data.reportedWakes = data.wakes;
    }
}
BO1_FRAME_CLOCK_FN inline void Hitch(int rawMsec) {
    std::fprintf(stderr,
        "BO1_NATIVE_HITCH raw_msec=%d dedicated_clamp_msec=5000 previous_main_valid=%d previous_main_wall_ms=%.3f previous_main_cpu_ms=%.3f previous_sv_main_wall_ms=%.3f previous_batch_wakes=%u\n",
        rawMsec, data.previousValid, data.previousValid ? Milliseconds(data.previousMainWall) : -1.0,
        data.previousCpuValid ? Milliseconds(data.previousMainCpu) : -1.0,
        data.previousSVValid ? Milliseconds(data.previousSVWall) : -1.0, data.previousBatchWakes);
}
#undef BO1_FRAME_CLOCK_FN
}
#endif
