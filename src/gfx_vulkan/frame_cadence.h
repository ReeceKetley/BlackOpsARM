#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
namespace bo1::vulkan {
// Submission cadence uses the already sampled steady-clock timestamp. No GPU
// wait or per-frame log is added. Every interval contributes to one window.
class FrameCadence {
public:
    struct Window {unsigned frames{};double seconds{},fps{},p50{},p95{},p99{},maximum{};};
    std::optional<Window> Observe(std::uint64_t nanoseconds) {
        if(!last_||nanoseconds<=last_) {last_=start_=nanoseconds;count_=0;return {};}
        intervals_[count_++]=double(nanoseconds-last_)/1e6;last_=nanoseconds;
        if(nanoseconds-start_<1000000000ull&&count_<intervals_.size())return {};
        std::sort(intervals_.begin(),intervals_.begin()+count_);
        Window result;result.frames=count_;result.seconds=double(nanoseconds-start_)/1e9;
        result.fps=double(count_)/result.seconds;
        const auto percentile=[&](unsigned percent){return intervals_[(std::size_t(count_)*percent+99)/100-1];};
        result.p50=percentile(50);result.p95=percentile(95);result.p99=percentile(99);result.maximum=intervals_[count_-1];
        start_=nanoseconds;count_=0;return result;
    }
private:
    std::uint64_t last_{},start_{};unsigned count_{};
    std::array<double,1024> intervals_{};
};
}
