#include <cstdio>
#include <cstdlib>
#include <mutex>
#if defined(BO1_NATIVE_SYSTEM_CORE)
#include "ui_platform.h"
#endif

void Platform_Print(char *message) {
    static std::mutex outputMutex;
    std::lock_guard<std::mutex> guard(outputMutex);
#if defined(BO1_NATIVE_SYSTEM_CORE)
    bo1::android::AppendAndroidConsoleOutput(message);
#endif
    if (!message || std::fputs(message, stdout) == EOF || std::fflush(stdout)) {
        std::fputs("Android console output failed\n", stderr);
        std::abort();
    }
}
