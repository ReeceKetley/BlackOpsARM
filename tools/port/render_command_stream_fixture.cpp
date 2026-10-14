// Isolated CPU command-stream contract checks; no engine/GPU startup claim.
#include <gfx_d3d/r_rendercmds.h>
#include <gfx_d3d/rb_backend.h>
#include <gfx_d3d/r_init.h>
#include <gfx_d3d/r_ui3d.h>
#include <qcommon/common.h>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstddef>
#include <cstdint>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

r_globals_t rg{};
r_global_permanent_t rgp{};
extern GfxCmdArray *s_cmdList;
extern GfxBackEndData s_backEndData[2];
extern GfxCmdArray g_frontEndCmds[2];
extern GfxViewInfo g_viewInfo[2][4];
extern GfxBackEndData *frontEndDataOut;
extern "C" __attribute__((no_instrument_function)) void __cyg_profile_func_enter(void *, void *) {}
extern "C" __attribute__((no_instrument_function)) void __cyg_profile_func_exit(void *, void *) {}
// Fixture diagnostics fail terminally and never imply a graphics provider exists.
void Com_PrintWarning(int, const char *format, ...) {
    va_list args;va_start(args,format);std::vfprintf(stderr,format,args);va_end(args);
}
void Com_Error(errorParm_t, const char *format, ...) {
    va_list args;va_start(args,format);std::vfprintf(stderr,format,args);va_end(args);std::fputc('\n',stderr);std::_Exit(77);
}
bool Assert_MyHandler(const char *file,int line,int,const char *format,...) {
    std::fprintf(stderr,"fixture assertion %s:%i ",file,line);
    va_list args;va_start(args,format);std::vfprintf(stderr,format,args);va_end(args);std::abort();
}
char *va(const char *format, ...) {
    static thread_local char text[2048];va_list args;va_start(args,format);std::vsnprintf(text,sizeof(text),format,args);va_end(args);return text;
}
static int checks;
static void Check(bool valid,const char *what) {++checks;if(!valid){std::fprintf(stderr,"FAIL %s\n",what);std::abort();}}
struct alignas(8) Buffer {unsigned char bytes[0x18000];unsigned char canary[16];};
static Buffer storage;
static Font_s font{};
static void Reset(int capacity=sizeof(storage.bytes)) {
    std::memset(&storage,0xA5,sizeof(storage));
    auto &list=g_frontEndCmds[0];list.cmds=storage.bytes;list.byteSize=capacity;list.warnSize=capacity;list.usedTotal=list.usedCritical=0;list.lastCmd=nullptr;
    s_cmdList=&list;frontEndDataOut=&s_backEndData[0];frontEndDataOut->viewInfo=g_viewInfo[0];frontEndDataOut->viewInfoCount=0;
    // Construct an isolated valid producer frame; this is not engine readiness.
    rg.inFrame=1;R_InitUI3DStack(R_GetUI3DStack());
}
static void Intact() {for(unsigned char byte:storage.canary)Check(byte==0xA5,"actual capacity canary");}
static void Aligned(const GfxCmdHeader *cmd) {Check(cmd && !(reinterpret_cast<uintptr_t>(cmd)&7),"actual command start eight aligned");Check(cmd->byteCount>=8 && !(cmd->byteCount&7),"published command stride eight aligned");}
static void Walk(const unsigned char *begin,int bytes,int expected) {
    int count=0;const unsigned char *cursor=begin;
    while(cursor<begin+bytes){const auto *header=reinterpret_cast<const GfxCmdHeader*>(cursor);Aligned(header);Check(header->byteCount<=begin+bytes-cursor,"header walk stays inside allocated span");cursor+=header->byteCount;++count;}
    Check(cursor==begin+bytes && count==expected,"byteCount walker reaches exact end/count");
}
static void Rejected(void (*operation)()) {
    const pid_t child=fork();Check(child>=0,"fork rejection owner");
    if(!child){operation();std::_Exit(99);}int status{};Check(waitpid(child,&status,0)==child,"wait rejection");Check(WIFEXITED(status)&&WEXITSTATUS(status)==77,"native guard rejects before publication");
}
static void TooLarge(){Reset();R_GetCommandBuffer(RC_CLEAR_SCREEN,65532);}
static void MisalignedBase(){Reset();s_cmdList->cmds=storage.bytes+4;R_GetCommandBuffer(RC_CLEAR_SCREEN,4);}
static void MisalignedCursor(){Reset();s_cmdList->usedTotal=4;R_GetCommandBuffer(RC_CLEAR_SCREEN,4);}
static void TooLongText(){Reset();std::string text(65528,'x');AddBaseDrawTextCmd(text.data(),1,&font,0,0,1,1,1,0,nullptr,0,0,'|');}
#undef main
int main() {
    Reset();int records=0;
    for(int len=0;len<24;++len){
        R_AddCmdEndOfList();Aligned(s_cmdList->lastCmd);Check(s_cmdList->lastCmd->byteCount==8,"four-byte end marker consumes aligned actual span");++records;
        std::string text(len,'a');auto *cmd=AddBaseDrawTextCmd(text.data(),1,&font,2,3,4,5,6,0,nullptr,3,0,'|');Aligned(&cmd->header);++records;
        Check(cmd->font==&font && cmd->x==2 && cmd->w==4 && cmd->renderFlags==(4|2),"real text producer fields preserved");
        Check(std::strcmp(cmd->text,text.c_str())==0 && offsetof(GfxCmdDrawText2D,text)+len+1<=cmd->header.byteCount,"real variable text/NUL owned by span");
        auto *pointer=reinterpret_cast<GfxCmdStretchPic*>(R_GetCommandBuffer(RC_FIRST_NONCRITICAL,sizeof(GfxCmdStretchPic)));Aligned(&pointer->header);pointer->material=nullptr;pointer->x=71;Check(pointer->x==71,"typed pointer-bearing successor is naturally aligned");++records;
    }
    Walk(storage.bytes,s_cmdList->usedTotal,records);Intact();
    Reset();R_BeginSharedCmdList();Check(frontEndDataOut->cmds==storage.bytes,"shared sublist begins at actual aligned cursor");
    R_GetCommandBuffer(RC_CLEAR_SCREEN,28);R_AddCmdEndOfList();R_BeginCompositingCmdList();Check(!(reinterpret_cast<uintptr_t>(frontEndDataOut->compositingCmds)&7),"sublist after scalar/end marker aligned");
    R_BeginClientCmdList2D();Check(frontEndDataOut->viewInfo[0].cmds==frontEndDataOut->compositingCmds,"client/composite capture identical aligned cursor");Intact();
    const char pool[]="abcdefghijklmno";Reset();records=0;
    for(int count=1;count<=15;++count){auto *cmd=AddBaseDrawConsoleTextCmd(const_cast<char*>(pool),15,13,count,&font,1,2,3,4,nullptr,0);Aligned(&cmd->header);++records;for(int i=0;i<count;++i)Check(cmd->text[i]==pool[(13+i)%15],"actual console ring-copy text preserved");Check(cmd->text[count]==0,"console text NUL owned");}
    Walk(storage.bytes,s_cmdList->usedTotal,records);Intact();
    Reset();auto *largest=R_GetCommandBuffer(RC_CLEAR_SCREEN,65528);Aligned(largest);Check(largest->byteCount==65528 && s_cmdList->usedTotal==65528 && s_cmdList->usedCritical==65528,"maximum aligned ushort span/accounting preserved");Intact();
    Reset(8200);R_GetCommandBuffer(RC_FIRST_NONCRITICAL,4);Check(s_cmdList->usedTotal==8 && s_cmdList->usedCritical==0,"noncritical allocation accounts aligned bytes");Check(!R_GetCommandBuffer(RC_FIRST_NONCRITICAL,4) && s_cmdList->usedTotal==8 && !s_cmdList->lastCmd,"critical reserve rejection leaves cursor intact");Intact();
    Reset(16);R_GetCommandBuffer(RC_CLEAR_SCREEN,4);R_AddCmdEndOfList();Check(s_cmdList->usedTotal==16 && s_cmdList->usedCritical==16,"exact buffer edge includes full end marker span");Intact();
    Rejected(TooLarge);Rejected(MisalignedBase);Rejected(MisalignedCursor);Rejected(TooLongText);
    std::printf("PASS %i actual native render command allocator/text/console/list/walk/alignment/ushort/reserve/capacity/canary checks; no GPU or engine readiness proof\n",checks);return 0;
}
