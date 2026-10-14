#include <win32/win_net.h>
#include <platform/android/udp_socket.h>
#include <platform/android/thread_services.h>
#include <platform/critical_section_services.h>
#include <clientscript/cscr_stringlist.h>
#include <qcommon/msg_mp.h>
#include <qcommon/common.h>
#include <stringed/stringed_hooks.h>
#include <universal/dvar.h>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <csignal>

namespace {
unsigned checks;std::atomic<unsigned long long> entered{},exited{};
dvar_s *enabled,*count;int peer;netadr_t target{};
void Check(bool ok,const char *why){if(!ok){std::fprintf(stderr,"FAIL: %s\n",why);_exit(91);}++checks;}
void Fault(int mode){const pid_t child=fork();Check(child>=0,"actual pool policy fault child");if(!child){
    Dvar_SetBool(enabled,true);Dvar_SetInt(count,mode==0?0:50);
    if(mode==2)count->current.integer=51; // Deliberate malformed actual Dvar state, no weakened registration domain.
    if(mode==0)NET_SocketPool_Init();
    if(mode==1)Sys_CheckForNATOverflow();
    if(mode==2)Sys_SocketPool_GetPacket(nullptr,nullptr);
    if(mode==3){unsigned char data[]={1,2,3};Sys_SendPacket(sizeof(data),data,target);}
    _exit(92);
}int status{};Check(waitpid(child,&status,0)==child && WIFSIGNALED(status) && WTERMSIG(status)==SIGABRT,"enabled/invalid pool request aborts after deliberately returning fatal reporter");}
}
extern "C" void __cyg_profile_func_enter(void*,void*){entered.fetch_add(1,std::memory_order_relaxed);}
extern "C" void __cyg_profile_func_exit(void*,void*){exited.fetch_add(1,std::memory_order_relaxed);}
void Com_Error(errorParm_t,const char *format,...){va_list args;va_start(args,format);std::vfprintf(stderr,format,args);va_end(args);std::fputc('\n',stderr);}
void Com_Printf(int,const char *format,...){va_list args;va_start(args,format);std::vfprintf(stderr,format,args);va_end(args);}
void Com_PrintWarning(int,const char *format,...){va_list args;va_start(args,format);std::vfprintf(stderr,format,args);va_end(args);}
void Com_PrintError(int,const char *format,...){va_list args;va_start(args,format);std::vfprintf(stderr,format,args);va_end(args);}
void Com_DPrintf(int,const char *format,...){va_list args;va_start(args,format);std::vfprintf(stderr,format,args);va_end(args);}
// Explicit isolated providers for unexecuted logging/localization/script failure paths.
const dvar_t *com_logfile;
void Com_PrintMessage(int,char *message,int){std::fputs(message,stderr);}
bool Com_LogFileOpen(){return false;}
[[noreturn]] static void Unexecuted(const char *name){std::fprintf(stderr,"UNEXECUTED: %s\n",name);std::abort();}
bool Assert_MyHandler(const char *file,int line,int,const char *format,...){std::fprintf(stderr,"assert %s:%d ",file,line);va_list args;va_start(args,format);std::vfprintf(stderr,format,args);va_end(args);std::abort();}
char *SEH_LocalizeTextMessage(const char*,const char*,msgLocErrType_t){Unexecuted("localization");}
void Scr_DumpScriptThreads(scriptInstance_t){Unexecuted("script dump");}
void Scr_DumpScriptVariablesDefault(scriptInstance_t){Unexecuted("script variable dump");}
void Scr_TerminalError(const char*,scriptInstance_t){Unexecuted("script terminal error");}
int main(){
    alarm(15);Sys_InitMainThread();Sys_InitializeCriticalSections();SL_Init(SCRIPTINSTANCE_SERVER);SL_Init(SCRIPTINSTANCE_CLIENT);Dvar_Init();
    enabled=const_cast<dvar_s *>(_Dvar_RegisterBool("cl_socketpool_enabled",false,0,"actual fixture pool policy"));
    count=const_cast<dvar_s *>(_Dvar_RegisterInt("cl_socketpool_size",10,0,50,0,"actual canonical pool bounds"));
    peer=socket(AF_INET,SOCK_DGRAM|SOCK_NONBLOCK|SOCK_CLOEXEC,0);Check(peer>0,"real peer before fd0 retirement");
    sockaddr_in address{};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    Check(!bind(peer,reinterpret_cast<sockaddr *>(&address),sizeof(address)),"native sockaddr real bind");socklen_t bytes=sizeof(address);
    Check(!getsockname(peer,reinterpret_cast<sockaddr *>(&address),&bytes) && bytes==sizeof(sockaddr_in),"actual native sockaddr result size");
    target.type=NA_IP;target.ip[0]=127;target.ip[3]=1;target.port=address.sin_port;
    close(0);Check(bo1::android::OpenUdp("127.0.0.1",0),"real native UDP opens after fd0 retirement");
    int kind{};bytes=sizeof(kind);Check(!getsockopt(0,SOL_SOCKET,SO_TYPE,&kind,&bytes) && kind==SOCK_DGRAM,"real fd0 is owned UDP socket");
    struct Storage {unsigned char data[32],canary[16];} storage;std::memset(&storage,0xA5,sizeof(storage));
    netadr_t from{};from.type=NA_LOOPBACK;const auto originalFrom=from;msg_t message{};message.data=storage.data;message.maxsize=32;message.cursize=7;message.readcount=4;const auto originalMessage=message;
    NET_SocketPool_Init();Check(!Sys_SocketPool_GetPacket(&from,&message),"disabled actual optional pool returns no owned packet");Sys_CheckForNATOverflow();
    Check(!std::memcmp(&from,&originalFrom,sizeof(from)) && !std::memcmp(&message,&originalMessage,sizeof(message)),"disabled pool preserves output metadata/canaries");
    for(auto byte:storage.canary)Check(byte==0xA5,"disabled pool storage canary");
    const unsigned char packet[]={0,1,255,7};Check(Sys_SendPacket(sizeof(packet),const_cast<unsigned char *>(packet),target)==1,"disabled pool uses genuine regular UDP send");
    pollfd ready{peer,POLLIN,0};Check(poll(&ready,1,1000)==1,"peer receives actual UDP");unsigned char incoming[8];sockaddr_in source{};bytes=sizeof(source);
    Check(recvfrom(peer,incoming,sizeof(incoming),0,reinterpret_cast<sockaddr *>(&source),&bytes)==4 && bytes==sizeof(source) && !std::memcmp(incoming,packet,4),"actual wire bytes/native source sockaddr");
    unsigned char large[40]{};Check(sendto(peer,large,sizeof(large),0,reinterpret_cast<sockaddr *>(&source),sizeof(source))==40,"actual truncated datagram send");
    ready={0,POLLIN,0};Check(poll(&ready,1,1000)==1 && !Sys_GetPacket(&from,&message) && errno==EMSGSIZE,"actual fd0 UDP truncation rejected");
    Check(!std::memcmp(&from,&originalFrom,sizeof(from)) && message.cursize==7 && message.readcount==4,"truncation never commits packet metadata");
    for(auto byte:storage.canary)Check(byte==0xA5,"actual truncated receive storage canary");
    for(int mode=0;mode<4;++mode)Fault(mode);
    Check(fcntl(0,F_GETFD)>=0 && bo1::android::BoundUdpPort()!=0,"pool faults never close parent's regular fd0 owner");
    ready={peer,POLLIN,0};Check(poll(&ready,1,0)==0,"enabled pool send failed before any real wire output");
    Check(bo1::android::CloseUdp(),"actual regular UDP owner retirement");close(peer);
    std::printf("PASS: %u actual disabled-pool/unsupported-enabled/UDP-boundary checks\n",checks);
}
