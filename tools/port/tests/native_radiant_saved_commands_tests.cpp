#include <qcommon/radiant_remote.h>
#include <platform/android/bounded_format.h>
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <sys/wait.h>
#include <unistd.h>

extern RadiantCommand savedCommands[128];
extern int savedCommandCount;
extern bool runningSavedCommands;
void Sys_Error(char *,...);
int DL_VPrintf(const char *,va_list);
static int PrintDownload(const char *format,...) {
    va_list args;va_start(args,format);const int count=DL_VPrintf(format,args);va_end(args);return count;
}
void Com_Printf(int,const char *format,...) {
    va_list args;va_start(args,format);std::vfprintf(stderr,format,args);va_end(args);
}
void Com_Error(errorParm_t,const char *format,...) {
    char text[512];va_list args;va_start(args,format);
    std::vsnprintf(text,sizeof(text),format,args);va_end(args);
    Sys_Error(const_cast<char *>("%s"),text);
}
static void Failure(void (*action)()) {
    const pid_t pid=fork();assert(pid>=0);
    if(!pid) {action();_exit(91);}
    int result;assert(waitpid(pid,&result,0)==pid);
    assert(WIFEXITED(result) && WEXITSTATUS(result)==2);
}
int main() {
    RadiantRemoteInit();
    for(int i=1;i<=128;++i) {
        RadiantCommand command{};command.type=RADIANT_COMMAND_CREATE;command.liveUpdateId=i;
        std::snprintf(command.strCommand,sizeof(command.strCommand),"owner-%d",i);
        AddSavedCommand(&command);
    }
    assert(savedCommandCount==128);
    RadiantCommand update{};update.liveUpdateId=128;
    update.type=RADIANT_COMMAND_UPDATE_SELECTED;std::strcpy(update.strCommand,"replacement");
    AddSavedCommand(&update);
    assert(savedCommandCount==128 && savedCommands[127].type==RADIANT_COMMAND_CREATE);
    assert(!std::strcmp(savedCommands[127].strCommand,"replacement"));
    update.type=RADIANT_COMMAND_DELETE;AddSavedCommand(&update);
    assert(savedCommandCount==128 && savedCommands[0].liveUpdateId==1);
    runningSavedCommands=true;update.type=RADIANT_COMMAND_UPDATE;std::strcpy(update.strCommand,"ignored");
    AddSavedCommand(&update);runningSavedCommands=false;
    assert(!std::strcmp(savedCommands[127].strCommand,"replacement"));
    G_AssignGameIdMapping(1,17);G_AssignGameIdMapping(128,99);
    CG_AssignGameIdMapping(1,71);CG_AssignGameIdMapping(128,199);
    assert(G_GetGameIdMapping(1)==17 && G_GetGameIdMapping(128)==99);
    assert(CG_GetGameIdMapping(1)==71 && CG_GetGameIdMapping(128)==199);
    assert(G_GetGameIdMapping(0)==-1 && CG_GetGameIdMapping(-1)==-1);
    Failure([]{G_AssignGameIdMapping(129,1);});
    Failure([]{CG_GetGameIdMapping(129);});
    Failure([]{AddSavedCommand(nullptr);});
    Failure([]{RadiantCommand value{};value.type=static_cast<RadiantCommandType>(7);AddSavedCommand(&value);});
    Failure([]{RadiantCommand value{};std::memset(value.strCommand,'x',sizeof(value.strCommand));AddSavedCommand(&value);});
    struct {unsigned before=0x10203040;char text[16]{};unsigned after=0x50607080;} guarded;
    assert(bo1::android::FormatChecked(guarded.text,"%d:%.1f:%s",-7,2.5,"abi")==10);
    assert(!std::strcmp(guarded.text,"-7:2.5:abi"));
    assert(guarded.before==0x10203040 && guarded.after==0x50607080);
    assert(bo1::android::FormatChecked(guarded.text,sizeof(guarded.text),"%llu",0x100000001ULL)==10);
    assert(!std::strcmp(guarded.text,"4294967297"));
    Failure([]{char text[4];bo1::android::FormatChecked(text,"%s","long");});
    Failure([]{bo1::android::FormatChecked(nullptr,0,"%d",1);});
    assert(PrintDownload("%d:%.1f:%llu\n",-7,2.5,0x100000001ULL)==18);
    std::puts("PASS actual Radiant bounds, native format capacities and ARM64 variadic callback");
}
