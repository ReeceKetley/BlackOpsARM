#include <platform/win32_compat/win32_errors.h>
#include <atomic>
#include <cstdio>
#include <thread>
#include <vector>

int TestWin32ErrorState()
{
    using namespace bo1::win32;
    if (GetLastError() != 0)
        return 1;
    SetLastError(0x12345678u);
    std::atomic<int> failures{0};
    std::vector<std::thread> workers;
    for (DWORD id = 1; id <= 8; ++id) {
        workers.emplace_back([id, &failures] {
            if (GetLastError() != 0)
                ++failures;
            for (DWORD n = 0; n < 10000; ++n) {
                const DWORD error = id * 100000u + n;
                SetLastError(error);
                if ((n % 17u) == 0)
                    std::this_thread::yield();
                if (GetLastError() != error)
                    ++failures;
            }
        });
    }
    for (auto& worker : workers)
        worker.join();
    if (GetLastError() != 0x12345678u || failures.load() != 0)
        return 2;
    SetLastError(0);
    std::puts("PASS: initial error, full DWORD round trip, 8-thread isolation, 80000 concurrent updates");
    return 0;
}

#if defined(BO1_ERROR_TEST_MAIN)
int main()
{
    return TestWin32ErrorState();
}
#endif
