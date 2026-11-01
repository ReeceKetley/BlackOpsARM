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
std::string graphics[3];
ResumeInfo resumeInfo;
int resumeAction;
}
void Publish(State value) { state.store(int(value), std::memory_order_release); }
void PublishGraphics(const std::string &name, const std::string &api, const std::string &driver) {
    std::lock_guard<std::mutex> lock(mutex);
    graphics[0]=name; graphics[1]=api; graphics[2]=driver;
}
static jobjectArray ReadGraphics(JNIEnv *env) {
    std::lock_guard<std::mutex> lock(mutex);
    jclass strings=env->FindClass("java/lang/String");
    if(!strings)return nullptr;
    jobjectArray result=env->NewObjectArray(3,strings,nullptr);
    env->DeleteLocalRef(strings);
    if(!result)return nullptr;
    for(int i=0;i<3;++i) {
        jstring value=env->NewStringUTF(graphics[i].c_str());
        if(!value)return nullptr;
        env->SetObjectArrayElement(result,i,value);env->DeleteLocalRef(value);
        if(env->ExceptionCheck())return nullptr;
    }
    return result;
}
State Read() { return State(state.load(std::memory_order_acquire)); }
void PublishResumeInfo(const ResumeInfo &info) {
    std::lock_guard<std::mutex> lock(mutex);
    resumeInfo=info;
    if(resumeAction)resumeInfo.busy=true;
}
bool RequestResumeAction(int action) {
    std::lock_guard<std::mutex> lock(mutex);
    if(action<1 || action>3 || resumeInfo.busy || resumeAction
        || (action==3 ? !resumeInfo.available : !resumeInfo.canSave))return false;
    resumeAction=action;resumeInfo.busy=true;return true;
}
int TakeResumeAction() {
    std::lock_guard<std::mutex> lock(mutex);
    const int action=resumeAction;resumeAction=0;return action;
}
bool ResumeLoading() {
    std::lock_guard<std::mutex> lock(mutex);return resumeInfo.loading;
}
static std::string JsonString(const std::string &s) {
    std::string out="\"";
    for(unsigned char c:s) {
        if(c=='\"' || c=='\\'){out+='\\';out+=char(c);}
        else if(c<32){char code[7];std::snprintf(code,sizeof(code),"\\u%04x",c);out+=code;}
        else out+=char(c);
    }
    return out+'\"';
}
std::string ResumeInfoJson() {
    std::lock_guard<std::mutex> lock(mutex);
    const auto flag=[](bool b){return b?"true":"false";};
    return std::string("{\"available\":")+flag(resumeInfo.available)+",\"canSave\":"+flag(resumeInfo.canSave)
        +",\"paused\":"+flag(resumeInfo.paused)+",\"busy\":"+flag(resumeInfo.busy)
        +",\"loading\":"+flag(resumeInfo.loading)+",\"success\":"+flag(resumeInfo.success)
        +",\"result\":"+std::to_string(resumeInfo.result)+",\"map\":"+JsonString(resumeInfo.map)
        +",\"message\":"+JsonString(resumeInfo.message)+"}";
}
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
extern "C" JNIEXPORT jobjectArray JNICALL
Java_com_blackopsarm_dev_EngineActivity_nativeReadGraphicsInfo(JNIEnv *env, jobject) {
    return bo1::android::launcher_session::ReadGraphics(env);
}
extern "C" JNIEXPORT jint JNICALL
Java_com_blackopsarm_dev_EngineActivity_nativeReadSessionState(JNIEnv *, jobject) {
    return jint(bo1::android::launcher_session::Read());
}
extern "C" JNIEXPORT jstring JNICALL
Java_com_blackopsarm_dev_EngineActivity_nativeReadResumeStatus(JNIEnv *env, jobject) {
    const auto json=bo1::android::launcher_session::ResumeInfoJson();return env->NewStringUTF(json.c_str());
}
extern "C" JNIEXPORT jboolean JNICALL
Java_com_blackopsarm_dev_EngineActivity_nativeRequestResumeAction(JNIEnv *, jobject, jint action) {
    return bo1::android::launcher_session::RequestResumeAction(action)?JNI_TRUE:JNI_FALSE;
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
