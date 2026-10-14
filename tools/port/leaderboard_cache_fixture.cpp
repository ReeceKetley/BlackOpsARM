// Actual native cache/registry bodies only. No DemonWare/network readiness.
#include <live/live_leaderboard.cpp>
#include <array>
#include <cstdio>
#include <vector>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
void Com_Error(errorParm_t,const char *,...){std::fputs("expected cache fixture fatal returned\n",stderr);}
bool Assert_MyHandler(const char *,int,int,const char *,...){std::abort();}
static void Check(bool value,const char *why){if(!value){std::fprintf(stderr,"FAIL: %s\n",why);std::abort();}}
static void MustAbort(void (*fn)(),const char *why){const pid_t child=fork();Check(child>=0,"fork");if(!child){fn();_exit(4);}int status{};Check(waitpid(child,&status,0)==child,"waitpid");Check(WIFSIGNALED(status)&&WTERMSIG(status)==SIGABRT,why);}
static void TypeNegative(){LB_NativeCachedLeaderboard(-1);}
static void TypeEnd(){LB_NativeCachedLeaderboard(LB_COUNT);}
static void BadOwner(){auto &cache=g_lbGlob.leaderboards[0];LB_NativeCachedRow(cache,&g_lbGlob.leaderboards[1].userStats,cache.offset);}
static void BadLower(){auto &cache=g_lbGlob.leaderboards[0];LB_NativeCachedRow(cache,&cache.userStats,cache.offset-1);}
static void BadUpper(){auto &cache=g_lbGlob.leaderboards[0];LB_NativeCachedRow(cache,&cache.userStats,cache.offset+cache.numResults);}
static void BadCount(){auto &cache=g_lbGlob.leaderboards[0];cache.numResults=101;LB_NativeCachedRow(cache,&cache.userStats,cache.offset);}
static void BadColumn(){LB_NativeFeederStat(LB_TDM,9);}
static void BadSort(){g_lbGlob.leaderboards[0].numResults=101;LB_SortXUserStatsByRank(&g_lbGlob.leaderboards[0]);}
#undef main
int main(){
    struct Canary {std::uint64_t before{0x4be429dc37761011};LbGlob global{};std::uint64_t after{0x729840cec13f195a};};
    static Canary storage;
    auto &global=storage.global;
    for(int index=0;index<16;++index){
        auto &cache=global.leaderboards[index];cache.type=LB_COUNT;cache.lastUpdate=731+index;cache.filter=77;cache.lbTypeByResetPeriod=77;cache.offset=17;cache.curIdx=21;cache.forceRefresh=false;
        cache.isUpdating=true;cache.notSameFilter=true;cache.notSameResetPeriod=true;cache.notSamePrestigeMode=true;cache.numResults=37;cache.totalResults=138;cache.numMatchesPlayed=719;cache.playerLbRow.m_rank=0x1122334455667788ULL;
        for(int row=0;row<100;++row){cache.userStats.m_leaderBoardEntries[row].m_rank=0x8000000000000000ULL+index*100+row;cache.userStats.m_leaderBoardEntries[row].m_columns[0]=index*100+row;}
    }
    std::vector<unsigned char> snapshot(sizeof(global)),allowed(sizeof(global));std::memcpy(snapshot.data(),&global,sizeof(global));
    const auto base=reinterpret_cast<std::uintptr_t>(&global);
    auto permit=[&](const auto &field){const auto start=reinterpret_cast<std::uintptr_t>(&field)-base;Check(start+sizeof(field)<=sizeof(global),"actual field within object");for(std::size_t n=0;n<sizeof(field);++n)allowed[start+n]=1;};
    for(auto &cache:global.leaderboards){permit(cache.lastUpdate);permit(cache.filter);permit(cache.lbTypeByResetPeriod);permit(cache.type);permit(cache.offset);permit(cache.curIdx);permit(cache.forceRefresh);}
    LB_NativeInitializeCache(global);
    const auto *bytes=reinterpret_cast<const unsigned char *>(&global);
    for(std::size_t n=0;n<sizeof(global);++n)if(!allowed[n])Check(bytes[n]==snapshot[n],"seven metadata writes preserve all other actual object bytes");
    for(int index=0;index<16;++index){const auto &cache=global.leaderboards[index];Check(cache.type==index && cache.lastUpdate==0 && cache.filter==3 && cache.lbTypeByResetPeriod==3 && !cache.offset && !cache.curIdx && cache.forceRefresh,"all sixteen actual cache initialization rows");}
    Check(storage.before==0x4be429dc37761011 && storage.after==0x729840cec13f195a,"outer canaries");
    for(int mode=0;mode<8;++mode){LbStructure structure{};LB_InitAndRegisterStructure(&structure,mode);for(int column=5;column<5+lbStatCounts[mode];++column)Check(LB_NativeFeederStat(static_cast<LbType>(mode),column)==structure.columns[6+column-5],"feeder columns match actual registered mode stats");}
    for(int mode=0;mode<4;++mode){LbGlobalStructure structure{};LB_InitAndRegisterStructureForSpecialLeaderboards(&structure,mode,mode);}
    Check(g_LbLookup.numLbs==8 && g_LbLookup.numGlobalLbs==4,"actual canonical initialization registries");
    auto &cache=g_lbGlob.leaderboards[0];cache.offset=1000;cache.numResults=2;cache.totalResults=2000;
    Check(&LB_NativeCachedRow(cache,&cache.userStats,1000)==&cache.userStats.m_leaderBoardEntries[0] && &LB_NativeCachedRow(cache,&cache.userStats,1001)==&cache.userStats.m_leaderBoardEntries[1],"actual populated cache interval uses typed row stride");
    MustAbort(TypeNegative,"negative cache type rejected");MustAbort(TypeEnd,"end cache type rejected");MustAbort(BadOwner,"foreign stats owner rejected");MustAbort(BadLower,"below cached window rejected");MustAbort(BadUpper,"above populated window rejected");MustAbort(BadCount,"cache capacity rejected");MustAbort(BadColumn,"unregistered display column rejected");MustAbort(BadSort,"sort capacity rejected");
    const std::uint64_t ranks[]={0,5,4,4,UINT64_MAX,1,0};cache.numResults=7;
    for(int row=0;row<7;++row)cache.userStats.m_leaderBoardEntries[row].m_rank=ranks[row];
    LB_SortXUserStatsByRank(&cache);Check(cache.numResults==5,"canonical empty-rank trimming");
    const std::uint64_t sorted[]={1,4,4,5,UINT64_MAX,0,0};for(int row=0;row<7;++row)Check(cache.userStats.m_leaderBoardEntries[row].m_rank==sorted[row],"actual native rank field comparison");
    std::printf("PASS actual16-cache seven-field initialization canaries, canonical8+4 registries, populated-row owner/bounds and typed rank sort; sizeof Leaderboard=%zu LbGlob=%zu; no online backend readiness\n",sizeof(Leaderboard),sizeof(LbGlob));
}
