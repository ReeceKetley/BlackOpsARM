#pragma once
#include <platform/bo1_native_width.h>
#include <server_mp/sv_init_mp.h>
#include <qcommon/net_address.h>
#include <gfx_d3d/vid_config.h>
#include <DemonWare/bdCore/bdSocket/bdSecurity.h>

struct Material;
struct Font_s;

struct clientLogo_t // sizeof=0x18
{                                                                             // XREF: clientStatic_t/r
        int startTime;                                            // XREF: CL_DrawLogo(int)+42/r
                                                                                // CL_PlayLogo_f+1E2/w
        int duration;                                             // XREF: CL_DrawLogo(int):loc_564A0E/r
                                                                                // CL_DrawLogo(int)+7F/r ...
        int fadein;                                                 // XREF: CL_DrawLogo(int)+4E/r
                                                                                // CL_DrawLogo(int)+5B/r ...
        int fadeout;                                                // XREF: CL_DrawLogo(int)+74/r
                                                                                // CL_DrawLogo(int)+8B/r ...
        Material *material[2];                            // XREF: CL_DrawLogo(int)+15A/r
                                                                                // CL_DrawLogo(int)+19F/r ...
};

struct XNADDR // sizeof=0x19
{                                                                             // XREF: serverInfo_t/r
    unsigned __int8 addrBuff[25];             // XREF: SND_SurfaceTypeToReflectance+19C/o
};

struct __declspec(align(4)) serverInfo_t // sizeof=0x178
{                                                                             // XREF: .data:s_quickmatchCandidates/r
        XNADDR xnaddr;                                            // XREF: SND_SurfaceTypeToReflectance+19C/o
        bdSecurityKey xnkey;                                // XREF: CL_LanConnect_f+13/o
                                                                                // CL_LanConnect_f+EC/o ...
        bdSecurityID xnkid;                                 // XREF: PM_Weapon_FireWeapon+17D/o
                                                                                // CL_LanConnect_f+1E/o ...
        // padding byte
        // padding byte
        // padding byte
        int publicSlots;
        int publicSlotsUsed;
        int privateSlots;
        int privateSlotsUsed;
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        unsigned __int64 nonce;                         // XREF: SND_SurfaceTypeToReflectance+164/o
                                                                                // .rdata:bits_ac_luminance/o ...
        netadr_t adr;                                             // XREF: CL_LanConnect_f+138/w
                                                                                // G_InitGrenadeEntity(gentity_s *,gentity_s *)+355/o ...
        unsigned __int8 allowAnonymous;
        unsigned __int8 bPassword;
        unsigned __int8 pure;
        char consoleDisabled;
        unsigned __int8 netType;
        unsigned __int8 clients;                        // XREF: CL_QuickMatch_ChooseSession(void)+A9/r
        unsigned __int8 maxClients;
        unsigned __int8 dirty;
        char friendlyfire;
        char killcam;
        char hardcore;
        unsigned __int8 hardware;
        unsigned __int8 mod;
        unsigned __int8 voice;
        unsigned __int8 punkbuster;
        unsigned __int8 requestCount;
        __int16 minPing;                                        // XREF: SV_ServerThread+1D6/o
                                                                                // SND_SurfaceTypeToReflectance+1B0/o ...
        __int16 maxPing;
        __int16 ping;                                             // XREF: CL_QuickMatch_Init(void)+80/w
                                                                                // CL_QuickMatch_ChooseSession(void)+BA/r
        // padding byte
        // padding byte
        int pingedTime;
        unsigned __int8 gotInfo;
        char hostName[32];                                    // XREF: CG_Calc3rdPersonVehicleViewValues(int)+F49/o
                                                                                // CG_PredictPlayerState_Internal+289/o ...
        char mapName[32];                                     // XREF: CG_CompassUpdateActors(int)+5E2/o
                                                                                // TRACK_cl_main(void)+20/o ...
        char game[64];                                            // XREF: AimTarget_ProcessEntityInternal(int,centity_s const *)+1FD/o
                                                                                // AimTarget_GetTargetBounds+34D/o ...
        char gameType[16];                                    // XREF: SND_SurfaceTypeToReflectance:loc_94A4B6/o
                                                                                // SND_SurfaceTypeToReflectance+11F/o ...
        // padding byte
        // padding byte
        // padding byte
        unsigned __int64 bdUserID;                    // XREF: CG_CanSeeFriendlyHead(int,centity_s const *)+3D2/o
                                                                                // CG_ScanForCrosshairEntityInternal+30A/o ...
        int lastRequestTime;                                // XREF: CL_CACValidate_Frame(void)+25B/r
                                                                                // CL_CACValidate_Frame(void)+29C/w
        int region;
        int score;                                                    // XREF: CL_QuickMatch_ChooseSession(void)+99/r
                                                                                // Flame_Server_Trace+80/o ...
        int wagerBet;                                             // XREF: CL_QuickMatch_ChooseSession(void)+EE/r
                                                                                // CL_QuickMatch_ChooseSession(void)+108/r ...
        int playlist;                                             // XREF: VEH_TouchEntities+2E9/o
        int licensetype;
        unsigned __int8 basictraining;            // XREF: BG_ParseCommands(char const * *,animScriptItem_t *,animScriptData_t *)+524/o
                                                                                // BG_ParseCommands(char const * *,animScriptItem_t *,animScriptData_t *)+8DF/o ...
        unsigned __int8 customclassmode;
        char city[64];                                            // XREF: PM_playerTrace(pmove_t *,trace_t *,float const * const,float const * const,float const * const,float const * const,int,int)+11A/o
                                                                                // PM_Weapon_FireWeapon+18D/o ...
        char countrycode[3];                                // XREF: CG_Laser_Add_Core+1C0/o
                                                                                // CScr_BulletTrace+21/o ...
        // padding byte
        // padding byte
        // padding byte
};

