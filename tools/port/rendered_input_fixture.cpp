// CPU/actual SDL event-owner contract fixture. No Activity, video surface or
// initialized game client is manufactured; real graphics/JNI remain unexecuted.
#include <platform/android/rendered_input.cpp>
#include <cstdio>
#include <cstring>
#include <vector>
#include <sys/wait.h>
#include <unistd.h>
struct Event {sysEventType_t type;int value,down;};
static std::vector<Event> events;
// Uninitialized client state, never a game/graphics readiness publication.
const dvar_t *in_mouse;
const dvar_t *r_fullscreen;
clientUIActive_t clientUIActives[1]{};
bool Assert_MyHandler(const char *,int,int,const char *,...){std::abort();}
int Dvar_GetInt(const char *){std::abort();}
void Dvar_SetBool(dvar_s *,bool){std::abort();}
void Cbuf_AddText(int,const char *){std::abort();}
void Com_PrintWarning(int,const char *,...){std::abort();}
unsigned int Sys_Milliseconds(){return 731;}
void Sys_QueEvent(unsigned time,sysEventType_t type,int value,int down,int bytes,void *payload) {
    if(time!=731 || bytes || payload)std::abort();events.push_back({type,value,down});
}
void Com_Error(errorParm_t,const char *,...){std::fputs("expected fixture fatal callback returned\n",stderr);}
void Sys_SetQuitRequested(){std::abort();}
extern "C" void *__wrap_SDL_AndroidGetJNIEnv(){std::abort();}
extern "C" void *__wrap_SDL_AndroidGetActivity(){std::abort();}
static void Check(bool value,const char *message){if(!value){std::fprintf(stderr,"FAIL: %s (%s)\n",message,SDL_GetError());std::abort();}}
static void MustAbort(void (*fn)(),const char *message){
    const pid_t child=fork();Check(child>=0,"fork");
    if(!child){fn();_exit(4);}
    int status{};Check(waitpid(child,&status,0)==child,"waitpid");Check(WIFSIGNALED(status) && WTERMSIG(status)==SIGABRT,message);
}
static void Overflow(){bo1::android::deltaX=INT_MAX;bo1::android::accumulate(nullptr,0,0,1,0);}
static void BadRelease(){bo1::android::release(200);}
static void WrongOwner(){bo1::android::ActivateRenderedInput(true);}
#undef main
int main(){
    using namespace bo1::android;
    SDL_SetMainReady();
    Check(!SDLJavaSetupComplete(),"no actual Java context in shell");
    Check(!InitializeRenderedInput() && !IsSDLEventOwner(),"absence cannot publish a rendered input owner");
    Check(!AdoptSDLActivityJava(nullptr,nullptr),"null adoption honestly fails before SDK getters");
    SDL_Keysym key{};key.scancode=SDL_SCANCODE_A;key.sym=SDLK_a;
    Check(mapKey(key)=='a',"canonical printable key");key.sym='A';Check(mapKey(key)=='a',"ASCII case normalization");
    key.scancode=SDL_SCANCODE_GRAVE;key.sym=SDLK_BACKQUOTE;Check(mapKey(key)==126,"canonical physical console key");
    key.scancode=SDL_SCANCODE_LSHIFT;key.sym=SDLK_LSHIFT;Check(mapKey(key)==160,"actual canonical left modifier lookup");
    key.sym=SDLK_RSHIFT;Check(mapKey(key)==160,"actual canonical right modifier shares key");
    key.sym=SDLK_KP_EQUALS;Check(mapKey(key)==199,"actual keypad lookup");
    key.sym=SDLK_DELETE;Check(mapKey(key)==162,"Delete is not ASCII backspace");
    for(int i=0;i<12;++i){key.sym=SDLK_F1+i;Check(mapKey(key)==167+i,"actual canonical F-key table");}
    key.sym=SDLK_UNKNOWN;Check(mapKey(key)==0,"unmapped input is not a fictional key");
    std::size_t length=999;
    Check(validUtf8("ASCII",6,&length) && length==5,"bounded committed ASCII");
    Check(validUtf8("\xc2\xa3\xe2\x82\xac\xf0\x9f\x98\x80",10,&length) && length==9,"complete two/three/four-byte UTF8");
    const char *bad[]={"\xc0\xaf","\xed\xa0\x80","\xf4\x90\x80\x80","\xe2\x82","\x80"};
    for(auto *text:bad)Check(!validUtf8(text,std::strlen(text)+1,&length),"malformed/overlong/surrogate UTF8 rejected");
    Check(!validUtf8("abcd",4,&length),"unterminated fixed SDL buffer rejected");
    retain(160);retain(160);release(160);Check(events.size()==1 && held[160]==1,"paired modifiers keep shared canonical key down");
    release(160);Check(events.size()==2 && events[1].down==0,"last producer releases shared key");
    events.clear();retain(200);retain(200);release(200);Check(events.size()==1,"touch and physical mouse ownership coexists");release(200);Check(events.size()==2,"last actual mouse source releases");
    events.clear();retain(160);retain(200);scanKeys[SDL_SCANCODE_LSHIFT]=160;buttons[0]=true;primary=true;
    releaseAll();Check(events.size()==4 && held[160]==0 && held[200]==0 && !primary && !buttons[0] && !scanKeys[SDL_SCANCODE_LSHIFT],"focus retirement clears exact held controls");
    accumulate(nullptr,73,41,7,-9);accumulate(nullptr,74,40,-3,2);Check(mouseX==74 && mouseY==40 && deltaX==4 && deltaY==-7,"real buffered frame arithmetic preserves signed deltas");
    Check(scale(75,100,200)==150 && scale(-7,100,200)==-14,"drawable coordinate scale with signed movement");deltaX=deltaY=0;
    MustAbort(Overflow,"delta overflow fatals even if ERR_FATAL returns");
    MustAbort(BadRelease,"unknown release is rejected");
    MustAbort(WrongOwner,"uninstalled owner is rejected");
    std::puts("PASS actual canonical key lookup, UTF8 boundaries, shared key/button retirement, frame delta checks and real Java absence; positive Activity/GPU input unexecuted");
}
