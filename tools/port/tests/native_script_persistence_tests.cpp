// Include the actual translation unit to exercise its internal serialization
// boundaries without substituting the VM or copying its serialization logic.
#include <clientscript/cscr_save.cpp>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <array>
#include <limits>
#include <pthread.h>

scrVarPub_t gScrVarPub[2]{};
scrCompilePub_t gScrCompilePub[2]{};
static unsigned checks;
static const pthread_t owner=pthread_self();
bool Sys_IsMainThread(){return pthread_equal(owner,pthread_self());}
bool Sys_IsServerThread(){return false;}
static void check(bool ok,const char *name) { if(!ok){fprintf(stderr,"FAIL %s\n",name);exit(1);} ++checks;printf("PASS %s\n",name);fflush(stdout); }
bool Assert_MyHandler(const char *,int,int,const char *format,...) { va_list args;va_start(args,format);vfprintf(stderr,format,args);va_end(args);abort(); }
void Com_Error(errorParm_t,const char *format,...) { va_list args;va_start(args,format);vfprintf(stderr,format,args);va_end(args);fprintf(stderr,"\nEXPECTED_SCRIPT_ERROR\n");_Exit(73); }
void Com_Printf(int,const char *format,...) { va_list args;va_start(args,format);vfprintf(stderr,format,args);va_end(args); }
static bool rejects(void (*action)()) { const pid_t pid=fork();if(!pid){action();_Exit(1);}check(pid>0,"fork isolated invalid-input probe");int status=0;check(waitpid(pid,&status,0)==pid,"join isolated invalid-input probe");return WIFEXITED(status)&&WEXITSTATUS(status)==73; }
static MemoryFile writeFile{};
static ScrSaveContext writer{SCRIPTINSTANCE_SERVER,&writeFile,{},0};
static MemoryFile readFile{};
static ScrSaveContext reader{SCRIPTINSTANCE_SERVER,&readFile,{},0};
static std::array<char,64> program;
static int malformedOffset;
static void saveOutside(){Scr_SaveCodepos(&writer,reinterpret_cast<const char *>(reinterpret_cast<uintptr_t>(program.data())-1));}
static void saveEnd(){Scr_SaveCodepos(&writer,program.data()+program.size());}
static void loadOutside(){readFile={};readFile.buffer=reinterpret_cast<unsigned char *>(&malformedOffset);readFile.bufferSize=4;readFile.errorOnOverflow=true;Scr_LoadCodepos(&reader);}
static void invalidStack(){VariableStackBuffer stack{};stack.size=2;stack.bufLen=22;Scr_CheckSaveStackBounds(&stack);}
int main(){
    gScrVarPub[0].programBuffer=program.data();gScrCompilePub[0].programLen=program.size();
    check(reinterpret_cast<uintptr_t>(program.data())>std::numeric_limits<unsigned int>::max(),"retained bytecode is at genuine native high address");
    Narrow32<const char> handle{};handle=program.data()+37;
    check(static_cast<const char *>(handle)==program.data()+37,"actual native Narrow32 code pointer round-trips");
    Scr_SaveByte(&writer,0x38);Scr_SaveCodepos(&writer,handle);
    check(writeFile.cacheBufferUsed==5 && writeFile.cacheBuffer[0]==0x38 && writeFile.cacheBuffer[1]==37 && !writeFile.cacheBuffer[2] && !writeFile.cacheBuffer[3] && !writeFile.cacheBuffer[4],"actual tagged code position writes unchanged fixed32 offset bytes");
    readFile.buffer=writeFile.cacheBuffer;readFile.bufferSize=writeFile.cacheBufferUsed;readFile.bytesUsed=1;readFile.errorOnOverflow=true;
    check(Scr_LoadCodepos(&reader)==program.data()+37 && readFile.bytesUsed==5,"actual MemFile reader reconstructs native pointer from fixed32 offset");
    writeFile={};Scr_SaveCodepos(&writer,nullptr);
    check(writeFile.cacheBufferUsed==4 && writeFile.cacheBuffer[0]==0xff && writeFile.cacheBuffer[3]==0xff,"null code position preserves minus-one disk sentinel");
    check(rejects(saveOutside)&&rejects(saveEnd),"save rejects code pointers outside actual retained bytecode");
    malformedOffset=64;check(rejects(loadOutside),"load rejects one-past program offset");
    malformedOffset=-2;check(rejects(loadOutside),"load rejects noncanonical negative offset");
    VariableStackBuffer stack{};stack.size=2;stack.bufLen=23;Scr_CheckSaveStackBounds(&stack);
    check(sizeof(VariableUnion)==4 && offsetof(VariableStackBuffer,buf)==13,"actual native VM stack header and five-byte slots remain fixed32");
    check(rejects(invalidStack),"stack traversal rejects size exceeding owned buffer length");
    printf("SCRIPT_PERSISTENCE PASS checks=%u\n",checks);
}
