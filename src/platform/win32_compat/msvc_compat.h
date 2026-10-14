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

// Windows Interlocked counters also appear as unsigned 32-bit engine storage.
// Preserve their full-barrier, wrapping bit semantics without pointer casts or
// treating Android's 64-bit long as a Windows LONG.
static_assert(sizeof(unsigned int)==4 && sizeof(int)==4);
inline int _InterlockedExchangeAdd(volatile unsigned int *pointer,unsigned int value) {
    return static_cast<int>(__atomic_fetch_add(pointer,value,__ATOMIC_SEQ_CST));
}
inline int _InterlockedExchange(volatile unsigned int *pointer,unsigned int value) {
    return static_cast<int>(__atomic_exchange_n(pointer,value,__ATOMIC_SEQ_CST));
}
inline int _InterlockedCompareExchange(volatile unsigned int *pointer,unsigned int value,unsigned int expected) {
    __atomic_compare_exchange_n(pointer,&expected,value,false,__ATOMIC_SEQ_CST,__ATOMIC_SEQ_CST);
    return static_cast<int>(expected);
}
inline int _InterlockedIncrement(volatile unsigned int *pointer) {
    return static_cast<int>(__atomic_add_fetch(pointer,1u,__ATOMIC_SEQ_CST));
}
inline int _InterlockedDecrement(volatile unsigned int *pointer) {
    return static_cast<int>(__atomic_sub_fetch(pointer,1u,__ATOMIC_SEQ_CST));
}
