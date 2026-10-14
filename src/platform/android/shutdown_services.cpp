#include "shutdown_services.h"
#include "thread_services.h"
#include "server_thread.h"
#include "process_guard.h"
#include "console_input.h"
#include "system_initialization.h"
#include "headless_policy.h"
#include "quit_services.h"
#include <universal/q_shared.h>
#include <atomic>
#include <cstdio>
#include <cstdlib>

void Sys_Error(char *,...);
void Com_Quit_f();
void Sys_ShutdownEvents();
char sys_exitCmdLine[1024];
namespace {
enum class Phase { Starting, InitializationReturned, PreludeComplete, FinalCleanup, CleanupComplete };
std::atomic<Phase> phase{Phase::Starting};
[[noreturn]] void fail(const char *reason) {
    Sys_Error(const_cast<char *>("Native shutdown lifecycle: %s"),reason);
    std::abort();
}
void requireMain(){if(!Sys_IsMainThread())fail("requires actual main thread owner");}
[[noreturn]] void partialStartupExit() {
    // Do not enter whole-engine teardown without the actual initialization
    // return. Retire owned platform producers, then end every process-owned OS
    // worker. Keep the running guard marker: this was not a clean engine exit.
    requireMain();Sys_SetQuitRequested();
    bo1::android::ShutdownInputIfInitialized();
    if(bo1::android::ConsoleInputOwnedByCaller())bo1::android::RetireConsoleInput();
    Sys_ShutdownEvents();
    std::fputs("Native shutdown before Com_Init return: platform producers retired; whole-engine teardown incomplete; dirty instance marker retained\n",stderr);
    std::fflush(stderr);
    std::_Exit(2);
}
}
void bo1::android::DedicatedInitializationCheckpoint() {
    requireMain();
    if (phase.load(std::memory_order_acquire) != Phase::Starting
        || !ProcessGuardHeld() || !dedicated || !IsDedicatedServer())
        fail("dedicated startup checkpoint requires starting phase and actual dedicated/guard owners");
    // Windows deletes its launcher semaphore here. Native retains the owned
    // user-root guard until actual cleanup; initialization is still in progress.
    std::fputs("Native dedicated startup checkpoint: running process guard retained\n", stderr);
}
void bo1::android::EngineInitializationReturned() {
    requireMain();
    Phase expected=Phase::Starting;
    if(!phase.compare_exchange_strong(expected,Phase::InitializationReturned,std::memory_order_acq_rel))fail("duplicate/out-of-order actual initialization return");
}
void bo1::android::RequireInitializedQuitEntry() {
    requireMain();
    const auto observed=phase.load(std::memory_order_acquire);
    if(observed==Phase::Starting)partialStartupExit();
    if(observed!=Phase::InitializationReturned)fail("recursive/out-of-order common quit entry");
}
void bo1::android::QuitPreludeCompleted() {
    requireMain();
    if(phase.load(std::memory_order_acquire)==Phase::Starting)partialStartupExit();
    if(!ServerTerminalShutdownEstablished())fail("prelude without actual server quiescence");
    Phase expected=Phase::InitializationReturned;
    if(!phase.compare_exchange_strong(expected,Phase::PreludeComplete,std::memory_order_acq_rel))fail("duplicate/out-of-order quit prelude");
}
void bo1::android::BeginFinalSystemCleanup() {
    requireMain();
    const auto observed=phase.load(std::memory_order_acquire);
    if(observed==Phase::Starting)partialStartupExit();
    if(observed==Phase::InitializationReturned){Com_Quit_f();std::abort();}
    Phase expected=Phase::PreludeComplete;
    if(!phase.compare_exchange_strong(expected,Phase::FinalCleanup,std::memory_order_acq_rel))fail("recursive/out-of-order final cleanup");
}
[[noreturn]] void bo1::android::FinishSystemQuit() {
    requireMain();
    if(phase.load(std::memory_order_acquire)!=Phase::FinalCleanup)fail("terminal exit before explicit cleanup");
    Phase expected=Phase::FinalCleanup;
    if(!phase.compare_exchange_strong(expected,Phase::CleanupComplete,std::memory_order_acq_rel))fail("cleanup completion publication failed");
    Sys_NormalExit(); // Clean marker only after all canonical explicit cleanup.
    // Endless DB/stream/server events and TLS remain process-owned. Avoid
    // global C++ destruction while those OS threads still own their storage.
    std::_Exit(0);
}
void Sys_NormalExit(){
    requireMain();
    if(phase.load(std::memory_order_acquire)!=Phase::CleanupComplete)
        fail("early Sys_NormalExit before actual final cleanup; dirty marker retained");
    bo1::android::ReleaseProcessGuardClean();
}
void Sys_DestroyConsole() {
    requireMain();
    if(bo1::android::ConsoleInputOwnedByCaller())bo1::android::RetireConsoleInput();
}
void Sys_SpawnQuitProcess() {
    if(!sys_exitCmdLine[0] || Sys_IsHeadless())return;
    fail("rendered process launch requires an actual Android Activity owner");
}
void Sys_HideSplashWindow() {
    if(Sys_IsHeadless())return; // Native headless entry never creates a splash.
    fail("rendered splash lifecycle requires an actual Android Activity owner");
}
void Sys_DestroySplashWindow(){Sys_HideSplashWindow();}
