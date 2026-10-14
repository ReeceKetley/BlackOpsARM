#include <cstdio>
#include <cstdlib>
void Sys_DirectXFatalError() {
    // Android has no Windows fatal-error dialog. Preserve terminal failure and
    // publish the engine's preceding error details through the native stderr log.
    std::fputs("Native renderer fatal error: initialization cannot continue\n",stderr);
    std::fflush(stderr);
    std::abort();
}
