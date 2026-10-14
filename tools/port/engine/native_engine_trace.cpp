// Bounded compiler instrumentation, outside the production function bodies.
#include <platform/common_boot_api.h>
#include <win32/win_localize.h>
#include <universal/timing.h>
#include <unistd.h>
#include <atomic>

static std::atomic<bool> initializationTraceEnabled{true};
extern "C" __attribute__((no_instrument_function))
bool Bo1_ConfigureInitializationTrace(bool rendered) {
    // Early constructor breadcrumbs stay enabled until the real entry has
    // validated its SDK roots/JNI/output capture. Ordinary engine prints and
    // native fatal diagnostics are independent of this optional address scan.
    const char *overrideValue = std::getenv("BO1_TRACE_STARTUP");
    if (overrideValue && std::strcmp(overrideValue, "0") && std::strcmp(overrideValue, "1")) {
        std::fputs("ENGINE ENTRY BO1_TRACE_STARTUP must be 0 or 1\n", stderr);
        return false;
    }
    const bool enabled = overrideValue ? !std::strcmp(overrideValue, "1") : !rendered;
    initializationTraceEnabled.store(enabled, std::memory_order_relaxed);
    std::fprintf(stderr, "ENGINE ENTRY pre-init startup trace scan enabled=%d owner=%s override=%d\n",
        enabled, rendered ? "SDK" : "CLI", overrideValue != nullptr);
    return true;
}
extern "C" __attribute__((no_instrument_function))
void Bo1_InitializationTraceReturned() {
    // Startup is genuinely complete. Retain an explicit baseline switch for
    // comparing the trace scan's CPU cost in the same executable.
    const char *baseline = std::getenv("BO1_TRACE_POSTINIT");
    if (baseline && std::strcmp(baseline,"0") && std::strcmp(baseline,"1")) {
        std::fputs("BO1_TRACE_POSTINIT must be 0 or 1\n",stderr);
        std::abort();
    }
    const bool retain = baseline && !std::strcmp(baseline,"1");
    initializationTraceEnabled.store(retain,std::memory_order_relaxed);
    std::fprintf(stderr,"ENGINE ENTRY post-init startup trace scan enabled=%d\n",retain);
}

struct TraceTarget {void *function; const char *name;};
static const TraceTarget targets[] = {
    {reinterpret_cast<void *>(Com_Init), "Com_Init"},
    {reinterpret_cast<void *>(Com_Init_Try_Block_Function), "Com_Init_Try_Block_Function"},
    {reinterpret_cast<void *>(Com_InitDvars), "Com_InitDvars"},
    {reinterpret_cast<void *>(Com_ParseCommandLine), "Com_ParseCommandLine"},
    {reinterpret_cast<void *>(Com_StartupVariable), "Com_StartupVariable"},
    {reinterpret_cast<void *>(Com_PrintMessage), "Com_PrintMessage"},
    {reinterpret_cast<void *>(PMem_Init), "PMem_Init"},
    {reinterpret_cast<void *>(Win_InitLocalization), "Win_InitLocalization"},
    {reinterpret_cast<void *>(Com_InitParse), "Com_InitParse"},
    {reinterpret_cast<void *>(Dvar_Init), "Dvar_Init"},
    {reinterpret_cast<void *>(InitTiming), "InitTiming"},
    {reinterpret_cast<void *>(SL_Init), "SL_Init"},
    {reinterpret_cast<void *>(Cbuf_Init), "Cbuf_Init"},
    {reinterpret_cast<void *>(Cmd_Init), "Cmd_Init"}}
;
extern "C" __attribute__((no_instrument_function))
void __cyg_profile_func_enter(void *function, void *) {
    if (!initializationTraceEnabled.load(std::memory_order_relaxed)) return;
    for (const auto &target : targets) if (target.function == function) {
        const char prefix[] = "T5 enter ";
        (void)write(2, prefix, sizeof(prefix)-1);
        (void)write(2, target.name, std::strlen(target.name));
        (void)write(2, "\n", 1);
        return;
    }
}
extern "C" __attribute__((no_instrument_function))
void __cyg_profile_func_exit(void *, void *) {}
