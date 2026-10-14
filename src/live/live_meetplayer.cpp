#include "live_meetplayer.h"
#if defined(__ANDROID__)
#include <platform/android/file_time.h>
#include <cstdlib>
#endif
#include "live_storage.h"
#include <win32/win_shared.h>
#include "live_win.h"
#include <qcommon/common.h>

MetPlayerInfo metPlayersXuidinfo[1];
int s_metPlayerLastUploadTime[1];
bool s_metPlayerListChanged[1];
int s_lastUpdateTime[1];
unsigned __int64 recentPlayerSessionIDs[50];
unsigned __int64 metPlayerXuid;

const dvar_t *metPlayerListUploadInterval;
const dvar_t *metPlayerListUpdateInterval;

void __cdecl LiveMeetPlayer_DownloadMetPlayersList(int localControllerIndex)
{
    LiveStorage_ReadMetPlayerList(
        localControllerIndex,
        (char*)"CompressedMetPlayer3.info",
        &metPlayersXuidinfo[localControllerIndex].version,
        2416);
}

void __cdecl LiveMeetPlayer_ClearUploadFlags(int controllerIndex)
{
    s_metPlayerLastUploadTime[controllerIndex] = Sys_Milliseconds();
    s_metPlayerListChanged[controllerIndex] = 0;
}

bool __cdecl LiveMeetPlayer_UploadMetPlayerListNeeded(int controllerIndex)
{
    signed int v1; // esi

    v1 = Sys_Milliseconds() - s_metPlayerLastUploadTime[controllerIndex];
    return v1 > Dvar_GetInt(metPlayerListUploadInterval) && s_metPlayerListChanged[controllerIndex];
}

void __cdecl LiveMeetPlayer_ResetData(int controllerIndex)
{
    memset(&metPlayersXuidinfo[controllerIndex].version, 0, sizeof(MetPlayerInfo));
    metPlayersXuidinfo[controllerIndex].version = 5;
}

void __cdecl LiveMeetPlayer_CheckDownloadedVersionAndResetData(int localControllerIndex)
{
    int i; // [esp+4h] [ebp-194h]
    unsigned __int64 metPlayerXuid[50]; // [esp+8h] [ebp-190h]

    if ( metPlayersXuidinfo[localControllerIndex].version == 5 )
    {
        for ( i = 0; i < metPlayersXuidinfo[localControllerIndex].metPlayerCount; ++i )
        {
            LODWORD(metPlayerXuid[i]) = metPlayersXuidinfo[localControllerIndex].playersXuidInfo[i].playerXuids;
            HIDWORD(metPlayerXuid[i]) = HIDWORD(metPlayersXuidinfo[localControllerIndex].playersXuidInfo[i].playerXuids);
        }
    }
    else
    {
        LiveMeetPlayer_ResetData(localControllerIndex);
        metPlayersXuidinfo[localControllerIndex].version = 5;
    }
}

void __cdecl LiveMeetPlayer_UploadMetPlayerList(int localControllerIndex)
{
    LiveStorage_WriteMetPlayerList(
        localControllerIndex,
        (char*)"CompressedMetPlayer3.info",
        &metPlayersXuidinfo[localControllerIndex].version,
        2416);
}

void __cdecl LiveMeetPlayer_Init()
{
    int i; // [esp+0h] [ebp-4h]

    memset(&metPlayersXuidinfo[0].version, 0, sizeof(metPlayersXuidinfo));
    for ( i = 0; i < 1; ++i )
        metPlayersXuidinfo[i].version = 5;
    metPlayerListUpdateInterval = _Dvar_RegisterInt(
                                                                    "metPlayerListUpdateInterval",
                                                                    10000,
                                                                    0,
                                                                    0x7FFFFFFF,
                                                                    0,
                                                                    "Time in milliseconds since uploaded recent met player list.");
    metPlayerListUploadInterval = _Dvar_RegisterInt(
                                                                    "metPlayerListUploadInterval",
                                                                    60000,
                                                                    0,
                                                                    0x7FFFFFFF,
                                                                    0,
                                                                    "This dvar is used to make sure that recent met player list get updates only when it changes.");
}

