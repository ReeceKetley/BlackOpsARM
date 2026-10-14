#include "oom_error.h"
#include <atomic>
#include <cstdint>
#include <cstddef>
#include <cerrno>
#include <cstdlib>
#include <fcntl.h>
#include <signal.h>
#include <sys/sysinfo.h>
#include <unistd.h>

bool Sys_IsHeadless();
bool Monkey_IsRunning();
namespace {
constinit std::atomic_flag reporting=ATOMIC_FLAG_INIT;
struct Text {
    char bytes[2048]{};std::size_t used{};
    void Char(char c){if(used<sizeof(bytes))bytes[used++]=c;}
    void String(const char *s,std::size_t maximum=512){if(!s){String("<unknown>");return;}std::size_t i=0;for(;i<maximum && s[i];++i)Char(s[i]);if(i==maximum)String("...");}
    void Number(std::uint64_t n){char digits[20];unsigned count=0;do{digits[count++]=static_cast<char>('0'+n%10);n/=10;}while(n);while(count)Char(digits[--count]);}
    void Signed(std::int64_t n){if(n<0){Char('-');Number(static_cast<std::uint64_t>(-(n+1))+1);}else Number(n);}
};
void EmergencyWrite(const char *data,std::size_t size){
    // No stdio/file/engine lock. A broken stderr pipe must not replace the OOM
    // exit code with SIGPIPE, and backpressure must not park the fatal thread.
    sigset_t blocked;sigemptyset(&blocked);sigaddset(&blocked,SIGPIPE);
    if(sigprocmask(SIG_BLOCK,&blocked,nullptr))return;
    const int flags=fcntl(STDERR_FILENO,F_GETFL);
    if(flags<0 || (!(flags&O_NONBLOCK) && fcntl(STDERR_FILENO,F_SETFL,flags|O_NONBLOCK)))return;
    for(unsigned attempts=0;size && attempts<8;++attempts){
        const auto wrote=write(STDERR_FILENO,data,size);
        if(wrote<0 && errno==EINTR)continue;
        if(wrote<=0)return;data+=wrote;size-=static_cast<std::size_t>(wrote);
    }
}
bool ReadProc(const char *path,char *output,std::size_t capacity,std::size_t &length){
    length=0;const int file=open(path,O_RDONLY|O_CLOEXEC|O_NONBLOCK);if(file<0)return false;
    bool good=true;
    for(unsigned attempts=0;length<capacity && attempts<16;++attempts){
        const auto bytes=read(file,output+length,capacity-length);
        if(bytes<0 && errno==EINTR)continue;
        if(bytes<0){good=false;break;}if(!bytes)break;length+=static_cast<std::size_t>(bytes);
    }
    close(file);return good && length>0;
}
bool KiBField(const char *input,std::size_t length,const char *key,std::size_t keyLength,std::uint64_t &value){
    for(std::size_t start=0;start<length;){
        std::size_t end=start;while(end<length && input[end]!='\n')++end;
        bool match=end-start>=keyLength;for(std::size_t i=0;match && i<keyLength;++i)match=input[start+i]==key[i];
        if(match){std::size_t p=start+keyLength;while(p<end && (input[p]==' ' || input[p]=='\t'))++p;
            if(p==end || input[p]<'0' || input[p]>'9')return false;
            value=0;while(p<end && input[p]>='0' && input[p]<='9'){const unsigned digit=input[p++]-'0';if(value>(UINT64_MAX-digit)/10)return false;value=value*10+digit;}
            while(p<end && (input[p]==' ' || input[p]=='\t'))++p;
            if(end-p<2 || input[p]!='k' || input[p+1]!='B')return false;p+=2;
            while(p<end && (input[p]==' ' || input[p]=='\t' || input[p]=='\r'))++p;
            return p==end;
        }
        start=end+1;
    }
    return false;
}
void Metric(Text &out,const char *label,bool known,std::uint64_t bytes){out.String(label);if(known)out.Number(bytes);else out.String("unavailable");out.Char('\n');}
bool Scale(std::uint64_t count,unsigned unit,std::uint64_t &bytes){if(!unit || count>UINT64_MAX/unit)return false;bytes=count*unit;return true;}
struct AllocationContext {const char *operation,*name;std::int64_t requested,deficit;bool deficitKnown;};
[[noreturn]] void Report(const char *filename,int line,const AllocationContext *context){
    // Both queries are existing nonallocating reads: immutable native headless
    // policy and the actual atomic Monkey running flag. Never call Monkey_Send,
    // Com_Error/Sys_Error, UI, suspension or cleanup under allocation failure.
    const int exitCode=Monkey_IsRunning()?255:(Sys_IsHeadless()?3:255);
    if(reporting.test_and_set(std::memory_order_acq_rel)){
        constexpr char recursive[]="Native OOM: recursive/concurrent allocation failure; immediate terminal exit; dirty marker retained\n";
        EmergencyWrite(recursive,sizeof(recursive)-1);std::_Exit(exitCode);
    }
    Text out;out.String("Native OOM: filename='");out.String(filename,384);out.String("' line=");out.Signed(line);
    if(context){out.String(" operation=");out.String(context->operation,128);out.String(" allocation='");out.String(context->name,128);out.String("' requestedBytes=");out.Signed(context->requested);out.String(" deficitBytes=");if(context->deficitKnown)out.Signed(context->deficit);else out.String("unavailable");}
    out.String(" pid=");out.Number(getpid());out.String(" tid=");out.Number(gettid());out.String("\n");
    struct sysinfo info{};const bool infoKnown=sysinfo(&info)==0;std::uint64_t total=0,free=0;
    const bool totalKnown=infoKnown && Scale(info.totalram,info.mem_unit,total);
    const bool freeKnown=infoKnown && Scale(info.freeram,info.mem_unit,free);
    Metric(out,"OS physical total bytes=",totalKnown,total);
    Metric(out,"OS physical free bytes=",freeKnown,free);
    char proc[8192];std::size_t length{};std::uint64_t available=0,rss=0,virtualSize=0;
    const bool memoryRead=ReadProc("/proc/meminfo",proc,sizeof(proc),length);
    const bool availableKnown=memoryRead && KiBField(proc,length,"MemAvailable:",13,available);
    Metric(out,"OS MemAvailable KiB=",availableKnown,available);
    const bool processRead=ReadProc("/proc/self/status",proc,sizeof(proc),length);
    const bool rssKnown=processRead && KiBField(proc,length,"VmRSS:",6,rss);
    const bool virtualKnown=processRead && KiBField(proc,length,"VmSize:",7,virtualSize);
    Metric(out,"Process VmRSS KiB=",rssKnown,rss);Metric(out,"Process VmSize KiB=",virtualKnown,virtualSize);
    out.String("Native OOM is terminal: whole-engine cleanup incomplete; dirty process marker retained\n");
    EmergencyWrite(out.bytes,out.used);std::_Exit(exitCode);
}

}
void Sys_OutOfMemErrorInternal(const char *filename,int line){Report(filename,line,nullptr);}
[[noreturn]] void bo1::android::ReportAllocationFailure(const char *filename,int line,const char *operation,const char *name,std::int64_t requested,std::int64_t deficit,bool known){
    const AllocationContext context{operation,name,requested,deficit,known};Report(filename,line,&context);
}
