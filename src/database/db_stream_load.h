#include <platform/bo1_native_width.h>
#pragma once

struct StreamDelayInfo // sizeof=0x8
{                                       // XREF: .data:StreamDelayInfo * g_streamDelayArray/r
    const void *ptr;                    // XREF: Load_Stream(bool,void const *,int)+CE/w
                                        // Load_DelayStream(void)+26/r
    int size;                           // XREF: Load_Stream(bool,void const *,int)+DD/w
                                        // Load_DelayStream(void)+33/r
};

void __cdecl Load_Stream(bool atStreamStart, unsigned __int8 *ptr, int size);
void __cdecl Load_DelayStream();
// FastFile offset → host pointer. Implementations write pointer-width values.
// Win32 call sites still pass (unsigned int *); both overloads share one body.
void __cdecl DB_ConvertOffsetToAlias(void *field);
void __cdecl DB_ConvertOffsetToPointer(void *field);
inline void __cdecl DB_ConvertOffsetToAlias(unsigned int *data)
{
    DB_ConvertOffsetToAlias(static_cast<void *>(data));
}
inline void __cdecl DB_ConvertOffsetToPointer(unsigned int *data)
{
    DB_ConvertOffsetToPointer(static_cast<void *>(data));
}
void __cdecl Load_XStringCustom(const char **str);
void __cdecl Load_TempStringCustom(const char **str);
#if BO1_NATIVE_64BIT
#endif
