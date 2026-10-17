#include "launcher_session.h"
#include <jni.h>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <vector>
namespace bo1::android::launcher_session {
namespace {
std::atomic<int> state{Booting};
std::mutex mutex;
std::string pending;
}
void Publish(State value) { state.store(int(value), std::memory_order_release); }
State Read() { return State(state.load(std::memory_order_acquire)); }
bool RequestMap(const std::string &map) {
    if (map.empty() || map.size() >= 64 || std::strspn(map.c_str(), "abcdefghijklmnopqrstuvwxyz0123456789_") != map.size()) return false;
    std::lock_guard<std::mutex> lock(mutex);
    pending = map;
    std::fprintf(stderr, "ENGINE ENTRY launcher session: map request %s\n", map.c_str());
    return true;
}
bool TakeRequestedMap(std::string &map) {
    std::lock_guard<std::mutex> lock(mutex);
    if (pending.empty()) return false;
    map.swap(pending);
    pending.clear();
    return true;
}
}
// These JNI methods never call SDL or engine APIs.
extern "C" JNIEXPORT jint JNICALL
Java_com_blackopsarm_dev_EngineActivity_nativeReadSessionState(JNIEnv *, jobject) {
    return jint(bo1::android::launcher_session::Read());
}
extern "C" JNIEXPORT jboolean JNICALL
Java_com_blackopsarm_dev_EngineActivity_nativeRequestMapStart(JNIEnv *env, jobject, jstring map) {
    if (!map) return JNI_FALSE;
    const char *chars = env->GetStringUTFChars(map, nullptr);
    if (!chars) return JNI_FALSE;
    const bool accepted = bo1::android::launcher_session::RequestMap(chars);
    env->ReleaseStringUTFChars(map, chars);
    return accepted ? JNI_TRUE : JNI_FALSE;
}

// Real fastfile loading progress. The loader reports each compressed 256 KB read; reads of the
// zones the chosen map loads are counted (the Java side divides by those zones' file sizes).
namespace {
std::mutex zoneMutex;
std::vector<std::string> loadZones;
std::atomic<unsigned> loadChunks{0};
}
void Android_NoteLoadChunk(const char *zone) {
    if (!zone) return;
    std::lock_guard<std::mutex> lock(zoneMutex);
    for (const auto &name : loadZones)
        if (name == zone) { loadChunks.fetch_add(1u, std::memory_order_relaxed); return; }
}
// "name,name,..." (zone names without .ff); restarts the count.
extern "C" JNIEXPORT void JNICALL
Java_com_blackopsarm_dev_EngineActivity_nativeSetLoadZones(JNIEnv *env, jobject, jstring csv) {
    std::vector<std::string> names;
    if (csv) {
        const char *chars = env->GetStringUTFChars(csv, nullptr);
        if (chars) {
            std::string current;
            for (const char *p = chars;; ++p) {
                if (*p == ',' || !*p) { if (!current.empty()) names.push_back(current); current.clear(); if (!*p) break; }
                else current += *p;
            }
            env->ReleaseStringUTFChars(csv, chars);
        }
    }
    std::lock_guard<std::mutex> lock(zoneMutex);
    loadZones.swap(names);
    loadChunks.store(0, std::memory_order_relaxed);
}
extern "C" JNIEXPORT jint JNICALL
Java_com_blackopsarm_dev_EngineActivity_nativeReadLoadProgress(JNIEnv *, jobject) {
    return jint(loadChunks.load(std::memory_order_relaxed));
}
