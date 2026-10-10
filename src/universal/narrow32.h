#pragma once
#include <cstdint>

// 32-bit "narrow" pointer for decompiled structures whose layout is built from 4-byte slots (the GSC parse tree, the script VM's
// VariableUnion, ...). On Win32 the type is the plain pointer; on x64 it is a trivial 32-bit holder (usable in unions) that
// converts to/from T*. It is valid because the x64 process is kept below 4 GB (BO1_X64_LOW_VA); a pointer above 4 GB is
// reported by Bo1_NarrowFail instead of being silently truncated.
#if defined(BO1_PORT_X64)
void Bo1_NarrowFail(const void *p);

template <class T>
struct Narrow32
{
    unsigned int v;
    Narrow32 &operator=(T *p)
    {
        if (reinterpret_cast<uintptr_t>(p) >> 32)
            Bo1_NarrowFail(p);
        v = static_cast<unsigned int>(reinterpret_cast<uintptr_t>(p));
        return *this;
    }
    operator T *() const { return reinterpret_cast<T *>(static_cast<uintptr_t>(v)); }
    // C-style casts to unrelated pointer types, e.g. (unsigned __int8 *)value.vectorValue
    template <class U>
    explicit operator U *() const { return (U *)reinterpret_cast<T *>(static_cast<uintptr_t>(v)); }
    T *operator->() const { return reinterpret_cast<T *>(static_cast<uintptr_t>(v)); }
};
#define BO1_NARROW(T) Narrow32<T>
#else
#define BO1_NARROW(T) T *
#endif
