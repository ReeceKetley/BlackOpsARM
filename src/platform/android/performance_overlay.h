#pragma once
#include <cstdint>
namespace bo1::android::performance {
// Fixed synchronized cache. Negative values are unavailable, never zero placeholders.
enum class GpuStatus { Unavailable, Pending, Valid, Unsupported, Failed, Disjoint };
struct Snapshot {
    double renderFps{-1}, intervalMs{-1}, renderCpuMs{-1}, mainCpuMs{-1};
    double processCpuPercent{-1}, rssMiB{-1};
    double gpuMs{-1}, gpuAgeSeconds{-1};
    GpuStatus gpuStatus{GpuStatus::Unavailable};
    double renderAgeSeconds{-1}, mainAgeSeconds{-1}, processAgeSeconds{-1}, rssAgeSeconds{-1};
};
bool SetEnabled(bool enabled);
bool Enabled();
unsigned Epoch();
void GpuObservation(unsigned epoch, GpuStatus status, double ms, std::uint64_t stamp);
void Present(bool activeSuccessfulSurface);
std::uint64_t MainBegin();
void MainEnd(std::uint64_t begin);
Snapshot Read();
}
