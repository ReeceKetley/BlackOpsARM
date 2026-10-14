#include <clientscript/cscr_variable.h>
#include <platform/bo1_bytecode_io.h>
#include <xanim/xanim.h>
#include <universal/com_memory.h>
#include <universal/mem_userhunk.h>
#include <universal/mem_firstfit.h>
#include <platform/android/thread_services.h>
#include <platform/android/virtual_memory.h>
#include <qcommon/common.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <cstdint>
#include <climits>
#include <atomic>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include <csignal>
extern unsigned char *s_hunkData;extern int s_hunkTotal;extern hunkUsed_t hunk_low,hunk_high;extern fileData_s *com_hunkData;extern bool g_anim_developer;
static unsigned checks;static int *fatalMarker;
static std::atomic<unsigned long long> entered{},exited{};
extern "C" void __cyg_profile_func_enter(void*,void*){entered.fetch_add(1,std::memory_order_relaxed);}
extern "C" void __cyg_profile_func_exit(void*,void*){exited.fetch_add(1,std::memory_order_relaxed);}
void Scr_TerminalError(const char*,scriptInstance_t){std::fputs("UNEXECUTED script terminal error\n",stderr);std::abort();}
static void Check(bool ok,const char *why){if(!ok){std::fprintf(stderr,"FAIL: %s\n",why);std::abort();}++checks;}
void Com_Error(errorParm_t,const char *format,...){if(fatalMarker)*fatalMarker=1;va_list args;va_start(args,format);std::vfprintf(stderr,format,args);va_end(args);std::fputc('\n',stderr);} // Deliberately returning fatal reporter.
bool Assert_MyHandler(const char *file,int line,int,const char *format,...){std::fprintf(stderr,"assert %s:%d ",file,line);va_list args;va_start(args,format);std::vfprintf(stderr,format,args);va_end(args);std::abort();}
void Com_Printf(int,const char *format,...){va_list args;va_start(args,format);std::vfprintf(stderr,format,args);va_end(args);}
void Com_PrintWarning(int,const char *format,...){va_list args;va_start(args,format);std::vfprintf(stderr,format,args);va_end(args);}
void Com_PrintError(int,const char *format,...){va_list args;va_start(args,format);std::vfprintf(stderr,format,args);va_end(args);}
void Com_DPrintf(int,const char *format,...){va_list args;va_start(args,format);std::vfprintf(stderr,format,args);va_end(args);}
void Bo1_Diag(const char *format,...){va_list args;va_start(args,format);std::vfprintf(stderr,format,args);va_end(args);}
void Bo1_DiagStack(const char*){std::fputs("UNEXECUTED hunk OOM stack path\n",stderr);std::abort();}

int main(){
 struct alignas(8) OwnedRecords {unsigned char before[8];unsigned char bytes[24];unsigned char after[8];} owned;
 std::memset(&owned,0xa5,sizeof(owned));
 const unsigned char records[]={'a','a',0,0x27,0x93,6,'b','b','b','b',0,0xff,0xff,5,'c',0,0x34,0x12,2,0};
 std::memcpy(owned.bytes,records,sizeof(records));gScrVarPub[0].fieldBuffer=reinterpret_cast<char *>(owned.bytes);
 int type=77;Check(Scr_FindField("AA",&type,SCRIPTINSTANCE_SERVER)==0x9327&&type==6,"actual odd field-index load and canonical casefold");
 type=77;Check(Scr_FindField("bbbb",&type,SCRIPTINSTANCE_SERVER)==65535&&type==5,"actual second odd index and variable name length walk");
 type=77;Check(Scr_FindField("c",&type,SCRIPTINSTANCE_SERVER)==0x1234&&type==2,"actual third record type/index");
 type=77;Check(!Scr_FindField("absent",&type,SCRIPTINSTANCE_SERVER)&&type==77,"actual terminal marker and absent type unchanged");
 bo1::bytecode::Store<uint16_t>(owned.bytes+3,static_cast<uint16_t>(0xa187));Check(owned.bytes[3]==0x87&&owned.bytes[4]==0xa1&&owned.bytes[2]==0&&owned.bytes[5]==6,"actual producer scalar helper exact2 little endian bytes with name/type neighbors");Check(Scr_FindField("aa",&type,SCRIPTINSTANCE_SERVER)==0xa187&&type==6,"actual consumer sees exact updated producer width");
 Check(!std::memcmp(owned.bytes+6,records+6,sizeof(records)-6),"other actual packed records/end marker unchanged");for(unsigned i=0;i<8;++i)Check(owned.before[i]==0xa5&&owned.after[i]==0xa5,"field record owner canaries");
 gScrVarPub[1].fieldBuffer=reinterpret_cast<char *>(owned.bytes+19);type=99;Check(!Scr_FindField("empty",&type,SCRIPTINSTANCE_CLIENT)&&type==99,"actual empty client buffer");
 gScrVarPub[0].fieldBuffer=nullptr;gScrVarPub[1].fieldBuffer=nullptr;std::printf("PASS: %u actual packed field consumer/scalar/canary observations\n",checks);
}
