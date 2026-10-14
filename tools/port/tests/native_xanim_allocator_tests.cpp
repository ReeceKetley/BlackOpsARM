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
#include <initializer_list>
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
static void Verify(XAnim_s *anims,unsigned count,unsigned values,bool developer,fileData_s *prior){
    Check(anims && Hunk_DataOnHunk(reinterpret_cast<unsigned char *>(anims)),"genuine animation storage belongs to actual owned hunk");Check(reinterpret_cast<std::uintptr_t>(anims)%alignof(XAnim_s)==0,"native animation alignment");Check(anims->size==count && anims->paramSize==values,"actual typed factory counts");
    if(values){auto *expected=reinterpret_cast<XAnimParam *>(reinterpret_cast<unsigned char *>(anims)+offsetof(XAnim_s,entries)+count*sizeof(XAnimEntry));Check(anims->params==expected && reinterpret_cast<std::uintptr_t>(anims->params)%alignof(XAnimParam)==0,"actual native parameter tail position/alignment");for(unsigned i=0;i<values;++i)Check(anims->params[i].fValue==0,"actual parameter storage cleared");anims->params[values-1].fValue=3.5f;}
    Check(anims->wasLoggedIfMissing && !anims->wasLoggedIfMissing[count-1],"actual missing-name metadata allocated in both modes");
    if(developer){Check(anims->debugName && !std::strcmp(anims->debugName,"actual-fixture"),"real debug name owner/copy");Check(anims->debugAnimNames && reinterpret_cast<std::uintptr_t>(anims->debugAnimNames)%alignof(const char *)==0,"actual pointer-array native alignment");for(unsigned i=0;i<count;++i)Check(!anims->debugAnimNames[i],"actual debug-name pointer rows cleared");}
    else Check(!anims->debugName && !anims->debugAnimNames,"actual developer-off metadata contract");
    Check(com_hunkData && com_hunkData->next==prior && com_hunkData->type==2 && com_hunkData->data==anims,"genuine typed Hunk_AddData publishes actual registry record");Check(Hunk_DataOnHunk(reinterpret_cast<unsigned char *>(com_hunkData)) && reinterpret_cast<std::uintptr_t>(com_hunkData)%alignof(fileData_s)==0,"real native anonymous registry object owner/alignment");
}
static void Fault(unsigned which){*fatalMarker=0;pid_t child=fork();Check(child>=0,"real invalid-count child");if(!child){if(which==0)XAnimCreateAnims("bad",-1,Hunk_AllocXAnimPrecache);if(which==1)XAnimCreateAnimsWithValues("bad",3,UINT_MAX,Hunk_AllocXAnimPrecacheSigned);if(which==2)Hunk_AllocXAnimPrecacheSigned(-1);if(which==3){alignas(HunkUserDefault) unsigned char owned[128];g_DebugHunkUser=Hunk_UserCreateFromBuffer(owned,sizeof(owned),HU_SCHEME_FIRSTFIT,0,nullptr,"small-real-debug-provider",0);g_anim_developer=true;XAnimCreateAnims("actual-fixture",5,Hunk_AllocXAnimPrecache);}_exit(94);}int status{};Check(waitpid(child,&status,0)==child && WIFSIGNALED(status) && WTERMSIG(status)==SIGABRT && *fatalMarker,"real signed/count guards abort after returning reporter");}
static int TreeCase(){
 alarm(20);Sys_InitMainThread();const auto reservations=bo1::android::VirtualReservationCount();s_hunkTotal=0x10000;s_hunkData=static_cast<unsigned char *>(Z_VirtualReserve(s_hunkTotal));hunk_low={};hunk_high={};
 XAnim_s owner{};owner.size=1;auto *signedTree=XAnimCreateTree(&owner,Hunk_AllocXAnimPrecacheSigned);auto *unsignedTree=XAnimCreateTree(&owner,Hunk_AllocXAnimPrecache);
 Check(XAnimTreeSize()==static_cast<int>(sizeof(XAnimTree_s))&&sizeof(XAnimTree_s)==16,"actual native tree allocation width");
 for(auto *tree:{signedTree,unsignedTree}){Check(Hunk_DataOnHunk(reinterpret_cast<unsigned char *>(tree))&&reinterpret_cast<uintptr_t>(tree)%alignof(XAnimTree_s)==0,"actual signed/unsigned provider tree ownership and alignment");Check(tree->anims==&owner&&!tree->children&&!tree->inst,"actual typed tree owner publication and cleared control fields");const auto *bytes=reinterpret_cast<const unsigned char *>(tree);for(std::size_t i=sizeof(tree->anims);i<sizeof(*tree);++i)Check(!bytes[i],"actual tree clears full native tail/padding");}
 Check(signedTree!=unsignedTree,"actual allocator owns separate live trees");
 fatalMarker=static_cast<int *>(mmap(nullptr,4096,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0));Check(fatalMarker!=MAP_FAILED,"actual tree failure marker");*fatalMarker=0;pid_t child=fork();Check(child>=0,"actual invalid tree callback child");if(!child){XAnimCreateTree(&owner,static_cast<void *(*)(int)>(nullptr));_exit(91);}int status{};Check(waitpid(child,&status,0)==child&&WIFSIGNALED(status)&&WTERMSIG(status)==SIGABRT&&*fatalMarker,"actual tree callback failure aborts after returning reporter");
 munmap(fatalMarker,4096);fatalMarker=nullptr;Z_VirtualFree(s_hunkData,11);s_hunkData=nullptr;s_hunkTotal=0;Check(bo1::android::VirtualReservationCount()==reservations,"actual tree arena retired");std::printf("PASS: %u actual typed native tree owner/allocator/failure observations\n",checks);return 0;
}
int main(int argc,char **argv){
 if(argc==2&&!std::strcmp(argv[1],"tree"))return TreeCase();
    alarm(20);Sys_InitMainThread();const auto reservations=bo1::android::VirtualReservationCount();
    // An isolated real OS-backed canonical hunk arena, not full-engine init.
    s_hunkTotal=0x100000;s_hunkData=static_cast<unsigned char *>(Z_VirtualReserve(s_hunkTotal));hunk_low={};hunk_high={};com_hunkData=nullptr;
    g_DebugHunkUser=Hunk_UserCreate(0x20000,HU_SCHEME_DEBUG,0,nullptr,"actual-fixture-debug",0);auto *debug=reinterpret_cast<FirstFitHunkUser *>(reinterpret_cast<HunkUserDebug *>(g_DebugHunkUser)->firstFit);const int baseline=debug->used;
    g_anim_developer=true;auto *first=XAnimCreateAnims("actual-fixture",3,Hunk_AllocXAnimPrecache);Verify(first,3,0,true,nullptr);
    fileData_s *previous=com_hunkData;auto *second=XAnimCreateAnimsWithValues("actual-fixture",5,1,Hunk_AllocXAnimPrecacheSigned);Verify(second,5,1,true,previous);
    previous=com_hunkData;g_anim_developer=false;auto *third=XAnimCreateAnimsWithValues("actual-fixture",2,3,Hunk_AllocXAnimPrecache);Verify(third,2,3,false,previous);
    Check(debug->used>baseline,"actual debug allocator owns metadata");Hunk_ClearDataFor(&com_hunkData,s_hunkData,s_hunkData+s_hunkTotal);Check(!com_hunkData && debug->used==baseline,"actual registry clear executes genuine free owner and retires metadata in both developer modes");Check(!first->debugName && !first->debugAnimNames && !first->wasLoggedIfMissing && !third->wasLoggedIfMissing,"actual animation metadata pointers retired");
    fatalMarker=static_cast<int *>(mmap(nullptr,4096,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0));Check(fatalMarker!=MAP_FAILED,"actual shared fatal marker");for(unsigned which=0;which<4;++which)Fault(which);
    Hunk_UserDestroy(g_DebugHunkUser);g_DebugHunkUser=nullptr;Z_VirtualFree(s_hunkData,11);s_hunkData=nullptr;s_hunkTotal=0;Check(bo1::android::VirtualReservationCount()==reservations,"actual fixture reservations retired");
    std::printf("PASS: %u actual typed XAnim allocator/registry/metadata observations\n",checks);
}