int __cdecl LiveMeetPlayer_GetXUIDPosInMetPlayerList(int localControllerIndex, unsigned __int64 currentPlayerXuid)
{
    int i; // [esp+8h] [ebp-4h]

    for ( i = 0; i < metPlayersXuidinfo[localControllerIndex].metPlayerCount; ++i )
    {
        if ( currentPlayerXuid == __PAIR64__(
                                                                HIDWORD(metPlayersXuidinfo[localControllerIndex].playersXuidInfo[i].playerXuids),
                                                                metPlayersXuidinfo[localControllerIndex].playersXuidInfo[i].playerXuids) )
            return i;
    }
    return -1;
}

int __cdecl LiveMeetPlayer_GetNoOfMetPlayers(int localControllerIndex)
{
    return metPlayersXuidinfo[localControllerIndex].metPlayerCount;
}

void __cdecl LiveMeetPlayer_SortMetPlayers(int localControllerIndex, unsigned int noOfPlayers)
{
    qsort(
        metPlayersXuidinfo[localControllerIndex].playersXuidInfo,
        noOfPlayers, sizeof(*(metPlayersXuidinfo[localControllerIndex].playersXuidInfo)),
        (int (__cdecl *)(const void *, const void *))LiveMeetPlayer_MetPlayerCompare);
}

int __cdecl LiveMeetPlayer_MetPlayerCompare(_QWORD *arg0, _QWORD *arg1)
{
    unsigned __int64 sysSec2; // [esp+0h] [ebp-28h]
    unsigned __int64 sysSec1; // [esp+18h] [ebp-10h]

    sysSec1 = arg0[5];
    sysSec2 = arg1[5];
    if ( sysSec1 == sysSec2 )
        return *(unsigned int *)arg0 - *(unsigned int *)arg1;
    if ( sysSec1 >= sysSec2 )
        return -1;
    return 1;
}

void __cdecl LiveMeetPlayer_UpdateMeetPlayerList(
    int localControllerIndex,
    unsigned __int64 playeruid,
    const char *playername)
{
    int i; // [esp+0h] [ebp-18h]
    int currentPlayerPosition; // [esp+8h] [ebp-10h]
    int currentPlayerPositiona; // [esp+8h] [ebp-10h]

    s_lastUpdateTime[localControllerIndex] = Sys_Milliseconds();
    if (!playeruid
        && !Assert_MyHandler(
            "C:\\projects_pc\\cod\\codsrc\\src\\live\\live_meetplayer.cpp",
            449,
            0,
            "%s",
            "currentPlayerXuid != 0"))
    {
        __debugbreak();
    }
    if (playeruid)
    {
        currentPlayerPosition = LiveMeetPlayer_GetXUIDPosInMetPlayerList(localControllerIndex, playeruid);
        s_metPlayerListChanged[localControllerIndex] = 1;
        if (currentPlayerPosition == -1)
        {
            if (metPlayersXuidinfo[localControllerIndex].metPlayerCount >= 0x32u)
            {
                if (metPlayersXuidinfo[localControllerIndex].metPlayerCount != 50
                    && !Assert_MyHandler(
                        "C:\\projects_pc\\cod\\codsrc\\src\\live\\live_meetplayer.cpp",
                        479,
                        0,
                        "%s",
                        "metPlayersXuidinfo[localControllerIndex].metPlayerCount == MAX_MET_FRIENDS"))
                {
                    __debugbreak();
                }
                currentPlayerPositiona = 0;
                for (i = 1; i < metPlayersXuidinfo[localControllerIndex].metPlayerCount; ++i)
                {
                    if (LiveMeetPlayer_MetPlayerCompare(
                        &metPlayersXuidinfo[localControllerIndex].playersXuidInfo[currentPlayerPositiona].playerXuids,
                        &metPlayersXuidinfo[localControllerIndex].playersXuidInfo[i].playerXuids) == -1)
                        currentPlayerPositiona = i;
                }
                LODWORD(metPlayerXuid) = metPlayersXuidinfo[localControllerIndex].playersXuidInfo[currentPlayerPositiona].playerXuids;
                HIDWORD(metPlayerXuid) = HIDWORD(metPlayersXuidinfo[localControllerIndex].playersXuidInfo[currentPlayerPositiona].playerXuids);
                if (!LiveMeetPlayer_AddPlayerAtSlot(localControllerIndex, playeruid, playername, currentPlayerPositiona))
                    Com_PrintWarning(23, "This might be bad.\n");
            }
            else if (LiveMeetPlayer_AddPlayerAtSlot(
                localControllerIndex,
                playeruid,
                playername,
                metPlayersXuidinfo[localControllerIndex].metPlayerCount))
            {
                ++metPlayersXuidinfo[localControllerIndex].metPlayerCount;
            }
        }
        else
        {
            if (currentPlayerPosition >= metPlayersXuidinfo[localControllerIndex].metPlayerCount
                && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\live\\live_meetplayer.cpp",
                    465,
                    0,
                    "%s",
                    "currentPlayerPosition < metPlayersXuidinfo[localControllerIndex].metPlayerCount"))
            {
                __debugbreak();
            }
            LiveMeetPlayer_SetCurrentTime(&metPlayersXuidinfo[localControllerIndex].playersXuidInfo[currentPlayerPosition].systemTime);
            I_strncpyz(
                metPlayersXuidinfo[localControllerIndex].playersXuidInfo[currentPlayerPosition].gamertag,
                playername,
                32);
        }
    }
}

