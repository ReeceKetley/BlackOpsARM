#pragma once

#include <Windows.h>
#if defined(__ANDROID__)
#include <chrono>
#include <thread>
#endif

namespace bdPlatformTiming
{
    inline LARGE_INTEGER __cdecl getHiResTimeStamp()
    {
        LARGE_INTEGER PerformanceCount; // [esp+0h] [ebp-8h] BYREF

#if defined(__ANDROID__)
        PerformanceCount.QuadPart=std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
#else
        QueryPerformanceCounter(&PerformanceCount);
#endif
        return PerformanceCount;
    }

    inline double __cdecl getElapsedTime(unsigned __int64 a1, unsigned __int64 a2)
    {
        LARGE_INTEGER Frequency; // [esp+14h] [ebp-10h] BYREF

#if defined(__ANDROID__)
        Frequency.QuadPart=1000000000;
#else
        QueryPerformanceFrequency(&Frequency);
#endif
        return (float)((double)(__int64)(a2 - a1) / (double)Frequency.QuadPart);
   }

    inline void sleep(DWORD dwMilliseconds)
    {
#if defined(__ANDROID__)
        std::this_thread::sleep_for(std::chrono::milliseconds(dwMilliseconds));
#else
        Sleep(dwMilliseconds);
#endif
    }

    inline unsigned __int64 getLoResElapsedTime(unsigned int a1, unsigned int a2)
    {
        __int64 v3; // [esp+8h] [ebp-8h]

        v3 = a2;
        if (a2 < a1)
            v3 = a2 + 0xFFFFFFFFLL;
        return (v3 - (unsigned __int64)a1) / 0x3E8;
    }
}
