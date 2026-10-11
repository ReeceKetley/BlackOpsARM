#pragma once
#if !defined(__ANDROID__) || !defined(__aarch64__)
#error Android profile services require the Android ARM64 target
#endif
struct BO1ProfZone;
unsigned long long BO1_ProfileTicks();
unsigned long BO1_ProfileThreadId();
long BO1_ProfileAdd(volatile long *value, long n);
long BO1_ProfileLoad(const volatile long *value);
void BO1_ProfileRegisterZone(BO1ProfZone *zone);
// Native reporting code must read the published list through this acquire load.
BO1ProfZone *BO1_ProfileZones();
constexpr unsigned long long BO1_PROFILE_TICKS_PER_SECOND = 1000000000ULL;
