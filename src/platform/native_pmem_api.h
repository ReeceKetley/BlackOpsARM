#pragma once
// Narrow declarations for the genuine PMem implementation; native OS memory
// behavior is supplied by the entry integration, never a fabricated allocation.
#include <platform/common_boot_api.h>
#include <qcommon/mem_track.h>
#include <cstddef>
char *GetCommandLineA();
void *VirtualAlloc(void *address, std::size_t size, unsigned int allocationType, unsigned int protection);
int VirtualFree(void *address, std::size_t size, unsigned int freeType);
void tlPrintf(const char *format, ...);
void __cdecl Sys_OutOfMemErrorInternal(const char *filename, int line);
