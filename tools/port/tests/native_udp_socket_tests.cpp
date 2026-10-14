#include <platform/android/udp_socket.h>
#include <qcommon/msg_mp.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <cstdio>
#include <cstring>
#include <cerrno>
int Sys_GetPacket(netadr_t *, msg_t *);

int main(int argc, char **) {
    if (argc > 1) {
        errno=0;
        const int flags=::fcntl(0,F_GETFD);
        const int descriptorError=errno;
        int kind=0; socklen_t bytes=sizeof(kind);
        errno=0;
        const int socketResult=::getsockopt(0,SOL_SOCKET,SO_TYPE,&kind,&bytes);
        const int socketError=errno;
        char target[128]{};
        const auto targetLength=::readlink("/proc/self/fd/0",target,sizeof(target)-1);
        if(targetLength>=0) target[targetLength]=0;
        std::fprintf(stderr,"post-exec FD0 flags=%d fd-error=%d socket-result=%d socket-error=%d target=%s\n",
            flags,descriptorError,socketResult,socketError,target);
        if(flags == -1 && descriptorError == EBADF) return 0;
        return flags >= 0 && socketResult == -1 && socketError == ENOTSOCK &&
            std::strcmp(target,"/dev/null")==0 ? 0 : 90;
    }
    const int peer = ::socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0);
    if (peer < 0) return 1;
    sockaddr_in local{}; local.sin_family=AF_INET; local.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    if (::bind(peer,reinterpret_cast<sockaddr *>(&local),sizeof(local))) return 2;
    socklen_t size=sizeof(local);
    if (::getsockname(peer,reinterpret_cast<sockaddr *>(&local),&size)) return 3;
    netadr_t destination{}; destination.type=NA_IP;
    destination.ip[0]=127; destination.ip[3]=1; destination.port=local.sin_port;
    const unsigned char packet[]={0,1,255,7};
    if (bo1::android::SendUdp(packet,sizeof(packet),destination) || errno != EBADF) return 4;
    if (bo1::android::OpenUdp("127.0.0.1",ntohs(local.sin_port)) || errno != EADDRINUSE) return 5;
    if (!bo1::android::OpenUdp("127.0.0.1",0)) return 6;
    const auto ownedPort=bo1::android::BoundUdpPort();
    if (!ownedPort || bo1::android::OpenUdp("127.0.0.1",0) || errno != EALREADY ||
        bo1::android::BoundUdpPort() != ownedPort) return 7;
    struct { unsigned char data[32]; unsigned char sentinel[4]; } incoming{};
    std::memset(incoming.sentinel,0xA5,4);
    netadr_t source{}; source.type=NA_BOT; source.addrHandleIndex=71;
    const auto originalSource=source;
    std::size_t length=123;
    msg_t engineMessage{};
    engineMessage.data=incoming.data;
    engineMessage.maxsize=32;
    engineMessage.readcount=99;
    engineMessage.cursize=171;
    if(Sys_GetPacket(&source,&engineMessage) != 0 || engineMessage.readcount != 99 ||
        engineMessage.cursize != 171 || std::memcmp(&source,&originalSource,sizeof(source))) return 20;
    if (bo1::android::ReceiveUdp(incoming.data,32,&source,&length) != 0 || length != 123 ||
        std::memcmp(&source,&originalSource,sizeof(source))) return 8;
    if (!bo1::android::SendUdp(packet,sizeof(packet),destination)) return 9;
    pollfd ready{peer,POLLIN,0};
    if (::poll(&ready,1,1000) != 1) return 10;
    sockaddr_in sender{}; size=sizeof(sender);
    const auto read=::recvfrom(peer,incoming.data,32,0,reinterpret_cast<sockaddr *>(&sender),&size);
    if (read != 4 || std::memcmp(incoming.data,packet,4) || ntohs(sender.sin_port) != ownedPort) return 11;
    auto receive=[&](std::size_t capacity) {
        for(int i=0;i<100;++i) {
            const int result=bo1::android::ReceiveUdp(incoming.data,capacity,&source,&length);
            if(result) return result;
            ::usleep(1000);
        }
        return 0;
    };
    if (::sendto(peer,packet,4,0,reinterpret_cast<sockaddr *>(&sender),sizeof(sender)) != 4 || receive(32) != 1 ||
        length != 4 || std::memcmp(incoming.data,packet,4) || source.port != local.sin_port || source.ip[0] != 127) return 12;
    source=originalSource; length=123;
    if (::sendto(peer,packet,4,0,reinterpret_cast<sockaddr *>(&sender),sizeof(sender)) != 4 || receive(4) != -1 ||
        errno != EMSGSIZE || length != 123 || std::memcmp(&source,&originalSource,sizeof(source))) return 13;
    const unsigned char bigger[40]={1};
    if (::sendto(peer,bigger,sizeof(bigger),0,reinterpret_cast<sockaddr *>(&sender),sizeof(sender)) != 40 ||
        receive(32) != -1 || errno != EMSGSIZE || length != 123 || std::memcmp(&source,&originalSource,sizeof(source))) return 14;
    for(auto byte: incoming.sentinel) if(byte != 0xA5) return 15;
    if (::sendto(peer,packet,4,0,reinterpret_cast<sockaddr *>(&sender),sizeof(sender)) != 4) return 21;
    int engineReceived=0;
    for(int i=0;i<100 && !engineReceived;++i) {
        engineReceived=Sys_GetPacket(&source,&engineMessage);
        if(!engineReceived) ::usleep(1000);
    }
    if(engineReceived != 1 || engineMessage.cursize != 4 || engineMessage.readcount != 0 ||
        std::memcmp(engineMessage.data,packet,4) || source.port != local.sin_port) return 22;
    if (!bo1::android::CloseUdp() || !bo1::android::CloseUdp() || bo1::android::BoundUdpPort() ||
        bo1::android::SendUdp(packet,4,destination) || errno != EBADF) return 16;
    const pid_t child=::fork();
    if(child==0) {
        ::close(0);
        if(!bo1::android::OpenUdp("LOCALHOST",0) || !bo1::android::BoundUdpPort()) ::_exit(80);
        sockaddr_in owned{}; socklen_t bytes=sizeof(owned);
        if(::getsockname(0,reinterpret_cast<sockaddr *>(&owned),&bytes) ||
            ntohs(owned.sin_port)!=bo1::android::BoundUdpPort() || owned.sin_addr.s_addr != INADDR_ANY ||
            (::fcntl(0,F_GETFD)&FD_CLOEXEC)==0) ::_exit(82);
        ::execl("/proc/self/exe","udp-fixture","check-cloexec",static_cast<char *>(nullptr));
        ::_exit(81);
    }
    if(child<0) return 17;
    int status;
    while(::waitpid(child,&status,0)<0) if(errno!=EINTR) return 18;
    if(!WIFEXITED(status) || WEXITSTATUS(status)) {
        std::fprintf(stderr,"UDP child status=%d\n",status);
        return 19;
    }
    ::close(peer);
    std::puts("actual UDP: bind-conflict/transactional-open/nonblocking/roundtrip/network-port/truncation/metadata-bounds/engine-Sys_GetPacket/close/FD0-CLOEXEC passed");
}
