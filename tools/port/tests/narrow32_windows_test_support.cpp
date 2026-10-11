// Standalone test diagnostics for the unchanged Windows handle implementation.
// These are linked only into the host probe, never into the engine.
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
void Bo1_Diag(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    std::vfprintf(stderr, fmt, args);
    va_end(args);
    std::fputc('\n', stderr);
}
void Bo1_DiagStack(const char *) { std::abort(); }
