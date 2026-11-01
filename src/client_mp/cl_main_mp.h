#include <platform/bo1_native_width.h>
#pragma once

#include <bgame/bg_local.h>
#include <qcommon/bitarray.h>
#include <qcommon/msg_mp.h>
#include <demo/demo_common.h>

// mod (L43): the client parseEntities ring, retail 2048 (a power of two: indices are masked with COUNT - 1). With 11-bit entity
// numbers a snapshot holds up to 2047 entities, so 2048 could not hold one delta source plus the frame parsed from it (a delta
// from two frames back of ~950 entities each left 128 slots for new entities). CL_ParseSnapshot rejects delta sources more than
// COUNT - (1 << g_entNumBits) entries old, so the frame being parsed never overwrites its unread source.
#define CL_PARSE_ENTITIES_COUNT 8192
#include <qcommon/net_chan_mp.h>
#include <client/screen_placement.h>
#include <universal/dvar.h>
#include <ui/ui_main.h>
#include <gfx_d3d/fxprimitives.h>
#include <cgame/cg_shellshock.h>
#include <ui/ui_shared.h>
#include <server/server.h>
#include <gfx_d3d/r_init.h>
#include <server_mp/sv_init_mp.h>

// zombies: capacity for SP levels; the live count is Com_GetMaxActorCorpses() (MP 8)
#define MAX_ACTOR_CORPSES MAX_ACTOR_CORPSES_SP

struct ClientArchiveData // sizeof=0x30
{                                                                             // XREF: demoInitialStateBuffer_t/r
        int serverTime;
        float origin[3];
        float velocity[3];
        int bobCycle;
        int movementDir;
        float viewangles[3];
};

struct demoInitialStateBuffer_t // sizeof=0x13010
{                                                                             // XREF: clientConnection_t/r
        unsigned __int8 compressedBuf[65536];
        int compressedBufSize;
        int serverMessageSequence;
        ClientArchiveData archive[256];
        int lastClientArchiveIndex;
        int continuousBufferStartingIndex;
};

struct demoContinuousStateBuffer_t // sizeof=0x1C
{                                                                             // XREF: clientConnection_t/r
        int serverMessageSequence;
        unsigned __int8 *msgData;
        int msgLen;
        unsigned __int8 *archive;
        int startClientArchiveIndex;
        int endClientArchiveIndex;
        int intialBufferStateIndex;
};

struct clSnapshot_t // sizeof=0x26DC
{                                                                             // XREF: .data:newSnap/r
                                                                                // .data:newSnap_0/r ...
        int valid;                                                    // XREF: CL_ParseSnapshot+BF/w
                                                                                // CL_ParseSnapshot:loc_571E2F/w ...
        int snapFlags;                                            // XREF: CL_ParseSnapshot+B1/w
                                                                                // Demo_ParseSnapshot(int,msg_t *)+D8/w ...
        int serverTime;                                         // XREF: CL_ParseSnapshot+52/w
                                                                                // CL_ParseSnapshot+21A/r ...
        int physicsTime;                                        // XREF: CL_ParseSnapshot+63/w
                                                                                // Demo_ParseSnapshot(int,msg_t *)+8A/w ...
        int messageNum;                                         // XREF: CL_ParseSnapshot+71/w
                                                                                // CL_ParseSnapshot:loc_571CD8/r ...
        int deltaNum;                                             // XREF: CL_ParseSnapshot+8C/w
                                                                                // CL_ParseSnapshot+A0/w ...
        int ping;
        int cmdNum;
        playerState_s ps;                                     // XREF: CL_ParseSnapshot+267/o
                                                                                // CL_ParseSnapshot+28E/o ...
        int numEntities;                                        // XREF: Demo_ParseSnapshot(int,msg_t *)+31E/w
                                                                                // Demo_ParseSnapshot(int,msg_t *)+32C/r
        int numClients;                                         // XREF: Demo_ParseSnapshot(int,msg_t *)+387/w
                                                                                // Demo_ParseSnapshot(int,msg_t *)+395/r
        int parseMatchStateNum;                         // XREF: Demo_ParseSnapshot(int,msg_t *)+1DE/w
        int parseEntitiesNum;                             // XREF: Demo_ParseSnapshot(int,msg_t *)+332/w
        int parseClientsNum;                                // XREF: Demo_ParseSnapshot(int,msg_t *)+39B/w
        int serverCommandNum;                             // XREF: CL_ParseSnapshot+41/w
                                                                                // Demo_ParseSnapshot(int,msg_t *)+68/w ...
};

