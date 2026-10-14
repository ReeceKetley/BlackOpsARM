#include "command_line_services.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits.h>
#include <cerrno>
#include <fcntl.h>
#include <mutex>
#include <unistd.h>

namespace {
char commandLine[PATH_MAX + 1024 + 2];
bool installed;
bool osCommandLineRead;
std::mutex &commandLineMutex() { static std::mutex value; return value; }
[[noreturn]] void invalidCommandLine() {
    std::fputs("Native engine command line missing, too long or installed twice\n", stderr);
    std::abort();
}
void readProcessCommandLine() {
    // GetCommandLineA is available during global construction on Windows.
    // Read actual native argv here; the explicit engine policy is installed
    // once by main before any engine worker can observe the buffer.
    char arguments[sizeof(commandLine)];
    int fd = open("/proc/self/cmdline", O_RDONLY | O_CLOEXEC);
    if (fd < 0) invalidCommandLine();
    std::size_t count = 0;
    for (;;) {
        char extra;
        auto bytes = read(fd, count < sizeof(arguments) ? arguments + count : &extra,
                          count < sizeof(arguments) ? sizeof(arguments) - count : 1);
        if (bytes < 0) { if (errno == EINTR) continue; close(fd); invalidCommandLine(); }
        if (!bytes) break;
        if (count == sizeof(arguments)) { close(fd); invalidCommandLine(); }
        count += static_cast<std::size_t>(bytes);
    }
    if (close(fd) || !count || arguments[count - 1] != 0) invalidCommandLine();
    std::size_t used = 0;
    auto append = [&](char c) {
        if (used + 1 >= sizeof(commandLine)) invalidCommandLine();
        commandLine[used++] = c;
    };
    for (std::size_t offset = 0; offset < count;) {
        if (offset) append(' ');
        append('"');
        unsigned slashes = 0;
        for (;;) {
            char c = arguments[offset++];
            if (c == '\\') { ++slashes; continue; }
            const auto copies = (c == '"' || c == 0) ? slashes * 2 : slashes;
            for (unsigned i = 0; i < copies; ++i) append('\\');
            slashes = 0;
            if (!c) break;
            if (c == '"') append('\\');
            append(c);
        }
        append('"');
    }
    commandLine[used] = 0;
    osCommandLineRead = true;
}
}
void bo1::android::InstallEngineCommandLine(const char *program, const char *arguments) {
    std::lock_guard<std::mutex> lock(commandLineMutex());
    if (installed || !program || !arguments) invalidCommandLine();
    const auto programBytes = strnlen(program, PATH_MAX + 1);
    const auto argumentBytes = strnlen(arguments, 1024);
    if (programBytes > PATH_MAX || argumentBytes >= 1024) invalidCommandLine();
    std::memcpy(commandLine, program, programBytes);
    commandLine[programBytes] = ' ';
    std::memcpy(commandLine + programBytes + 1, arguments, argumentBytes + 1);
    installed = true;
}
char *GetCommandLineA() {
    std::lock_guard<std::mutex> lock(commandLineMutex());
    if (!installed && !osCommandLineRead) readProcessCommandLine();
    return commandLine;
}
