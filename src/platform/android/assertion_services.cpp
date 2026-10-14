#include "assertion_services.h"
#include "stack_trace.h"
#include <universal/assertive.h>
#include <atomic>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

void Sys_Error(char *format,...);
namespace {
std::atomic<bo1::android::AssertionReporter> reporter{nullptr};
std::atomic_flag handling=ATOMIC_FLAG_INIT;
void output(const char *text) {
    std::size_t remaining=std::strlen(text);
    while (remaining) {
        const ssize_t count=write(STDERR_FILENO,text,remaining);
        if (count<0 && errno==EINTR) continue;
        if (count<=0) break;
        text+=count;
        remaining-=static_cast<std::size_t>(count);
    }
}
[[noreturn]] void fatal(const char *message) {
    Sys_Error(const_cast<char *>("%s"),message);
    std::abort(); // Fail closed if the actual fatal owner unexpectedly returns.
}
}
void bo1::android::SetAssertionReporter(AssertionReporter callback) {
    reporter.store(callback,std::memory_order_release);
}
[[noreturn]] void bo1::android::ReportAssertion(const char *filename,int line,int type,
                                               const char *format,va_list arguments) {
    // Never block another assertion behind a mutex or an engine print callback.
    // State is intentionally not cleared: the handler always terminates.
    if (handling.test_and_set(std::memory_order_acq_rel)) {
        output("Native recursive/concurrent assertion: terminating\n");
        fatal("Recursive/concurrent assertion while handling a prior assertion");
    }
    char expression[1024]{};
    if (!format || std::vsnprintf(expression,sizeof(expression),format,arguments)<0)
        std::snprintf(expression,sizeof(expression),"Assertion expression formatting unavailable");
    expression[sizeof(expression)-1]=0;
    char message[1536]{};
    std::snprintf(message,sizeof(message),"ASSERT %s(%d) type=%d: %s\n",
                  filename?filename:"<unknown>",line,type,expression);
    message[sizeof(message)-1]=0;
    output(message);
    char stack[2048]{};
    output("Native assertion stack (program counters):\n");
    if (CaptureStackTrace(stack,sizeof(stack),1)) output(stack);
    else output("  No unwind frames available\n");
    if (const auto callback=reporter.load(std::memory_order_acquire)) {
        // No native or engine lock is held across reporting callbacks.
        try { callback(message); }
        catch (...) { fatal("Assertion reporting callback threw an exception"); }
    }
    fatal(message);
}
void Assert_SetMonkeyCallbackHandler(void (*callback)(const char *)) {
    bo1::android::SetAssertionReporter(callback);
}
bool Assert_MyHandler(const char *filename,int line,int type,const char *format,...) {
    va_list arguments;
    va_start(arguments,format);
    bo1::android::ReportAssertion(filename,line,type,format,arguments);
}
