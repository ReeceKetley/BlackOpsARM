#include <sys/wait.h>
#include <unistd.h>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <string>

void Sys_Error(char *format, ...);

static bool run(int test) {
    int channel[2];
    if (::pipe(channel)) return false;
    const pid_t child = ::fork();
    if (child == 0) {
        ::close(channel[0]);
        if (::dup2(channel[1], STDERR_FILENO) < 0) ::_exit(99);
        ::close(channel[1]);
        if (test == 0) Sys_Error(const_cast<char *>("failure=%d text=%s"), 7, "100% literal");
        if (test == 1) {
            const std::string longText(6000, 'x');
            Sys_Error(const_cast<char *>("%s"), longText.c_str());
        }
        Sys_Error(nullptr);
        ::_exit(99);
    }
    ::close(channel[1]);
    if (child < 0) { ::close(channel[0]); return false; }
    std::string captured;
    char buffer[1024];
    for (;;) {
        const ssize_t count = ::read(channel[0], buffer, sizeof(buffer));
        if (count > 0) captured.append(buffer, static_cast<size_t>(count));
        else if (count < 0 && errno == EINTR) continue;
        else break;
    }
    ::close(channel[0]);
    int status;
    while (::waitpid(child, &status, 0) < 0) if (errno != EINTR) return false;
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 2) return false;
    const std::string prefix = "Native engine fatal error: ";
    const std::string expected = test == 0 ? prefix + "failure=7 text=100% literal\n"
        : test == 1 ? prefix + std::string(4095, 'x') + "\n"
        : prefix + "Fatal error without a message format\n";
    return captured.compare(0, expected.size(), expected) == 0
        && captured.find("Native fatal stack (program counters):\n", expected.size()) == expected.size()
        && captured.find("  #0 0x", expected.size()) != std::string::npos;
}
int main() {
    for (int i = 0; i < 3; ++i) if (!run(i)) return 1;
    std::puts("actual native fatal error: formatting/literal-percent/bounded-output/null-format/native-stack/terminal-exit2 passed");
}
