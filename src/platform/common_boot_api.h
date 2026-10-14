#pragma once
// Canonical declarations for the complete genuine common startup bodies.
// This header supplies no substitute startup sequence or successful endpoints.
#include <platform/startup_services_api.h>
#include <database/db_zone_info.h>
void DB_InitThread();
void DB_LoadXAssets(XZoneInfo *zones, unsigned int count, int sync);
void DB_SyncXAssets();
#include <universal/thread_data.h>
#include <client_mp/client_static.h>
#include <client/print_destinations.h>
#include <universal/physicalmemory.h>
#include <universal/mem_userhunk.h>
#include <universal/com_buildinfo.h>
#include <universal/assertive.h>
#include <clientscript/script_variable_limits.h>
#include <xanim/xanim_limits.h>
#include <platform/win32_compat/win32_types.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
void __cdecl BG_EmblemsInit();
void __cdecl CG_SndGameReset();
void __cdecl CL_ConsolePrint(int localClientNum, int channel, const char *txt, int duration, int pixelWidth, int flags);
void __cdecl CL_Init(int localClientNum);
void __cdecl CL_InitDedicated();
void __cdecl CL_InitGamepadAxisBindings();
void __cdecl CL_InitGamepadCommands();
void __cdecl CL_InitKeyCommands();
void __cdecl CL_InitOnceForAllClients();
void __cdecl CL_InitRenderer();
void __cdecl CL_StartHunkUsers();
void COM_PlayIntroMovies();
void __cdecl Cbuf_Execute(int localClientNum, int controllerIndex);
void __cdecl Cbuf_ExecuteBuffer(int localClientNum, int controllerIndex, char *buffer);
void __cdecl Cbuf_Init();
void __cdecl Cmd_AddCommandInternal(const char *cmdName, void (__cdecl *function)(), cmd_function_s *allocedCmd);
int __cdecl Cmd_Argc();
const char *__cdecl Cmd_Argv(int argIndex);
void __cdecl Cmd_EndTokenizedString();
void __cdecl Cmd_Init();
void __cdecl Cmd_TokenizeString(const char *text_in);
void Com_AddStartupCommands();
void __cdecl Com_Assert_f();
unsigned int __cdecl Com_CalculateStreamBuffer();
void __cdecl Com_CheckSetRecommended(int localClientNum);
void __cdecl Com_Crash_f();
void Com_DPrintf(int channel, const char *fmt, ...);
void __cdecl Com_DvarDump(int channel, const char *match);
void Com_Error(errorParm_t,const char *,...);
void Com_ErrorCleanup();
void __cdecl Com_Error_f();
void __cdecl Com_Freeze_f();
const char *__cdecl Com_GetBuildDisplayName();
const char *__cdecl Com_GetBuildName();
char *__cdecl Com_GetBuildVersion();
void __cdecl Com_Init(char *commandLine);
void __cdecl Com_InitClientGameStates();
void __cdecl Com_InitCodeXAssets();
void Com_InitDvars();
void Com_InitHunkMemory();
void __cdecl Com_InitUIAndCommonXAssets();
void __cdecl Com_Init_Try_Block_Function(char *commandLine);
void __cdecl Com_LoadFrontEnd();
void Com_LoadUiFastFile();
int __cdecl Com_LocalClient_GetControllerIndex(int localClientNum);
void __cdecl Com_LocalClient_SetControllerIndex(int localClientNum, int controllerIndex);
void __cdecl Com_LocalClient_SetPrimary(int localClientNum, bool primary);
void __cdecl Com_LogPrintMessage(int channel, char *msg);
void Com_OpenLogFile();
void __cdecl Com_ParseCommandLine(char *commandLine);
const char *__cdecl Com_PlayerConfigName();
void __cdecl Com_PlayerConfigPath(char *path, int size);
void Com_PrintError(int,const char *,...);
void __cdecl Com_PrintMessage(int channel, char *msg, int error);
void Com_PrintWarning(int channel, const char *fmt, ...);
void Com_Printf(int,const char *,...);
void __cdecl Com_SetScriptSettings();
void __cdecl Com_StartupConfigs(int localClientNum);
bool __cdecl Com_StartupProcessSetCommand(int lineIndex, const char *match);
void __cdecl Com_StartupVariable(const char *match);
int Com_sprintf(char *dest, unsigned int size, const char *fmt, ...);
void __cdecl Con_InitChannels();
bool __cdecl Con_IsChannelVisible(print_msg_dest_t dest, unsigned int channel, int errorflags);
void __cdecl Con_Restricted_InitLists();
void __cdecl Con_Restricted_SecureConfigs();
void __cdecl DB_SetInitializing(bool inUse);
void __cdecl DDL_Init();
void __cdecl DObjInit();
void __cdecl Demo_InitFileHandlerSystem();
void __cdecl Dvar_ClearModified(const dvar_s *dvar);
const dvar_s *__cdecl Dvar_FindVar(const char *dvarName);
bool __cdecl Dvar_GetBool(const char *dvarName);
const char *__cdecl Dvar_GetString(const char *);
char __cdecl Dvar_IsValidName(const char *dvarName);
void __cdecl Dvar_SetA_f();
void __cdecl Dvar_SetFromString(dvar_s *dvar, const char *string);
void __cdecl Dvar_SetInt(dvar_s *dvar, int value);
void __cdecl Dvar_SetString(dvar_s *dvar, const char *value);
void __cdecl Dvar_Set_f();
void __cdecl Expression_Init();
void __cdecl FS_Flush(int f);
void __cdecl FS_InitFilesystem(bool allow_devraw);
bool __cdecl FS_Initialized();
unsigned int __cdecl FS_WriteToDemo(char *buffer, unsigned int len, int h);
void __cdecl G_RegisterRegisterToolDvars();
void __cdecl Hunk_UserStartup();
void __cdecl IK_InitSystem();
int __cdecl I_strcmp(const char *s0, const char *s1);
int __cdecl I_stricmp(const char *,const char *);
const char *__cdecl I_stristr(const char *,const char *);
void I_strncat(char *dest, int size, const char *src);
void I_strncpyz(char *dest, const char *src, int destsize);
void __cdecl LiveSteam_Init();
void __cdecl Live_Init();
void Live_LogInProfileLocally_SP(int controllerIndex);
void __cdecl Monkey_ComPrintHook(const char *msg);
bool __cdecl Monkey_IsRunning();
void __cdecl Monkey_Start();
void __cdecl NET_Init();
void __cdecl Netchan_Init(__int16 port);
void __cdecl PC_InitSigninState();
void __cdecl PMem_BeginAlloc(const char *name, unsigned int allocType, EMemTrack memTrack);
void __cdecl PMem_EndAlloc(const char *name, unsigned int allocType);
void __cdecl PMem_Init();
void __cdecl Playlist_Init();
void __cdecl Pregame_RegisterDvars();
void __cdecl ProfLoad_Init();
void __cdecl RMsg_Init();
void __cdecl R_BeginRemoteScreenUpdate();
void __cdecl R_Cinematic_Init();
void __cdecl R_EndRemoteScreenUpdate(void (__cdecl *pumpfunc)());
void __cdecl R_InitThreads();
void __cdecl R_InitWorkerThreads();
void __cdecl R_StreamSetInitData(int buffer_size);
void __cdecl SEH_UpdateLanguageInfo();
void __cdecl SL_Init(scriptInstance_t inst);
void __cdecl SND_Init();
void __cdecl SV_Init();
void __cdecl SV_InitServerThread();
void __cdecl Scr_Init(scriptInstance_t inst);
void __cdecl Scr_InitVariables(scriptInstance_t inst);
void __cdecl Scr_Settings(int developer, int developer_script, int abort_on_error, scriptInstance_t inst);
char __cdecl Stream_Init();
void __cdecl Swap_Init();
bool __cdecl Sys_DebugSocketReady(int channel);
void __cdecl Sys_EndWriteDebugSocket();
void __cdecl Sys_EnterCriticalSection(CriticalSection);
void    Sys_Error(char *error, ...);
unsigned int __cdecl Sys_GetCpuCount();
unsigned int __cdecl Sys_GetDefaultWorkerThreadsCount();
void *__cdecl Sys_GetValue(int valueIndex);
void __cdecl Sys_Init();
bool __cdecl Sys_IsHeadless();
int __cdecl Sys_IsRemoteDebugServer();
void __cdecl Sys_LeaveCriticalSection(CriticalSection);
void __cdecl Sys_LoadingKeepAlive();
// Common frame calls the platform hotkey owner; native implementation remains
// a real platform dependency, not a successful startup fallback.
void __cdecl Sys_UpdateHotkeyBlock();
unsigned int __cdecl Sys_Milliseconds();
void __cdecl Sys_WriteDebugSocketMessageType(unsigned __int8 type);
void __cdecl Sys_WriteDebugSocketString(char *text);
void __cdecl TaskManager2_Init();
void __cdecl UI_LoadArenas();
void __cdecl UI_LoadCustomMatchGameTypes();
void __cdecl UI_LoadMaps();
bool __cdecl Voice_Init();
void __cdecl XAnimInit();
const dvar_s *__cdecl _Dvar_RegisterBool(const char *dvarName, bool value, unsigned __int16 flags, const char *description);
const dvar_s *__cdecl _Dvar_RegisterEnum(
                const char *dvarName,
                const char **valueList,
                int defaultIndex,
                unsigned __int16 flags,
                const char *description);