enum StanceState : __int32
{                                                                             // XREF: clientActive_t/r
                                                                                // clientActive_t/r ...
        CL_STANCE_STAND                 = 0x0,
        CL_STANCE_CROUCH                = 0x1,
        CL_STANCE_PRONE                 = 0x2,
        CL_STANCE_DIVE_TO_PRONE = 0x3,
};

struct outPacket_t // sizeof=0xC
{                                                                             // XREF: clientActive_t/r
        int p_cmdNumber;
        int p_serverTime;
        int p_realtime;
};

struct clientActive_t // sizeof=0x1A6100
{
        bool usingAds;
        // padding byte
        // padding byte
        // padding byte
        int timeoutcount;
        clSnapshot_t snap;
        bool alwaysFalse;
        // padding byte
        // padding byte
        // padding byte
        int serverTime;
        int oldServerTime;
        int oldFrameServerTime;
        int serverTimeDelta;
        int oldSnapServerTime;
        int extrapolatedSnapshot;
        int newSnapshots;
        int serverId;
        char mapname[64];
        int parseMatchStateNum;
        int parseEntitiesNum;
        int parseClientsNum;
        int mouseDx[2];
        int mouseDy[2];
        int mouseIndex;
        bool stanceHeld;
        // padding byte
        // padding byte
        // padding byte
        StanceState stance;
        StanceState stancePosition;
        int stanceTime;
        int cgameUserCmdWeapon;
        int cgameUserCmdOffHandIndex;
        int cgameUserCmdLastWeaponForAlt;
        float cgameFOVSensitivityScale;
        float cgameMaxPitchSpeed;
        float cgameMaxYawSpeed;
        float cgameKickAngles[3];
        float cgameOrigin[3];
        float cgameVelocity[3];
        float cgameViewangles[3];
        int cgameBobCycle;
        int cgameMovementDir;
        bitarray<51> cgameExtraButton_bits;
        int cgamePredictedDataServerTime;
        PredictedVehicleInfo cgameVehicle;
        float viewangles[3];
        int skelTimeStamp;
        volatile unsigned int skelMemPos;
        char skelMemory[262144];
        char *skelMemoryStart;
        bool allowedAllocSkel;
        // padding byte
        // padding byte
        // padding byte
        usercmd_s cmds[128];
        int cmdNumber;
        ClientArchiveData clientArchive[256];
        int clientArchiveIndex;
        outPacket_t outPackets[32];
        clSnapshot_t snapshots[32];
        entityState_s entityBaselines[1024];
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        MatchState parseMatchStates[32];
        entityState_s parseEntities[CL_PARSE_ENTITIES_COUNT]; // mod (L43): retail 2048
        clientState_s parseClients[2048];
        int corruptedTranslationFile;
        char translationVersion[256];
        int lastFireTime;
        bool useHeld;
        // padding byte
        // padding byte
        // padding byte
        int useTime;
        int useCount;
        int wasInVehicle;
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
};
// mod (L43): bytes the parseEntities ring adds over retail (sizeof=0x1A6100 with 2048); cl_main_mp.cpp allocates them.
#define CL_PARSE_ENTITIES_EXTRA_BYTES ((CL_PARSE_ENTITIES_COUNT - 2048) * (int)sizeof(entityState_s))
#if !BO1_NATIVE_64BIT
static_assert(sizeof(clientActive_t) <= 0x1A6100 + CL_PARSE_ENTITIES_EXTRA_BYTES, "clientActive_t outgrew its allocation");
#endif

