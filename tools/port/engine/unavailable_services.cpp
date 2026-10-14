// Integration-only fail-closed endpoints. These never report implementation,
// readiness, completed initialization or recovered state for absent services.
#include <platform/native_pmem_api.h>
[[noreturn]] static void unavailable(const char *name) {
    std::fprintf(stderr, "ENGINE ENTRY unsupported dependency: %s\n", name);
    std::fflush(stderr);
    std::_Exit(78);
}
#if !defined(BO1_NATIVE_FULL_DATABASE)
void Com_Frame(){unavailable("Com_Frame");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
void __cdecl BG_EmblemsInit() {unavailable("BG_EmblemsInit");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
void __cdecl CG_SndGameReset() {unavailable("CG_SndGameReset");}
#endif


#if !defined(BO1_NATIVE_FULL_DATABASE)
void __cdecl CL_Init(int localClientNum) {unavailable("CL_Init");}
void __cdecl CL_InitDedicated() {unavailable("CL_InitDedicated");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
void __cdecl CL_InitGamepadAxisBindings() {unavailable("CL_InitGamepadAxisBindings");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
void __cdecl CL_InitGamepadCommands() {unavailable("CL_InitGamepadCommands");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
void __cdecl CL_InitKeyCommands() {unavailable("CL_InitKeyCommands");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
void __cdecl CL_InitOnceForAllClients() {unavailable("CL_InitOnceForAllClients");}

void __cdecl CL_InitRenderer() {unavailable("CL_InitRenderer");}

void __cdecl CL_StartHunkUsers() {unavailable("CL_StartHunkUsers");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
void __cdecl Cbuf_Execute(int localClientNum, int controllerIndex) {unavailable("Cbuf_Execute");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
unsigned int __cdecl Com_CalculateStreamBuffer() {unavailable("Com_CalculateStreamBuffer");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
void __cdecl Com_CheckSetRecommended(int localClientNum) {unavailable("Com_CheckSetRecommended");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
void Com_Error(errorParm_t,const char *,...) {unavailable("Com_Error");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
void Com_ErrorCleanup() {unavailable("Com_ErrorCleanup");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
void __cdecl Com_InitClientGameStates() {unavailable("Com_InitClientGameStates");}
#endif


#if !defined(BO1_NATIVE_FULL_DATABASE)
void Com_InitHunkMemory() {unavailable("Com_InitHunkMemory");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
void __cdecl Com_InitUIAndCommonXAssets() {unavailable("Com_InitUIAndCommonXAssets");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
void __cdecl Com_LoadFrontEnd() {unavailable("Com_LoadFrontEnd");}

void Com_LoadUiFastFile() {unavailable("Com_LoadUiFastFile");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
int __cdecl Com_LocalClient_GetControllerIndex(int localClientNum) {unavailable("Com_LocalClient_GetControllerIndex");}

void __cdecl Com_LocalClient_SetControllerIndex(int localClientNum, int controllerIndex) {unavailable("Com_LocalClient_SetControllerIndex");}

void __cdecl Com_LocalClient_SetPrimary(int localClientNum, bool primary) {unavailable("Com_LocalClient_SetPrimary");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
void Com_OpenLogFile() {unavailable("Com_OpenLogFile");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
void __cdecl Com_StartupConfigs(int localClientNum) {unavailable("Com_StartupConfigs");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
void __cdecl Con_InitChannels() {unavailable("Con_InitChannels");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
bool __cdecl Con_IsChannelVisible(print_msg_dest_t dest, unsigned int channel, int errorflags) {unavailable("Con_IsChannelVisible");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
void __cdecl Con_Restricted_InitLists() {unavailable("Con_Restricted_InitLists");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
void __cdecl Con_Restricted_SecureConfigs() {unavailable("Con_Restricted_SecureConfigs");}
#endif


#if !defined(BO1_NATIVE_FULL_DATABASE)
void __cdecl DDL_Init() {unavailable("DDL_Init");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
void __cdecl DObjInit() {unavailable("DObjInit");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
void __cdecl Demo_InitFileHandlerSystem() {unavailable("Demo_InitFileHandlerSystem");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
void __cdecl Expression_Init() {unavailable("Expression_Init");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
void __cdecl FS_Flush(int f) {unavailable("FS_Flush");}

void __cdecl FS_InitFilesystem(bool allow_devraw) {unavailable("FS_InitFilesystem");}

bool __cdecl FS_Initialized() {unavailable("FS_Initialized");}

unsigned int __cdecl FS_WriteToDemo(char *buffer, unsigned int len, int h) {unavailable("FS_WriteToDemo");}
#endif


#if !defined(BO1_NATIVE_FULL_DATABASE)
void __cdecl Hunk_UserStartup() {unavailable("Hunk_UserStartup");}
#endif



#if !defined(BO1_NATIVE_FULL_DATABASE)
void __cdecl Live_Init() {unavailable("Live_Init");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
void Live_LogInProfileLocally_SP(int controllerIndex) {unavailable("Live_LogInProfileLocally_SP");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
void __cdecl Monkey_ComPrintHook(const char *msg) {unavailable("Monkey_ComPrintHook");}
#endif


#if !defined(BO1_NATIVE_FULL_DATABASE)
void __cdecl Monkey_Start() {unavailable("Monkey_Start");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
void __cdecl NET_Init() {unavailable("NET_Init");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
void __cdecl Netchan_Init(__int16 port) {unavailable("Netchan_Init");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
void __cdecl PC_InitSigninState() {unavailable("PC_InitSigninState");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
void __cdecl Playlist_Init() {unavailable("Playlist_Init");}
#endif


#if !defined(BO1_NATIVE_FULL_DATABASE)
void __cdecl ProfLoad_Init() {unavailable("ProfLoad_Init");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
void __cdecl RMsg_Init() {unavailable("RMsg_Init");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
void __cdecl R_BeginRemoteScreenUpdate() {unavailable("R_BeginRemoteScreenUpdate");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
void __cdecl R_Cinematic_Init() {unavailable("R_Cinematic_Init");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
void __cdecl R_EndRemoteScreenUpdate(void (__cdecl *pumpfunc)()) {unavailable("R_EndRemoteScreenUpdate");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
void __cdecl R_InitThreads() {unavailable("R_InitThreads");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
void __cdecl R_InitWorkerThreads() {unavailable("R_InitWorkerThreads");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
void __cdecl R_StreamSetInitData(int buffer_size) {unavailable("R_StreamSetInitData");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
void __cdecl SEH_UpdateLanguageInfo() {unavailable("SEH_UpdateLanguageInfo");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
void __cdecl SND_Init() {unavailable("SND_Init");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
void __cdecl SV_Init() {unavailable("SV_Init");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
void __cdecl SV_InitServerThread() {unavailable("SV_InitServerThread");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
void __cdecl Scr_Init(scriptInstance_t inst) {unavailable("Scr_Init");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
void __cdecl Scr_InitVariables(scriptInstance_t inst) {unavailable("Scr_InitVariables");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
void __cdecl Scr_Settings(int developer, int developer_script, int abort_on_error, scriptInstance_t inst) {unavailable("Scr_Settings");}
#endif


#if !defined(BO1_NATIVE_FULL_DATABASE)
bool __cdecl Sys_DebugSocketReady(int channel) {unavailable("Sys_DebugSocketReady");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
void __cdecl Sys_EndWriteDebugSocket() {unavailable("Sys_EndWriteDebugSocket");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
void    Sys_Error(char *error, ...) {unavailable("Sys_Error");}
#endif



#if !defined(BO1_NATIVE_FULL_DATABASE)
void __cdecl Sys_Init() {unavailable("Sys_Init");}
#endif



#if !defined(BO1_NATIVE_FULL_DATABASE)
void __cdecl Sys_LoadingKeepAlive() {unavailable("Sys_LoadingKeepAlive");}
#endif


#if !defined(BO1_NATIVE_FULL_DATABASE)
void __cdecl Sys_WriteDebugSocketMessageType(unsigned __int8 type) {unavailable("Sys_WriteDebugSocketMessageType");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
void __cdecl Sys_WriteDebugSocketString(char *text) {unavailable("Sys_WriteDebugSocketString");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
void __cdecl TaskManager2_Init() {unavailable("TaskManager2_Init");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
void __cdecl UI_LoadArenas() {unavailable("UI_LoadArenas");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
void __cdecl UI_LoadCustomMatchGameTypes() {unavailable("UI_LoadCustomMatchGameTypes");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
void __cdecl UI_LoadMaps() {unavailable("UI_LoadMaps");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
bool __cdecl Voice_Init() {unavailable("Voice_Init");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
void __cdecl XAnimInit() {unavailable("XAnimInit");}
#endif

void __cdecl dwInit() {unavailable("dwInit");}

#if !defined(BO1_NATIVE_FULL_DATABASE)
void mjpeg_initonce() {unavailable("mjpeg_initonce");}
#endif

#include <universal/com_fileaccess.h>
#if !defined(BO1_NATIVE_FULL_DATABASE)
bool Assert_MyHandler(const char*,int,int,const char*,...){unavailable("Assert_MyHandler");}
#endif
#if !defined(BO1_NATIVE_FULL_DATABASE)
void Com_Quit_f(){unavailable("Com_Quit_f");}
#endif
#if !defined(BO1_NATIVE_FULL_DATABASE)
void Com_WriteConfig_f(){unavailable("Com_WriteConfig_f");}
#endif
#if !defined(BO1_NATIVE_FULL_DATABASE)
void Com_WriteKeyConfig_f(){unavailable("Com_WriteKeyConfig_f");}
#endif
#if !defined(BO1_NATIVE_FULL_DATABASE)
void Com_SaveKeys_f(){unavailable("Com_SaveKeys_f");}
#endif
#if !defined(BO1_NATIVE_FULL_DATABASE)
void Com_RestoreKeys_f(){unavailable("Com_RestoreKeys_f");}
#endif
#if !defined(BO1_NATIVE_FULL_DATABASE)
void Com_WriteDefaults_f(){unavailable("Com_WriteDefaults_f");}
#endif
#if !defined(BO1_NATIVE_FULL_DATABASE)
char DB_FileExists(const char*,FF_DIR){unavailable("DB_FileExists");}
#endif
#if !defined(BO1_NATIVE_FULL_DATABASE)
void Sys_OutOfMemErrorInternal(const char*,int){unavailable("Sys_OutOfMemErrorInternal");}
#endif
#if !defined(BO1_NATIVE_FULL_DATABASE)
void tlPrintf(const char*,...){unavailable("tlPrintf");}
#endif
#if !defined(BO1_NATIVE_FULL_DATABASE)
int SV_Cmd_Argc(){unavailable("SV_Cmd_Argc");}
const char *SV_Cmd_Argv(int){unavailable("SV_Cmd_Argv");}
#endif
#if !defined(BO1_NATIVE_FULL_DATABASE)
char *SEH_LocalizeTextMessage(const char*,const char*,msgLocErrType_t){unavailable("SEH_LocalizeTextMessage");}
#endif
#if !defined(BO1_NATIVE_FULL_DATABASE)
int SV_GameCommand(){unavailable("SV_GameCommand");}
#endif
#if !defined(BO1_NATIVE_FULL_DATABASE)
void CL_ForwardCommandToServer(int,const char*){unavailable("CL_ForwardCommandToServer");}
#endif
#if !defined(BO1_NATIVE_FULL_DATABASE)
void SV_WaitServer(){unavailable("SV_WaitServer");}
#endif
#if !defined(BO1_NATIVE_FULL_DATABASE)
void Scr_DumpScriptThreads(scriptInstance_t){unavailable("Scr_DumpScriptThreads");}
void Scr_DumpScriptVariablesDefault(scriptInstance_t){unavailable("Scr_DumpScriptVariablesDefault");}
#endif
#if !defined(BO1_NATIVE_FULL_DATABASE)
const char *SEH_StringEd_GetString(char*){unavailable("SEH_StringEd_GetString");}
#endif
#if !defined(BO1_NATIVE_FULL_DATABASE)
char *GetExpressionResultStringCompile(int,itemDef_s*,const char**,void*,int){unavailable("GetExpressionResultStringCompile");}
#endif
#if !defined(BO1_NATIVE_FULL_DATABASE)
XAssetHeader DB_FindXAssetHeader(XAssetType,char*,bool,int){unavailable("DB_FindXAssetHeader");}
bool DB_IsMinimumFastFileLoaded(){unavailable("DB_IsMinimumFastFileLoaded");}
#endif
#if !defined(BO1_NATIVE_FULL_DATABASE)
int FS_ReadFile(const char*,void**){unavailable("FS_ReadFile");}
void FS_FreeFile(void*){unavailable("FS_FreeFile");}
#endif
#if !defined(BO1_NATIVE_FULL_DATABASE)
void Con_Restricted_ExecuteBuf(int,int,char*){unavailable("Con_Restricted_ExecuteBuf");}
#endif
#if !defined(BO1_NATIVE_FULL_DATABASE)
void Scr_TerminalError(const char*,scriptInstance_t){unavailable("Scr_TerminalError");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
void Con_OneTimeInit(){unavailable("Con_OneTimeInit");}
void CL_ConsolePrint_AddLine(int,int,const char*,int,int,char,int){unavailable("CL_ConsolePrint_AddLine");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
void Material_PreventOverrideTechniqueGeneration(){unavailable("Material_PreventOverrideTechniqueGeneration");}
void R_StreamUpdate_ReadTextures(){unavailable("R_StreamUpdate_ReadTextures");}
#endif
#if !defined(BO1_NATIVE_FULL_DATABASE)
void Snd_StreamUpdate(){unavailable("Snd_StreamUpdate");}
#endif

#if !defined(BO1_NATIVE_FULL_DATABASE)
void DB_InitThread(){unavailable("DB_InitThread");}
void DB_LoadXAssets(XZoneInfo*,unsigned int,int){unavailable("DB_LoadXAssets");}
void DB_SyncXAssets(){unavailable("DB_SyncXAssets");}
#endif
