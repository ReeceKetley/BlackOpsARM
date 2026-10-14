#include "ui_platform.h"
#include "controller_service.h"
#include "headless_policy.h"
#include "renderer_window.h"
#include "window_lifecycle.h"
#include <gfx_d3d/r_init.h>
#include <client/client.h>
#include <qcommon/common.h>
#include <qcommon/cmd.h>
#include <universal/memory_services.h>
#include <win32/win_local.h>
#include <SDL.h>
#include <SDL_system.h>
#include <jni.h>
#include <array>
#include <atomic>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <thread>

void Sys_QueEvent(unsigned,sysEventType_t,int,int,int,void *);
namespace {
constexpr size_t OutputCapacity=128*1024;
constexpr size_t ClipboardCapacity=1024*1024;
struct Command { char bytes[1024]{}; unsigned length{}; };
std::mutex outputMutex,commandMutex;
std::array<char,OutputCapacity> output;
size_t outputLength{};
std::array<Command,32> commands;
unsigned commandHead{},commandTail{};
std::thread::id commandOwner;
bool Exception(JNIEnv *env) { if(!env->ExceptionCheck())return false;env->ExceptionDescribe();env->ExceptionClear();return true; }
struct Frame { JNIEnv *env; ~Frame(){env->PopLocalFrame(nullptr);} };
jstring Text(JNIEnv *env,const char *value) {
    if(!value)return nullptr;
    size_t length=std::strlen(value);if(length>INT_MAX)return nullptr;
    jbyteArray bytes=env->NewByteArray(static_cast<jsize>(length));if(Exception(env)||!bytes)return nullptr;
    env->SetByteArrayRegion(bytes,0,static_cast<jsize>(length),reinterpret_cast<const jbyte*>(value));if(Exception(env))return nullptr;
    jclass stringClass=env->FindClass("java/lang/String");if(Exception(env)||!stringClass)return nullptr;
    jmethodID constructor=env->GetMethodID(stringClass,"<init>","([BLjava/lang/String;)V");if(Exception(env)||!constructor)return nullptr;
    jstring encoding=env->NewStringUTF("UTF-8");if(Exception(env)||!encoding)return nullptr;
    jstring result=static_cast<jstring>(env->NewObject(stringClass,constructor,bytes,encoding));if(Exception(env))return nullptr;return result;
}
bool Context(JNIEnv *env,jobject *activity,jclass *helper) {
    *activity=static_cast<jobject>(SDL_AndroidGetActivity());if(Exception(env)||!*activity)return false;
    jclass expected=env->FindClass("android/app/Activity");if(Exception(env)||!expected||!env->IsInstanceOf(*activity,expected))return false;
    JavaVM *vm{};if(env->GetJavaVM(&vm)!=JNI_OK||!vm)return false;
    jclass type=env->GetObjectClass(*activity);if(Exception(env)||!type)return false;
    jmethodID finishing=env->GetMethodID(type,"isFinishing","()Z");if(Exception(env)||!finishing)return false;
    jmethodID destroyed=env->GetMethodID(type,"isDestroyed","()Z");if(Exception(env)||!destroyed)return false;
    jboolean ending=env->CallBooleanMethod(*activity,finishing);bool failed=Exception(env);if(failed||ending)return false;
    ending=env->CallBooleanMethod(*activity,destroyed);failed=Exception(env);if(failed||ending)return false;
    jmethodID getLoader=env->GetMethodID(type,"getClassLoader","()Ljava/lang/ClassLoader;");if(Exception(env)||!getLoader)return false;
    jobject loader=env->CallObjectMethod(*activity,getLoader);if(Exception(env)||!loader)return false;
    jclass loaderType=env->GetObjectClass(loader);if(Exception(env)||!loaderType)return false;
    jmethodID load=env->GetMethodID(loaderType,"loadClass","(Ljava/lang/String;)Ljava/lang/Class;");if(Exception(env)||!load)return false;
    jstring name=env->NewStringUTF("com.blackopsarm.dev.NativeUi");if(Exception(env)||!name)return false;
    *helper=static_cast<jclass>(env->CallObjectMethod(loader,load,name));return !Exception(env)&&*helper;
}
bool CommandReady() { std::lock_guard lock(commandMutex);return commandOwner!=std::thread::id{}; }
static jboolean JNICALL NativeCommandReady(JNIEnv *,jclass) { return CommandReady()?JNI_TRUE:JNI_FALSE; }
bool QueueCommand(const char *bytes,unsigned length) {
    if(!bytes||!length||length>=1024)return false;
    Command command{};command.length=length;
    std::memcpy(command.bytes,bytes,length);
    for(unsigned i=0;i<length;++i)if(!command.bytes[i]||command.bytes[i]=='\n'||command.bytes[i]=='\r'||command.bytes[i]=='\b')return false;
    std::lock_guard lock(commandMutex);
    if(commandOwner==std::thread::id{}||commandHead-commandTail==commands.size())return false;
    commands[commandHead%commands.size()]=command;++commandHead;return true;
}
static jboolean JNICALL NativeSubmit(JNIEnv *env,jclass,jbyteArray bytes) {
    if(!bytes||env->ExceptionCheck())return JNI_FALSE;
    jsize length=env->GetArrayLength(bytes);if(Exception(env)||length<=0||length>=1024)return JNI_FALSE;
    Command command{};
    env->GetByteArrayRegion(bytes,0,length,reinterpret_cast<jbyte*>(command.bytes));if(Exception(env))return JNI_FALSE;
    return QueueCommand(command.bytes,static_cast<unsigned>(length))?JNI_TRUE:JNI_FALSE;
}
static jbyteArray JNICALL NativeReadOutput(JNIEnv *env,jclass) {
    if(env->ExceptionCheck())return nullptr;
    auto *snapshot=static_cast<char*>(std::malloc(OutputCapacity));if(!snapshot)return nullptr;
    size_t length{};
    {std::lock_guard lock(outputMutex);length=outputLength;std::memcpy(snapshot,output.data(),length);}
    jbyteArray bytes=env->NewByteArray(static_cast<jsize>(length));
    if(!Exception(env)&&bytes)env->SetByteArrayRegion(bytes,0,static_cast<jsize>(length),reinterpret_cast<const jbyte*>(snapshot));
    std::free(snapshot);if(Exception(env))return nullptr;return bytes;
}
bool RegisterConsole(JNIEnv *env,jclass helper) {
    const JNINativeMethod methods[]{
        {const_cast<char*>("nativeReadConsoleOutput"),const_cast<char*>("()[B"),reinterpret_cast<void*>(NativeReadOutput)},
        {const_cast<char*>("nativeSubmitConsoleCommand"),const_cast<char*>("([B)Z"),reinterpret_cast<void*>(NativeSubmit)},
        {const_cast<char*>("nativeConsoleCommandReady"),const_cast<char*>("()Z"),reinterpret_cast<void*>(NativeCommandReady)}
    };
    jint result=env->RegisterNatives(helper,methods,3);bool failed=Exception(env);return result==JNI_OK&&!failed;
}
enum class Action { URL, Confirm, Console, Clipboard };
bool Invoke(Action action,const char *first,const char *second,char **clipboard=nullptr,bool *delivered=nullptr) {
    if(delivered)*delivered=false;
    if(!bo1::android::SDLJavaSetupComplete())return false; // Guard SDL's real JNI pthread key.
    auto *env=static_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());if(!env||env->ExceptionCheck())return false;
    if(env->PushLocalFrame(32)<0){Exception(env);return false;}Frame frame{env};
    jobject activity{};jclass helper{};if(!Context(env,&activity,&helper))return false;
    if(action==Action::Clipboard) {
        auto method=env->GetStaticMethodID(helper,"clipboardText","(Landroid/app/Activity;)[B");if(Exception(env)||!method)return false;
        auto bytes=static_cast<jbyteArray>(env->CallStaticObjectMethod(helper,method,activity));if(Exception(env)||!bytes||!clipboard)return false;
        jsize length=env->GetArrayLength(bytes);if(Exception(env)||length<0||static_cast<size_t>(length)>ClipboardCapacity)return false;
        auto *copy=static_cast<char*>(std::malloc(static_cast<size_t>(length)+1));if(!copy)return false;
        env->GetByteArrayRegion(bytes,0,length,reinterpret_cast<jbyte*>(copy));if(Exception(env)){std::free(copy);return false;}
        copy[length]=0;*clipboard=copy;return true;
    }
    if(action==Action::Console) {
        if(!RegisterConsole(env,helper))return false;
        auto method=env->GetStaticMethodID(helper,"showConsole","(Landroid/app/Activity;)Z");if(Exception(env)||!method)return false;
        auto result=env->CallStaticBooleanMethod(helper,method,activity);return !Exception(env)&&result==JNI_TRUE;
    }
    auto method=env->GetStaticMethodID(helper,action==Action::URL?"launchUrl":"confirm",action==Action::URL?
        "(Landroid/app/Activity;Ljava/lang/String;)Z":"(Landroid/app/Activity;Ljava/lang/String;Ljava/lang/String;)Z");
    if(Exception(env)||!method)return false;
    jstring a=Text(env,first),b=action==Action::Confirm?Text(env,second):nullptr;
    if(!a||(action==Action::Confirm&&!b))return false;
    auto result=action==Action::URL?env->CallStaticBooleanMethod(helper,method,activity,a):env->CallStaticBooleanMethod(helper,method,activity,a,b);
    bool failed=Exception(env);if(delivered)*delivered=!failed;return !failed&&result==JNI_TRUE;
}
char *ClipboardPayload(char *copy) {
    if(!copy)return nullptr;
    size_t length=std::strlen(copy);
    if(length>ClipboardCapacity){std::free(copy);return nullptr;}
    auto *data=reinterpret_cast<char*>(Z_Malloc(static_cast<int>(length+1),"Sys_GetClipboardData",11));
    if(!data){std::free(copy);return nullptr;}
    std::memcpy(data,copy,length+1);std::free(copy);
    char *cursor{};strtok_r(data,"\n\r\b",&cursor);
    return data;
}
struct Cursor { unsigned x,y; };
bool WarpOnOwner(void *opaque) {
    auto &cursor=*static_cast<Cursor*>(opaque);
    if(!bo1::android::PrepareRendererVideo())return false;
    SDL_Window *window=reinterpret_cast<SDL_Window*>(dx.windows[0].hwnd);
    bo1::android::WindowLifecycleState tracked{};bo1::android::RendererWindowInfo actual{};
    if(!bo1::android::ReadSDLWindow(window,&tracked)||tracked.closing||!bo1::android::InspectRendererWindow(window,&actual))return false;
    if(cursor.x>=static_cast<unsigned>(actual.drawableWidth)||cursor.y>=static_cast<unsigned>(actual.drawableHeight)) {SDL_SetError("cursor target is outside actual drawable");return false;}
    if(SDL_GetMouseFocus()!=window){SDL_SetError("actual window has no SDL mouse focus");return false;}
    int x=static_cast<int>(static_cast<uint64_t>(cursor.x)*actual.width/actual.drawableWidth);
    int y=static_cast<int>(static_cast<uint64_t>(cursor.y)*actual.height/actual.drawableHeight);
    SDL_WarpMouseInWindow(window,x,y);
    int gotX{},gotY{};SDL_GetMouseState(&gotX,&gotY);
    if(gotX!=x||gotY!=y){SDL_SetError("SDL did not apply per-window cursor position");return false;}
    return true;
}
}
namespace bo1::android {
bool ConfirmConfigurationChange(const char *title,const char *message) {
    if(Sys_IsHeadless()||!title||!message)return false;
    bool delivered{};bool answer=Invoke(Action::Confirm,title,message,nullptr,&delivered);
    // False is also a legitimate No/dismissal. Java logs actual dispatch errors.
    if(!delivered)Com_PrintWarning(16,"Configuration confirmation unavailable: actual Activity/UI dispatch is required; no Yes response received\n");
    return answer;
}
bool LaunchAndroidURL(const char *url) {
    if(Sys_IsHeadless()||!url||!*url)return false;
    bool launched=Invoke(Action::URL,url,nullptr);
    if(!launched)Com_PrintWarning(16,"Android URL launch failed: requires live Activity, intent handler and real UI acknowledgement\n");
    return launched;
}
void AppendAndroidConsoleOutput(const char *message) {
    if(!message)return;
    size_t length=std::strlen(message);
    std::lock_guard lock(outputMutex);
    if(length>=output.size()){std::memcpy(output.data(),message+length-output.size(),output.size());outputLength=output.size();return;}
    size_t removed=outputLength+length>output.size()?outputLength+length-output.size():0;
    if(removed){std::memmove(output.data(),output.data()+removed,outputLength-removed);outputLength-=removed;}
    std::memcpy(output.data()+outputLength,message,length);outputLength+=length;
}
void InstallAndroidConsoleCommandOwner() {
    bool valid=IsSDLEventOwner();
    {std::lock_guard lock(commandMutex);if(commandOwner!=std::thread::id{})valid=false;else if(valid){commandOwner=std::this_thread::get_id();commandHead=commandTail=0;}}
    if(!valid){Com_Error(ERR_FATAL,"Android UI console requires the actual initialized SDL/engine input owner");std::abort();}
}
bool PumpAndroidConsoleCommandsIfOwner() {
    if(!IsSDLEventOwner())return false;
    for(unsigned n=0;n<commands.size();++n){
        Command command{};
        {std::lock_guard lock(commandMutex);if(commandOwner!=std::this_thread::get_id())return false;if(commandHead==commandTail)return true;command=commands[commandTail%commands.size()];++commandTail;}
        auto *payload=Z_Malloc(command.length+1,"Android UI console command",11);
        if(!payload){Com_Error(ERR_FATAL,"Android UI console command allocation failed");std::abort();}
        std::memcpy(payload,command.bytes,command.length+1);
        Sys_QueEvent(0,SE_CONSOLE,0,0,static_cast<int>(command.length+1),payload);
    }
    return true;
}
void RetireAndroidConsoleCommandOwner() {
    bool valid;
    {std::lock_guard lock(commandMutex);valid=commandOwner==std::this_thread::get_id();if(valid){commandOwner={};commandHead=commandTail=0;}}
    if(!valid){Com_Error(ERR_FATAL,"Android UI console retirement requires its actual input owner");std::abort();}
}
}
void IN_SetCursorPos(unsigned x,unsigned y) {
    if(Sys_IsHeadless())return; // Reference: no desktop cursor warp in headless mode.
    if(!bo1::android::SDLJavaSetupComplete()){Com_PrintWarning(16,"Android cursor warp refused: actual SDL Java setup is absent\n");return;}
    Cursor cursor{x,y};
    if(!bo1::android::RunRendererOwnerTask(WarpOnOwner,&cursor))Com_PrintWarning(16,"Android per-window cursor warp failed: %s\n",SDL_GetError());
}
char *Sys_GetClipboardData() {
    if(Sys_IsHeadless())return nullptr; // No Activity clipboard in the windowless process.
    char *copy{};if(!Invoke(Action::Clipboard,nullptr,nullptr,&copy))return nullptr;
    // JNI frames are retired before entering the real engine allocator. The
    // existing first-token policy uses thread-local strtok_r state.
    return ClipboardPayload(copy);
}
void Sys_ShowConsole() {
    if(Sys_IsHeadless())return;
    if(!Invoke(Action::Console,nullptr,nullptr))Com_PrintWarning(16,"Android console was not shown: actual Activity/UI/JNI console binding required\n");
}
void Sys_OpenURL(const char *url,int doexit) {
    if(Sys_IsHeadless())return;
    if(bo1::android::LaunchAndroidURL(url)&&doexit)Cbuf_AddText(0,"quit\n");
}
void Sys_UpdateHotkeyBlock() {
    if(Sys_IsHeadless()||!vidConfig.isFullscreen)return;
    auto *window=reinterpret_cast<SDL_Window*>(dx.windows[0].hwnd);
    bo1::android::WindowLifecycleState state{};
    if(!bo1::android::ReadSDLWindow(window,&state)||!state.focused||state.closing||CL_GetLocalClientConnectionState(0)==0)return;
    static std::atomic<bool> warned{};
    if(!warned.exchange(true))Com_PrintWarning(16,"Android does not provide Windows global system-hotkey interception; no desktop hotkey block installed\n");
}