struct trDebugString_t // sizeof=0x80
{
        float xyz[3];
        float color[4];
        float scale;
        char text[96];
};

struct clientDebugStringInfo_t // sizeof=0x10
{                                                                             // XREF: clientDebug_t/r
                                                                                // clientDebug_t/r ...
        int max;                                                        // XREF: CreateDebugStringsIfNeeded:loc_54DF9F/w
                                                                                // CreateDebugStringsIfNeeded+D9/w ...
        int num;                                                        // XREF: CreateDebugStringsIfNeeded+124/w
                                                                                // CreateDebugStringsIfNeeded+165/w ...
        trDebugString_t *strings;                     // XREF: CreateDebugStringsIfNeeded+6/r
                                                                                // CreateDebugStringsIfNeeded+F/r ...
        int *durations;                                         // XREF: CreateDebugStringsIfNeeded:loc_54DF0B/r
                                                                                // CreateDebugStringsIfNeeded+4D/r ...
};

struct trDebugLine_t // sizeof=0x2C
{
        float start[3];
        float end[3];
        float color[4];
        int depthTest;
};

struct clientDebugLineInfo_t // sizeof=0x10
{                                                                             // XREF: clientDebug_t/r
                                                                                // clientDebug_t/r ...
        int max;                                                        // XREF: CreateDebugLinesIfNeeded:loc_54E5C9/w
                                                                                // CreateDebugLinesIfNeeded+B3/w ...
        int num;                                                        // XREF: CreateDebugLinesIfNeeded+F5/w
                                                                                // CreateDebugLinesIfNeeded+12D/w ...
        trDebugLine_t *lines;                             // XREF: CreateDebugLinesIfNeeded+6/r
                                                                                // CreateDebugLinesIfNeeded+18/r ...
        int *durations;                                         // XREF: CreateDebugLinesIfNeeded+F/r
                                                                                // CreateDebugLinesIfNeeded+21/r ...
};

struct trDebugSphere_t // sizeof=0x28
{
        float center[3];
        float radius;
        int sideCount;
        float color[4];
        int depthTest;
};

struct clientDebugSphereInfo_t // sizeof=0x10
{                                                                             // XREF: clientDebug_t/r
                                                                                // clientDebug_t/r ...
        int max;                                                        // XREF: CreateDebugSpheresIfNeeded:loc_54E2C3/w
                                                                                // CreateDebugSpheresIfNeeded+AD/w ...
        int num;                                                        // XREF: CreateDebugSpheresIfNeeded+EF/w
                                                                                // CreateDebugSpheresIfNeeded+127/w ...
        trDebugSphere_t *spheres;                     // XREF: CreateDebugSpheresIfNeeded+6/r
                                                                                // CreateDebugSpheresIfNeeded+18/r ...
        int *durations;                                         // XREF: CreateDebugSpheresIfNeeded+F/r
                                                                                // CreateDebugSpheresIfNeeded+21/r ...
};

