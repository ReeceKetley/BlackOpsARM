#pragma once
#include <universal/critical_sections.h>
#include <universal/com_errors.h>
#include <universal/q_shared.h>
#include <clientscript/cscr_main.h>

// Native sections: script string, memory tree and DBHASH.
#include <platform/critical_section_services.h>
bool __cdecl Sys_IsServerThread();
void Com_Printf(int channel,const char*format,...);
void Com_PrintError(int channel,const char*format,...);
void Com_Error(errorParm_t,const char*format,...);
void __cdecl Scr_DumpScriptThreads(scriptInstance_t);
void __cdecl Scr_DumpScriptVariablesDefault(scriptInstance_t);
void __cdecl Scr_TerminalError(const char*,scriptInstance_t);
