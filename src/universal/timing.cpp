#include "timing.h"

#if defined(__ANDROID__)
#include <platform/win32_compat/win32_time.h>
using bo1::win32::_LARGE_INTEGER;
using bo1::win32::Sleep;
using bo1::win32::QueryPerformanceFrequency;
#else
#include <Windows.h>
#endif

long double msecPerRawTimerTick;

void __cdecl InitTiming()
{
    msecPerRawTimerTick = SecondsPerTick() * 1000.0;
}

double __cdecl SecondsPerTick()
{
    _LARGE_INTEGER qpcFrequency; // [esp+0h] [ebp-10h] BYREF

    Sleep(0);
    QueryPerformanceFrequency(&qpcFrequency);
    return 1.0 / (double)qpcFrequency.QuadPart;
}

