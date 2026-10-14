// Actual adapter source included only to test private bounded FIFO/copy helpers;
// no Java VM, Activity, positive GPU or engine readiness provider is invented.
#include "../../src/platform/android/ui_platform.cpp"
#include <vector>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <csignal>
DxGlobals dx;
vidConfig_t vidConfig;
clientUIActive_t clientUIActives[1];
static bool headless=true,zoneFail{},reenterAllocation{};
static int allocations{},quitCommands{},warnings{},watchedFrees{};
static void *watchedCopy;
static std::vector<std::string> events;
static void Check(bool ok,const char *message){if(!ok){std::fprintf(stderr,"FAIL %s\n",message);std::abort();}}
bool Sys_IsHeadless(){return headless;}
void Com_PrintWarning(int,const char*,...){++warnings;}
void Com_Error(errorParm_t,const char*,...){bo1::android::AppendAndroidConsoleOutput("actual fixture error callback\n");QueueCommand("error callback",14);}
bool Assert_MyHandler(const char*,int,int,const char*,...){std::abort();}
void Cbuf_AddText(int,const char*){++quitCommands;}
void Sys_SetQuitRequested(){std::abort();}
extern "C" void *__wrap_SDL_AndroidGetJNIEnv(){std::abort();}
extern "C" void *__wrap_SDL_AndroidGetActivity(){std::abort();}
extern "C" void __real_free(void*);
extern "C" void __wrap_free(void *value){if(value && value==watchedCopy){++watchedFrees;watchedCopy=nullptr;}__real_free(value);}
unsigned int *Z_Malloc(int size,const char *name,int tag){Check(size>0&&size<=1024*1024+1&&name&&tag==11,"actual payload allocation contract");++allocations;bo1::android::AppendAndroidConsoleOutput("allocation callback\n");if(reenterAllocation){reenterAllocation=false;Check(QueueCommand("nested",6),"allocator callback can reenter FIFO without deadlock");}if(zoneFail)return nullptr;return static_cast<unsigned int*>(std::malloc(size));}
void Sys_QueEvent(unsigned time,sysEventType_t type,int a,int b,int length,void *payload){Check(time==0&&type==SE_CONSOLE&&a==0&&b==0&&length>1&&length<=1024&&payload,"actual SE_CONSOLE payload contract");Check(static_cast<char*>(payload)[length-1]==0,"payload terminator");events.emplace_back(static_cast<char*>(payload));std::free(payload);}
void Consume(const SDL_Event &,void*){}
static void FatalChild(int kind){pid_t pid=fork();Check(pid>=0,"fork");if(!pid){if(kind==0)bo1::android::InstallAndroidConsoleCommandOwner();if(kind==1){std::thread wrong([]{bo1::android::RetireAndroidConsoleCommandOwner();});wrong.join();}if(kind==2){zoneFail=true;QueueCommand("allocation failure",18);bo1::android::PumpAndroidConsoleCommandsIfOwner();}_Exit(9);}int status{};Check(waitpid(pid,&status,0)==pid&&WIFSIGNALED(status)&&WTERMSIG(status)==SIGABRT,"error callbacks reenter without lock and cannot fake completion");}
#undef main
int main(){
    Check(!bo1::android::SDLJavaSetupComplete(),"actual Java setup absent in shell");
    IN_SetCursorPos(5,5);Sys_ShowConsole();Sys_OpenURL("https://example.invalid/",1);Sys_UpdateHotkeyBlock();
    Check(!Sys_GetClipboardData()&&!bo1::android::ConfirmConfigurationChange("title","message"),"headless actual services do not fabricate UI/data/Yes");
    headless=false;IN_SetCursorPos(5,5);Sys_ShowConsole();Sys_OpenURL("https://example.invalid/",1);Sys_UpdateHotkeyBlock();
    Check(!Sys_GetClipboardData()&&!bo1::android::ConfirmConfigurationChange("title","message"),"absent real JNI context refuses UI/data/Yes before unsafe getters");
    Check(allocations==0&&quitCommands==0,"failed URL never queues quit and clipboard never allocates fake empty data");
    bo1::android::AppendAndroidConsoleOutput("first\nsecond\n");
    {std::lock_guard lock(outputMutex);Check(std::string(output.data(),outputLength)=="first\nsecond\n","actual retained output bytes");}
    std::string large(OutputCapacity+27,'x');large.back()='z';bo1::android::AppendAndroidConsoleOutput(large.c_str());
    {std::lock_guard lock(outputMutex);Check(outputLength==OutputCapacity&&output.front()=='x'&&output.back()=='z',"actual tail retention cap");}
    char *copy=static_cast<char*>(std::malloc(40));std::strcpy(copy,"first\nsecond\rthird\b");watchedCopy=copy;
    char *clip=ClipboardPayload(copy);Check(clip&&std::strcmp(clip,"first")==0&&watchedFrees==1,"real canonical first token/tagged-owned clipboard copy");std::free(clip);
    copy=static_cast<char*>(std::malloc(10));std::strcpy(copy,"\nvalue\n");clip=ClipboardPayload(copy);Check(clip&&std::strcmp(clip,"\nvalue")==0,"original ignored strtok return/leading separator behavior preserved");std::free(clip);
    copy=static_cast<char*>(std::malloc(8));std::strcpy(copy,"owned");watchedCopy=copy;zoneFail=true;Check(!ClipboardPayload(copy)&&watchedFrees==2,"allocation failure retires borrowed JNI-copy heap and yields no clipboard payload");zoneFail=false;
    Check(!QueueCommand("none",4)&&!CommandReady(),"missing actual console consumer refuses admission");
    SDL_SetMainReady();bo1::android::InstallSDLEventOwner(Consume,nullptr);bo1::android::InstallAndroidConsoleCommandOwner();
    Check(CommandReady(),"actual fixture SDL/command owner installed");
    FatalChild(0);FatalChild(1);FatalChild(2);
    Check(!QueueCommand("",0)&&!QueueCommand("a\nb",3)&&!QueueCommand("a\0b",3)&&!QueueCommand(large.data(),1024),"invalid byte/line/capacity admission rejected");
    for(int i=0;i<32;++i){char line[32];int length=std::snprintf(line,sizeof(line),"echo %i",i);Check(QueueCommand(line,length),"bounded FIFO admission");}
    Check(!QueueCommand("overflow",8),"full FIFO rejects without dropping accepted commands");
    Check(bo1::android::PumpAndroidConsoleCommandsIfOwner()&&events.size()==32,"actual owner queues all tag11 payloads");
    for(int i=0;i<32;++i)Check(events[i]=="echo "+std::to_string(i),"FIFO payload ordering");
    reenterAllocation=true;Check(QueueCommand("outer",5)&&bo1::android::PumpAndroidConsoleCommandsIfOwner(),"reentrant allocator callback");Check(events.size()==34&&events[32]=="outer"&&events[33]=="nested","reentrant FIFO ordering");
    std::atomic<int> done{};std::vector<std::thread> producers;
    for(int thread=0;thread<4;++thread)producers.emplace_back([&done,thread]{for(int i=0;i<20;++i){char text[32];int length=std::snprintf(text,sizeof(text),"thread %i %i",thread,i);while(!QueueCommand(text,length))std::this_thread::yield();}++done;});
    for(;;){bo1::android::PumpAndroidConsoleCommandsIfOwner();bool empty;{std::lock_guard lock(commandMutex);empty=commandHead==commandTail;}if(done==4&&empty)break;std::this_thread::yield();}
    for(auto &thread:producers)thread.join();Check(events.size()==114,"concurrent commands owned/delivered once");
    std::thread wrong([]{Check(!bo1::android::PumpAndroidConsoleCommandsIfOwner(),"wrong owner cannot drain commands");});wrong.join();
    Check(QueueCommand("retired pending",15),"owned pending command before shutdown");bo1::android::RetireAndroidConsoleCommandOwner();
    Check(!CommandReady()&&!QueueCommand("late",4)&&!bo1::android::PumpAndroidConsoleCommandsIfOwner()&&events.size()==114,"retirement rejects/retire pending data without claiming execution");
    bo1::android::RetireSDLEventOwner();
    std::puts("PASS actual UI adapter CPU/real SDL owner: headless and absent-JNI refusal, no false quit/clipboard/Yes, output cap, tagged clipboard cleanup, FIFO bounds/concurrency/reentry/payload/retirement; no actual Activity/UI/GPU proof");return 0;
}
