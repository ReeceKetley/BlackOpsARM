#include <platform/android/water_mutex.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <future>
#include <thread>

static void require(bool condition, const char *message) {
    if (!condition) { std::fprintf(stderr, "FAIL: %s\n", message); std::abort(); }
}
int main() {
    using namespace bo1::android;
    using namespace std::chrono_literals;
    void *handle = InitializeWaterUpdateMutex();
    require(InitializeWaterUpdateMutex() == handle, "process owner reused");
    require(LockWaterUpdateMutex(nullptr) == 0xFFFFFFFFu && !UnlockWaterUpdateMutex(nullptr), "invalid handles fail");
    require(LockWaterUpdateMutex(handle) == 0 && LockWaterUpdateMutex(handle) == 0, "recursive acquisition");
    std::promise<void> attempting, acquired;
    auto attemptingFuture = attempting.get_future();
    auto acquiredFuture = acquired.get_future();
    std::thread contender([&] {
        require(!UnlockWaterUpdateMutex(handle), "non-owner release rejected");
        attempting.set_value();
        require(LockWaterUpdateMutex(handle) == 0, "contender acquisition");
        acquired.set_value();
        require(UnlockWaterUpdateMutex(handle), "contender release");
    });
    attemptingFuture.wait();
    require(acquiredFuture.wait_for(50ms) == std::future_status::timeout, "other thread waits");
    require(UnlockWaterUpdateMutex(handle), "first recursive release");
    require(acquiredFuture.wait_for(50ms) == std::future_status::timeout, "one release retains ownership");
    require(UnlockWaterUpdateMutex(handle), "final recursive release");
    require(acquiredFuture.wait_for(2s) == std::future_status::ready, "final release admits contender");
    contender.join();
    require(!UnlockWaterUpdateMutex(handle), "unowned release rejected");
    std::puts("PASS native water mutex: recursive ownership, contention, retirement and invalid handles");
}
