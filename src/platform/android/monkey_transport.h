#pragma once
#include <cstddef>
namespace bo1::android {
enum class MonkeyIo { Complete, Pending, Stopped, Failed };
struct MonkeyResult { MonkeyIo state; int error; };
MonkeyResult MonkeyConnect(const char *ipv4);
void MonkeyClose();
MonkeyResult MonkeySend(const char *first, const char *second, bool pair);
MonkeyResult MonkeyReceive(char *first, std::size_t firstCapacity, char *second, std::size_t secondCapacity, bool pair);
MonkeyResult MonkeyReadable();
bool MonkeyConnected();
}
