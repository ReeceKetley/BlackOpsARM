#pragma once
#include <cstddef>
namespace bo1::android {
// Zero is empty; live descriptor handles are fd + 1 (including descriptor zero).
unsigned OpenDebugTcp(const char *host, int port, bool connectSocket);
bool DebugTcpConnected(unsigned handle, int timeoutMs);
bool ListenDebugTcp(unsigned handle);
unsigned AcceptDebugTcp(unsigned handle);
void CloseDebugTcp(unsigned handle);
int ReadDebugTcp(unsigned handle, void *buffer, std::size_t length);
int WriteDebugTcp(unsigned handle, const void *buffer, std::size_t length);
bool DebugTcpWouldBlock();
}
