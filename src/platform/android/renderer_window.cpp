#include "renderer_window.h"
#if defined(BO1_NATIVE_RENDERED_INPUT)
#include "rendered_input.h"
#endif
#include "window_lifecycle.h"
#include "controller_service.h"
#include <SDL_system.h>
#include <SDL_vulkan.h>
#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <jni.h>

namespace bo1::android {
namespace {
enum class Phase { Pending, Running, Done, Cancelled };
struct Request { RendererOwnerTask task; void *context; Phase phase{Phase::Pending}; bool result{}; std::string error; std::condition_variable changed; };
std::mutex queueMutex;
std::deque<std::shared_ptr<Request>> requests;
// Accessed only by the installed SDL owner, never used as a readiness proxy.
bool videoReference{};
SDL_Window *ownedWindow{};
bool Fail(const char *message) { SDL_SetError("%s", message); return false; }
bool ValidateActivity() {
    if (!SDLJavaSetupComplete()) return Fail("renderer: matching SDL Java setup is absent");
    JNIEnv *env = static_cast<JNIEnv *>(SDL_AndroidGetJNIEnv());
    if (!env || env->ExceptionCheck()) return Fail("renderer: JNI environment is unavailable or has a pending exception");
    if (env->PushLocalFrame(8) < 0) { if(env->ExceptionCheck()) env->ExceptionClear(); return Fail("renderer: JNI local frame allocation failed"); }
    jobject activity = static_cast<jobject>(SDL_AndroidGetActivity());
    jclass cls = activity ? env->GetObjectClass(activity) : nullptr;
    jmethodID finishing = cls ? env->GetMethodID(cls, "isFinishing", "()Z") : nullptr;
    jmethodID destroyed = !env->ExceptionCheck() && cls ? env->GetMethodID(cls, "isDestroyed", "()Z") : nullptr;
    JavaVM *vm{};
    bool ok = activity && finishing && destroyed && !env->ExceptionCheck() && env->GetJavaVM(&vm) == JNI_OK && vm;
    if (ok) ok = !env->CallBooleanMethod(activity, finishing) && !env->ExceptionCheck();
    if (ok) ok = !env->CallBooleanMethod(activity, destroyed) && !env->ExceptionCheck();
    if (env->ExceptionCheck()) { env->ExceptionDescribe(); env->ExceptionClear(); ok = false; }
    env->PopLocalFrame(nullptr);
    return ok || Fail("renderer: real Android Activity is unavailable, finishing or destroyed");
}
bool PrepareOnOwner(void *) {
    if (!IsSDLEventOwner()) return Fail("renderer: SDL video operation on non-owner");
    if (!ValidateActivity()) return false;
    if (!videoReference) {
        if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) return false;
        videoReference = true;
    }
    if (!(SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO) || !SDL_GetCurrentVideoDriver()) return Fail("renderer: SDL video driver is unavailable");
    return true;
}
bool ValidWindow(SDL_Window *window) {
    WindowLifecycleState state{};
    return window && window == ownedWindow && ReadSDLWindow(window, &state) && SDL_GetWindowID(window) == state.windowId;
}
bool ReadDisplay(int index, RendererDisplay *out) {
    if (!out || index < 0 || index >= SDL_GetNumVideoDisplays()) return Fail("renderer: invalid display index/output");
    SDL_Rect bounds{}; SDL_DisplayMode mode{};
    if (SDL_GetDisplayBounds(index, &bounds) || SDL_GetCurrentDisplayMode(index, &mode)) return false;
    if (bounds.w <= 0 || bounds.h <= 0 || mode.w <= 0 || mode.h <= 0) return Fail("renderer: invalid actual display dimensions");
    float dpi{};
    // DPI is optional capability data. Zero means SDL did not supply it.
    if (SDL_GetDisplayDPI(index, &dpi, nullptr, nullptr)) { dpi = 0; SDL_ClearError(); }
    *out = {index, bounds.x, bounds.y, bounds.w, bounds.h, mode.refresh_rate, dpi};
    return true;
}
struct DisplayCall { int index, preferred, x, y; RendererDisplay *out; bool choose; };
bool DisplayOnOwner(void *opaque) {
    auto &call = *static_cast<DisplayCall *>(opaque);
    if (!PrepareOnOwner(nullptr)) return false;
    if (!call.choose) return ReadDisplay(call.index, call.out);
    int count = SDL_GetNumVideoDisplays();
    if (count <= 0) return Fail("renderer: SDL has no actual displays");
    if (call.preferred > 0 && call.preferred <= count) return ReadDisplay(call.preferred - 1, call.out);
    for (int i=0; i<count; ++i) {
        RendererDisplay display{}; if (!ReadDisplay(i, &display)) return false;
        if (static_cast<int64_t>(call.x) >= display.x && static_cast<int64_t>(call.x) < static_cast<int64_t>(display.x)+display.width &&
            static_cast<int64_t>(call.y) >= display.y && static_cast<int64_t>(call.y) < static_cast<int64_t>(display.y)+display.height) { *call.out=display; return true; }
    }
    return ReadDisplay(0, call.out);
}
bool ValidateSpec(const RendererWindowSpec &spec) {
    return (spec.width > 0 && spec.height > 0 && spec.refreshRate >= 0 && !spec.hidden && !(spec.fullscreen && spec.borderless)) || Fail("renderer: invalid window dimensions/mode");
}
bool ApplySpec(SDL_Window *window, const RendererWindowSpec &spec) {
    if (!ValidateSpec(spec)) return false;
    if (SDL_SetWindowFullscreen(window, 0)) return false;
    SDL_SetWindowBordered(window, spec.borderless ? SDL_FALSE : SDL_TRUE);
    SDL_SetWindowPosition(window, spec.x, spec.y);
    SDL_SetWindowSize(window, spec.width, spec.height);
    if (spec.fullscreen) {
        int display = SDL_GetWindowDisplayIndex(window);
        if (display < 0) return false;
        SDL_DisplayMode requested{}, closest{};
        requested.w=spec.width; requested.h=spec.height; requested.refresh_rate=spec.refreshRate;
        if (!SDL_GetClosestDisplayMode(display, &requested, &closest)) return false;
        if (closest.w != spec.width || closest.h != spec.height || (spec.refreshRate && closest.refresh_rate != spec.refreshRate)) return Fail("renderer: requested exclusive display mode is unsupported");
        if (SDL_SetWindowDisplayMode(window, &closest) || SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN)) return false;
    } else if (spec.borderless && SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP)) return false;
    unsigned flags=SDL_GetWindowFlags(window);
    if (spec.fullscreen && (!(flags&SDL_WINDOW_FULLSCREEN) || (flags&SDL_WINDOW_FULLSCREEN_DESKTOP)==SDL_WINDOW_FULLSCREEN_DESKTOP)) return Fail("renderer: exclusive fullscreen mode was not applied");
    if (spec.borderless && (flags&SDL_WINDOW_FULLSCREEN_DESKTOP)!=SDL_WINDOW_FULLSCREEN_DESKTOP) return Fail("renderer: desktop fullscreen mode was not applied");
    if (!spec.fullscreen && !spec.borderless && (flags&SDL_WINDOW_FULLSCREEN)) return Fail("renderer: windowed mode was not applied");
    int w{}, h{}; SDL_Vulkan_GetDrawableSize(window,&w,&h);
    return (w>0 && h>0) || Fail("renderer: configured window has no actual Vulkan drawable");
}
struct WindowCall { SDL_Window *window{}; const RendererWindowSpec *spec{}; SDL_Window **out{}; RendererWindowInfo *info{}; int operation{}; };
bool WindowOnOwner(void *opaque) {
    auto &call=*static_cast<WindowCall *>(opaque);
    // Retirement remains possible after Activity destruction; it still needs
    // the actual installed owner and tracked SDL window identity.
    if (!IsSDLEventOwner()) return Fail("renderer: window operation on non-owner");
    if (call.operation != 4 && !PrepareOnOwner(nullptr)) return false;
    if (call.operation==0) {
        if (!call.out || !call.spec || !call.spec->title || !ValidateSpec(*call.spec)) return Fail("renderer: invalid create request");
        *call.out=nullptr;
        if (ownedWindow) return Fail("renderer: a window already belongs to this lifecycle");
        unsigned flags=SDL_WINDOW_VULKAN|SDL_WINDOW_ALLOW_HIGHDPI|SDL_WINDOW_HIDDEN;
        SDL_Window *window=SDL_CreateWindow(call.spec->title,call.spec->x,call.spec->y,call.spec->width,call.spec->height,flags);
        if (!window) return false;
        if (!SDL_GetWindowID(window) || !ApplySpec(window,*call.spec)) { SDL_DestroyWindow(window); return false; }
        ownedWindow=window;
        AttachSDLWindow(window);
        *call.out=window;
        return true;
    }
    if (!ValidWindow(call.window)) return Fail("renderer: window does not belong to the live SDL lifecycle");
    if (call.operation==1) return call.spec && ApplySpec(call.window,*call.spec);
    if (call.operation==2) {
        if (!call.info) return Fail("renderer: missing window output");
        RendererWindowInfo value{};
        SDL_GetWindowPosition(call.window,&value.x,&value.y);
        SDL_GetWindowSize(call.window,&value.width,&value.height);
        SDL_Vulkan_GetDrawableSize(call.window,&value.drawableWidth,&value.drawableHeight);
        value.displayIndex=SDL_GetWindowDisplayIndex(call.window); value.flags=SDL_GetWindowFlags(call.window);
        if(value.displayIndex < 0 || value.width <= 0 || value.height <= 0 || value.drawableWidth <= 0 || value.drawableHeight <= 0) return Fail("renderer: window has no valid drawable surface");
        *call.info=value; return true;
    }
    if (call.operation==3) { SDL_ShowWindow(call.window); return (SDL_GetWindowFlags(call.window)&SDL_WINDOW_SHOWN) || Fail("renderer: SDL window was not shown"); }
    if (call.operation==4) { DetachSDLWindow(call.window); ownedWindow=nullptr; SDL_DestroyWindow(call.window);
        return true; }
    return Fail("renderer: invalid window operation");
}
bool RetireOnOwner(void *) {
    if (ownedWindow) return Fail("renderer: video retirement requires destroyed window/graphics owners");
    if(videoReference) { SDL_QuitSubSystem(SDL_INIT_VIDEO); videoReference=false; }
    return true;
}
}
bool RunRendererOwnerTask(RendererOwnerTask task, void *context, unsigned timeoutMs) {
    if (!task) return Fail("renderer: null owner task");
    if (IsSDLEventOwner()) return task(context);
    auto request=std::make_shared<Request>(); request->task=task; request->context=context;
    std::unique_lock lock(queueMutex);
    requests.push_back(request);
    auto deadline=std::chrono::steady_clock::now()+std::chrono::milliseconds(timeoutMs);
    while(request->phase == Phase::Pending) {
        if(request->changed.wait_until(lock,deadline)==std::cv_status::timeout && request->phase==Phase::Pending) {
            request->phase=Phase::Cancelled;
            auto it=std::find(requests.begin(),requests.end(),request); if(it!=requests.end()) requests.erase(it);
            lock.unlock(); return Fail("renderer: SDL owner did not service pending request");
        }
    }
    while(request->phase==Phase::Running) request->changed.wait(lock);
    bool result=request->result; std::string error=request->error;
    lock.unlock();
    if(!result) SDL_SetError("%s",error.empty()?"renderer: owner operation failed":error.c_str());
    return result;
}
bool PumpRendererOwnerRequestsIfOwner() {
    if(!IsSDLEventOwner()) return false;
    for(unsigned serviced=0; serviced<64; ++serviced) {
        std::shared_ptr<Request> request;
        { std::lock_guard lock(queueMutex); if(requests.empty()) break; request=requests.front(); requests.pop_front(); if(request->phase!=Phase::Pending) continue; request->phase=Phase::Running; request->changed.notify_all(); }
        bool result{}; std::string error;
        try { result=request->task(request->context); if(!result) error=SDL_GetError(); }
        catch (...) { error="renderer: owner task threw an exception"; }

        { std::lock_guard lock(queueMutex); request->result=result; request->error=std::move(error); request->phase=Phase::Done; request->changed.notify_all(); }
    }
    return true;
}
bool PrepareRendererVideo() { return RunRendererOwnerTask(PrepareOnOwner,nullptr); }
bool GetRendererDisplay(int index,RendererDisplay *out) { DisplayCall call{index,0,0,0,out,false}; return RunRendererOwnerTask(DisplayOnOwner,&call); }
bool ChooseRendererDisplay(int preferred,int x,int y,RendererDisplay *out) { if(!out) return Fail("renderer: null display output"); DisplayCall call{0,preferred,x,y,out,true}; return RunRendererOwnerTask(DisplayOnOwner,&call); }
bool CreateRendererWindow(const RendererWindowSpec &spec,SDL_Window **out) { WindowCall call{nullptr,&spec,out,nullptr,0}; return RunRendererOwnerTask(WindowOnOwner,&call); }
bool ConfigureRendererWindow(SDL_Window *window,const RendererWindowSpec &spec) { WindowCall call{window,&spec,nullptr,nullptr,1}; return RunRendererOwnerTask(WindowOnOwner,&call); }
bool InspectRendererWindow(SDL_Window *window,RendererWindowInfo *out) { WindowCall call{window,nullptr,nullptr,out,2}; return RunRendererOwnerTask(WindowOnOwner,&call); }
bool ShowRendererWindow(SDL_Window *window) { WindowCall call{window,nullptr,nullptr,nullptr,3}; return RunRendererOwnerTask(WindowOnOwner,&call); }
bool DestroyRendererWindow(SDL_Window *window) { WindowCall call{window,nullptr,nullptr,nullptr,4}; return RunRendererOwnerTask(WindowOnOwner,&call); }
bool RendererVideoResourcesOwned() { return IsSDLEventOwner() && (videoReference || ownedWindow); }
bool RetireRendererVideo() { return RunRendererOwnerTask(RetireOnOwner,nullptr); }
}
