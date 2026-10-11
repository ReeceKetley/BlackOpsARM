// Compare the Android implementation with the actual installed Windows CRT.
#include <platform/engine_random.h>
#include <platform/android/engine_random.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <vector>

static void check(bool condition) { if (!condition) std::abort(); }
int main(int argc, char **argv)
{
    if (argc == 2 && !std::strcmp(argv[1], "--emit-vectors")) {
        const unsigned seeds[] = {0, 1, 2, 7, 42, 32767, 32768, 65535,
            0x7fffffffu, 0x80000000u, 0xfffffffeu, 0xffffffffu};
        std::puts("// Captured from actual MSVC Windows CRT rand(), Win32/x64 agree.\n#pragma once\n#include <cstdint>\nnamespace bo1::test {\nstruct RngVector { std::uint32_t seed; int draws[128]; };\ninline constexpr RngVector windowsRng[] = {");
        for (unsigned seed : seeds) {
            std::srand(seed);
            std::printf("{0x%08xu, {", seed);
            for (int i = 0; i < 128; ++i) std::printf("%s%d", i ? "," : "", std::rand());
            std::puts("}},");
        }
        std::puts("};\n}");
        return 0;
    }
    check(RAND_MAX == 32767);
    check(bo1::android::CrtRand() == std::rand()); // default seed, before srand
    const unsigned edges[] = {0u, 1u, 0x7fffffffu, 0x80000000u, 0xffffffffu};
    for (unsigned seed : edges) {
        std::srand(seed); bo1::android::CrtSrand(seed);
        for (int i = 0; i < 100000; ++i) check(bo1::android::CrtRand() == std::rand());
    }
    for (unsigned seed = 0; seed < 5000; ++seed) {
        std::srand(seed); bo1::android::CrtSrand(seed);
        for (int i = 0; i < 64; ++i) check(bo1::android::CrtRand() == std::rand());
    }
    // Windows wrappers must share the real CRT, including mixed direct draws.
    int expected[128];
    std::srand(42);
    for (int &value : expected) value = std::rand();
    BO1_CrtSrand(42);
    for (int i = 0; i < 128; ++i) check((i & 1 ? BO1_CrtRand() : std::rand()) == expected[i]);
    std::vector<std::thread> threads;
    for (unsigned t = 0; t < 8; ++t) threads.emplace_back([t] {
        check(bo1::android::CrtRand() == std::rand()); // independently default 1
        std::srand(t + 42); bo1::android::CrtSrand(t + 42);
        for (int i = 0; i < 100000; ++i) {
            check(bo1::android::CrtRand() == std::rand());
            if (!(i % 31)) std::this_thread::yield();
        }
    });
    for (auto &thread : threads) thread.join();
    std::puts("PASS actual Windows CRT oracle: 5000 seeds x 64, five edge seeds x 100000, eight threads x 100000, default seeds and shared Windows CRT draws");
}