void __cdecl LiveMeetPlayer_SetCurrentTime(Bo1MeetPlayerTime *currentTime)
{
#if defined(__ANDROID__)
    if (!bo1::android::ReadFileTime(currentTime))
    {
        Com_Error(ERR_FATAL, "Recent-player wall clock failed: %d", errno);
        std::abort();
    }
#else
    GetSystemTimeAsFileTime(currentTime);
#endif
}

char __cdecl LiveMeetPlayer_AddPlayerAtSlot(
                int localControllerIndex,
                unsigned __int64 playeruid,
                const char *playername,
                int slot)
{
    int v5; // edx
    int v6; // eax

    if ( slot >= 50
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\live\\live_meetplayer.cpp",
                    328,
                    0,
                    "%s",
                    "slot < MAX_MET_FRIENDS") )
    {
        __debugbreak();
    }
    if ( slot >= 50 )
        return 0;
    if ( !playeruid
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\live\\live_meetplayer.cpp",
                    337,
                    0,
                    "%s",
                    "currentPlayerXuid != 0") )
    {
        __debugbreak();
    }
    if ( !playeruid )
        return 0;
    if ( !*playername )
        return 0;
#if defined(__ANDROID__)
    if (localControllerIndex != 0 || slot < 0)
    {
        if (!Assert_MyHandler(__FILE__, __LINE__, 0, "%s", "controller == 0 && slot >= 0")) __debugbreak();
        return 0;
    }
    auto &record = metPlayersXuidinfo[localControllerIndex].playersXuidInfo[slot];
    record.playerXuids = playeruid;
    I_strncpyz(record.gamertag, playername, sizeof(record.gamertag));
    LiveMeetPlayer_SetCurrentTime(&record.systemTime);
#else
    v5 = localControllerIndex;
    v6 = slot;
    LODWORD(metPlayersXuidinfo[v5].playersXuidInfo[v6].playerXuids) = playeruid;
    *(unsigned int *)(v5 * 2416 + v6 * 48 + 173153028) = HIDWORD(playeruid);
    I_strncpyz((char *)(2416 * localControllerIndex + 48 * slot + 173153032), playername, 32);
    LiveMeetPlayer_SetCurrentTime((_FILETIME *)(2416 * localControllerIndex + 48 * slot + 173153064));
#endif
    Live_RequestSessionsFromRecentPlayers();
    return 1;
}

