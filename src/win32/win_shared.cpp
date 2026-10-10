#include "win_shared.h"

#include <Windows.h>
#include <xmmintrin.h>

int sys_timeBase;

unsigned int __cdecl Sys_Milliseconds_Win32()
{
    static int initialized_1 = 0;
    if ( !initialized_1 )
    {
        sys_timeBase = timeGetTime();
        initialized_1 = 1;
    }
    return timeGetTime() - sys_timeBase;
}

unsigned int __cdecl Sys_MillisecondsRaw_Win32()
{
    return timeGetTime();
}

unsigned int __cdecl Sys_Milliseconds()
{
    return Sys_Milliseconds_Win32();
}

unsigned int __cdecl Sys_MillisecondsRaw()
{
    return Sys_MillisecondsRaw_Win32();
}

// Banker's rounding via SSE cvtss2si (matches SnapFloat in qcommon/common.h).
static inline int Sys_SnapFloatComponent(float f)
{
    return _mm_cvtss_si32(_mm_set_ss(f));
}

// https://github.com/id-Software/Quake-III-Arena/blob/master/code/win32/win_shared.c
void Sys_SnapVector(float *v)
{
    v[0] = (float)Sys_SnapFloatComponent(v[0]);
    v[1] = (float)Sys_SnapFloatComponent(v[1]);
    v[2] = (float)Sys_SnapFloatComponent(v[2]);
}
