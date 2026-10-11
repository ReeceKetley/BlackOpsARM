#pragma once
#include "win32_types.h"

namespace bo1::win32 {
DWORD GetLastError();
void SetLastError(DWORD error);
}
