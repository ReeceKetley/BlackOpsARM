#include <jni.h>
// Defined by the controller service's genuine SDK adoption path. This entry
// must be linked directly into the engine shared library, not discarded from
// an otherwise unreferenced static archive.
namespace bo1::android { bool AdoptSDLActivityJava(JNIEnv *, jobject); }
extern "C" JNIEXPORT jboolean JNICALL
Java_com_blackopsarm_dev_EngineActivity_nativeAdoptSDLActivity(JNIEnv *env,jobject activity) {
    return bo1::android::AdoptSDLActivityJava(env,activity)?JNI_TRUE:JNI_FALSE;
}

#include "performance_overlay.h"
extern "C" JNIEXPORT jboolean JNICALL
Java_com_blackopsarm_dev_EngineActivity_nativeSetPerformanceOverlayEnabled(JNIEnv *, jobject, jboolean enabled) {
    return bo1::android::performance::SetEnabled(enabled == JNI_TRUE) ? JNI_TRUE : JNI_FALSE;
}
extern "C" JNIEXPORT jboolean JNICALL
Java_com_blackopsarm_dev_EngineActivity_nativeReadPerformanceOverlay(JNIEnv *env, jobject, jdoubleArray output) {
    if (!output || env->GetArrayLength(output) < 13) return JNI_FALSE;
    const auto s = bo1::android::performance::Read();
    const jdouble values[13] = {s.renderFps, s.intervalMs, s.renderCpuMs, s.mainCpuMs,
        s.processCpuPercent, s.rssMiB, s.renderAgeSeconds, s.mainAgeSeconds, s.processAgeSeconds, s.rssAgeSeconds,
        s.gpuMs, static_cast<jdouble>(s.gpuStatus), s.gpuAgeSeconds};
    env->SetDoubleArrayRegion(output, 0, 13, values);
    return env->ExceptionCheck() ? JNI_FALSE : JNI_TRUE;
}