#include <client_mp/client_static.h>

struct __declspec(align(8)) clientConnection_t // sizeof=0xAFF48
{
        int qport;
        int clientNum;
        int lastPacketSentTime;
        int lastPacketTime;
        netadr_t serverAddress;
        int connectTime;
        int connectPacketCount;
        char serverMessage[256];
        int challenge;
        int checksumFeed;
        int reliableSequence;
        int reliableAcknowledge;
        char *reliableCommands[128];
        int reliableCommandBufferNext;
        char reliableCommandBuffer[16384];
        int serverMessageSequence;
        int serverCommandSequence;
        int lastExecutedServerCommand;
        char serverCommands[128][1024];
        bool isServerRestarting;
        // padding byte
        // padding byte
        // padding byte
        int lastClientArchiveIndex;
        char demoName[64];
        int demorecording;
        int demoplaying;
        int isTimeDemo;
        int demowaiting;
        int firstDemoFrameSkipped;
        int demofile;
        int demoRecordStartTime;
        int demoUseMemoryBuffer;
        int demoMemoryPoolBegin;
        int demoMemoryPoolEnd;
        unsigned __int8 demoMemoryPool[262144];
        int demoRequestUncompressedPacket;
        int demoRequestUncompressedPacketTime;
        int demoReceivedUncompressedPacket;
        demoInitialStateBuffer_t demoInitialStateBuffer[3];
        demoContinuousStateBuffer_t demoContinuousStateBuffer[200];
        int demoInitialStateBufferIndex;
        int demoContinuousStateBufferIndex;
        int demoCoolRecordExtendedCapture;
        int demoCoolRecordProcessingExtendedCapture;
        int demoCoolRecordProcessingTime;
        int demoLiveStream;
        int demoPrevServerTime;
        int timeDemoLog;
        int timeDemoFrames;
        int timeDemoStart;
        int timeDemoPrev;
        int timeDemoBaseTime;
        netchan_t netchan;
        char netchanOutgoingBuffer[2048];
        char netchanIncomingBuffer[65536];
        netProfileInfo_t OOBProf;
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        __int64 statPacketsToSend;
        int statPacketSendTime[34];
        unsigned int serverChallenge;
        unsigned int clientChallenge;
        unsigned int nonce;
        // padding byte
        // padding byte
        // padding byte
        // padding byte
};

struct cgs_t // sizeof=0x31A4
{
    int viewX;
    int viewY;
    int viewWidth;
    int viewHeight;
    float viewAspect;
    int serverCommandSequence;
    int processedSnapshotNum;
    int localServer;
    char gametype[32];
    char szHostName[256];
    int maxclients;
    char mapname[64];
    int gameEndTime;
    int voteTime;
    int voteYes;
    int voteNo;
    char voteString[256];
    XModel *gameModels[512];
    const FxEffectDef *fxs[196];
    const FxEffectDef *grenadeFx[27];
    const FxEffectDef *playerFireFx[3];
    shellshock_parms_t holdBreathParams;
    char teamChatMsgs[8][241];
    int teamChatMsgTimes[8];
    int teamChatPos;
    int teamLastChatPos;
    float compassWidth;
    float compassHeight;
    float compassY;
    char _pad[4];
    clientInfo_t corpseinfo[4];
    actorInfo_t actorCorpseInfo[MAX_ACTOR_CORPSES];
    bool entUpdateToggleContextKey;
    // padding byte
    // padding byte
    // padding byte
};

struct voiceCommunication_t;
struct GfxConfiguration;
struct serverInfo_t;
struct Font_s;

