#pragma once
#include <universal/critical_sections.h>
#include <universal/com_errors.h>
#include <universal/q_shared.h>
#include <clientscript/cscr_main.h>

// Native pool boundary supports exactly SCRIPT_STRING and MEMORY_TREE. Other
// sections must fail explicitly until their real platform implementations exist.
void __cdecl Sys_InitializeCriticalSections();
void __cdecl Sys_EnterCriticalSection(CriticalSection);
bool __cdecl Sys_TryEnterCriticalSection(CriticalSection);
void __cdecl Sys_LeaveCriticalSection(CriticalSection);
bool __cdecl Sys_IsServerThread();
void Com_Printf(int channel,const char*format,...);
void Com_PrintError(int channel,const char*format,...);
void Com_Error(errorParm_t,const char*format,...);
void __cdecl Scr_DumpScriptThreads(scriptInstance_t);
void __cdecl Scr_DumpScriptVariablesDefault(scriptInstance_t);
void __cdecl Scr_TerminalError(const char*,scriptInstance_t);
