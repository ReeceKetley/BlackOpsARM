#include <platform/win32_compat/win32_time.h>
#include <platform/win32_compat/win32_errors.h>
#include <cstdio>
#include <cstdlib>
#include <signal.h>
#include <pthread.h>
#include <sys/wait.h>
#include <thread>
#include <time.h>
#include <unistd.h>

int TestWin32ErrorState();
#define CHECK(condition) do { if (!(condition)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); std::abort(); } } while (0)

namespace {
volatile sig_atomic_t signalReceived = 0;
void catchSignal(int) { signalReceived = 1; }
long long monotonicNanoseconds()
{
    timespec time{};
    CHECK(clock_gettime(CLOCK_MONOTONIC, &time) == 0);
    return static_cast<long long>(time.tv_sec) * 1000000000LL + time.tv_nsec;
}
}

int main()
{
    using namespace bo1::win32;
    CHECK(TestWin32ErrorState() == 0);
    LARGE_INTEGER frequency{};
    SetLastError(0x1234u);
    CHECK(QueryPerformanceFrequency(&frequency) != 0);
    CHECK(frequency.QuadPart == 1000000000LL);
    CHECK(GetLastError() == 0x1234u);
    CHECK(QueryPerformanceFrequency(nullptr) == 0);
    CHECK(GetLastError() == ERROR_INVALID_PARAMETER);
    SetLastError(0x5678u);
    Sleep(0);
    CHECK(GetLastError() == 0x5678u);
    const auto start = monotonicNanoseconds();
    Sleep(20);
    CHECK(monotonicNanoseconds() - start >= 20000000LL);
    CHECK(GetLastError() == 0x5678u);

    struct sigaction action{};
    action.sa_handler = catchSignal;
    CHECK(sigemptyset(&action.sa_mask) == 0);
    CHECK(sigaction(SIGUSR1, &action, nullptr) == 0);
    const pthread_t sleeper = pthread_self();
    std::thread interrupter([sleeper] {
        bo1::win32::Sleep(10);
        CHECK(pthread_kill(sleeper, SIGUSR1) == 0);
    });
    const auto interruptedStart = monotonicNanoseconds();
    Sleep(60);
    interrupter.join();
    CHECK(signalReceived != 0);
    CHECK(monotonicNanoseconds() - interruptedStart >= 60000000LL);
    CHECK(GetLastError() == 0x5678u);

    int ready[2];
    CHECK(pipe(ready) == 0);
    const pid_t child = fork();
    CHECK(child >= 0);
    if (child == 0) {
        close(ready[0]);
        const char byte = 'r';
        CHECK(write(ready[1], &byte, 1) == 1);
        Sleep(INFINITE);
        _exit(99); // Must never be reached.
    }
    close(ready[1]);
    char byte = 0;
    CHECK(read(ready[0], &byte, 1) == 1);
    close(ready[0]);
    CHECK(kill(child, SIGUSR1) == 0);
    Sleep(20);
    int status = 0;
    CHECK(waitpid(child, &status, WNOHANG) == 0);
    CHECK(kill(child, SIGKILL) == 0); // Only this test's own forked child.
    CHECK(waitpid(child, &status, 0) == child);
    CHECK(WIFSIGNALED(status) && WTERMSIG(status) == SIGKILL);
    std::puts("PASS: frequency/null failure, preserved error, zero/finite/EINTR/infinite Sleep");
    return 0;
}
