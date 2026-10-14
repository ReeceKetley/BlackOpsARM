#include "udp_socket.h"
#include "network_addresses.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <mutex>
#include <cstring>
#include <cerrno>
#include <strings.h>

namespace bo1::android {
namespace {
std::mutex socketLock;
int socketFd = -1;
bool Dispose(int fd, int error) { ::close(fd); errno = error; return false; }
}
bool OpenUdp(const char *interfaceName, std::uint16_t hostPort) {
    std::lock_guard lock(socketLock);
    if (socketFd >= 0) { errno = EALREADY; return false; }
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(hostPort);
    // Preserve the reference NET_IPSocket localhost/default = wildcard policy.
    if (interfaceName && *interfaceName && ::strcasecmp(interfaceName, "localhost")) {
        netadr_t resolved{};
        if (!ResolveIPv4Host(interfaceName, &resolved)) return false;
        std::memcpy(&address.sin_addr, resolved.ip, 4);
    }
    const int fd = ::socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (fd < 0) return false;
    const int broadcast = 1;
    if (::setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast))) return Dispose(fd, errno);
    if (::bind(fd, reinterpret_cast<const sockaddr *>(&address), sizeof(address))) return Dispose(fd, errno);
    socketFd = fd;
    return true;
}
bool CloseUdp() {
    std::lock_guard lock(socketLock);
    if (socketFd < 0) return true;
    const int owned = socketFd;
    socketFd = -1;
    // Linux releases the descriptor even if close reports EINTR; never retry it.
    return ::close(owned) == 0;
}
std::uint16_t BoundUdpPort() {
    std::lock_guard lock(socketLock);
    if (socketFd < 0) { errno = EBADF; return 0; }
    sockaddr_in address{};
    socklen_t size = sizeof(address);
    if (::getsockname(socketFd, reinterpret_cast<sockaddr *>(&address), &size)) return 0;
    return ntohs(address.sin_port);
}
bool SendUdp(const void *data, std::size_t length, const netadr_t &destination) {
    if ((!data && length) || length > 65507 ||
        (destination.type != NA_IP && destination.type != NA_BROADCAST)) { errno = EINVAL; return false; }
    std::lock_guard lock(socketLock);
    if (socketFd < 0) { errno = EBADF; return false; }
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = destination.port;
    if (destination.type == NA_BROADCAST) address.sin_addr.s_addr = INADDR_BROADCAST;
    else std::memcpy(&address.sin_addr, destination.ip, 4);
    ssize_t result;
    do { result = ::sendto(socketFd, data, length, 0, reinterpret_cast<const sockaddr *>(&address), sizeof(address)); }
    while (result < 0 && errno == EINTR);
    if (result < 0) return false;
    if (static_cast<std::size_t>(result) != length) { errno = EIO; return false; }
    return true;
}
int ReceiveUdp(void *data, std::size_t capacity, netadr_t *source, std::size_t *length) {
    if (!data || !capacity || !source || !length) { errno = EINVAL; return -1; }
    std::lock_guard lock(socketLock);
    if (socketFd < 0) { errno = EBADF; return -1; }
    sockaddr_in address{};
    iovec vector{data, capacity};
    msghdr message{};
    message.msg_name = &address;
    message.msg_namelen = sizeof(address);
    message.msg_iov = &vector;
    message.msg_iovlen = 1;
    ssize_t received;
    do { received = ::recvmsg(socketFd, &message, MSG_TRUNC); } while (received < 0 && errno == EINTR);
    if (received < 0) return errno == EAGAIN || errno == EWOULDBLOCK ? 0 : -1;
    if ((message.msg_flags & MSG_TRUNC) || static_cast<std::size_t>(received) >= capacity) {
        errno = EMSGSIZE; return -1;
    }
    netadr_t resolved{};
    resolved.type = NA_IP;
    std::memcpy(resolved.ip, &address.sin_addr, 4);
    resolved.port = address.sin_port;
    *source = resolved;
    *length = static_cast<std::size_t>(received);
    return 1;
}
}
