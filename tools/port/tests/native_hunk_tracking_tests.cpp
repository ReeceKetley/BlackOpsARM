#include <universal/mem_userhunk.h>
#include <qcommon/mem_track.h>
#include <universal/com_errors.h>
#include <platform/android/virtual_memory.h>
#include <platform/android/hunk_services.h>
#include <platform/android/stack_trace.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <cstdint>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include <csignal>
static unsigned checks;static int *fatalMarker;
static void Check(bool ok,const char *why){if(!ok){std::fprintf(stderr,"FAIL: %s\n",why);std::abort();}++checks;}
bool Assert_MyHandler(const char *file,int line,int,const char *format,...){std::fprintf(stderr,"assert %s:%d ",file,line);va_list args;va_start(args,format);std::vfprintf(stderr,format,args);va_end(args);std::abort();}
void Com_Error(errorParm_t,const char *format,...){(void)track_userhunk_snapshot();if(fatalMarker)*fatalMarker=1;va_list args;va_start(args,format);std::vfprintf(stderr,format,args);va_end(args);std::fputc('\n',stderr);} // Intentionally returns and reenters the actual tracker.
void Bo1_Diag(const char *format,...){va_list args;va_start(args,format);std::vfprintf(stderr,format,args);va_end(args);std::fputc('\n',stderr);}
void Bo1_DiagStack(const char *context){char stack[1024];bo1::android::CaptureStackTrace(stack,sizeof(stack),1);std::fprintf(stderr,"%s actual stack:\n%s",context,stack);}
char *va(const char *format,...){thread_local char output[1024];va_list args;va_start(args,format);std::vsnprintf(output,sizeof(output),format,args);va_end(args);return output;}
struct Borrowed{
    unsigned char *map,*base;std::size_t page;int bytes;
    explicit Borrowed(int size=2048):page(bo1::android::VirtualPageSize()),bytes(size){map=static_cast<unsigned char *>(mmap(nullptr,page*3,PROT_NONE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0));Check(map!=MAP_FAILED,"real borrowed guard mapping");Check(!mprotect(map+page,page,PROT_READ|PROT_WRITE),"real caller-owned buffer page");base=map+page+64;Check(size<=static_cast<int>(page-128),"isolated borrowed buffer fits actual page");std::memset(map+page,0xA5,page);}
    ~Borrowed(){Check(!munmap(map,page*3),"actual caller retires borrowed mapping");}
    void Canaries(){for(unsigned i=0;i<64;++i)Check(map[page+i]==0xA5,"borrowed prefix canary");for(unsigned i=64+bytes;i<page;++i)Check(map[page+i]==0xA5,"borrowed suffix canary");}
};
static void Empty(){const auto state=track_userhunk_snapshot();Check(!state.count && !state.bytes,"actual tracked ownership fully retired");for(auto bytes:state.typeBytes)Check(!bytes,"actual per-type bytes retired");}
static void Fault(int which){pid_t child=fork();Check(child>=0,"real hunk failure child");*fatalMarker=0;if(!child){
    Borrowed a(1024),b(1024);auto *owner=Hunk_UserCreateFromBuffer(a.base,a.bytes,HU_SCHEME_DEFAULT,8,nullptr,"borrowed-fault",3);
    if(which==0){Hunk_UserAlloc(owner,768,16,"first");Hunk_UserAlloc(owner,256,16,"fixed-overflow");}
    if(which==1){auto *other=Hunk_UserCreateFromBuffer(b.base,b.bytes,HU_SCHEME_DEFAULT,8,nullptr,"other",3);void *item=Hunk_UserAlloc(other,16,8,"foreign");Hunk_UserFree(owner,item);}
    if(which==2){void *item=Hunk_UserAlloc(owner,16,8,"double");Hunk_UserFree(owner,item);Hunk_UserFree(owner,item);}
    if(which==3){for(unsigned i=0;i<257;++i)Hunk_UserAlloc(owner,1,1,"capacity");}
    _exit(93);
}int status{};Check(waitpid(child,&status,0)==child && WIFSIGNALED(status) && WTERMSIG(status)==SIGABRT && *fatalMarker==1,"genuine guard calls unlocked reentrant fatal and aborts on return");}
int main(){
    alarm(20);bo1::android::RegisterHunkMainThread();const auto reservations=bo1::android::VirtualReservationCount();Empty();
    {
        Borrowed buffer;auto *owner=Hunk_UserCreateFromBuffer(buffer.base,buffer.bytes,HU_SCHEME_DEFAULT,8|HF_FIXEDSIZE,nullptr,"borrowed",3);
        auto *heap=reinterpret_cast<HunkUserDefault *>(owner);Check((owner->flags & 10)==10 && heap->end==reinterpret_cast<psize_int>(buffer.base+buffer.bytes) && heap->locked==heap->end,"actual borrowed/track flags and locked bound");
        char name[]="specific-object";void *first=Hunk_UserAlloc(owner,73,64,name);name[0]='X';void *second=Hunk_UserAlloc(owner,29,16,nullptr);
        NativeUserHunkRecord record{};Check(track_userhunk_find(first,&record) && record.position==first && record.size==73 && record.type==3 && !std::strcmp(record.name,"specific-object"),"native pointer/name/type exact tracking publication");Check(reinterpret_cast<std::uintptr_t>(first)>UINT32_MAX,"actual native position retains high bits");auto totals=track_userhunk_snapshot();Check(totals.count==2 && totals.bytes==102 && totals.typeBytes[3]==102,"actual allocation accounting");
        Hunk_UserFree(owner,second);Check(track_userhunk_snapshot().bytes==73,"actual default logical free retires record without moving arena cursor");Hunk_UserReset(owner);Empty();Check(heap->locked==heap->end && heap->pos>=reinterpret_cast<psize_int>(heap->buf) && heap->pos<=heap->end,"borrowed reset remains within actual buffer");
        first=Hunk_UserAlloc(owner,32,16,"rewind");Hunk_UserSetPos(owner,static_cast<unsigned char *>(first)+10);Empty();Hunk_UserAlloc(owner,16,8,"after-rewind");Hunk_UserDestroy(owner);Empty();Check(bo1::android::VirtualReservationCount()==reservations,"borrowed reset/destroy never owns virtual-memory operations");buffer.Canaries();
    }
    {
        auto *owner=Hunk_UserCreate(8192,HU_SCHEME_DEFAULT,8,nullptr,"chain",4);Hunk_UserAlloc(owner,3000,16,"a");Hunk_UserAlloc(owner,3000,16,"b");Hunk_UserAlloc(owner,6000,16,"c");Check(track_userhunk_snapshot().count==3 && track_userhunk_snapshot().bytes==12000 && bo1::android::VirtualReservationCount()==reservations+2,"actual chained blocks tracked with native range ownership");
        Hunk_UserReset(owner);Empty();Check(bo1::android::VirtualReservationCount()==reservations+1,"actual chain reset retires/releases additional owner");Hunk_UserAlloc(owner,64,32,"reuse");Hunk_UserDestroy(owner);Empty();Check(bo1::android::VirtualReservationCount()==reservations,"actual owned arena destruction releases reservation");
    }
    {
        auto *owner=Hunk_UserCreate(8192,HU_SCHEME_DEBUG,8,nullptr,"debug",5);void *item=Hunk_UserAlloc(owner,63,64,"debug-object");Check(item && track_userhunk_snapshot().bytes==63,"actual nested firstfit debug allocation tracked");Hunk_UserFree(owner,item);Empty();Hunk_UserAlloc(owner,31,16,"debug-reset");Hunk_UserReset(owner);Empty();Hunk_UserAlloc(owner,45,16,"debug-destroy");Hunk_UserDestroy(owner);Empty();Check(bo1::android::VirtualReservationCount()==reservations,"actual debug reservation retired");
    }
    {
        Borrowed buffer;auto *owner=Hunk_UserCreateFromBuffer(buffer.base,buffer.bytes,HU_SCHEME_FIRSTFIT,8,nullptr,"firstfit",6);void *item=Hunk_UserAlloc(owner,55,64,"firstfit-object");Check(item && track_userhunk_snapshot().typeBytes[6]==55,"actual borrowed firstfit tracked");Hunk_UserFree(owner,item);Empty();Hunk_UserAlloc(owner,35,16,"firstfit-reset");Hunk_UserReset(owner);Empty();Hunk_UserAlloc(owner,47,16,"firstfit-destroy");Hunk_UserDestroy(owner);Empty();buffer.Canaries();
    }
    fatalMarker=static_cast<int *>(mmap(nullptr,4096,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0));Check(fatalMarker!=MAP_FAILED,"real shared fatal publication");for(int which=0;which<4;++which)Fault(which);Empty();
    std::printf("PASS: %u actual tracked hunk/pointer/range/lifetime observations\n",checks);
}
