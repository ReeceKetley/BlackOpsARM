#pragma once
// Lightweight declarations for the genuine startup-service translation units.
// Remaining filesystem, localization, expression and server dependencies are
// real application services, not successful replacement implementations.
#include <universal/q_shared.h>
#include <universal/com_errors.h>
#include <universal/q_parse.h>
#include <universal/com_shared.h>
#include <universal/com_hash.h>
#include <qcommon/cmd.h>
#include <qcommon/dvar_cmds.h>
#include <platform/android/thread_services.h>
#include <platform/critical_section_services.h>
#include <platform/fast_critical_section.h>
#include <clientscript/cscr_main.h>
#include <stringed/stringed_hooks.h>
#include <universal/com_files.h>
#include <database/db_registry_add_api.h>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <platform/android/legacy_crt.h>

void Com_Printf(int channel, const char *fmt, ...);
void Com_DPrintf(int channel, const char *fmt, ...);
void Com_PrintError(int channel, const char *fmt, ...);
void Com_PrintWarning(int channel, const char *fmt, ...);
void __cdecl Com_PrintMessage(int channel, char *msg, int error);
void Com_Error(errorParm_t code, const char *fmt, ...);
bool __cdecl Com_LogFileOpen();
const char *__cdecl Com_PlayerConfigName();
void __cdecl Com_PlayerConfigPath(char *path, int size);
extern const dvar_t *com_sv_running;
extern const dvar_t *com_logfile;
extern const dvar_t *bo1_zombies;
char *__cdecl CopyString(char *in, const char *name, int type, scriptInstance_t inst);
void __cdecl FreeString(const char *str, int type, scriptInstance_t inst);
void __cdecl Scr_MonitorCommand(const char *text, scriptInstance_t inst);
bool __cdecl Sys_IsServerThread();
unsigned int __cdecl Sys_Milliseconds();
void __cdecl CL_ForwardCommandToServer(int localClientNum, const char *string);
int __cdecl SV_GameCommand();
int __cdecl SV_Cmd_Argc();
const char *__cdecl SV_Cmd_Argv(int arg);
void __cdecl SV_WaitServer();
void __cdecl Con_Restricted_ExecuteBuf(int localClientNum, int controllerIndex, char *buffer);
bool __cdecl Key_IsValidGamePadChar(char key);
bool __cdecl DB_IsMinimumFastFileLoaded();
XAssetHeader __cdecl DB_FindXAssetHeader(XAssetType type, char *name, bool errorIfMissing, int waitTime);
char *__cdecl GetExpressionResultStringCompile(int localClientNum, itemDef_s *item,
    const char **text, void *compileBuffer, int compileBufferSize);

extern cmd_function_s *cmd_functions;
extern CmdText cmd_textArray[1];
extern ParseThreadInfo g_parse[4];
extern FastCriticalSection g_dvarCritSect;
extern int dvarCount;
extern dvar_s dvarPool[4096];
extern const dvar_s *dvar_cheats;
extern const dvar_s *dvar_restoreDvarsOnLive;
extern int logfile;