const dvar_s *__cdecl _Dvar_RegisterFloat(
                const char *dvarName,
                float value,
                float min,
                float max,
                unsigned __int16 flags,
                const char *description);
const dvar_s *__cdecl _Dvar_RegisterInt(
                const char *dvarName,
                int value,
                int min,
                int max,
                unsigned __int16 flags,
                const char *description);
const dvar_s *__cdecl _Dvar_RegisterString(
                const char *dvarName,
                const char *value,
                unsigned __int16 flags,
                const char *description);
void __cdecl dwInit();
void mjpeg_initonce();
char *va(const char *,...);
static void Com_InitContentPacks_SP();
void Platform_Print(char *message);
bo1::win32::BOOL QueryPerformanceCounter(bo1::win32::LARGE_INTEGER *counter);
extern const dvar_t *sv_mapname;
extern const dvar_t *com_developer;
extern const dvar_s *r_smp_worker_threads;
void __cdecl Com_Quit_f();
void __cdecl Com_WriteConfig_f();
void __cdecl Com_WriteKeyConfig_f();
void __cdecl Com_SaveKeys_f();
void __cdecl Com_RestoreKeys_f();
void __cdecl Com_WriteDefaults_f();
extern const dvar_t *onlinegame;
extern const dvar_t *com_statmon;
char __cdecl DB_FileExists(const char *zoneName, FF_DIR source);
