#include "win32_errors.h"

namespace bo1::win32 {
namespace {
thread_local DWORD lastError = 0;
}

DWORD GetLastError()
{
    return lastError;
}

void SetLastError(DWORD error)
{
    lastError = error;
}
}
