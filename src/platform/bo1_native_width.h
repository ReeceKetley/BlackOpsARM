#pragma once
#include <stdint.h>

// The Windows option still selects the legacy Win32/Win64 comparison paths.
// Other platforms select native-width runtime layouts from the actual ABI.
// This does not change fixed-width FastFile records or four-byte VM slots.
#if defined(_WIN32)
# if defined(BO1_PORT_X64)
#  define BO1_NATIVE_64BIT 1
# else
#  define BO1_NATIVE_64BIT 0
# endif
#elif UINTPTR_MAX == UINT64_MAX
# define BO1_NATIVE_64BIT 1
#else
# define BO1_NATIVE_64BIT 0
#endif

#if defined(__ANDROID__) && defined(__aarch64__)
# if !BO1_NATIVE_64BIT
#  error Android ARM64 requires native-width runtime layouts
# endif
# if defined(BO1_NARROW_HANDLES) && !BO1_NARROW_HANDLES
#  error Android ARM64 requires region-tagged Narrow32 handles
# endif
#endif
