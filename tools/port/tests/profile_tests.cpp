#include <universal/profile.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <vector>

static void check(bool value) { if (!value) std::abort(); }
static void delay() { std::this_thread::sleep_for(std::chrono::milliseconds(2)); }
int main()
{
    BO1ProfZone zone{"test", nullptr, 0, 0, 0, 0, 0};
    { BO1ProfScope disabled(&zone); BO1_RenderCount(BO1_RC_BUF_LOCK, 1); }
    check(!zone.registered && !zone.mainCalls && !g_bo1RenderCount[BO1_RC_BUF_LOCK]);
    g_bo1ProfMainThread = BO1_ProfileThreadId();
    g_bo1ProfOn = true; // Set before thread creation; stable until all join.
    const auto start = BO1_ProfileTicks();
    { BO1ProfScope outer(&zone); { BO1ProfScope inner(&zone); delay(); } }
    check(zone.mainDepth == 0 && zone.mainCalls == 1 && zone.mainTicks > 0);
    check(BO1_ProfileTicks() >= start + zone.mainTicks);
    std::vector<std::thread> workers;
    for (int t = 0; t < 8; ++t) workers.emplace_back([&] {
        check(BO1_ProfileThreadId() != g_bo1ProfMainThread);
        BO1ProfScope other(&zone);
        for (int i = 0; i < 10000; ++i) {
            BO1_RenderCount(BO1_RC_BUF_LOCK, 1);
            BO1_ProfFirstUse(BO1_FIRST_PASS, static_cast<unsigned long long>(i), nullptr);
        }
        delay();
    });
    for (auto &worker : workers) worker.join();
    check(BO1_ProfileLoad(&g_bo1RenderCount[BO1_RC_BUF_LOCK]) == 80000);
    check(BO1_ProfileLoad(&g_bo1FirstUse[BO1_FIRST_PASS]) == 10000);
    check(BO1_ProfileLoad(&g_bo1RenderFirstUse[BO1_FIRST_PASS]) == 10000);
    check(BO1_ProfileLoad(&zone.otherTicks1k) > 0 && zone.mainCalls == 1);
#if defined(__ANDROID__)
    check(BO1_ProfileZones() == &zone && zone.next == nullptr);
    const auto nativeStart = BO1_ProfileTicks();
    delay();
    check(BO1_ProfileTicks() - nativeStart >= BO1_PROFILE_TICKS_PER_SECOND / 1000);
    // Concurrent first entry into the same previously unregistered zone.
    BO1ProfZone fresh{"fresh", nullptr, 0, 0, 0, 0, 0};
    workers.clear();
    for (int t = 0; t < 8; ++t) workers.emplace_back([&] { BO1ProfScope first(&fresh); delay(); });
    for (auto &worker : workers) worker.join();
    check(BO1_ProfileZones() == &fresh && fresh.next == &zone && zone.next == nullptr);
#endif
    BO1_ProfFirstUse(BO1_FIRST_TEXTURE, 1, "material:technique");
    BO1_ProfFirstUse(BO1_FIRST_TEXTURE, 1, "duplicate");
    char name[64];
    check(BO1_ProfFirstUseNames(name, sizeof(name)) == 0 && !std::strcmp(name, "material:technique"));
    check(BO1_ProfFirstUseNames(name, sizeof(name)) == 0 && !name[0]);
    BO1_ProfFirstUse(BO1_FIRST_TEXTURE, 2, "abcdef");
    char shortName[3] = {};
    BO1_ProfFirstUseNames(shortName, sizeof(shortName));
    check(!std::strcmp(shortName, "ab"));
    BO1_ProfFirstUse(BO1_FIRST_TEXTURE, 3, "x");
    char one[1] = {'?'};
    BO1_ProfFirstUseNames(one, 1);
    check(!one[0]);
    g_svProfThread = BO1_ProfileThreadId();
    g_svProfOn = 1;
    g_svProfLast = BO1_ProfileTicks();
    { SvProfScope script(SVPROF_SCRIPT); delay(); { SvProfBuiltin builtin(3); delay(); } }
    check(g_svProfDepth == 0 && g_svProfCalls[SVPROF_SCRIPT] == 1
        && g_svProfCalls[SVPROF_BUILTIN] == 1 && g_svProfBuiltinCalls[3] == 1);
    check(g_svProfTicks[SVPROF_SCRIPT] > 0 && g_svProfTicks[SVPROF_BUILTIN] > 0
        && g_svProfBuiltinTicks[3] > 0);
    std::thread otherServer([] { SvProfScope wrongThread(SVPROF_TRACE); check(!wrongThread.on); });
    otherServer.join();
    g_svProfDepth = 63;
    { SvProfScope depthLimit(SVPROF_TRACE); check(!depthLimit.on); }
    g_svProfDepth = 0;
    g_svProfOn = 0;
    { SvProfScope disabled(SVPROF_TRACE); check(!disabled.on); }
    g_bo1ProfOn = false;
    std::puts("PASS real profile.cpp: monotonic/native ticks, thread filtering, nested/exclusive/builtin scopes, 80000 atomic adds, 10000 deduplicated keys, registration and bounded names");
}
