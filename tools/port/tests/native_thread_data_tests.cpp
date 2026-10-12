#include <platform/android/thread_services.h>
#include <universal/thread_data.h>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#if defined(_WIN32)
#include <crtdbg.h>
#endif
#if defined(__ANDROID__)
#include <platform/android/hunk_services.h>
#endif

namespace {
std::atomic<int> ready{0};
std::atomic<bool> release{false};
void check(bool condition) { if (!condition) { std::fputs("Thread data test assertion failed\n", stderr); std::abort(); } }
void ownJump() {
    // No owning C++ locals inside this jump scope. This proves TLS/native libc
    // mechanics only, not production Com_Error cleanup or recovery.
    volatile int value = 0;
    jmp_buf *buffer = &g_com_error[Sys_GetThreadContext()];
    check(Sys_GetValue(2) == *buffer);
    switch (setjmp(*buffer)) {
    case 0: value = 19; longjmp(*buffer, 7);
    case 7: break;
    default: check(false);
    }
    check(value == 19);
}
void verify(int role) {
    check(Sys_GetThreadContext() == role);
    check(Sys_IsMainThread() == (role == 0));
    check(Sys_IsRenderThread() == (role == 1));
    check(Sys_IsDatabaseThread() == (role == 13));
    check(Sys_GetValue(1) == &va_info[role]);
    check(Sys_GetValue(2) == g_com_error[role]);
    check(Sys_GetValue(3) == &g_traceThreadInfo[role]);
    check(Sys_GetValue(4) == &g_cmd_args[role == 1 ? 1 : 0]);
    int fullWidth = 41 + role;
    Sys_SetValue(0, &fullWidth);
    check(Sys_GetValue(0) == &fullWidth);
    if (sizeof(void *) == 8) check(reinterpret_cast<std::uintptr_t>(Sys_GetValue(0)) > UINT32_MAX);
    va_info[role].index = role + 100;
    va_info[role].va_string[3][1023] = static_cast<char>(role + 5);
    g_traceThreadInfo[role].checkcount.global = role + 200;
    g_traceThreadInfo[role].checkcount.partitions = &fullWidth;
    Sys_InitThread(role);
    check(Sys_GetValue(0) == &fullWidth);
    check(va_info[role].index == role + 100 && va_info[role].va_string[3][1023] == role + 5);
    check(g_traceThreadInfo[role].checkcount.global == role + 200);
    check(g_traceThreadInfo[role].checkcount.partitions == &fullWidth);
    ownJump();
    ready.fetch_add(1, std::memory_order_release);
    if (role == 0) {
        while (ready.load(std::memory_order_acquire) != 3) std::this_thread::yield();
        release.store(true, std::memory_order_release);
    }
    while (!release.load(std::memory_order_acquire)) std::this_thread::yield();
    check(va_info[role].index == role + 100);
    check(g_traceThreadInfo[role].checkcount.global == role + 200);
    // No concurrent mutation of the original shared main/database CmdArgs.
    Sys_SetValue(0, nullptr);
    g_traceThreadInfo[role].checkcount.partitions = nullptr;
}
}
int main(int argc, char **argv) {
    // Test-owned watchdog also terminates a stuck device child after an ADB
    // client timeout. No worker or allocation loop can run indefinitely.
    std::thread([] {
        std::this_thread::sleep_for(std::chrono::seconds(20));
        std::fputs("Thread data test watchdog expired\n", stderr);
        std::fflush(stderr);
        std::_Exit(2);
    }).detach();
#if defined(_WIN32)
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
#endif
    if (argc == 2) {
        const char *mode = argv[1];
        if (!std::strcmp(mode, "unregistered_get")) Sys_GetValue(0);
        else if (!std::strcmp(mode, "unregistered_set")) Sys_SetValue(0, nullptr);
        else if (!std::strcmp(mode, "unregistered_context")) Sys_GetThreadContext();
        else if (!std::strcmp(mode, "invalid_role_negative")) Sys_InitThread(-1);
        else if (!std::strcmp(mode, "invalid_role_count")) Sys_InitThread(15);
        else if (!std::strcmp(mode, "unsupported_server")) Sys_InitThread(10);
        else {
            Sys_InitMainThread();
            if (!std::strcmp(mode, "slot_negative_get")) Sys_GetValue(-1);
            else if (!std::strcmp(mode, "slot_count_get")) Sys_GetValue(5);
            else if (!std::strcmp(mode, "slot_negative_set")) Sys_SetValue(-1, nullptr);
            else if (!std::strcmp(mode, "slot_count_set")) Sys_SetValue(5, nullptr);
            else if (!std::strcmp(mode, "role_switch")) Sys_InitThread(1);
            else if (!std::strcmp(mode, "main_retire")) bo1::android::RetireThreadRole();
            else if (!std::strcmp(mode, "duplicate_main")) { std::thread thread([]{Sys_InitMainThread();}); thread.join(); }
            else if (!std::strcmp(mode, "duplicate_database")) { std::thread thread([] { Sys_InitThread(13); ready = 1; while (!release) std::this_thread::yield(); bo1::android::RetireThreadRole(); }); while (!ready) std::this_thread::yield(); std::thread duplicate([]{Sys_InitThread(13);}); duplicate.join(); release=true; thread.join(); }
            else if (!std::strcmp(mode, "exit_without_retirement")) { std::thread thread([] { Sys_InitThread(13); }); thread.join(); }
            else if (!std::strcmp(mode, "retired_get")) { std::thread thread([] { Sys_InitThread(13); bo1::android::RetireThreadRole(); Sys_GetValue(1); }); thread.join(); }
            else if (!std::strcmp(mode, "retired_rebind")) { std::thread thread([] { Sys_InitThread(13); bo1::android::RetireThreadRole(); Sys_InitThread(13); }); thread.join(); }
            else if (!std::strcmp(mode, "retired_role_reuse")) { std::thread first([] { Sys_InitThread(13); bo1::android::RetireThreadRole(); }); first.join(); std::thread second([] { Sys_InitThread(13); }); second.join(); }
            else return 2;
        }
        return 2; // rejection must abort rather than silently return.
    }
    check(!Sys_IsMainThread() && !Sys_IsRenderThread() && !Sys_IsDatabaseThread());
    Sys_InitMainThread();
#if defined(__ANDROID__)
    check(bo1::android::IsHunkMainThread());
#endif
    for (auto &args : g_cmd_args) {
        args.nesting = 9; args.totalUsedArgvPool = 42; args.totalUsedTextPool = 71;
        args.usedTextPool[4] = 67; args.textPool[8191] = 'Q';
        Cmd_ResetArgs(&args);
        check(args.nesting == -1 && args.totalUsedArgvPool == 0 && args.totalUsedTextPool == 0);
        check(args.usedTextPool[4] == 67 && args.textPool[8191] == 'Q');
    }
    std::thread backend([] { Sys_InitThread(1); verify(1); bo1::android::RetireThreadRole(); check(!Sys_IsRenderThread()); });
    std::thread database([] { Sys_InitThread(13); verify(13); bo1::android::RetireThreadRole(); check(!Sys_IsDatabaseThread()); });
    verify(0);
    backend.join(); database.join();
    std::printf("{\"record\":\"layout\",\"pointer_bytes\":%zu,\"cmdargs_bytes\":%zu,\"trace_bytes\":%zu,\"checkcount_bytes\":%zu,\"va_bytes\":%zu,\"va_index_offset\":%zu,\"cmd_item_offset\":%zu,\"cmd_argv_offset\":%zu,\"cmd_text_offset\":%zu,\"cmd_argvpool_offset\":%zu,\"cmd_usedtext_offset\":%zu,\"trace_partitions_offset\":%zu,\"trace_brush_offset\":%zu}\n", sizeof(void*),sizeof(CmdArgs),sizeof(TraceThreadInfo),sizeof(TraceCheckCount),sizeof(va_info_t),offsetof(va_info_t,index),offsetof(CmdArgs,itemDef),offsetof(CmdArgs,argv),offsetof(CmdArgs,textPool),offsetof(CmdArgs,argvPool),offsetof(CmdArgs,usedTextPool),offsetof(TraceThreadInfo,checkcount)+offsetof(TraceCheckCount,partitions),offsetof(TraceThreadInfo,box_brush));
    std::puts("{\"record\":\"verified\",\"live_roles\":3,\"canonical_slots\":12,\"native_jumps\":3,\"cmd_resets\":2,\"retired_workers\":2,\"pass\":true}");
}
