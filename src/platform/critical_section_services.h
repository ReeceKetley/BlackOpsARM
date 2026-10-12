#pragma once
#include <universal/critical_sections.h>
void __cdecl Sys_InitializeCriticalSections();
void __cdecl Sys_EnterCriticalSection(CriticalSection);
bool __cdecl Sys_TryEnterCriticalSection(CriticalSection);
void __cdecl Sys_LeaveCriticalSection(CriticalSection);
