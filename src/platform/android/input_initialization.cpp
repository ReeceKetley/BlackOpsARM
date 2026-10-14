#include "system_initialization.h"
#if defined(BO1_NATIVE_RENDERED_INPUT)
#include "rendered_input.h"
#include <SDL.h>
#endif
#include <qcommon/common.h>
#include <platform/android/headless_policy.h>
#include <platform/android/console_input.h>
#include <win32/win_gamepad.h>
#include <pthread.h>
#include <atomic>
#include <mutex>
#include <cstdlib>

const dvar_t *in_mouse;
namespace {
std::atomic<bool> initialized{false};
std::once_flag ownerPublication;
pthread_t owner{};
void requireOwner() {
    if (!initialized.load(std::memory_order_acquire) || !pthread_equal(owner,pthread_self())) {
        Com_Error(ERR_FATAL,"Native input lifecycle requires its actual initialized owner thread");
        std::abort();
    }
}
}
void IN_Init() {
    // The native entry's input owner lives for the process; shutdown/restart
    // retires device state, never transfers an immutable thread identity.
    std::call_once(ownerPublication,[]{owner=pthread_self();});
    if (!pthread_equal(owner,pthread_self())) {
        Com_Error(ERR_FATAL,"Native input initialization cannot change its owner thread");
        std::abort();
    }
    if (initialized.load(std::memory_order_acquire)) {
        requireOwner();
        return;
    }
#if !defined(BO1_NATIVE_RENDERED_INPUT)
    if (!Sys_IsHeadless()) {
        Com_Error(ERR_FATAL,"Native rendered input initialization requires a bound keyboard/mouse/touch SDL translator");
        std::abort();
    }
#endif
    in_mouse = _Dvar_RegisterBool("in_mouse",1,0x21u,"Initialize the mouse drivers");
    // The actual producer is installed before Com_Init. Polling validates the
    // existing owner and queues genuine input; restart never replaces its stream.
#if defined(BO1_NATIVE_RENDERED_INPUT)
    if(!Sys_IsHeadless()) {
        if(!bo1::android::InitializeRenderedInput()) {
            Com_Error(ERR_FATAL,"Actual rendered SDL input initialization failed: %s",SDL_GetError());
            std::abort();
        }
    } else
#endif
    bo1::android::PollConsoleInput();
    GPad_InitAll();
    initialized.store(true,std::memory_order_release);
}
void IN_Shutdown() {
    requireOwner();
    GPad_ShutdownAll();
#if defined(BO1_NATIVE_RENDERED_INPUT)
    if(!Sys_IsHeadless())bo1::android::ShutdownRenderedInput();
#endif
    initialized.store(false,std::memory_order_release);
}
void IN_Activate(int active) {
    requireOwner();
#if defined(BO1_NATIVE_RENDERED_INPUT)
    if(!Sys_IsHeadless()){bo1::android::ActivateRenderedInput(active!=0);return;}
#endif
    // A windowless console has no focus transition or physical device capture.
    // Its producer remains owned and does not follow a fabricated window state.
    bo1::android::PollConsoleInput();
}
void IN_Frame_Native() {
    requireOwner();
    Platform_InputFrame();
#if defined(BO1_NATIVE_RENDERED_INPUT)
    if(!Sys_IsHeadless())bo1::android::FrameRenderedInput();
#endif
}

void bo1::android::ShutdownInputIfInitialized() {
    if(initialized.load(std::memory_order_acquire))IN_Shutdown();
}
