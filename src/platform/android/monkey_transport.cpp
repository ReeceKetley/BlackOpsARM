#include "monkey_transport.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>
#include <unistd.h>
#include <atomic>
#include <chrono>
#include <climits>
#include <cerrno>
#include <cstring>
#include <memory>
#include <mutex>
#include <vector>
#include <new>

namespace bo1::android {
namespace {
using Clock=std::chrono::steady_clock;
struct Connection {
    explicit Connection(int descriptor): fd(descriptor) {}
    ~Connection() { if(fd>=0) ::close(fd); }
    int fd=-1;
    std::atomic<bool> closing{false}, connected{false};
    std::mutex send, receive;
    std::vector<unsigned char> bytes;
};
std::mutex owner;
std::shared_ptr<Connection> current;
MonkeyResult Result(MonkeyIo state,int error=0) { return {state,error}; }
std::shared_ptr<Connection> Acquire() { std::lock_guard<std::mutex> lock(owner);return current; }
void Retire(const std::shared_ptr<Connection>& connection) {
    if(!connection) return;
    connection->connected.store(false,std::memory_order_release);
    if(!connection->closing.exchange(true,std::memory_order_acq_rel)) ::shutdown(connection->fd,SHUT_RDWR);
    std::lock_guard<std::mutex> lock(owner);
    if(current==connection) current.reset();
}
MonkeyResult Fail(const std::shared_ptr<Connection>& connection,int error) {
    const bool cancelled=connection->closing.load(std::memory_order_acquire);
    Retire(connection);
    return Result(cancelled?MonkeyIo::Stopped:MonkeyIo::Failed,error);
}
MonkeyResult Wait(const std::shared_ptr<Connection>& connection,short events,Clock::time_point deadline) {
    for(;;) {
        if(connection->closing.load(std::memory_order_acquire)) return Result(MonkeyIo::Stopped);
        auto remaining=std::chrono::duration_cast<std::chrono::milliseconds>(deadline-Clock::now()).count();
        if(remaining<=0) return Fail(connection,ETIMEDOUT);
        pollfd p{connection->fd,events,0};
        // A bounded poll slice also cancels connect when shutdown reports ENOTCONN.
        int status=::poll(&p,1,static_cast<int>(remaining>50?50:remaining));
        if(status<0) { if(errno==EINTR) continue;return Fail(connection,errno); }
        if(!status) continue;
        if(connection->closing.load(std::memory_order_acquire)) return Result(MonkeyIo::Stopped);
        if(p.revents&events) return Result(MonkeyIo::Complete);
        return Fail(connection,ECONNRESET);
    }
}
MonkeyResult Write(const std::shared_ptr<Connection>& connection,const void *data,std::size_t size,Clock::time_point deadline) {
    const auto *bytes=static_cast<const unsigned char *>(data);
    while(size) {
        if(connection->closing.load(std::memory_order_acquire)) return Result(MonkeyIo::Stopped);
        ssize_t count=::send(connection->fd,bytes,size>INT_MAX?INT_MAX:size,MSG_NOSIGNAL);
        if(count>0) { bytes+=count;size-=count;continue; }
        if(count<0 && errno==EINTR) continue;
        if(count<0 && (errno==EAGAIN || errno==EWOULDBLOCK)) {
            auto waited=Wait(connection,POLLOUT,deadline);if(waited.state!=MonkeyIo::Complete) return waited;continue;
        }
        return Fail(connection,count==0?EPIPE:errno);
    }
    return Result(MonkeyIo::Complete);
}
MonkeyResult Ensure(const std::shared_ptr<Connection>& connection,std::size_t count) {
    if(connection->closing.load(std::memory_order_acquire)) return Result(MonkeyIo::Stopped);
    while(connection->bytes.size()<count) {
        if(connection->closing.load(std::memory_order_acquire)) return Result(MonkeyIo::Stopped);
        unsigned char chunk[4096];
        std::size_t remaining=count-connection->bytes.size();
        ssize_t read=::recv(connection->fd,chunk,remaining<sizeof(chunk)?remaining:sizeof(chunk),0);
        if(read>0) {
            try { connection->bytes.insert(connection->bytes.end(),chunk,chunk+read); }
            catch(const std::bad_alloc&) { return Fail(connection,ENOMEM); }
            continue;
        }
        if(read<0 && errno==EINTR) continue;
        if(read<0 && (errno==EAGAIN || errno==EWOULDBLOCK)) return Result(MonkeyIo::Pending);
        return Fail(connection,read==0?ECONNRESET:errno);
    }
    return Result(MonkeyIo::Complete);
}
MonkeyResult Frame(const std::shared_ptr<Connection>& connection,std::size_t offset,std::size_t capacity,std::size_t &length) {
    auto status=Ensure(connection,offset+4);if(status.state!=MonkeyIo::Complete) return status;
    uint32_t network;std::memcpy(&network,connection->bytes.data()+offset,sizeof(network));
    length=ntohl(network);
    if(length>=capacity) return Fail(connection,EMSGSIZE);
    return Ensure(connection,offset+4+length);
}
}
MonkeyResult MonkeyConnect(const char *ipv4) {
    sockaddr_in address{};address.sin_family=AF_INET;address.sin_port=htons(7000);
    if(!ipv4 || !*ipv4 || ::inet_pton(AF_INET,ipv4,&address.sin_addr)!=1) return Result(MonkeyIo::Failed,EINVAL);
    int fd=::socket(AF_INET,SOCK_STREAM|SOCK_NONBLOCK|SOCK_CLOEXEC,IPPROTO_TCP);
    if(fd<0) return Result(MonkeyIo::Failed,errno);
    std::shared_ptr<Connection> connection;
    try { connection=std::make_shared<Connection>(fd); }
    catch(const std::bad_alloc&) { ::close(fd);return Result(MonkeyIo::Failed,ENOMEM); }
    {
        std::lock_guard<std::mutex> lock(owner);
        if(current) return Result(MonkeyIo::Failed,EALREADY);
        current=connection; // Pending descriptor is cancellable; not connected readiness.
    }
    if(::connect(fd,reinterpret_cast<sockaddr*>(&address),sizeof(address))!=0) {
        if(errno!=EINPROGRESS) return Fail(connection,errno);
        auto waited=Wait(connection,POLLOUT,Clock::now()+std::chrono::seconds(5));
        if(waited.state!=MonkeyIo::Complete) return waited;
        int error=0;socklen_t size=sizeof(error);
        if(::getsockopt(fd,SOL_SOCKET,SO_ERROR,&error,&size)!=0) return Fail(connection,errno);
        if(error) return Fail(connection,error);
    }
    {
        std::lock_guard<std::mutex> lock(owner);
        if(current!=connection || connection->closing.load(std::memory_order_acquire)) return Result(MonkeyIo::Stopped);
        connection->connected.store(true,std::memory_order_release);
    }
    return Result(MonkeyIo::Complete);
}
void MonkeyClose() { Retire(Acquire()); }
bool MonkeyConnected() { auto connection=Acquire();return connection && connection->connected.load(std::memory_order_acquire); }
MonkeyResult MonkeySend(const char *first,const char *second,bool pair) {
    auto connection=Acquire();if(!connection || !connection->connected.load(std::memory_order_acquire)) return Result(MonkeyIo::Stopped);
    if(!first || (pair && !second)) return Fail(connection,EINVAL);
    const std::size_t lengths[2]={std::strlen(first),pair?std::strlen(second):0};
    if(lengths[0]>INT_MAX || lengths[1]>INT_MAX) return Fail(connection,EMSGSIZE);
    std::lock_guard<std::mutex> lock(connection->send);
    const auto deadline=Clock::now()+std::chrono::seconds(5);
    const char *strings[2]={first,second};
    for(int i=0;i<(pair?2:1);++i) {
        uint32_t network=htonl(static_cast<uint32_t>(lengths[i]));
        auto status=Write(connection,&network,sizeof(network),deadline);if(status.state!=MonkeyIo::Complete) return status;
        status=Write(connection,strings[i],lengths[i],deadline);if(status.state!=MonkeyIo::Complete) return status;
    }
    return Result(MonkeyIo::Complete);
}
MonkeyResult MonkeyReceive(char *first,std::size_t firstCapacity,char *second,std::size_t secondCapacity,bool pair) {
    auto connection=Acquire();if(!connection || !connection->connected.load(std::memory_order_acquire)) return Result(MonkeyIo::Stopped);
    if(!first || !firstCapacity || (pair && (!second || !secondCapacity)) || firstCapacity>INT_MAX || secondCapacity>INT_MAX) return Fail(connection,EINVAL);
    std::lock_guard<std::mutex> lock(connection->receive);
    std::size_t firstLength=0,secondLength=0;
    auto status=Frame(connection,0,firstCapacity,firstLength);if(status.state!=MonkeyIo::Complete) return status;
    const std::size_t secondOffset=4+firstLength;
    if(pair) { status=Frame(connection,secondOffset,secondCapacity,secondLength);if(status.state!=MonkeyIo::Complete) return status; }
    if(connection->closing.load(std::memory_order_acquire)) return Result(MonkeyIo::Stopped);
    std::memcpy(first,connection->bytes.data()+4,firstLength);first[firstLength]=0;
    if(pair) { std::memcpy(second,connection->bytes.data()+secondOffset+4,secondLength);second[secondLength]=0; }
    connection->bytes.clear(); // Only exact current frame bytes were read, never the next message.
    return Result(MonkeyIo::Complete);
}
MonkeyResult MonkeyReadable() {
    auto connection=Acquire();if(!connection || !connection->connected.load(std::memory_order_acquire)) return Result(MonkeyIo::Stopped);
    std::lock_guard<std::mutex> lock(connection->receive);
    if(!connection->bytes.empty()) return Result(MonkeyIo::Complete);
    unsigned char byte;ssize_t read;
    do { read=::recv(connection->fd,&byte,1,MSG_PEEK); } while(read<0 && errno==EINTR);
    if(read>0) return Result(MonkeyIo::Complete);
    if(read<0 && (errno==EAGAIN || errno==EWOULDBLOCK)) return Result(MonkeyIo::Pending);
    return Fail(connection,read==0?ECONNRESET:errno);
}
}
