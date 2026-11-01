#pragma once
#include <SDL_system.h>
#include <android/native_window_jni.h>
#include <jni.h>
#include <string>

namespace bo1::android {
// SDL's cached native_window is cleared by the Java surfaceDestroyed callback.
// Obtain our own reference through Java instead of racing that unowned pointer.
// Null with no error means the Surface is temporarily unavailable.
inline ANativeWindow* AcquireVulkanWindow(std::string& error)
{
    auto* env = static_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());
    if (!env) { error = "Vulkan surface: no JNI environment"; return nullptr; }
    if (env->PushLocalFrame(8) < 0) {
        env->ExceptionClear(); error = "Vulkan surface: JNI local frame failed"; return nullptr;
    }
    ANativeWindow* window = nullptr;
    jobject activity = static_cast<jobject>(SDL_AndroidGetActivity());
    if (!activity) error = "Vulkan surface: no SDL activity";
    else {
        jclass activityClass = env->GetObjectClass(activity);
        jmethodID getSurface = activityClass ? env->GetStaticMethodID(activityClass,
            "getNativeSurface", "()Landroid/view/Surface;") : nullptr;
        jobject surface = getSurface ? env->CallStaticObjectMethod(activityClass, getSurface) : nullptr;
        if (surface && !env->ExceptionCheck()) {
            jclass surfaceClass = env->GetObjectClass(surface);
            jmethodID isValid = surfaceClass ? env->GetMethodID(surfaceClass, "isValid", "()Z") : nullptr;
            if (isValid && env->CallBooleanMethod(surface, isValid) && !env->ExceptionCheck())
                window = ANativeWindow_fromSurface(env, surface);
        }
    }
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        error = "Vulkan surface: Java surface acquisition failed";
        if (window) ANativeWindow_release(window);
        window = nullptr;
    }
    env->PopLocalFrame(nullptr);
    return window;
}
}
