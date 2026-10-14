#include <platform/android/legacy_crt.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

static void require(bool condition, const char *message) {
    if (!condition) { std::fprintf(stderr, "FAIL %s\n", message); std::abort(); }
}
int main() {
    const char *name = "BO1_NATIVE_ENV_OWNERSHIP_TEST";
    char assignment[96] = "BO1_NATIVE_ENV_OWNERSHIP_TEST=first value=with equals";
    require(bo1::crt::Putenv(assignment) == 0, "create");
    std::memset(assignment, 'X', sizeof(assignment));
    const char *value = std::getenv(name);
    require(value && !std::strcmp(value, "first value=with equals"), "copied caller storage");
    require(bo1::crt::Putenv("BO1_NATIVE_ENV_OWNERSHIP_TEST=replacement") == 0, "replace");
    require(!std::strcmp(std::getenv(name), "replacement"), "replacement value");
    const char *invalid[] = {nullptr, "", "missing_separator", "=empty_name"};
    for (const char *entry : invalid) {
        errno = 0;
        require(bo1::crt::Putenv(entry) == -1 && errno == EINVAL, "invalid assignment");
        require(!std::strcmp(std::getenv(name), "replacement"), "invalid preserves existing value");
    }
    require(bo1::crt::Putenv("BO1_NATIVE_ENV_OWNERSHIP_TEST=") == 0, "remove");
    require(!std::getenv(name), "removed variable");
    require(bo1::crt::Putenv("BO1_NATIVE_ENV_OWNERSHIP_TEST=") == 0, "remove absent");
    std::puts("PASS native environment copied lifetime, replace, removal, invalid assignments");
}
