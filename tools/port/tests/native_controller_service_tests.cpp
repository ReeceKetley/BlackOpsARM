#define SDL_MAIN_HANDLED
#include <platform/android/controller_service.h>
#include <platform/android/window_lifecycle.h>
#include <SDL.h>
#include <thread>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#if defined(BO1_NATIVE_GAMEPAD_HOST_TEST)
#include <win32/win_gamepad.h>
#include <universal/com_errors.h>
#include <cstdarg>
static dvar_s fixtureDvars[32];
static int fixtureDvarCount;
static dvar_s *Register(const char *name) {
    if(fixtureDvarCount>=32) std::abort();
    auto *dvar=&fixtureDvars[fixtureDvarCount++]; dvar->name=name; return dvar;
}
const dvar_s *_Dvar_RegisterInt(const char *name,int value,int,int,unsigned short,const char *) {
    auto *dvar=Register(name); dvar->current.integer=value; return dvar;
}
const dvar_s *_Dvar_RegisterFloat(const char *name,float value,float,float,unsigned short,const char *) {
    auto *dvar=Register(name); dvar->current.value=value; return dvar;
}
const dvar_s *_Dvar_RegisterBool(const char *name,bool value,unsigned short,const char *) {
    auto *dvar=Register(name); dvar->current.enabled=value; return dvar;
}
const dvar_s *_Dvar_RegisterString(const char *name,const char *value,unsigned short,const char *) {
    auto *dvar=Register(name); dvar->current.string=value; return dvar;
}
double Dvar_GetFloat(const char *name) {
    for(int i=0;i<fixtureDvarCount;++i)if(!strcmp(name,fixtureDvars[i].name))return fixtureDvars[i].current.value;
    std::abort();
}
bool Assert_MyHandler(const char *file,int line,int,const char *format,...) {
    fprintf(stderr,"engine assertion %s:%d %s\n",file,line,format); std::abort();
}
void Com_Error(errorParm_t,const char *format,...) {
    va_list args;va_start(args,format);vfprintf(stderr,format,args);va_end(args);std::abort();
}
static bool fixtureHeadless;
bool Sys_IsHeadless() { return fixtureHeadless; } // Explicit fixture modes; actual SDL provider remains required.
char *va(const char *format,...) {
    static thread_local char output[4096];va_list args;va_start(args,format);
    vsnprintf(output,sizeof(output),format,args);va_end(args);return output;
}
// Unused functions from the complete canonical math TU fail if accidentally consumed.
double vectosignedpitch(const float *) { std::abort(); }
bool Com_IsSPLevel() { std::abort(); }
void AngleVectors(const float *,float *,float *,float *) { std::abort(); }
#endif
using namespace bo1::android;
// Isolated terminal quit consumer; controller/window ownership is the real provider.
static bool quitRequested;
void Sys_SetQuitRequested() { quitRequested=true; }
static void Input(const SDL_Event &, void *) {}
static void Check(bool result,int line) { if(!result) { std::fprintf(stderr,"FAIL %d (%s)\n",line,SDL_GetError()); std::abort(); } }
#define CHECK(x) Check((x),__LINE__)
static unsigned rumbleCalls;
static Uint16 lowRumble,highRumble;
static int SDLCALL Rumble(void *,Uint16 low,Uint16 high) { ++rumbleCalls; lowRumble=low; highRumble=high; return 0; }
static int Attach() {
    SDL_VirtualJoystickDesc desc{};
    desc.version=SDL_VIRTUAL_JOYSTICK_DESC_VERSION; desc.type=SDL_JOYSTICK_TYPE_GAMECONTROLLER;
    desc.naxes=SDL_CONTROLLER_AXIS_MAX; desc.nbuttons=SDL_CONTROLLER_BUTTON_MAX;
    desc.axis_mask=(1u<<SDL_CONTROLLER_AXIS_MAX)-1;
    desc.button_mask=(1u<<SDL_CONTROLLER_BUTTON_MAX)-1;
    desc.name="BlackOpsARM explicit SDL virtual controller fixture"; desc.Rumble=Rumble;
    const int device=SDL_JoystickAttachVirtualEx(&desc); CHECK(device>=0); return device;
}
int main() {
    SDL_SetMainReady();
    ControllerSample sample{}; sample.wButtons=0xA55A;
    CHECK(!ControllersReady()); CHECK(ReadController(0,&sample)==-1 && sample.wButtons==0xA55A);
    CHECK(!InitControllers());
#if defined(BO1_NATIVE_GAMEPAD_HOST_TEST)
    fixtureHeadless=true; GPad_InitAll(); CHECK(fixtureDvarCount==15 && !ControllersReady());
    fixtureHeadless=false; fixtureDvarCount=0; memset(fixtureDvars,0,sizeof(fixtureDvars));
#endif
    InstallSDLEventOwner(Input,nullptr);
#if defined(__ANDROID__)
    // Native ADB shell has no JavaVM/SDLControllerManager. Actual driver would dereference JNI.
    CHECK(!SetupSDLControllerJava(nullptr));
    CHECK(!InitControllers());
    CHECK(strstr(SDL_GetError(),"JNI owner required"));
    CHECK(ReadController(0,&sample)==-1 && sample.wButtons==0xA55A);
    RetireSDLEventOwner();
    puts("PASS native controller service refuses missing SDL/JNI owner without neutral readiness");
#else
    CHECK(InitControllers());
    // Fixture assumes no physical game controller occupies the sole canonical port.
    const int initial=ReadController(0,&sample);
    CHECK(initial==0 && sample.wButtons==0xA55A);
    CHECK(!InitControllers());
    bool wrongReady=true; int wrongRead=0;
    std::thread other([&]{ wrongReady=ControllersReady(); wrongRead=ReadController(0,&sample); }); other.join();
    CHECK(!wrongReady && wrongRead==-1);
    CHECK(ReadController(-1,&sample)==-1 && ReadController(ControllerSlots,&sample)==-1);
    CHECK(ReadController(0,nullptr)==-1);
    const int first=Attach();
    SDL_Joystick *joystick=SDL_JoystickOpen(first); CHECK(joystick);
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,"1");
    PumpSDLEvents(); CHECK(RefreshControllers()); // Open/controller-bind before publishing axis changes.
    for(int button=0;button<SDL_CONTROLLER_BUTTON_MAX;++button) CHECK(!SDL_JoystickSetVirtualButton(joystick,button,1));
    CHECK(!SDL_JoystickSetVirtualAxis(joystick,SDL_CONTROLLER_AXIS_LEFTX,-32768));
    CHECK(!SDL_JoystickSetVirtualAxis(joystick,SDL_CONTROLLER_AXIS_LEFTY,-32768));
    CHECK(!SDL_JoystickSetVirtualAxis(joystick,SDL_CONTROLLER_AXIS_RIGHTX,12345));
    CHECK(!SDL_JoystickSetVirtualAxis(joystick,SDL_CONTROLLER_AXIS_RIGHTY,23456));
    CHECK(!SDL_JoystickSetVirtualAxis(joystick,SDL_CONTROLLER_AXIS_TRIGGERLEFT,32767));
    CHECK(!SDL_JoystickSetVirtualAxis(joystick,SDL_CONTROLLER_AXIS_TRIGGERRIGHT,-32768));
    PumpSDLEvents(); CHECK(RefreshControllers()); CHECK(ReadController(0,&sample)==1);
    fprintf(stderr,"sample buttons=%04x L=%d,%d R=%d,%d trigger=%u,%u\n",sample.wButtons,sample.sThumbLX,sample.sThumbLY,sample.sThumbRX,sample.sThumbRY,sample.bLeftTrigger,sample.bRightTrigger);
    CHECK(sample.wButtons==0xF3FF && sample.sThumbLX==-32768 && sample.sThumbLY==32767);
    CHECK(sample.sThumbRX==12345 && sample.sThumbRY==-23456);
    CHECK(sample.bLeftTrigger==255 && sample.bRightTrigger==0);
    const auto generation=ControllerGeneration(0); CHECK(generation);
    CHECK(RumbleController(0,12000,34000,100)); CHECK(rumbleCalls && lowRumble==12000 && highRumble==34000);
    const int second=Attach(); CHECK(second!=first);
    PumpSDLEvents(); CHECK(RefreshControllers()); CHECK(ControllerGeneration(0)==generation);
    // Detach first: provider retires its handle, then assigns previously extra actual SDL device.
    CHECK(!SDL_JoystickDetachVirtual(first)); SDL_JoystickClose(joystick);
    PumpSDLEvents(); CHECK(RefreshControllers()); CHECK(ControllerGeneration(0)>generation);
    CHECK(ReadController(0,&sample)==1);
    CHECK(!SDL_JoystickDetachVirtual(first)); // remaining device index shifts after first removal
    PumpSDLEvents(); CHECK(RefreshControllers());
    sample.wButtons=0xA55A; CHECK(ReadController(0,&sample)==0 && sample.wButtons==0xA55A);
    CHECK(!RumbleController(0,1,1,10));
    CHECK(ShutdownControllers()); CHECK(!ControllersReady()); CHECK(ReadController(0,&sample)==-1);
    CHECK(InitControllers()); CHECK(ReadController(0,&sample)==0); CHECK(ShutdownControllers());