void __cdecl CL_AddReliableCommand(int localClientNum, const char *cmd);
void __cdecl CL_AnimateUI_f();
void __cdecl CL_ShowContextualItemUI_f();
void __cdecl CL_ToggleDemoRecording_f();
void __cdecl CL_WriteDemoEpilog(int localClientNum);
void __cdecl CL_WriteDemoClientArchive(int demoFile, unsigned __int8 msgType, ClientArchiveData *archive, int index);
void __cdecl CL_WriteDemoFromContinuousStateBuffer(int localClientNum, int continuousStateBufferIndex);
void __cdecl CL_StopRecord_f();
void __cdecl CL_Record_f();
void __cdecl CL_WriteInitialStateToDemo(
                int demoFile,
                unsigned __int8 type,
                int serverMessageSequence,
                unsigned __int8 *compressedBuf,
                int compressedBufSize);
void __cdecl CL_WriteUncompressedDemoInfo(int localClientNum);
void __cdecl CL_WriteAllDemoClientArchive(int localClientNum);
void __cdecl CL_WriteAllDemoClientArchiveToMemory(int localClientNum);
void __cdecl CL_WriteDemoClientArchiveToMemory(const clientActive_t *cl, ClientArchiveData *archive, int index);
void __cdecl CL_ReadDemoMessage(int localClientNum);
void __cdecl CL_DemoCompleted(int localClientNum);
void __cdecl CL_ReadDemoClientArchive(int localClientNum);
void __cdecl CL_ReadDemoNetworkPacket(int localClientNum);
void __cdecl CL_NextDemo(int localClientNum);
void __cdecl CL_ShutdownDevGui();
void __cdecl CL_ShutdownHunkUsers();
void __cdecl CL_ShutdownAll();
void __cdecl CL_ShutdownDemo();
void __cdecl CL_MapLoading(const char *mapname);
void __cdecl CL_ResetSkeletonCache(unsigned int localClientNum);
void __cdecl CL_ClearStaticDownload();
void __cdecl CL_ClearState(int localClientNum);
void __cdecl CL_UploadStatsForController(int localControllerIndex);
void __cdecl CL_Disconnect(unsigned int localClientNum, bool deactivateClient);
void __cdecl CL_UpdateProfileAfterGame(int controllerIndex);
void __cdecl CL_CanNotConnect(int localClientNum, const char *msg);
void __cdecl CL_ForwardCommandToServer(int localClientNum, const char *string);
void __cdecl CL_ForwardToServer_f();
void __cdecl CL_Setenv_f();
void __cdecl CL_DisconnectLocalClient(int localClientNum);
void __cdecl CL_Reconnect_f();
void __cdecl CL_SendPureChecksums(int localClientNum);
void __cdecl CL_Vid_Restart_f();
void __cdecl CL_Snd_Restart_f();
void __cdecl CL_OpenedIWDList_f();
void __cdecl CL_ReferencedIWDList_f();
void __cdecl CL_Configstrings_f();
void __cdecl CL_Clientinfo_f();
bool __cdecl CL_WasMapAlreadyLoaded();
void __cdecl CL_DownloadsComplete(int localClientNum);
void __cdecl LoadMapLoadscreen(char *mapname);
void __cdecl CL_BeginDownload(const char *localName, const char *remoteName);
void __cdecl CL_NextDownload(int localClientNum);
void __cdecl CL_InitDownloads(int localClientNum);
void __cdecl CL_CheckForResend(int localClientNum);
void __cdecl CL_BuildMd5StrFromCDKey(char *md5Str);
char *__cdecl strtrm(char *str);
void __cdecl CL_DisconnectError(const char *message);
bool __cdecl CL_HandleVoiceTypePacket(int localClientNum, msg_t *msg);
char __cdecl CL_ConnectionlessPacket(int localClientNum, netadr_t from, msg_t *msg, int time);
char    CL_DispatchConnectionlessPacket(int localClientNum, netadr_t from, msg_t *msg);
void __cdecl CL_DisconnectPacket(int localClientNum, netadr_t from, const char *reason);
void __cdecl CL_SteamServerAuth(unsigned __int64 serverSteamID);
void __cdecl CL_AllocatePerLocalClientMemory();
void __cdecl AllocatePerLocalClientMemory(int maxLocalClients, int maxClients, unsigned int flags);
void __cdecl CL_FreePerLocalClientMemory();
void    CL_InitLoad(char *mapname, const char *gametype);
bool __cdecl CL_PacketEvent(int localClientNum, netadr_t from, msg_t *msg, int time, bool connectionlesss_packets_only);
void __cdecl CL_WriteNewDemoClientArchive(int localClientNum);
void __cdecl CL_WriteNewDemoClientArchiveToMemory(int localClientNum);
unsigned __int8 *__cdecl CL_DemoMemoryPoolAllocate(clientConnection_t *clc, int size);
void __cdecl CL_DemoMemoryPoolDeallocate(clientConnection_t *clc, int size);
int __cdecl CL_GetClientArchiveCount(int start, int end);
void __cdecl CL_WriteDemoMessage(int localClientNum, msg_t *msg, int headerBytes);
void __cdecl CL_WriteDemoMessageToMemory(int localClientNum, msg_t *msg, int headerBytes);
void __cdecl CL_VoiceTransmit(int localClientNum);
voiceCommunication_t *__cdecl CL_GetLocalClientVoiceCommunication(int localClientNum);
int __cdecl Client_SendVoiceData(int bytes, char *enc_buffer);
char __cdecl Voice_SendVoiceData();
void __cdecl CL_RunOncePerClientFrame(int localClientNum, int msec);
void __cdecl CL_Frame(int localClientNum, int msec);
void __cdecl CL_CheckTimeout(int localClientNum);
void __cdecl CL_ServerTimedOut(int localClientNum);
void __cdecl CL_CheckUserinfo(int localClientNum);
void __cdecl CL_UpdateInGameState(int localClientNum);
void __cdecl CL_VoiceFrame(int localClientNum);
char __cdecl CL_IsClientLocal(int clientNum);
void __cdecl CL_ParseBadPacket_f();
void __cdecl CL_ShutdownRef();
void __cdecl CL_ShutdownWorld();
void __cdecl CL_SetupViewport();
void __cdecl CL_InitRenderer();
void __cdecl CL_ShutdownRenderer(int destroyWindow);
void __cdecl CL_StartHunkUsers();
void CL_InitDevGui();
void __cdecl CL_DevGuiDvar_f();
void __cdecl CL_DevGuiCmd_f();
void __cdecl CL_DevGuiOpen_f();
int __cdecl CL_ScaledMilliseconds();
// zombies: SP's always-running client clock (SP 0x02B10CA8) = cls.realtime + time spent paused; UI time.
extern int cl_pausedRealtime;
int __cdecl CL_UIRealtime();
void __cdecl CL_InitRef();
extern int cl_gfxEntCount; // mod (L43): renderer entity count (1536 retail, 2048 with bo1_mod_netents)
void __cdecl SetupGfxConfig(GfxConfiguration *config);
void __cdecl CL_InitDedicated();
void __cdecl CL_startSingleplayer_f();
void __cdecl CL_DrawLogo(int localClientNum);
void __cdecl CL_StopLogo(int localClientNum);
void __cdecl CL_StopLogoOrCinematic(int localClientNum);
void __cdecl CL_ToggleMenu_f();
bool __cdecl CL_ShouldAllowInGameMenu(int localClientNum);
char __cdecl Playlist_ReadFromDisk();
void __cdecl CL_PauseGame(bool state);
#if defined(__ANDROID__)
bool CL_PauseRestoredSoloSession();
#endif
void CL_UpdateZombiePresentationDefaults();
void __cdecl CL_InitOnceForAllClients();
void __cdecl CL_PlayDemo_f();
void __cdecl CL_Disconnect_f();
void __cdecl CL_SetupViewport_f();
void __cdecl CL_PlayLogo_f();
void __cdecl CL_OpenScriptMenu_f();
void __cdecl Playlist_CmdReadFromDisk();
void __cdecl Playlist_CmdSetCategoryFilter();
void __cdecl Playlist_CmdSetPrevPlaylist();
void __cdecl Playlist_CmdGetPrevPlaylist();
void __cdecl CL_CmdGetOldCustomName();
void __cdecl CL_CmdSetNewCustomName();
void __cdecl CL_ResetStats_f();
void __cdecl CL_Init(int localClientNum);
int __cdecl CountBitsEnabled(unsigned int num);
void __cdecl CL_Shutdown(int localClientNum);
bool __cdecl CL_GetServerList(int source, serverInfo_t **servers, int **count);
int __cdecl CL_UpdateDirtyPings(int localClientNum, unsigned int source);
void __cdecl CL_ShowIP_f();
char *__cdecl CL_GetServerIPAddress();
void    CL_SetupForNewServerMap(int localClientNum, char *pszMapName, const char *pszGametype);
bool __cdecl CL_IsServerLoadingMap();
bool __cdecl CL_IsWaitingOnServerToLoadMap(int localClientNum);
void __cdecl CL_SetWaitingOnServerToLoadMap(int localClientNum, bool waiting);
void __cdecl CL_SyncGpu();
void __cdecl CL_DrawTextPhysical(
                char *text,
                int maxChars,
                Font_s *font,
                float x,
                float y,
                float w,
                float xScale,
                float yScale,
                const float *color,
                int style);
