#pragma once

#if !defined(__ANDROID__) && !defined(BO1_WIN32_COMPAT_HOST_TEST)
#error "Win32 compatibility types are Android-only (or an explicit POSIX host test)."
#endif

#include <cstdint>

// Keep these names scoped: DXVK Native owns its global Windows ABI types.
// Never define Windows.h, override long/wchar_t, or shadow DXVK's definitions.
namespace bo1::win32 {
using BOOL = std::int32_t;
using DWORD = std::uint32_t;
struct FILETIME {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
};
static_assert(sizeof(FILETIME) == 8 && alignof(FILETIME) == 4);

// Only QuadPart is required in this phase. This is not a full LARGE_INTEGER
// union replacement; code needing LowPart/HighPart must extend/test that contract.
struct _LARGE_INTEGER {
    std::int64_t QuadPart;
};
using LARGE_INTEGER = _LARGE_INTEGER;

inline constexpr DWORD INFINITE = 0xffffffffu;
inline constexpr DWORD ERROR_INVALID_PARAMETER = 87u;
static_assert(sizeof(BOOL) == 4 && sizeof(DWORD) == 4);
static_assert(sizeof(LARGE_INTEGER) == 8 && alignof(LARGE_INTEGER) == 8);
}
