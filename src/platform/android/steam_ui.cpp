#include "steam_ui.h"
#include "controller_service.h"
#include "headless_policy.h"
#include <SDL.h>
#include <jni.h>
#include <qcommon/common.h>
#include <cstring>
#include <climits>

namespace {
struct LocalFrame {
    JNIEnv *environment;
    explicit LocalFrame(JNIEnv *env):environment(env) {}
    ~LocalFrame() { environment->PopLocalFrame(nullptr); }
};
bool Exception(JNIEnv *environment) {
    if(!environment->ExceptionCheck()) return false;
    // This service owns these JNI failures; preserve evidence before clearing.
    environment->ExceptionDescribe();environment->ExceptionClear();return true;
}
jstring Text(JNIEnv *env,const char *text) {
    const std::size_t length=std::strlen(text);
    if(length>INT_MAX) return nullptr;
    auto bytes=env->NewByteArray(static_cast<jsize>(length));
    if(Exception(env) || !bytes) return nullptr;
    env->SetByteArrayRegion(bytes,0,static_cast<jsize>(length),reinterpret_cast<const jbyte*>(text));
    if(Exception(env)) return nullptr;
    auto stringClass=env->FindClass("java/lang/String");
    if(Exception(env) || !stringClass) return nullptr;
    auto constructor=env->GetMethodID(stringClass,"<init>","([BLjava/lang/String;)V");
    if(Exception(env) || !constructor) return nullptr;
    auto encoding=env->NewStringUTF("UTF-8");
    if(Exception(env) || !encoding) return nullptr;
    auto string=static_cast<jstring>(env->NewObject(stringClass,constructor,bytes,encoding));
    if(Exception(env)) return nullptr;
    return string;
}
bool Invoke(bool dialog,const char *first,const char *second) {
    // SDL's JNI getter touches a pthread key created only by actual Java setup.
    if(!bo1::android::SDLJavaSetupComplete()) return false;
    auto *env=static_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());
    if(!env || env->ExceptionCheck()) return false;
    if(env->PushLocalFrame(32)<0) { Exception(env);return false; }
    LocalFrame frame(env);
    auto activity=static_cast<jobject>(SDL_AndroidGetActivity());
    if(Exception(env) || !activity) return false;
    auto expectedActivity=env->FindClass("android/app/Activity");
    if(Exception(env) || !expectedActivity || !env->IsInstanceOf(activity,expectedActivity)) return false;
    JavaVM *vm=nullptr;
    if(env->GetJavaVM(&vm)!=JNI_OK || !vm || Exception(env)) return false;
    auto activityClass=env->GetObjectClass(activity);
    if(Exception(env) || !activityClass) return false;
    auto getLoader=env->GetMethodID(activityClass,"getClassLoader","()Ljava/lang/ClassLoader;");
    if(Exception(env) || !getLoader) return false;
    auto loader=env->CallObjectMethod(activity,getLoader);
    if(Exception(env) || !loader) return false;
    auto loaderClass=env->GetObjectClass(loader);
    if(Exception(env) || !loaderClass) return false;
    auto loadClass=env->GetMethodID(loaderClass,"loadClass","(Ljava/lang/String;)Ljava/lang/Class;");
    if(Exception(env) || !loadClass) return false;
    auto name=env->NewStringUTF("com.blackopsarm.dev.SteamUi");
    if(Exception(env) || !name) return false;
    // App classes must resolve through the real Activity loader on attached workers.
    auto helper=static_cast<jclass>(env->CallObjectMethod(loader,loadClass,name));
    if(Exception(env) || !helper) return false;
    auto method=env->GetStaticMethodID(helper,dialog?"showErrorAndWait":"launchSteam",
        dialog?"(Landroid/app/Activity;Ljava/lang/String;Ljava/lang/String;)Z":"(Landroid/app/Activity;Ljava/lang/String;)Z");
    if(Exception(env) || !method) return false;
    auto argument=Text(env,first);
    auto message=dialog?Text(env,second):nullptr;
    if(!argument || (dialog && !message)) return false;
    const jboolean success=dialog?env->CallStaticBooleanMethod(helper,method,activity,argument,message)
                                  :env->CallStaticBooleanMethod(helper,method,activity,argument);
    if(Exception(env)) return false;
    return success==JNI_TRUE;
}
}
bool Platform_LaunchSteamURI(const char *uri) {
    if(Sys_IsHeadless()) { Com_PrintWarning(16,"Steam URI launch refused in headless mode\n");return false; }
    if(!uri || std::strncmp(uri,"steam://",8)!=0) { Com_PrintWarning(16,"Invalid Steam launch URI\n");return false; }
    const bool launched=Invoke(false,uri,nullptr);
    if(!launched) Com_PrintWarning(16,"Steam URI launch failed: requires actual SDL Java setup, live Activity, Steam intent handler and UI dispatch\n");
    return launched;
}
void Platform_ShowSteamError(const char *title,const char *message) {
    if(Sys_IsHeadless()) { Com_PrintWarning(16,"Steam dialog refused in headless mode\n");return; }
    if(!title || !message || !Invoke(true,title,message))
        Com_PrintWarning(16,"Steam dialog did not complete: requires actual SDL Java setup, live Activity and non-UI waiting caller\n");
}