void __cdecl CL_DrawTextPhysicalWithEffects(
                char *text,
                int maxChars,
                Font_s *font,
                float x,
                float y,
                float w,
                float xScale,
                float yScale,
                const float *color,
                int style,
                const float *glowColor,
                Material *fxMaterial,
                Material *fxMaterialGlow,
                int fxBirthTime,
                int fxLetterTime,
                int fxDecayStartTime,
                int fxDecayDuration);
void __cdecl CL_DrawText(
                const ScreenPlacement *scrPlace,
                char *text,
                int maxChars,
                Font_s *font,
                float x,
                float y,
                int horzAlign,
                int vertAlign,
                float xScale,
                float yScale,
                const float *color,
                int style);
void __cdecl CL_DrawTextWithEffects(
                const ScreenPlacement *scrPlace,
                char *text,
                int maxChars,
                Font_s *font,
                float x,
                float y,
                float rotation,
                int horzAlign,
                int vertAlign,
                float xScale,
                float yScale,
                const float *color,
                int style,
                const float *glowColor,
                Material *fxMaterial,
                Material *fxMaterialGlow,
                int fxBirthTime,
                int fxLetterTime,
                int fxDecayStartTime,
                int fxDecayDuration);
void __cdecl CL_DrawTextWithCOD7TypeWriterEffects(
                const ScreenPlacement *scrPlace,
                char *text,
                int maxChars,
                Font_s *font,
                float x,
                float y,
                float rotation,
                int horzAlign,
                int vertAlign,
                float xScale,
                float yScale,
                const float *color,
                int style,
                const float *glowColor,
                Material *fxMaterial,
                Material *fxMaterialGlow,
                int fxBirthTime,
                int fxLetterTime,
                int fxDecayStartTime,
                int fxDecayDuration);