unsigned int __cdecl LiveMeetPlayer_GetLastMetTime(int localControllerIndex, int index)
{
    Bo1MeetPlayerTime sysTime1; // [esp+44h] [ebp-14h]
    Bo1MeetPlayerTime sysTime2; // [esp+50h] [ebp-8h] BYREF

    sysTime1.dwLowDateTime = metPlayersXuidinfo[localControllerIndex].playersXuidInfo[index].systemTime.dwLowDateTime;
    sysTime1.dwHighDateTime = metPlayersXuidinfo[localControllerIndex].playersXuidInfo[index].systemTime.dwHighDateTime;
    LiveMeetPlayer_SetCurrentTime(&sysTime2);
#if defined(__ANDROID__)
    const std::uint64_t now = (std::uint64_t(sysTime2.dwHighDateTime) << 32) | sysTime2.dwLowDateTime;
    const std::uint64_t then = (std::uint64_t(sysTime1.dwHighDateTime) << 32) | sysTime1.dwLowDateTime;
    if (now < then) return 0; // Wall clock can move backwards; elapsed time cannot.
    const auto seconds = (now-then) / 10000000;
    return seconds > UINT32_MAX ? UINT32_MAX : static_cast<unsigned int>(seconds);
#else
    return (__int64)((double)(unsigned __int64)(*(_QWORD *)&sysTime2 - *(_QWORD *)&sysTime1) / pow(10.0, 7.0));
#endif
}

XuidInfo *__cdecl LiveMeetPlayer_GetMetFriendInfo(XuidInfo *result, int localControllerIndex, int index)
{
    memcpy(result, &metPlayersXuidinfo[localControllerIndex].playersXuidInfo[index], sizeof(XuidInfo));
    return result;
}

char __cdecl LiveMeetPlayer_GetRecentPlayerInfoByID(
                int controllerIndex,
                unsigned __int64 id,
                XuidInfo *recentMetPlayerInfo)
{
    unsigned int playerIndex; // [esp+8h] [ebp-8h]

    if ( !recentMetPlayerInfo
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\live\\live_meetplayer.cpp",
                    554,
                    0,
                    "%s",
                    "recentMetPlayerInfo") )
    {
        __debugbreak();
    }
    if ( !recentMetPlayerInfo )
        return 0;
    playerIndex = LiveMeetPlayer_GetRecentPlayerIndexByID(controllerIndex, id);
    if ( playerIndex == -1 )
        return 0;
    memcpy(recentMetPlayerInfo, &metPlayersXuidinfo[controllerIndex].playersXuidInfo[playerIndex], sizeof(XuidInfo));
    return 1;
}

unsigned int __cdecl LiveMeetPlayer_GetRecentPlayerIndexByID(int controllerIndex, unsigned __int64 id)
{
    unsigned int i; // [esp+8h] [ebp-8h]
    MetPlayerInfo *metPlayersXuidData; // [esp+Ch] [ebp-4h]

    metPlayersXuidData = &metPlayersXuidinfo[controllerIndex];
    for ( i = 0; i < metPlayersXuidData->metPlayerCount; ++i )
    {
        if ( metPlayersXuidData->playersXuidInfo[i].playerXuids == id )
            return i;
    }
    return -1;
}

void __cdecl LiveMeetPlayer_UpdatePlayerSession(unsigned __int64 playerUid, unsigned __int64 sessionUid)
{
    unsigned int i; // [esp+8h] [ebp-8h]

    for (i = 0; i < metPlayersXuidinfo[0].metPlayerCount; ++i)
    {
        if (metPlayersXuidinfo[0].playersXuidInfo[i].playerXuids == playerUid)
        {
            LODWORD(recentPlayerSessionIDs[i]) = sessionUid;
            HIDWORD(recentPlayerSessionIDs[i]) = HIDWORD(sessionUid);
        }
    }
}

int __cdecl LiveMeetPlayer_GetPlayerSessionByID(unsigned __int64 playerUid)
{
    unsigned int i; // [esp+8h] [ebp-8h]

    for ( i = 0; i < metPlayersXuidinfo[0].metPlayerCount; ++i )
    {
        if ( metPlayersXuidinfo[0].playersXuidInfo[i].playerXuids == playerUid )
            return recentPlayerSessionIDs[i];
    }
    return 0;
}

