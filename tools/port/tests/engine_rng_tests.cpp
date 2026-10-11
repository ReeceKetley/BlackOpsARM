#include <platform/engine_random.h>
#include "windows_crt_rng_vectors.h"
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <vector>

static void check(bool condition) { if (!condition) std::abort(); }
static void defaultStream()
{
    for (int expected : bo1::test::windowsRng[1].draws) check(BO1_CrtRand() == expected);
}
static void vectors()
{
    for (const auto &sequence : bo1::test::windowsRng) {
        BO1_CrtSrand(sequence.seed);
        for (int expected : sequence.draws) check(BO1_CrtRand() == expected);
    }
}
int main()
{
    defaultStream();
    vectors();
    BO1_CrtSrand(42);
    for (int i = 0; i < 64; ++i) check(BO1_CrtRand() == bo1::test::windowsRng[4].draws[i]);
    std::vector<std::thread> workers;
    for (int t = 0; t < 8; ++t) workers.emplace_back([] {
        defaultStream();
        for (int repeat = 0; repeat < 64; ++repeat) {
            vectors();
            std::this_thread::yield();
        }
    });
    for (auto &worker : workers) worker.join();
    for (int i = 64; i < 128; ++i) check(BO1_CrtRand() == bo1::test::windowsRng[4].draws[i]);
    BO1_CrtSrand(1);
    for (int i = 0; i < 100000; ++i) {
        const int value = BO1_CrtRand();
        check(value >= 0 && value <= 32767);
    }
    std::puts("PASS Windows-captured RNG vectors: 12 edge seeds x 128 draws, default/reseed, eight independent threads x 64 vector cycles, parent-stream isolation, 15-bit range");
}