void __cdecl CL_DrawTextWithRedactEffects(
                const ScreenPlacement *scrPlace,
                char *text,
                int maxChars,
                Font_s *font,
                float x,
                float y,
                int horzAlign,
                int vertAlign,
                float xScale,
                float yScale,
                const float *color,
                int style,
                const float *glowColor,
                Material *fxMaterial,
                Material *fxMaterialGlow,
                int fxBirthTime,
                int fxLetterTime,
                int fxDecayStartTime,
                int fxDecayDuration,
                int fxRedactDecayStartTime,
                int fxRedactDecayDuration);
void __cdecl CL_DrawTextPhysicalWithCOD7DecodeEffects(
                char *text,
                int maxChars,
                Font_s *font,
                float x,
                float y,
                float xScale,
                float yScale,
                const float *color,
                int style,
                const float *glowColor,
                Material *fxMaterial,
                Material *fxMaterialGlow,
                int fxBirthTime,
                int fxLetterTime,
                int fxDecayStartTime,
                int fxDecayDuration);
void __cdecl CL_DrawTextPhysicalWithRedactEffects(
                char *text,
                int maxChars,
                Font_s *font,
                float x,
                float y,
                float xScale,
                float yScale,
                const float *color,
                int style,
                const float *glowColor,
                Material *fxMaterial,
                Material *fxMaterialGlow,
                int fxBirthTime,
                int fxLetterTime,
                int fxDecayStartTime,
                int fxDecayDuration,
                int fxRedactDecayStartTime,
                int fxRedactDecayDuration);
