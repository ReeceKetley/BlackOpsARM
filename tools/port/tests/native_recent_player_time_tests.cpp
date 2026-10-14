#include <live/live_meetplayer.h>
#include <platform/android/file_time.h>
#include <qcommon/common.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
extern MetPlayerInfo metPlayersXuidinfo[1];
static int requests;
// Isolated engine boundaries; no native backend readiness is supplied.
void Live_RequestSessionsFromRecentPlayers() { ++requests; }
void I_strncpyz(char *out, const char *in, int length) { std::snprintf(out,length,"%s",in); }
bool Assert_MyHandler(const char *, int, int, const char *, ...) { std::abort(); }
void Com_Error(errorParm_t, const char *, ...) { std::abort(); }
static void Check(bool ok) { if (!ok) std::abort(); }
static std::uint64_t Ticks(const Bo1MeetPlayerTime &time) {
    return (std::uint64_t(time.dwHighDateTime)<<32)|time.dwLowDateTime;
}
static std::uint64_t Now() {
    timespec value{}; Check(!clock_gettime(CLOCK_REALTIME,&value));
    return (std::uint64_t(value.tv_sec)+11644473600ULL)*10000000+value.tv_nsec/100;
}
static Bo1MeetPlayerTime Pack(std::uint64_t ticks) {
    return {static_cast<std::uint32_t>(ticks),static_cast<std::uint32_t>(ticks>>32)};
}
int main() {
    static_assert(sizeof(XuidInfo)==48 && sizeof(MetPlayerInfo)==2416);
    errno=0; Check(!bo1::android::ReadFileTime(nullptr) && errno==EINVAL);
    Bo1MeetPlayerTime value{};
    const auto before=Now(); LiveMeetPlayer_SetCurrentTime(&value); const auto after=Now();
    Check(Ticks(value)>=before && Ticks(value)<=after);
    std::memset(metPlayersXuidinfo,0,sizeof(metPlayersXuidinfo));
    char longName[100]; std::memset(longName,'x',99); longName[99]=0;
    Check(LiveMeetPlayer_AddPlayerAtSlot(0,0xFEDCBA9800000100ULL,longName,49)==1);
    const auto &last=metPlayersXuidinfo[0].playersXuidInfo[49];
    Check(last.playerXuids==0xFEDCBA9800000100ULL && std::strlen(last.gamertag)==31);
    Check(requests==1 && Ticks(last.systemTime)>=before && Ticks(last.systemTime)<=Now());
    for(int i=0;i<49;++i) Check(!metPlayersXuidinfo[0].playersXuidInfo[i].playerXuids);
    auto &first=metPlayersXuidinfo[0].playersXuidInfo[0];
    first.systemTime=Pack(Now()-50000000);
    const auto elapsed=LiveMeetPlayer_GetLastMetTime(0,0); Check(elapsed>=5 && elapsed<=6);
    first.systemTime=Pack(Now()+100000000); Check(!LiveMeetPlayer_GetLastMetTime(0,0));
    first.systemTime={0,0}; Check(LiveMeetPlayer_GetLastMetTime(0,0)==UINT32_MAX);
    std::puts("PASS actual recent-player clock/FILETIME bounds, fixed disk sizes, typed final slot/high UID, elapsed/future/saturation");
}
