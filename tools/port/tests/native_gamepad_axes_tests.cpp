#include <client/cl_gamepad.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>

extern GpadAxesGlob gaGlobs[1];
int dvar_modifiedFlags;
static bool devGuiActive;
// Isolated engine-consumer providers. No controller/SDL/platform readiness owner.
bool DevGui_IsActive() { return devGuiActive; }
bool Assert_MyHandler(const char *file, int line, int, const char *format, ...) {
    fprintf(stderr,"engine assertion %s:%d %s\n",file,line,format); std::abort();
}
static void Check(bool ok, int line) { if(!ok) { fprintf(stderr,"FAIL line %d\n",line); std::abort(); } }
#define CHECK(x) Check((x),__LINE__)
static void Rejected(int which) {
    const pid_t child=fork(); CHECK(child>=0);
    if(!child) {
        if(which==0) Gamepad_BindAxis(nullptr,GPAD_PHYSAXIS_LSTICK_X,GPAD_VIRTAXIS_SIDE,GPAD_MAP_LINEAR);
        if(which==1) Gamepad_BindAxis(gaGlobs,GPAD_PHYSAXIS_COUNT,GPAD_VIRTAXIS_SIDE,GPAD_MAP_LINEAR);
        if(which==2) Gamepad_BindAxis(gaGlobs,GPAD_PHYSAXIS_LSTICK_X,static_cast<GamepadVirtualAxis>(-2),GPAD_MAP_LINEAR);
        if(which==3) Gamepad_BindAxis(gaGlobs,GPAD_PHYSAXIS_LSTICK_X,GPAD_VIRTAXIS_SIDE,GPAD_MAP_NONE);
        _exit(90);
    }
    int status=0; CHECK(waitpid(child,&status,0)==child && WIFSIGNALED(status));
}
int main() {
    alarm(10);
    CL_InitGamepadAxisBindings();
    for(int axis=0;axis<6;++axis) {
        CHECK(gaGlobs[0].axesValues[axis]==0);
        CHECK(gaGlobs[0].virtualAxes[axis].physicalAxis==GPAD_PHYSAXIS_NONE);
        CHECK(gaGlobs[0].virtualAxes[axis].mapType==GPAD_MAP_SQUARED);
    }
    Gamepad_BindAxis(gaGlobs,GPAD_PHYSAXIS_LSTICK_X,GPAD_VIRTAXIS_SIDE,GPAD_MAP_LINEAR);
    Gamepad_BindAxis(gaGlobs,GPAD_PHYSAXIS_LSTICK_X,GPAD_VIRTAXIS_YAW,GPAD_MAP_SQUARED);
    Gamepad_BindAxis(gaGlobs,GPAD_PHYSAXIS_RTRIGGER,GPAD_VIRTAXIS_ATTACK,GPAD_MAP_SQUARED);
    gaGlobs[0].axesValues[GPAD_PHYSAXIS_LSTICK_X]=12000;
    gaGlobs[0].axesValues[GPAD_PHYSAXIS_LSTICK_Y]=-16000;
    gaGlobs[0].axesValues[GPAD_PHYSAXIS_RTRIGGER]=18000;
    const float scale=0.000015259022f;
    const float x=12000*scale, y=-16000*scale, trigger=18000*scale;
    CHECK(std::fabs(CL_GamepadAxisValue(0,GPAD_VIRTAXIS_SIDE)-x)<1e-6);
    CHECK(std::fabs(CL_GamepadAxisValue(0,GPAD_VIRTAXIS_YAW)-std::sqrt(x*x+y*y)*x)<1e-6);
    CHECK(std::fabs(CL_GamepadAxisValue(0,GPAD_VIRTAXIS_ATTACK)-trigger*trigger)<1e-6);
    devGuiActive=true; CHECK(CL_GamepadAxisValue(0,GPAD_VIRTAXIS_SIDE)==0); devGuiActive=false;
    int before[6]; memcpy(before,gaGlobs[0].axesValues,sizeof(before));
    Gamepad_BindAxis(gaGlobs,GPAD_PHYSAXIS_LSTICK_X,GPAD_VIRTAXIS_NONE,GPAD_MAP_SQUARED);
    CHECK(!memcmp(before,gaGlobs[0].axesValues,sizeof(before)));
    CHECK(gaGlobs[0].virtualAxes[GPAD_VIRTAXIS_SIDE].physicalAxis==GPAD_PHYSAXIS_NONE);
    CHECK(gaGlobs[0].virtualAxes[GPAD_VIRTAXIS_YAW].physicalAxis==GPAD_PHYSAXIS_NONE);
    CHECK(gaGlobs[0].virtualAxes[GPAD_VIRTAXIS_ATTACK].physicalAxis==GPAD_PHYSAXIS_RTRIGGER);
    CHECK(CL_GamepadAxisValue(0,GPAD_VIRTAXIS_SIDE)==0 && (dvar_modifiedFlags&1));
    for(int which=0;which<4;++which) Rejected(which);
    Axis_Unbindall_f();
    for(int axis=0;axis<6;++axis) CHECK(gaGlobs[0].virtualAxes[axis].physicalAxis==GPAD_PHYSAXIS_NONE);
    puts("PASS canonical gamepad axes: initialization, linear/radial/trigger mapping, devgui, physical unbind preserves values/other bindings, invalid enum/pointer assertions");
}
