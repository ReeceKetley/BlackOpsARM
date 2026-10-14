#include <platform/android/debug_tcp.h>
#include <platform/win32_compat/win32_time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <thread>
#include <chrono>
#include <vector>
#include <cstdint>
#include <cstdlib>

void NET_InitDebug();
void NET_ShutdownDebug();
void NET_RestartDebug();
int Sys_ReadDebugSocketData(char *, int, int);
void Sys_SendDebugReadBytesInternal();
void Sys_DebugSend(int, const char *, int, const char *);
bool Sys_DebugCanSend();
int Sys_UpdateDebugSocket();
void Sys_AckDebugSocket();
extern unsigned ip_debugSocket[2], ip_debugServerSocket[2];
extern int g_debugWriteBytes, g_debugReadBytesRemote, g_debugReadBytesSent;
extern int g_debugServer, g_debugClient;
// Isolated logging/time providers; actual transport and canonical stream owners run.
void Com_Printf(int, const char *format, ...) {
    va_list args; va_start(args, format); vfprintf(stderr, format, args); va_end(args);
}
bool Assert_MyHandler(const char *file, int line, int, const char *format, ...) {
    fprintf(stderr,"engine assertion %s:%d %s\n",file,line,format); std::abort();
}
char *va(const char *format, ...) {
    static thread_local char output[4096];
    va_list args; va_start(args,format); vsnprintf(output,sizeof(output),format,args); va_end(args);
    return output;
}
namespace bo1::win32 { void Sleep(DWORD ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); } }
static void Check(bool ok, int line) { if (!ok) { fprintf(stderr, "FAIL line %d errno %d\n", line, errno); std::abort(); } }
#define CHECK(x) Check((x), __LINE__)
using namespace bo1::android;
static unsigned Pair(unsigned *listener, unsigned *peer) {
    *listener = OpenDebugTcp("127.0.0.1", 0, false); CHECK(*listener);
    CHECK(ListenDebugTcp(*listener));
    sockaddr_in address{}; socklen_t size=sizeof(address);
    CHECK(!getsockname(static_cast<int>(*listener-1), reinterpret_cast<sockaddr *>(&address), &size));
    *peer = OpenDebugTcp("127.0.0.1", ntohs(address.sin_port), true); CHECK(*peer);
    unsigned accepted=AcceptDebugTcp(*listener); CHECK(accepted);
    const int flags=fcntl(static_cast<int>(accepted-1), F_GETFL);
    CHECK(flags & O_NONBLOCK);
    CHECK(fcntl(static_cast<int>(accepted-1),F_GETFD) & FD_CLOEXEC);
    return accepted;
}
static void ReadExact(unsigned handle, void *buffer, unsigned length) {
    auto *bytes=static_cast<char *>(buffer);
    while(length) {
        const int n=ReadDebugTcp(handle,bytes,length);
        if(n>0) { bytes+=n; length-=n; }
        else { CHECK(n<0 && DebugTcpWouldBlock()); std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
    }
}
int main() {
    alarm(20);
    NET_InitDebug();
    // Descriptor zero must remain representable and owned, then retire on reset.
    close(0);
    unsigned peer[2]{};
    ip_debugSocket[0]=Pair(&ip_debugServerSocket[0], &peer[0]);
    CHECK(ip_debugServerSocket[0] == 1);
    ip_debugSocket[1]=Pair(&ip_debugServerSocket[1], &peer[1]);
    g_debugServer=1;
    char read[9]{};
    CHECK(ReadDebugTcp(ip_debugSocket[0],nullptr,1)==-1 && errno==EINVAL);
    CHECK(WriteDebugTcp(ip_debugSocket[0],nullptr,1)==-1 && errno==EINVAL);
    CHECK(ReadDebugTcp(ip_debugSocket[0],read,0x80000000ULL)==-1 && errno==EMSGSIZE);
    CHECK(WriteDebugTcp(ip_debugSocket[0],read,0x80000000ULL)==-1 && errno==EMSGSIZE);
    CHECK(!Sys_ReadDebugSocketData(read,1,0));
    CHECK(WriteDebugTcp(peer[0],"abc",3)==3);
    std::thread fragment([&]{ std::this_thread::sleep_for(std::chrono::milliseconds(10)); CHECK(WriteDebugTcp(peer[0],"defghi",6)==6); });
    CHECK(Sys_ReadDebugSocketData(read,9,1)); fragment.join();
    CHECK(!memcmp(read,"abcdefghi",9));
    Sys_SendDebugReadBytesInternal();
    std::uint32_t ack=0;
    do { ReadExact(peer[1],&ack,4); } while(ack<9);
    CHECK(ack==9 && g_debugReadBytesSent==9);
    // Force send backpressure and consume the real stream in small pieces.
    int small=1024;
    CHECK(!setsockopt(static_cast<int>(ip_debugSocket[0]-1),SOL_SOCKET,SO_SNDBUF,&small,sizeof(small)));
    std::vector<char> sent(40000), received(sent.size());
    for(unsigned i=0;i<sent.size();++i)sent[i]=static_cast<char>(i);
    std::thread reader([&]{
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        unsigned offset=0;
        while(offset<received.size()) {
            const unsigned length=received.size()-offset < 37 ? received.size()-offset : 37;
            int n=ReadDebugTcp(peer[0],received.data()+offset,length);
            if(n>0)offset+=n; else { CHECK(n<0 && DebugTcpWouldBlock()); std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
        }
    });
    Sys_DebugSend(0,sent.data(),static_cast<int>(sent.size()),"fixture"); reader.join();
    CHECK(sent==received && g_debugWriteBytes==40000);
    // Acknowledgements themselves may be fragmented by TCP.
    g_debugWriteBytes=40960;
    std::uint32_t remote=100;
    std::thread reply([&]{
        CHECK(WriteDebugTcp(peer[1],&remote,1)==1);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        CHECK(WriteDebugTcp(peer[1],reinterpret_cast<char *>(&remote)+1,3)==3);
    });
    Sys_DebugSend(0,"x",1,"fragment ack"); reply.join();
    CHECK(g_debugReadBytesRemote==100);
    ReadExact(peer[0],read,1);
    CHECK(read[0]=='x');
    g_debugWriteBytes=-16; g_debugReadBytesRemote=-32;
    CHECK(Sys_DebugCanSend());
    const unsigned retired=ip_debugSocket[0];
    CloseDebugTcp(peer[0]); peer[0]=0;
    CHECK(!Sys_ReadDebugSocketData(read,1,1));
    CHECK(!ip_debugSocket[0] && !ip_debugSocket[1] && !ip_debugServerSocket[0] && !g_debugServer);
    CHECK(fcntl(static_cast<int>(retired-1),F_GETFD)==-1 && errno==EBADF);
    CloseDebugTcp(peer[1]);
    // Run the canonical acceptance/message lifecycle, including a listener with no peer.
    for(int i=0;i<2;++i) {
        ip_debugServerSocket[i]=OpenDebugTcp("127.0.0.1",0,false);
        CHECK(ip_debugServerSocket[i] && ListenDebugTcp(ip_debugServerSocket[i]));
    }
    CHECK(!Sys_UpdateDebugSocket() && !ip_debugSocket[0]);
    for(int i=0;i<2;++i) {
        sockaddr_in endpoint{}; socklen_t bytes=sizeof(endpoint);
        CHECK(!getsockname(static_cast<int>(ip_debugServerSocket[i]-1),reinterpret_cast<sockaddr *>(&endpoint),&bytes));
        peer[i]=OpenDebugTcp("127.0.0.1",ntohs(endpoint.sin_port),true); CHECK(peer[i]);
    }
    CHECK(WriteDebugTcp(peer[0],"\x07",1)==1);
    int type=0;
    while(!(type=Sys_UpdateDebugSocket())) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    CHECK(type==7 && ip_debugSocket[0] && ip_debugSocket[1]);
    CHECK(Sys_UpdateDebugSocket()==7);
    Sys_AckDebugSocket(); CHECK(!Sys_UpdateDebugSocket());
    NET_ShutdownDebug(); CloseDebugTcp(peer[0]); CloseDebugTcp(peer[1]);
    // A refused nonblocking connect must fail SO_ERROR validation, not report readiness.
    unsigned bound=OpenDebugTcp("127.0.0.1",0,false); CHECK(bound);
    sockaddr_in address{}; socklen_t size=sizeof(address);
    CHECK(!getsockname(static_cast<int>(bound-1),reinterpret_cast<sockaddr *>(&address),&size));
    CHECK(!OpenDebugTcp("127.0.0.1",ntohs(address.sin_port),true) && errno==ECONNREFUSED);
    CloseDebugTcp(bound);
    NET_RestartDebug(); NET_ShutdownDebug();
    CHECK(!g_debugClient && !g_debugServer && !g_debugWriteBytes && !g_debugReadBytesRemote);
    puts("PASS native canonical debug TCP: fd0, nonblocking/exec flags, fragmented read/ack, backpressure, wrap, EOF restart, canonical accept/message/ack, refused connect, shutdown");
}
