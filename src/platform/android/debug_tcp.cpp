#include "debug_tcp.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <poll.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <strings.h>
#include <chrono>
#include <climits>

namespace bo1::android {
namespace {
int Descriptor(unsigned handle) {
    if (!handle || handle > static_cast<unsigned>(0x7fffffff)) { errno = EBADF; return -1; }
    return static_cast<int>(handle - 1);
}
unsigned Dispose(int fd, int error) { ::close(fd); errno = error; return 0; }
}
bool DebugTcpWouldBlock() { return errno == EAGAIN || errno == EWOULDBLOCK; }
bool DebugTcpConnected(unsigned handle, int timeoutMs) {
    const int fd = Descriptor(handle);
    if (fd < 0 || timeoutMs < 0) return false;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    pollfd event{fd, POLLOUT, 0};
    int result;
    do {
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now()).count();
        result = ::poll(&event, 1, remaining > 0 ? static_cast<int>(remaining) : 0);
    } while (result < 0 && errno == EINTR);
    if (result == 0) { errno = ETIMEDOUT; return false; }
    if (result < 0) return false;
    int error = 0;
    socklen_t size = sizeof(error);
    if (::getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &size)) return false;
    if (error) { errno = error; return false; }
    if (!(event.revents & POLLOUT)) { errno = ECONNABORTED; return false; }
    return true;
}
unsigned OpenDebugTcp(const char *host, int port, bool connectSocket) {
    if (port < -1 || port > 65535) { errno = EINVAL; return 0; }
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port == -1 ? 0 : port);
    if (host && *host && ::strcasecmp(host, "localhost")) {
        addrinfo hint{};
        hint.ai_family = AF_INET;
        hint.ai_socktype = SOCK_STREAM;
        addrinfo *resolved = nullptr;
        const int result = ::getaddrinfo(host, nullptr, &hint, &resolved);
        if (result) { if (result != EAI_SYSTEM) errno = EHOSTUNREACH; return 0; }
        address.sin_addr = reinterpret_cast<sockaddr_in *>(resolved->ai_addr)->sin_addr;
        ::freeaddrinfo(resolved);
    } else if (connectSocket) {
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    }
    const int fd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (fd < 0) return 0;
    if (connectSocket) {
        if (::connect(fd, reinterpret_cast<sockaddr *>(&address), sizeof(address))) {
            if (errno != EINPROGRESS) return Dispose(fd, errno);
            if (!DebugTcpConnected(static_cast<unsigned>(fd + 1), 5000)) return Dispose(fd, errno);
        }
    } else {
        const int reuse = 1;
        if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse))) return Dispose(fd, errno);
        if (::bind(fd, reinterpret_cast<sockaddr *>(&address), sizeof(address))) return Dispose(fd, errno);
    }
    return static_cast<unsigned>(fd + 1);
}
bool ListenDebugTcp(unsigned handle) { const int fd = Descriptor(handle); return fd >= 0 && ::listen(fd, 5) == 0; }
unsigned AcceptDebugTcp(unsigned handle) {
    const int fd = Descriptor(handle);
    if (fd < 0) return 0;
    int accepted;
    do { accepted = ::accept4(fd, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC); } while (accepted < 0 && errno == EINTR);
    return accepted < 0 ? 0 : static_cast<unsigned>(accepted + 1);
}
void CloseDebugTcp(unsigned handle) {
    const int fd = Descriptor(handle);
    if (fd >= 0) ::close(fd); // Linux retires the descriptor on EINTR: never retry close.
}
int ReadDebugTcp(unsigned handle, void *buffer, std::size_t length) {
    if (length > INT_MAX) { errno = EMSGSIZE; return -1; }
    if (!buffer && length) { errno = EINVAL; return -1; }
    const int fd = Descriptor(handle);
    if (fd < 0) return -1;
    ssize_t result;
    do { result = ::recv(fd, buffer, length, 0); } while (result < 0 && errno == EINTR);
    return static_cast<int>(result);
}
int WriteDebugTcp(unsigned handle, const void *buffer, std::size_t length) {
    if (length > INT_MAX) { errno = EMSGSIZE; return -1; }
    if (!buffer && length) { errno = EINVAL; return -1; }
    const int fd = Descriptor(handle);
    if (fd < 0) return -1;
    ssize_t result;
    do { result = ::send(fd, buffer, length, MSG_NOSIGNAL); } while (result < 0 && errno == EINTR);
    return static_cast<int>(result);
}
}
