#include "controller_service.h"
#include <SDL.h>
#include "window_lifecycle.h"
#include <thread>
#include <limits>
#include <atomic>
#include <mutex>
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <functional>
#include <string>

namespace bo1::android {
namespace {
struct Slot { SDL_GameController *controller{}; SDL_JoystickID identity{-1}; std::uint64_t generation{}; };
Slot slots[ControllerSlots];
std::thread::id owner;
#if defined(__ANDROID__)
std::mutex javaSetupLock;
bool javaSetupAttempted;
std::atomic<bool> javaSetupReady{};
std::atomic<bool> javaActivityAdopted{};
#endif
bool CheckOwner() {
    if(owner == std::thread::id{} || owner != std::this_thread::get_id() || !IsSDLEventOwner()) {
        SDL_SetError("controller service uninitialized or called outside SDL owner thread"); return false;
    }
    return true;
}
bool ValidSlot(int slot) {
    if(slot < 0 || slot >= ControllerSlots) { SDL_SetError("invalid controller slot"); return false; }
    return true;
}
void Close(Slot &slot) {
    SDL_GameControllerClose(slot.controller); slot.controller=nullptr; slot.identity=-1;
    ++slot.generation;
}
std::int16_t InvertY(Sint16 value) {
    // SDL down-positive and engine up-positive; avoid signed-short negation overflow.
    return value == std::numeric_limits<Sint16>::min() ? std::numeric_limits<Sint16>::max() : -value;
}
std::uint8_t Trigger(Sint16 value) { return value > 0 ? static_cast<std::uint8_t>((static_cast<int>(value)*255+16383)/32767) : 0; }
}
bool ControllersReady() { return CheckOwner(); }
#if defined(__ANDROID__)
bool SDLJavaSetupComplete() { return javaSetupReady.load(std::memory_order_acquire); }
bool SDLActivityAdopted() { return javaActivityAdopted.load(std::memory_order_acquire); }
bool SetupSDLControllerJava(JNIEnv *environment) {
    if(!environment || environment->ExceptionCheck()) { SDL_SetError("valid JNI startup environment required"); return false; }
    JavaVM *vm=nullptr;
    if(environment->GetJavaVM(&vm)!=JNI_OK || !vm) { SDL_SetError("JNI startup JavaVM unavailable"); return false; }
    std::lock_guard guard(javaSetupLock);
    if(javaSetupAttempted) { SDL_SetError("SDL Java setup already attempted; duplicate setup is invalid"); return false; }
    // Verify this is the actual Java main/UI startup thread, before SDL publishes JNI state.
    jclass looper=environment->FindClass("android/os/Looper");
    if(!looper) { SDL_SetError("Android UI Looper unavailable"); return false; }
    const auto myLooper=environment->GetStaticMethodID(looper,"myLooper","()Landroid/os/Looper;");
    const auto mainLooper=environment->GetStaticMethodID(looper,"getMainLooper","()Landroid/os/Looper;");
    if(!myLooper || !mainLooper) { environment->DeleteLocalRef(looper); SDL_SetError("Android UI Looper methods unavailable"); return false; }
    jobject current=environment->CallStaticObjectMethod(looper,myLooper);
    jobject main=environment->ExceptionCheck()?nullptr:environment->CallStaticObjectMethod(looper,mainLooper);
    const bool uiOwner=!environment->ExceptionCheck() && current && main && environment->IsSameObject(current,main);
    if(current) environment->DeleteLocalRef(current);
    if(main) environment->DeleteLocalRef(main);
    environment->DeleteLocalRef(looper);
    if(!uiOwner) { SDL_SetError("SDL Java setup requires the actual Android UI startup thread"); return false; }
    if(environment->PushLocalFrame(16)<0) { SDL_SetError("JNI local frame allocation failed"); return false; }
    bool success=false;
    do {
        jclass sdl=environment->FindClass("org/libsdl/app/SDL");
        jclass controllers=sdl?environment->FindClass("org/libsdl/app/SDLControllerManager"):nullptr;
        if(!sdl || !controllers) break;
        const auto setup=environment->GetStaticMethodID(sdl,"setupJNI","()V");
        const auto initialize=setup?environment->GetStaticMethodID(controllers,"initialize","()V"):nullptr;
        const auto poll=initialize?environment->GetStaticMethodID(controllers,"pollInputDevices","()V"):nullptr;
        const auto pollHaptic=poll?environment->GetStaticMethodID(controllers,"pollHapticDevices","()V"):nullptr;
        const auto rumble=pollHaptic?environment->GetStaticMethodID(controllers,"hapticRun","(IFI)V"):nullptr;
        const auto stop=rumble?environment->GetStaticMethodID(controllers,"hapticStop","(I)V"):nullptr;
        const auto handler=stop?environment->GetStaticFieldID(controllers,"mJoystickHandler","Lorg/libsdl/app/SDLJoystickHandler;"):nullptr;
        if(!handler || environment->ExceptionCheck()) break;
        javaSetupAttempted=true;
        environment->CallStaticVoidMethod(sdl,setup);
        if(environment->ExceptionCheck()) break;
        environment->CallStaticVoidMethod(controllers,initialize);
        if(environment->ExceptionCheck()) break;
        jobject joystickHandler=environment->GetStaticObjectField(controllers,handler);
        if(!joystickHandler || environment->ExceptionCheck()) break;
        // Only now is SDL's pthread JNI key guaranteed to have been created by real setup.
        if(SDL_AndroidGetJNIEnv()!=environment) break;
        success=true;
    } while(false);
    environment->PopLocalFrame(nullptr);
    if(!success) { SDL_SetError("actual matching SDL Java/controller setup failed"); return false; }
    javaSetupReady.store(true,std::memory_order_release);
    return true;
}
bool AdoptSDLActivityJava(JNIEnv *env,jobject activity) {
    if(!env || !activity || env->ExceptionCheck()) { SDL_SetError("valid Activity adoption environment required"); return false; }
    if(env->PushLocalFrame(24)<0) { if(env->ExceptionCheck())env->ExceptionClear(); SDL_SetError("Activity adoption local frame unavailable"); return false; }
    bool success=false;
    {
        std::lock_guard guard(javaSetupLock);
        do {
            if(javaSetupAttempted) break;
            JavaVM *vm=nullptr;
            if(env->GetJavaVM(&vm)!=JNI_OK || !vm)break;
            jclass looper=env->FindClass("android/os/Looper");
            if(!looper)break;
            auto my=env->GetStaticMethodID(looper,"myLooper","()Landroid/os/Looper;");
            auto main=env->GetStaticMethodID(looper,"getMainLooper","()Landroid/os/Looper;");
            if(!my || !main)break;
            jobject current=env->CallStaticObjectMethod(looper,my);
            jobject ui=env->ExceptionCheck()?nullptr:env->CallStaticObjectMethod(looper,main);
            if(env->ExceptionCheck() || !current || !ui || !env->IsSameObject(current,ui))break;
            jclass actual=env->GetObjectClass(activity);
            auto loaderMethod=actual?env->GetMethodID(actual,"getClassLoader","()Ljava/lang/ClassLoader;"):nullptr;
            auto finishing=actual?env->GetMethodID(actual,"isFinishing","()Z"):nullptr;
            auto destroyed=actual?env->GetMethodID(actual,"isDestroyed","()Z"):nullptr;
            if(!loaderMethod || !finishing || !destroyed)break;
            if(env->CallBooleanMethod(activity,finishing) || env->ExceptionCheck())break;
            if(env->CallBooleanMethod(activity,destroyed) || env->ExceptionCheck())break;
            jobject loader=env->CallObjectMethod(activity,loaderMethod);
            jclass loaderClass=loader?env->GetObjectClass(loader):nullptr;
            auto load=loaderClass?env->GetMethodID(loaderClass,"loadClass","(Ljava/lang/String;)Ljava/lang/Class;"):nullptr;
            if(!load)break;
            jstring name=env->NewStringUTF("org.libsdl.app.SDLActivity");
            jclass sdk=name?static_cast<jclass>(env->CallObjectMethod(loader,load,name)):nullptr;
            if(!sdk || env->ExceptionCheck() || !env->IsInstanceOf(activity,sdk))break;
            auto broken=env->GetStaticFieldID(sdk,"mBrokenLibraries","Z");
            auto singleton=env->GetStaticFieldID(sdk,"mSingleton","Lorg/libsdl/app/SDLActivity;");
            auto surface=env->GetStaticFieldID(sdk,"mSurface","Lorg/libsdl/app/SDLSurface;");
            auto context=env->GetStaticMethodID(sdk,"getContext","()Landroid/content/Context;");
            if(!broken || !singleton || !surface || !context || env->ExceptionCheck())break;
            if(env->GetStaticBooleanField(sdk,broken))break;
            jobject instance=env->GetStaticObjectField(sdk,singleton);
            jobject view=env->GetStaticObjectField(sdk,surface);
            jobject ctx=env->CallStaticObjectMethod(sdk,context);
            if(env->ExceptionCheck() || !view || !instance || !ctx || !env->IsSameObject(instance,activity) || !env->IsSameObject(ctx,activity))break;
            name=env->NewStringUTF("org.libsdl.app.SDLControllerManager");
            jclass controllers=name?static_cast<jclass>(env->CallObjectMethod(loader,load,name)):nullptr;
            auto handler=controllers?env->GetStaticFieldID(controllers,"mJoystickHandler","Lorg/libsdl/app/SDLJoystickHandler;"):nullptr;
            if(!handler || env->ExceptionCheck() || !env->GetStaticObjectField(controllers,handler))break;
            // Surface creation follows actual setupJNI/initialize in this pinned
            // SDK. Only after these identities are validated are its TLS and
            // Activity getters safe; no SDK initialization is repeated.
            javaSetupAttempted=true;
            if(SDL_AndroidGetJNIEnv()!=env)break;
            jobject nativeActivity=static_cast<jobject>(SDL_AndroidGetActivity());
            if(!nativeActivity || env->ExceptionCheck() || !env->IsSameObject(nativeActivity,activity))break;
            success=true;
        } while(false);
    }
    if(env->ExceptionCheck())env->ExceptionClear();
    env->PopLocalFrame(nullptr);
    if(!success) { SDL_SetError("matching completed SDLActivity UI owner required; adoption failed"); return false; }
    javaActivityAdopted.store(true,std::memory_order_release);
    javaSetupReady.store(true,std::memory_order_release);
    return true;
}

#endif
bool InitControllers() {
    if(owner != std::thread::id{}) { SDL_SetError("controller service already initialized"); return false; }
    if(!IsSDLEventOwner() || !(SDL_WasInit(SDL_INIT_EVENTS)&SDL_INIT_EVENTS)) { SDL_SetError("SDL event owner must initialize events before controllers"); return false; }
#if defined(__ANDROID__)
    // Guard before SDL's TLS getter/backend: neither is safe before actual Java setup.
    if(!javaSetupReady.load(std::memory_order_acquire)) { SDL_SetError("SDLActivity/SDLControllerManager JNI owner required for Android controllers"); return false; }
#endif
    if(SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER)) return false;
    owner=std::this_thread::get_id();
    if(!RefreshControllers()) { ShutdownControllers(); return false; }
    return true;
}
bool RefreshControllers() {
    if(!CheckOwner()) return false;
    SDL_GameControllerUpdate();
    for(auto &slot : slots) if(slot.controller && !SDL_GameControllerGetAttached(slot.controller)) Close(slot);
    const int count=SDL_NumJoysticks();
    if(count<0) return false;
    for(int device=0; device<count; ++device) {
        if(!SDL_IsGameController(device)) {
            // A joystick SDL has no controller mapping for: its buttons cannot reach the engine.
            static std::uint64_t reportedUnmapped;
            char guid[64]{};SDL_JoystickGetGUIDString(SDL_JoystickGetDeviceGUID(device),guid,sizeof guid);
            const std::uint64_t key=std::hash<std::string>{}(std::string(guid)+std::to_string(SDL_JoystickGetDeviceInstanceID(device)));
            if(key!=reportedUnmapped){reportedUnmapped=key;std::fprintf(stderr,"CONTROLLER unmapped joystick '%s' guid=%s buttons/axes unavailable\n",SDL_JoystickNameForIndex(device)?SDL_JoystickNameForIndex(device):"?",guid);}
            continue;
        }
        const SDL_JoystickID identity=SDL_JoystickGetDeviceInstanceID(device);
        if(identity<0) return false;
        bool known=false;
        for(auto &slot : slots) if(slot.controller && slot.identity==identity) known=true;
        if(known) continue;
        Slot *empty=nullptr;
        for(auto &slot : slots) if(!slot.controller) { empty=&slot; break; }
        if(!empty) break;
        SDL_GameController *controller=SDL_GameControllerOpen(device);
        if(!controller) return false;
        empty->controller=controller; empty->identity=identity; ++empty->generation;
        {
            char guid[64]{};SDL_JoystickGetGUIDString(SDL_JoystickGetDeviceGUID(device),guid,sizeof guid);
            char *mapping=SDL_GameControllerMapping(controller);
            std::fprintf(stderr,"CONTROLLER opened slot '%s' guid=%s mapping=%s\n",SDL_GameControllerName(controller)?SDL_GameControllerName(controller):"?",guid,mapping?mapping:"none");
            if(mapping)SDL_free(mapping);
        }
    }
    return true;
}
int ReadController(int slot, ControllerSample *sample) {
    if(!CheckOwner() || !ValidSlot(slot)) return -1;
    if(!sample) { SDL_SetError("null controller output sample"); return -1; }
    auto *controller=slots[slot].controller;
    if(!controller || !SDL_GameControllerGetAttached(controller)) return 0;
    // Raw input trace (first 600 changes): which physical button/axis indexes this pad sends and
    // what SDL's mapping made of them. Used to diagnose controllers with unexpected layouts.
    {
        static int traced; static Uint8 lastButtons[SDL_CONTROLLER_BUTTON_MAX+32]; static Sint16 lastAxes[16]; static Uint8 lastHats[4];
        if(traced<600) {
            SDL_Joystick *joystick=SDL_GameControllerGetJoystick(controller);
            const int buttons=std::min(SDL_JoystickNumButtons(joystick),int(sizeof lastButtons));
            for(int i=0;i<buttons;++i) { const Uint8 now=SDL_JoystickGetButton(joystick,i);
                if(now!=lastButtons[i]) { lastButtons[i]=now; ++traced; std::fprintf(stderr,"CONTROLLER raw button %d %s\n",i,now?"down":"up"); } }
            const int axes=std::min(SDL_JoystickNumAxes(joystick),16);
            for(int i=0;i<axes;++i) { const Sint16 now=SDL_JoystickGetAxis(joystick,i);
                if(std::abs(int(now)-int(lastAxes[i]))>12000) { lastAxes[i]=now; ++traced; std::fprintf(stderr,"CONTROLLER raw axis %d = %d\n",i,int(now)); } }
            const int hats=std::min(SDL_JoystickNumHats(joystick),4);
            for(int i=0;i<hats;++i) { const Uint8 now=SDL_JoystickGetHat(joystick,i);
                if(now!=lastHats[i]) { lastHats[i]=now; ++traced; std::fprintf(stderr,"CONTROLLER raw hat %d = %d\n",i,int(now)); } }
        }
    }
    ControllerSample out{};
    struct Mapping { SDL_GameControllerButton button; std::uint16_t mask; };
    static constexpr Mapping maps[] = {
        {SDL_CONTROLLER_BUTTON_DPAD_UP,0x0001}, {SDL_CONTROLLER_BUTTON_DPAD_DOWN,0x0002},
        {SDL_CONTROLLER_BUTTON_DPAD_LEFT,0x0004}, {SDL_CONTROLLER_BUTTON_DPAD_RIGHT,0x0008},
        {SDL_CONTROLLER_BUTTON_START,0x0010}, {SDL_CONTROLLER_BUTTON_BACK,0x0020},
        {SDL_CONTROLLER_BUTTON_LEFTSTICK,0x0040}, {SDL_CONTROLLER_BUTTON_RIGHTSTICK,0x0080},
        {SDL_CONTROLLER_BUTTON_LEFTSHOULDER,0x0100}, {SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,0x0200},
        {SDL_CONTROLLER_BUTTON_A,0x1000}, {SDL_CONTROLLER_BUTTON_B,0x2000},
        {SDL_CONTROLLER_BUTTON_X,0x4000}, {SDL_CONTROLLER_BUTTON_Y,0x8000}
    };
    for(const auto &map : maps) if(SDL_GameControllerGetButton(controller,map.button)) out.wButtons|=map.mask;
    out.sThumbLX=SDL_GameControllerGetAxis(controller,SDL_CONTROLLER_AXIS_LEFTX);
    out.sThumbLY=InvertY(SDL_GameControllerGetAxis(controller,SDL_CONTROLLER_AXIS_LEFTY));
    out.sThumbRX=SDL_GameControllerGetAxis(controller,SDL_CONTROLLER_AXIS_RIGHTX);
    out.sThumbRY=InvertY(SDL_GameControllerGetAxis(controller,SDL_CONTROLLER_AXIS_RIGHTY));
    out.bLeftTrigger=Trigger(SDL_GameControllerGetAxis(controller,SDL_CONTROLLER_AXIS_TRIGGERLEFT));
    out.bRightTrigger=Trigger(SDL_GameControllerGetAxis(controller,SDL_CONTROLLER_AXIS_TRIGGERRIGHT));
    *sample=out;
    return 1;
}
std::uint64_t ControllerGeneration(int slot) { if(!CheckOwner() || !ValidSlot(slot)) return 0; return slots[slot].generation; }
bool RumbleController(int slot, std::uint16_t low, std::uint16_t high, std::uint32_t milliseconds) {
    if(!CheckOwner() || !ValidSlot(slot)) return false;
    if(!slots[slot].controller) { SDL_SetError("rumble requires attached controller"); return false; }
    return SDL_GameControllerRumble(slots[slot].controller,low,high,milliseconds)==0;
}
bool ShutdownControllers() {
    if(!CheckOwner()) return false;
    for(auto &slot : slots) if(slot.controller) Close(slot);
    SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER); owner={}; return true;
}
}