#if defined(BO1_NATIVE_GAMEPAD_HOST_TEST)
    GPad_InitAll(); CHECK(fixtureDvarCount==15);
    const int canonicalDevice=Attach();
    SDL_Joystick *canonicalJoystick=SDL_JoystickOpen(canonicalDevice); CHECK(canonicalJoystick);
    PumpSDLEvents(); GPad_UpdateAll(); CHECK(GPad_IsActive(0));
    CHECK(!SDL_JoystickSetVirtualButton(canonicalJoystick,SDL_CONTROLLER_BUTTON_A,1));
    CHECK(!SDL_JoystickSetVirtualAxis(canonicalJoystick,SDL_CONTROLLER_AXIS_LEFTX,32767));
    CHECK(!SDL_JoystickSetVirtualAxis(canonicalJoystick,SDL_CONTROLLER_AXIS_TRIGGERLEFT,32767));
    PumpSDLEvents(); GPad_UpdateAll();
    CHECK(GPad_GetButton(0,GPAD_A)==1 && GPad_IsButtonPressed(0,GPAD_A));
    CHECK(GPad_GetStick(0,GPAD_LX)>0.99 && GPad_IsStickPressed(0,GPAD_LX,GPAD_STICK_POS));
    CHECK(GPad_GetButton(0,GPAD_L_TRIG)>0.99 && GPad_IsButtonPressed(0,GPAD_L_TRIG));
    GPad_UpdateAll(); CHECK(!GPad_IsButtonPressed(0,GPAD_A) && !GPad_IsStickPressed(0,GPAD_LX,GPAD_STICK_POS));
    CHECK(!SDL_JoystickSetVirtualButton(canonicalJoystick,SDL_CONTROLLER_BUTTON_A,0));
    CHECK(!SDL_JoystickSetVirtualAxis(canonicalJoystick,SDL_CONTROLLER_AXIS_LEFTX,0));
    CHECK(!SDL_JoystickSetVirtualAxis(canonicalJoystick,SDL_CONTROLLER_AXIS_TRIGGERLEFT,-32768));
    PumpSDLEvents(); GPad_UpdateAll();
    CHECK(GPad_IsButtonReleased(0,GPAD_A) && GPad_IsButtonReleased(0,GPAD_L_TRIG));
    CHECK(GPad_IsStickReleased(0,GPAD_LX,GPAD_STICK_POS));
    CHECK(!SDL_JoystickDetachVirtual(canonicalDevice)); SDL_JoystickClose(canonicalJoystick);
    PumpSDLEvents(); GPad_UpdateAll(); CHECK(!GPad_IsActive(0));
    GPad_ShutdownAll(); CHECK(!ControllersReady());
    puts("PASS canonical GPad registration/deadzone/masked-stick/button/trigger/pressed/released/removal from real SDL samples (isolated dvar storage)");
#endif
    RetireSDLEventOwner();
    puts("PASS actual SDL virtual controller owner: enumeration, slots, axes/buttons/triggers, rumble, hotplug retirement/reassignment, wrong owner, shutdown/restart");
#endif
}
