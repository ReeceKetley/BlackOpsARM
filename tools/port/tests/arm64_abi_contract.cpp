#include <platform/win32_compat/win32_types.h>
#include <climits>
#include <type_traits>

static_assert(sizeof(void*) == 8 && sizeof(std::uintptr_t) == 8);
static_assert(sizeof(int) == 4 && CHAR_BIT == 8);
static_assert(sizeof(long) == 8 && sizeof(wchar_t) == 4);
static_assert(sizeof(long double) == 16);
// Clang's __int8 spelling is char (a distinct C++ type from signed char).
// Its width and signedness, not type identity, are the relevant scalar contract.
static_assert(sizeof(__int8) == 1 && std::is_signed_v<__int8>);
static_assert(std::is_same_v<__int16, short>);
static_assert(std::is_same_v<__int32, int>);
static_assert(std::is_same_v<__int64, long long>);
// Windows-compatible API widths are fixed independently of Android native long.
static_assert(sizeof(bo1::win32::DWORD) == 4 && sizeof(bo1::win32::BOOL) == 4);
