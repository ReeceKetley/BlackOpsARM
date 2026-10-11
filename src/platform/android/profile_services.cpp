#include "profile_services.h"
#include <universal/profile.h>
#include <time.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <mutex>

unsigned long long BO1_ProfileTicks()
{
    timespec now{};
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
        std::perror("BO1_ProfileTicks: clock_gettime");
        std::abort();
    }
    return static_cast<unsigned long long>(now.tv_sec) * BO1_PROFILE_TICKS_PER_SECOND
        + static_cast<unsigned long long>(now.tv_nsec);
}
unsigned long BO1_ProfileThreadId() { return static_cast<unsigned long>(gettid()); }
long BO1_ProfileAdd(volatile long *value, long n)
{
    return __atomic_fetch_add(value, n, __ATOMIC_SEQ_CST);
}
long BO1_ProfileLoad(const volatile long *value)
{
    return __atomic_load_n(value, __ATOMIC_ACQUIRE);
}
void BO1_ProfileRegisterZone(BO1ProfZone *zone)
{
    static std::mutex registration;
    std::lock_guard<std::mutex> lock(registration);
    if (BO1_ProfileLoad(&zone->registered)) return;
    zone->next = __atomic_load_n(&g_bo1ProfZones, __ATOMIC_ACQUIRE);
    __atomic_store_n(&g_bo1ProfZones, zone, __ATOMIC_RELEASE);
    __atomic_store_n(&zone->registered, 1L, __ATOMIC_RELEASE);
}
BO1ProfZone *BO1_ProfileZones()
{
    return __atomic_load_n(&g_bo1ProfZones, __ATOMIC_ACQUIRE);
}
