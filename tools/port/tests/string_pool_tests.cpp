#include <platform/script_pool_services.h>
#include <platform/android/script_string_services.h>
#include <platform/atomic32.h>
#include <clientscript/cscr_stringlist.h>
#include <clientscript/cscr_memorytree.h>
#include <clientscript/cscr_instance.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <set>
#include "string_pool_fixture.h"
#if defined(_WIN32)
#include <crtdbg.h>
#endif
static_assert(sizeof(RefString)==8&&alignof(RefString)==4&&offsetof(RefString,str)==4);
static_assert(sizeof(MemoryNode)==16&&sizeof(HashEntry)==8&&sizeof(RefString::data)==4);
static void require(bool ok,const char*why){if(!ok){std::fprintf(stderr,"FAIL pool %s\n",why);std::abort();}}
using pool_fixture::baseline;
static unsigned intern(const std::string&s,unsigned user,scriptInstance_t inst){unsigned id=SL_GetString(const_cast<char*>(s.c_str()),user,inst);
    require(id&&id<65536,"actual nonzero bounded pool ID");require(SL_ConvertToString(id,inst)&&!std::memcmp(SL_ConvertToString(id,inst),s.c_str(),s.size()+1),"actual ID roundtrip bytes including NUL");
    require(SL_GetStringLen(inst,id)==static_cast<int>(s.size()),"actual public length excludes terminator");
    require(SL_ConvertFromRefString(inst,GetRefString(inst,id))==static_cast<int>(id),"actual native pointer back to fixed ID");return id;}
int main(int argc,char**argv){
#if defined(_WIN32)
    _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
#endif
    require(argc==2,"fixtures/max-reject");Sys_InitializeCriticalSections();bo1::script::RegisterServerThread();require(Sys_IsServerThread(),"actual main registered server remains alive");
    if(!std::strcmp(argv[1],"max-reject")){SL_Init(SCRIPTINSTANCE_SERVER);std::string oversized(65531,'x');intern(oversized,4,SCRIPTINSTANCE_SERVER);require(false,"actual MT max request must terminate");}
    require(!std::strcmp(argv[1],"fixtures"),"unknown mode");
    std::vector<std::string>strings={"","alpha","alpha",std::string("\x80\xff",2),std::string(255,'a'),std::string(255,'b'),std::string(511,'c'),std::string(767,'d'),std::string(1023,'e'),std::string(65530,'m')};
    unsigned cycles=0;
    for(unsigned round=0;round<3;++round)for(unsigned instance=0;instance<2;++instance){auto inst=static_cast<scriptInstance_t>(instance);SL_Init(inst);baseline(inst);std::vector<unsigned>ids;
        for(const auto&s:strings)ids.push_back(intern(s,4,inst));require(ids[1]==ids[2]&&ids[4]!=ids[5],"dedup and equal-length hash collision");std::set<unsigned>unique(ids.begin(),ids.end());
        require(gScrMemTreeGlob[inst].totalAlloc==static_cast<int>(unique.size()),"one MT allocation per unique user4 string");require(bo1::atomic32::AtomicLoad32(&gScrStringDebugGlob[inst]->totalRefCount)==unique.size(),"same-user duplicates do not add references");
        for(auto id:unique){require(SL_GetUser(id,inst)==4&&GetRefString(inst,id)->refCount==1,"user4 exact packed ownership");}
        std::printf("{\"round\":%u,\"instance\":%u,\"ids\":[",round,instance);for(std::size_t i=0;i<ids.size();++i)std::printf("%s%u",i?",":"",ids[i]);std::printf("],\"unique_strings\":%zu,\"live_allocations\":%d,\"live_buckets\":%d}\n",unique.size(),gScrMemTreeGlob[inst].totalAlloc,gScrMemTreeGlob[inst].totalAllocBuckets);
        // Existinguser4 plususer1 yields two refs; removing user4 leaves user1.
        require(intern(strings[1],1,inst)==ids[1]&&SL_GetUser(ids[1],inst)==5&&GetRefString(inst,ids[1])->refCount==2,"distinct user adds one reference");SL_ShutdownSystem(inst,4);
        require(gScrMemTreeGlob[inst].totalAlloc==1&&SL_GetUser(ids[1],inst)==1&&GetRefString(inst,ids[1])->refCount==1,"shutdown user4 preserves distinct user1");SL_ShutdownSystem(inst,1);baseline(inst);
        auto transfer=intern("transfer",4,inst);SL_TransferSystem(4,1,inst);require(SL_GetUser(transfer,inst)==1&&GetRefString(inst,transfer)->refCount==1,"sole-user transfer preserves one reference");SL_ShutdownSystem(inst,1);baseline(inst);
        auto loose=intern("loose",0,inst);require(intern("loose",0,inst)==loose&&GetRefString(inst,loose)->refCount==2,"user0 duplicate independent references");SL_RemoveRefToString(inst,loose);require(GetRefString(inst,loose)->refCount==1,"user0 first release");SL_RemoveRefToString(inst,loose);baseline(inst);
        std::printf("{\"release\":true,\"round\":%u,\"instance\":%u,\"strings_used\":[",round,instance);for(unsigned i=0;i<32;++i)std::printf("%s%d",i?",":"",gScrStringGlob[inst].stringsUsed[i]);std::printf("],\"live_allocations\":0,\"live_buckets\":0,\"debug_refs\":0}\n");std::fflush(stdout);
        SL_Shutdown(inst);require(!gScrStringGlob[inst].inited&&!gScrStringDebugGlob[inst],"actual shutdown clears live init/debug pointer");++cycles;
    }
    std::printf("{\"schema\":\"BlackOpsARM.TASK-0017.string-pool.v1\",\"pointer_bytes\":%zu,\"cycles_passed\":%u,\"strings_per_cycle\":%zu,\"transfer_cases\":6,\"user0_cases\":6,\"max_text_bytes\":65530,\"pool_baseline_restored\":true,\"ref_string_bytes\":8,\"ref_data_bytes\":4,\"ref_string_offset\":4,\"memory_node_bytes\":16,\"hash_entry_bytes\":8,\"hash_free_list_restored\":true}\n",sizeof(void*),cycles,strings.size());
}