struct clientDebug_t // sizeof=0x98
{                                                                             // XREF: clientStatic_t/r
        int prevFromServer;
        int fromServer;                                         // XREF: CL_AddDebugString(float const * const,float const * const,float,char const *,int)+77/w
                                                                                // CL_AddDebugSphere(float const * const,float,float const * const,int,int,int)+7B/w ...
        clientDebugStringInfo_t clStrings;    // XREF: CL_AddDebugString(float const * const,float const * const,float,char const *,int):loc_54DE87/o
                                                                                // CreateDebugStringsIfNeeded+6/r ...
        clientDebugStringInfo_t svStringsBuffer;
                                                                                // XREF: CreateDebugStringsIfNeeded+56/r
                                                                                // CreateDebugStringsIfNeeded:loc_54DF73/r ...
        clientDebugStringInfo_t svStrings;    // XREF: CL_AddDebugString(float const * const,float const * const,float,char const *,int)+3E/o
                                                                                // CreateDebugStringsIfNeeded+F/r ...
        clientDebugLineInfo_t clLines;            // XREF: CL_AddDebugLine(float const * const,float const * const,float const * const,int,int):loc_54E4D7/o
                                                                                // CreateDebugLinesIfNeeded+6/r ...
        clientDebugLineInfo_t svLinesBuffer;
                                                                                // XREF: CreateDebugLinesIfNeeded+2A/r
                                                                                // CreateDebugLinesIfNeeded:loc_54E59A/r ...
        clientDebugLineInfo_t svLines;            // XREF: CL_AddDebugLine(float const * const,float const * const,float const * const,int,int)+3E/o
                                                                                // CreateDebugLinesIfNeeded+18/r ...
        clientDebugSphereInfo_t clSpheres;    // XREF: CL_AddDebugSphere(float const * const,float,float const * const,int,int,int):loc_54E1D7/o
                                                                                // CreateDebugSpheresIfNeeded+6/r ...
        clientDebugSphereInfo_t svSpheresBuffer;
                                                                                // XREF: CreateDebugSpheresIfNeeded+2A/r
                                                                                // CreateDebugSpheresIfNeeded:loc_54E297/r ...
        clientDebugSphereInfo_t svSpheres;    // XREF: CL_AddDebugSphere(float const * const,float,float const * const,int,int,int)+3E/o
};

struct gameState_t // sizeof=0x132F8
{                                                                             // XREF: clientStatic_t/r
        int stringOffsets[MAX_CONFIGSTRINGS];                        // XREF: CL_ConfigstringModified+1FF/w
                                                                                // CL_GetConfigString(int)+6A/r ...
        char stringData[65536];                         // XREF: CL_ConfigstringModified+13B/o
                                                                                // CL_ConfigstringModified+216/o ...
        int dataCount;                                            // XREF: CL_ConfigstringModified+148/w
                                                                                // CL_ConfigstringModified+1D3/r ...
        int matchUIVisibilityFlags;                 // XREF: CG_SetupGameInformation+16/r
                                                                                // CL_WriteUncompressedDemoInfo(int)+83B/r ...
};
#if !defined(BO1_PORT_X64)
static_assert(sizeof(gameState_t) == 78584);
#endif