void __cdecl CL_DrawTextWithPopInEffects(
                const ScreenPlacement *scrPlace,
                char *text,
                int maxChars,
                Font_s *font,
                float x,
                float y,
                float rotation,
                int horzAlign,
                int vertAlign,
                float xScale,
                float yScale,
                const float *color,
                int style,
                const float *glowColor,
                Material *fxMaterial,
                Material *fxMaterialGlow,
                int fxBirthTime,
                int fxLetterTime,
                int fxDecayStartTime,
                int fxDecayDuration);
void __cdecl CL_DrawTextRotate(
                const ScreenPlacement *scrPlace,
                char *text,
                int maxChars,
                Font_s *font,
                float x,
                float y,
                float rotation,
                int horzAlign,
                int vertAlign,
                float xScale,
                float yScale,
                const float *color,
                int style);
void __cdecl CL_DrawTextPhysicalWithCursor(
                char *text,
                int maxChars,
                Font_s *font,
                float x,
                float y,
                float xScale,
                float yScale,
                const float *color,
                int style,
                int cursorPos,
                char cursor);
void __cdecl CL_DrawTextWithCursor(
                const ScreenPlacement *scrPlace,
                char *text,
                int maxChars,
                Font_s *font,
                float x,
                float y,
                int horzAlign,
                int vertAlign,
                float xScale,
                float yScale,
                const float *color,
                int style,
                int cursorPos,
                char cursor);
bool __cdecl CL_ShouldDisplayHud(int localClientNum);
bool __cdecl CL_IsUIActive(int localClientNum);
Font_s *__cdecl CL_RegisterFont(const char *fontName, int imageTrack);
float (*__cdecl CL_GetMapCenter())[3];
void __cdecl CL_SanitizeClanName();
int __cdecl CL_FilterChar(unsigned __int8 input);
//bdTrulyRandomImpl *__cdecl bdSingleton<bdTrulyRandomImpl>::getInstance();
//void bdSingleton<bdTrulyRandomImpl>::destroyInstance();
//bdSingletonRegistryImpl *__cdecl bdSingleton<bdSingletonRegistryImpl>::getInstance();
//bdSingletonRegistryImpl *__thiscall bdSingletonRegistryImpl::bdSingletonRegistryImpl(bdSingletonRegistryImpl *this);
//void bdSingleton<bdSingletonRegistryImpl>::destroyInstance();
//clientStatic_t *__thiscall clientStatic_t::clientStatic_t(clientStatic_t *this);
bool __cdecl CL_IsLocalClientInGame(int localClientNum);


extern clientStatic_t cls;
extern clientConnection_t *clientConnections;
extern UiContext cgDC[1];

extern bool cl_serverLoadingMap;
extern bool g_waitingForServer;
extern bool cl_waitingOnServerToLoadMap[1];


