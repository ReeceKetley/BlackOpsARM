#pragma once
#include <cstdint>

// 32-bit "narrow" pointer for decompiled structures whose layout is built from 4-byte slots (the GSC parse tree, the script VM's
// VariableUnion, the compiled code stream, ...). On Win32 the type is the plain pointer.
//
// On x64 a narrow pointer is a 32-bit handle. Two encodings exist:
//  * BO1_NARROW_HANDLES = 0: the raw address (valid only while the process stays below 4 GB, see BO1_X64_LOW_VA);
//  * BO1_NARROW_HANDLES = 1: a region-tagged handle that works at any address (Windows ASLR, Android):
//        bits 31..24 = chunk index (1..255), bits 23..0 = offset inside a 16 MB-aligned chunk of the address space, 0 = null.
//    Chunks are registered the first time a pointer into them is encoded (Bo1_NarrowEncodeSlow); decoding is one table load.
//
// All narrow pointers - the Narrow32<T> members and the raw 32-bit slots in the code stream - go through
// Bo1_NarrowEncode / Bo1_NarrowDecode so the two encodings can be switched in one place.
#ifndef BO1_NARROW_HANDLES
#define BO1_NARROW_HANDLES 1
#endif

#if defined(BO1_PORT_X64)

void Bo1_NarrowFail(const void *p);

#if BO1_NARROW_HANDLES
extern uintptr_t g_bo1NarrowBase[256];
extern volatile uint64_t g_bo1NarrowKey[2048];
extern volatile uint8_t g_bo1NarrowIdx[2048];
unsigned int Bo1_NarrowEncodeSlow(const void *p);

inline unsigned int Bo1_NarrowEncode(const void *p)
{
    const uintptr_t a = reinterpret_cast<uintptr_t>(p);
    if (!a)
        return 0;
    const uint64_t chunk = static_cast<uint64_t>(a >> 24) + 1;
    unsigned int i = static_cast<unsigned int>((chunk * 0x9E3779B97F4A7C15ull) >> 53);
    for (unsigned int n = 0; n < 2048; ++n, i = (i + 1) & 2047)
    {
        const uint64_t k = g_bo1NarrowKey[i];
        if (k == chunk)
            return (static_cast<unsigned int>(g_bo1NarrowIdx[i]) << 24) | static_cast<unsigned int>(a & 0xFFFFFF);
        if (!k)
            break;
    }
    return Bo1_NarrowEncodeSlow(p);
}

inline void *Bo1_NarrowDecode(unsigned int v)
{
    if (!v)
        return nullptr;
    return reinterpret_cast<void *>(g_bo1NarrowBase[v >> 24] + (v & 0xFFFFFF));
}
#else
inline unsigned int Bo1_NarrowEncode(const void *p)
{
    if (reinterpret_cast<uintptr_t>(p) >> 32)
        Bo1_NarrowFail(p);
    return static_cast<unsigned int>(reinterpret_cast<uintptr_t>(p));
}

inline void *Bo1_NarrowDecode(unsigned int v)
{
    return reinterpret_cast<void *>(static_cast<uintptr_t>(v));
}
#endif

template <class T>
struct Narrow32
{
    unsigned int v;
    Narrow32 &operator=(T *p)
    {
        v = Bo1_NarrowEncode(p);
        return *this;
    }
    operator T *() const { return static_cast<T *>(Bo1_NarrowDecode(v)); }
    // C-style casts to unrelated pointer types, e.g. (unsigned __int8 *)value.vectorValue
    template <class U>
    explicit operator U *() const { return (U *)static_cast<T *>(Bo1_NarrowDecode(v)); }
    T *operator->() const { return static_cast<T *>(Bo1_NarrowDecode(v)); }
    // the raw 32 bits, for slots that are copied or compared without being dereferenced
    unsigned int bits() const { return v; }
    void setBits(unsigned int b) { v = b; }
};
#define BO1_NARROW(T) Narrow32<T>
// raw slot bits of a narrow pointer member (copied/compared without dereferencing)
template <class T> inline unsigned int Bo1_NarrowBits(const Narrow32<T> &n) { return n.bits(); }
template <class T> inline void Bo1_NarrowSetBits(Narrow32<T> &n, unsigned int b) { n.setBits(b); }
// A pointer stored in a raw 32-bit slot (code stream, 4-byte struct fields): Win32 is the plain address.
#define BO1_NARROW_ENCODE(p) Bo1_NarrowEncode(p)
#define BO1_NARROW_DECODE(T, v) (static_cast<T *>(Bo1_NarrowDecode(v)))
#else
#define BO1_NARROW(T) T *
template <class T> inline unsigned int Bo1_NarrowBits(T *const &p) { return static_cast<unsigned int>(reinterpret_cast<uintptr_t>(p)); }
template <class T> inline void Bo1_NarrowSetBits(T *&p, unsigned int b) { p = reinterpret_cast<T *>(static_cast<uintptr_t>(b)); }
#define BO1_NARROW_ENCODE(p) (static_cast<unsigned int>(reinterpret_cast<uintptr_t>(p)))
#define BO1_NARROW_DECODE(T, v) (reinterpret_cast<T *>(static_cast<uintptr_t>(v)))
#endif
