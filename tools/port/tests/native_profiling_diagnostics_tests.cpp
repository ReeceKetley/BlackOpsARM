#include <platform/android/profiling_diagnostics.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/mman.h>

static void require(bool condition, const char *message) {
    if (!condition) { std::fprintf(stderr, "FAIL %s\n", message); std::abort(); }
}
extern "C" __attribute__((noinline, visibility("default"))) int BO1_ProfileExportedFixture() { return 17; }
static __attribute__((noinline)) int LocalFixture() { return 23; }

int main() {
    std::uint64_t before = 0, during = 0, after = 0;
    require(bo1::android::ProcessVirtualBytes(&before) && before > 0, "real process measurement");
    constexpr std::size_t reserve = 128 * 1024 * 1024;
    void *mapping = ::mmap(nullptr, reserve, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    require(mapping != MAP_FAILED, "reserve untouched mapping");
    require(bo1::android::ProcessVirtualBytes(&during) && during >= before + reserve, "reserved VA included without page touches");
    require(::munmap(mapping, reserve) == 0, "release mapping");
    require(bo1::android::ProcessVirtualBytes(&after) && during >= after + reserve, "released VA removed");
    errno = 0;
    require(!bo1::android::ProcessVirtualBytes(nullptr) && errno == EINVAL, "invalid output");
    char symbol[256];
    require(bo1::android::ProfileSymbolName(reinterpret_cast<void *>(&BO1_ProfileExportedFixture), symbol, sizeof(symbol)), "exported address resolves");
    require(!std::strcmp(symbol, "BO1_ProfileExportedFixture"), "actual ELF symbol name");
    require(bo1::android::ProfileSymbolName(reinterpret_cast<void *>(&LocalFixture), symbol, sizeof(symbol)), "local address resolves to module");
    require(std::strstr(symbol, "+0x") != nullptr, "unexported address reports real module offset");
    struct { char output[2]; char guard[8]; } bounded{};
    std::memset(bounded.guard, 0x5a, sizeof(bounded.guard));
    require(!bo1::android::ProfileSymbolName(reinterpret_cast<void *>(&BO1_ProfileExportedFixture), bounded.output, sizeof(bounded.output)), "short output rejected");
    for (char value : bounded.guard) require(value == 0x5a, "output boundary");
    require(!bo1::android::ProfileSymbolName(nullptr, symbol, sizeof(symbol)), "invalid address");
    require(!bo1::android::ProfileSymbolName(reinterpret_cast<void *>(&LocalFixture), nullptr, 8), "invalid name output");
    require(!bo1::android::ProfileSymbolName(reinterpret_cast<void *>(&LocalFixture), symbol, 0), "zero capacity");
    std::printf("PASS native profiler VA before=%llu reserved=%llu released=%llu; exported/local symbols and bounds\n",
                static_cast<unsigned long long>(before), static_cast<unsigned long long>(during), static_cast<unsigned long long>(after));
}