extern const dvar_t *cl_noprint;
extern const dvar_t *playlist;
extern const dvar_t *category;
extern const dvar_t *categoryPlaylist;
extern const dvar_t *wagerCategory;
extern const dvar_t *wagerCategoryPlaylist;
extern const dvar_t *customGameMode;
extern const dvar_t *customclass[10];
extern const dvar_t *cl_hudDrawsBehindUI;
extern const dvar_t *cl_voice;
extern const dvar_t *cl_timeout;
extern const dvar_t *cl_connectTimeout;
extern const dvar_t *cl_connectionAttempts;
extern const dvar_t *cl_shownet;
extern const dvar_t *cl_shownuments;
extern const dvar_t *cl_showServerCommands;
extern const dvar_t *cl_showSend;
extern const dvar_t *cl_showTimeDelta;
extern const dvar_t *cl_freezeDemo;
extern const dvar_t *cl_activeAction;
extern const dvar_t *cl_avidemo;
extern const dvar_t *cl_forceavidemo;
extern const dvar_t *cl_clientDemoName;
extern const dvar_t *cl_clientDemoUseMemoryBuffer;
extern const dvar_t *cl_clientDemoType;
extern const dvar_t *cl_clientDemoMaxRecordTime;
extern const dvar_t *cl_clientDemoRequestPingTime;
extern const dvar_t *cl_clientDemoCoolMomentExtendEnabled;
extern const dvar_t *cl_clientDemoCoolMomentExtendTime;
extern const dvar_t *cl_yawspeed;
extern const dvar_t *cl_pitchspeed;
extern const dvar_t *cl_anglespeedkey;
extern const dvar_t *cl_maxpackets;
extern const dvar_t *cl_packetdup;
extern const dvar_t *cl_sensitivity;
extern const dvar_t *cl_mouseAccel;
extern const dvar_t *cl_freelook;
extern const dvar_t *cl_showMouseRate;
extern const dvar_t *cl_allowDownload;
extern const dvar_t *cl_wwwDownload;
extern const dvar_t *cl_talking;
extern const dvar_t *cl_inGameVideo;
extern const dvar_t *cl_serverStatusResendTime;
extern const dvar_t *cl_maxppf;
extern const dvar_t *cl_bypassMouseInput;
extern const dvar_t *m_pitch;
extern const dvar_t *m_yaw;
extern const dvar_t *m_forward;
extern const dvar_t *m_side;
extern const dvar_t *m_filter;
extern const dvar_t *ui_custom_newname;
extern const dvar_t *cl_socketpool_enabled;
extern const dvar_t *cl_socketpool_size;
extern const dvar_t *ui_clantag_new;
extern const dvar_t *cl_demoFFSpeed;
extern const dvar_t *cl_demoSkipToTime;
extern const dvar_t *cl_demoLiveStreaming;
extern const dvar_t *input_invertPitch;
extern const dvar_t *input_viewSensitivity;
extern const dvar_t *team_indicator;
extern const dvar_t *cl_playerRank;
extern const dvar_t *cl_playerPrestige;
extern const dvar_t *cl_motdString;
extern const dvar_t *cl_ingame;
extern const dvar_t *cl_wasconnected;
extern const dvar_t *cl_maxPing;
extern const dvar_t *cl_profileTextHeight;
extern const dvar_t *cl_profileTextY;
extern const dvar_t *name;
extern const dvar_t *clanName;
extern const dvar_t *rate;
extern const dvar_t *snaps;
extern const dvar_t *cl_punkbuster;
extern const dvar_t *password;
extern const dvar_t *nextdemo;
extern const dvar_t *mortarStrikesLeft;
extern const dvar_t *hud_enable;
extern const dvar_t *cg_blood;
extern const dvar_t *cg_allow_mature;
extern const dvar_t *cg_mature;
extern const dvar_t *cl_serverchallenge;
extern const dvar_t *cl_debugMessageKey;
extern const dvar_t *motd;

extern unsigned int frame_msec;
extern int old_com_frameTime;


static const char *customClassDvars[11] =
{
  "customclass1",
  "customclass2",
  "customclass3",
  "customclass4",
  "customclass5",
  "prestigeclass1",
  "prestigeclass2",
  "prestigeclass3",
  "prestigeclass4",
  "prestigeclass5",
  NULL
};
