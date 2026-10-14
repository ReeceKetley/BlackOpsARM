#pragma once

void *__cdecl Z_VirtualReserve(int size);
char *__cdecl Z_TryVirtualAlloc(int size, const char *name, int type);
char *__cdecl Z_VirtualAlloc(int size, const char *name, int type);
void __cdecl Z_VirtualCommit(char *ptr, int size, int type);
void __cdecl Z_VirtualFree(void *ptr, int type);
void __cdecl Z_VirtualDecommit(char *ptr, int size, int type);
void __cdecl Z_Free(char *ptr, int type);
unsigned int *__cdecl Z_Malloc(int size, const char *name, int type);