struct clientStatic_t // sizeof=0x1CF2800
{                                                                             // XREF: .data:clientStatic_t cls/r
        int hunkUsersStarted;                             // XREF: CL_ShutdownHunkUsers(void)+8/r
                                                                                // CL_ShutdownHunkUsers(void)+73/w ...
        char servername[256];                             // XREF: CL_SetCGameTime(int)+18F/o
                                                                                // CL_MapLoading(char const *,bool)+B4/o ...
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        unsigned __int64 serveruid;                 // XREF: CL_Connect(serverInfo_t *)+34D/w
                                                                                // CL_Connect(serverInfo_t *)+359/w ...
        int rendererStarted;                                // XREF: CL_AddDebugString(float const * const,float const * const,float,char const *,int)+6/r
                                                                                // CL_AddDebugSphere(float const * const,float,float const * const,int,int,int)+6/r ...
        int soundStarted;                                     // XREF: CL_StartHunkUsers(void):loc_5645F3/r
                                                                                // Com_Init_Try_Block_Function+5F4/w
        int uiStarted;                                            // XREF: CL_GamepadButtonEvent(int,int,int,int,uint,int)+CB2/r
                                                                                // CL_GamepadButtonEvent(int,int,int,int,uint,int)+D07/r ...
        int devGuiStarted;                                    // XREF: CL_ShutdownHunkUsers(void)+1D/r
                                                                                // CL_ShutdownHunkUsers(void)+2B/w ...
        int frametime;                                            // XREF: CL_AdjustAngles+26/r
                                                                                // CL_GamepadMove:loc_5538FF/r ...
        int inputRealMsec;                                    // XREF: CL_GamepadMove+338/r
                                                                                // Com_Frame_Try_Block_Function+302/w
        int realtime;                                             // XREF: CL_FirstSnapshot(int)+63/r
                                                                                // CL_SetCGameTime(int)+221/w ...
        int realFrametime;                                    // XREF: CL_RunOncePerClientFrame(int,int)+C4/w
        clientLogo_t logo;                                    // XREF: CL_DrawLogo(int)+42/r
                                                                                // CL_DrawLogo(int)+4E/r ...
        float mapCenter[3];                                 // XREF: CL_WriteUncompressedDemoInfo(int)+75A/r
                                                                                // CL_WriteUncompressedDemoInfo(int)+76A/r ...
        int numlocalservers;
        serverInfo_t localServers[128];
        int lastServerPinged;                             // XREF: CL_UpdateDirtyPings(int,int)+66/r
                                                                                // CL_UpdateDirtyPings(int,int)+70/w ...
        int pingedServerCount;                            // XREF: CL_UpdateDirtyPings(int,int)+1B3/r
                                                                                // CL_UpdateDirtyPings(int,int)+1BB/w ...
        int totalServersParsed;
        int waitdwfindsessionsresponse;         // XREF: CL_ServersResponsePacket(MatchMakingInfo *,int,bool):loc_5693FC/w
                                                                                // CL_FindServers_f(void):def_569581/w ...
        int numrankedservers;                             // XREF: CL_GetServerList(int,serverInfo_t * *,int * *)+A3/o
                                                                                // CL_CACValidate_Frame(void)+8A/r ...
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        serverInfo_t rankedServers[20000];    // XREF: AimTarget_ProcessEntityInternal(int,centity_s const *)+1FD/o
                                                                                // AimTarget_GetTargetBounds+34D/o ...
        int numunrankedservers;                         // XREF: CL_GetServerList(int,serverInfo_t * *,int * *)+88/o
                                                                                // GetServerCounts:$LN5_114/r
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        serverInfo_t unrankedServers[20000]; // XREF: TRACK_cl_main(void)+20/o
                                                                                // CL_Shutdown(int)+26E/o ...
        int numwagerservers;                                // XREF: CL_GetServerList(int,serverInfo_t * *,int * *)+BE/o
                                                                                // GetServerCounts:$LN4_136/r
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        serverInfo_t wagerServers[20000];     // XREF: BG_ParseCommands(char const * *,animScriptItem_t *,animScriptData_t *)+524/o
                                                                                // BG_ParseCommands(char const * *,animScriptItem_t *,animScriptData_t *)+8DF/o ...
        int numfriendsservers;                            // XREF: CL_GetServerList(int,serverInfo_t * *,int * *)+D6/o
                                                                                // GetServerCounts:$LN2_138/r
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        serverInfo_t friendsServers[256];     // XREF: CL_GetServerList(int,serverInfo_t * *,int * *)+CD/o
        int numrecentservers;                             // XREF: CL_GetServerList(int,serverInfo_t * *,int * *)+106/o
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        serverInfo_t recentServers[20000];    // XREF: CG_Calc3rdPersonVehicleViewValues(int)+F49/o
                                                                                // CG_CanSeeFriendlyHead(int,centity_s const *)+3D2/o ...
        int lastFindSessionsTime;                     // XREF: CL_FindServers_f(void)+164/w
                                                                                // CL_CACValidate_Frame(void)+7C/r ...
        int numfavoriteservers;                         // XREF: CL_GetServerList(int,serverInfo_t * *,int * *)+EE/o
                                                                                // GetServerCounts:$LN3_139/r
        serverInfo_t favoriteServers[128];    // XREF: CL_GetServerList(int,serverInfo_t * *,int * *)+E5/o
        int pingUpdateSource;                             // XREF: CL_UpdateDirtyPings(int,int)+3A/w
                                                                                // CL_ServerInfoPacket(bdSecurityID *,msg_t *,int)+1E/r ...
        Material *whiteMaterial;                        // XREF: ConDraw_Box:loc_54B8D0/r
                                                                                // ConDraw_Box+C7/r ...
        Material *consoleMaterial;                    // XREF: CL_InitRenderer(void)+E9/w
                                                                                // CL_ShutdownRenderer+7D/w
        Material *spinnerMaterial;                    // XREF: CL_DrawSpinnerPhysical(float,float,float,float,float const * const)+1BD/r
                                                                                // CL_DrawSpinnerLoadbarPhysical(float,float,float,float,float const * const,float):loc_55D532/r ...
        Font_s *consoleFont;                                // XREF: Con_CheckResize(void)+100/r
                                                                                // Con_CheckResize(void)+10D/r ...
        vidConfig_t vidConfig;                            // XREF: CG_Init(int,int,int,int)+A21/r
                                                                                // CL_GetScreenDimensions(int *,int *,float *)+81/r ...
        clientDebug_t debug;                                // XREF: CL_AddDebugString(float const * const,float const * const,float,char const *,int)+3E/o
                                                                                // CL_AddDebugString(float const * const,float const * const,float,char const *,int):loc_54DE87/o ...
        int download;                                             // XREF: CL_Disconnect(int,bool)+112/r
                                                                                // CL_Disconnect(int,bool)+11B/r ...
        char downloadTempName[256];                 // XREF: CL_ReadyToSendPacket:loc_55294D/r
                                                                                // CL_ReadyToSendPacket+8B/r ...
        char downloadName[256];                         // XREF: CL_ClearStaticDownload(void)+43/w
                                                                                // CL_Disconnect(int,bool):loc_55FCF4/w ...
        int downloadNumber;
        int downloadBlock;                                    // XREF: CL_BeginDownload(char const *,char const *)+8C/w
                                                                                // CL_ParseDownload:loc_572BD8/r ...
        int downloadCount;                                    // XREF: CL_BeginDownload(char const *,char const *)+96/w
                                                                                // CL_ParseDownload+278/r ...
        int downloadSize;                                     // XREF: CL_ParseWWWDownload(int,msg_t *)+6C/w
                                                                                // CL_ParseWWWDownload(int,msg_t *):loc_571901/r ...
        char downloadList[1024];                        // XREF: CL_NextDownload(int)+15/r
                                                                                // CL_NextDownload(int):loc_560D98/o ...
        int downloadRestart;                                // XREF: CL_ClearStaticDownload(void):loc_55FAC2/w
                                                                                // CL_DownloadsComplete(int)+22/r ...
        int doVidRestart;                                     // XREF: CL_DownloadsComplete(int):loc_560AC9/r
                                                                                // CL_SystemInfoChanged(int)+1AD/w ...
        int wwwDlDisconnected;                            // XREF: CL_ClearStaticDownload(void)+3/r
                                                                                // CL_ClearStaticDownload(void)+5B/w ...
        int wwwDlInProgress;                                // XREF: CL_ClearStaticDownload(void)+51/w
                                                                                // CL_InitDownloads(int)+25/w ...
        int downloadFlags;                                    // XREF: CL_ClearStaticDownload(void)+65/w
                                                                                // CL_ParseWWWDownload(int,msg_t *)+7D/w ...
        char originalDownloadName[64];            // XREF: CL_ClearStaticDownload(void)+4A/w
                                                                                // CL_WWWDownload(void)+82/o ...
        volatile unsigned int scriptError;                     // XREF: CL_Frame(int,int)+49/o
                                                                                // Com_Error(errorParm_t,char const *,...)+1A7/o
        float debugRenderForward[3];                // XREF: CL_GetDebugViewForward(float * const)+6/r
                                                                                // CL_GetDebugViewForward(float * const)+15/r ...
        float debugRenderPos[3];                        // XREF: CL_GetDebugViewPos(float * const)+6/r
                                                                                // CL_GetDebugViewPos(float * const)+15/r ...
        int serverId;                                             // XREF: CG_InitVote(int)+4A/r
                                                                                // CL_ConfigstringModified+E3/r ...
        gameState_t gameState;                            // XREF: CG_SetupGameInformation+16/r
                                                                                // CL_ConfigstringModified+10F/o ...
        // padding byte
        // padding byte
        // padding byte
        // padding byte

        // should be implicit now with constructors
        //clientStatic_t();
};


extern clientStatic_t cls;
