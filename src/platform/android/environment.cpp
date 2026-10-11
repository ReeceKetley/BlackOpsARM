#include <jni.h>

#if !defined(__ANDROID__) || !defined(__aarch64__)
#error "The development environment probe requires Android ARM64."
#endif

static_assert(sizeof(void*) == 8, "ARM64 requires native 64-bit pointers.");

extern "C" JNIEXPORT jstring JNICALL
Java_com_blackopsarm_dev_DevelopmentActivity_nativeEnvironment(JNIEnv* env, jclass)
{
    return env->NewStringUTF("Android arm64-v8a / 64-bit pointers / C++20");
}
