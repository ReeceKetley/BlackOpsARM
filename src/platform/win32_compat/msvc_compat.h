#pragma once

// This prefix belongs only to the bounded NDK audit target. Clang's
// -fms-extensions supplies genuine fixed-width __int8/16/32/64 keywords.
// Do not define _MSC_VER, WIN32, BO1_PORT_X64 or rewrite native type widths.
#if !defined(__ANDROID__) || !defined(__aarch64__) || !defined(__clang__)
#error "This MSVC spelling adapter is scoped to Android ARM64 Clang."
#endif

// Android ARM64 defaults to unsigned char, while this engine's MSVC build uses
// signed char. The audited target explicitly preserves that source assumption.
static_assert(static_cast<char>(-1) < 0, "Compile this adapter with -fsigned-char.");

// ARM64 has one native calling convention; the Windows x86 cdecl spelling
// does not select a different ABI here. Other conventions are not emulated.
#ifndef __cdecl
#define __cdecl
#endif

// A real debugger breakpoint, never a success/no-op assertion replacement.
#define __debugbreak() __builtin_debugtrap()
