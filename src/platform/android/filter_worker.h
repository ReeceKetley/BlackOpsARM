#pragma once
#include <atomic>
#include <functional>
#include <thread>
#include <utility>
#include <cstdio>
#include <cstdlib>
namespace bo1::android {
class FilterWorker {
    std::atomic<bool> active_{true};
    std::thread thread_;
public:
    explicit FilterWorker(std::function<void()> work)
        : thread_([this, work=std::move(work)] {
            work();
            active_.store(false,std::memory_order_release);
        }) {}
    FilterWorker(const FilterWorker &) = delete;
    bool Active() const { return active_.load(std::memory_order_acquire); }
    void Join() {
        if (!thread_.joinable()) return;
        if (thread_.get_id()==std::this_thread::get_id()) {
            std::fputs("CubeMap filter worker cannot join itself\n",stderr);
            std::abort();
        }
        thread_.join();
    }
    ~FilterWorker() { Join(); }
};
}
