#pragma once
#include "win32_types.h"

namespace bo1::win32 {
// The future counter must use monotonic nanoseconds to match this frequency.
// QueryPerformanceCounter itself is intentionally not implemented in Phase 1.
BOOL QueryPerformanceFrequency(LARGE_INTEGER* frequency);
void Sleep(DWORD milliseconds);
}
