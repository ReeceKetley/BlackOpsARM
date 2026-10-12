#include <database/db_asset_entry_types.h>
#include <platform/critical_section_services.h>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <vector>
#if defined(_WIN32)
#include <crtdbg.h>
#endif
static void require(bool ok,const char*message){if(!ok){std::fprintf(stderr,"FAIL %s\n",message);std::abort();}}
static void wait(const std::atomic<bool>&ready){auto end=std::chrono::steady_clock::now()+std::chrono::seconds(15);while(!ready.load(std::memory_order_acquire)){require(std::chrono::steady_clock::now()<end,"worker timeout");std::this_thread::yield();}}
static void locks(){
 std::vector<std::thread> workers;for(unsigned i=0;i<8;++i)workers.emplace_back([]{for(unsigned j=0;j<20;++j)Sys_InitializeCriticalSections();});for(auto&w:workers)w.join();
 Sys_EnterCriticalSection(CRITSECT_DBHASH);Sys_EnterCriticalSection(CRITSECT_DBHASH);require(Sys_TryEnterCriticalSection(CRITSECT_DBHASH),"same-thread recursive DBHASH try");Sys_LeaveCriticalSection(CRITSECT_DBHASH);
 std::atomic<bool> checked{false},partial{false},checkedPartial{false},done{false};
 std::thread other([&]{require(!Sys_TryEnterCriticalSection(CRITSECT_DBHASH),"other-thread DBHASH unavailable");for(auto section:{CRITSECT_SCRIPT_STRING,CRITSECT_MEMORY_TREE}){require(Sys_TryEnterCriticalSection(section),"DBHASH independent of script sections");Sys_LeaveCriticalSection(section);}checked.store(true,std::memory_order_release);wait(partial);require(!Sys_TryEnterCriticalSection(CRITSECT_DBHASH),"DBHASH remains held at recursive depth1");checkedPartial.store(true,std::memory_order_release);Sys_EnterCriticalSection(CRITSECT_DBHASH);done.store(true,std::memory_order_release);Sys_LeaveCriticalSection(CRITSECT_DBHASH);});
 wait(checked);Sys_LeaveCriticalSection(CRITSECT_DBHASH);partial.store(true,std::memory_order_release);wait(checkedPartial);require(!done.load(std::memory_order_acquire),"partial leave cannot release DBHASH");Sys_LeaveCriticalSection(CRITSECT_DBHASH);wait(done);other.join();
 unsigned updates=0;workers.clear();for(unsigned i=0;i<8;++i)workers.emplace_back([&]{for(unsigned j=0;j<2000;++j){Sys_EnterCriticalSection(CRITSECT_DBHASH);++updates;Sys_LeaveCriticalSection(CRITSECT_DBHASH);}});for(auto&w:workers)w.join();require(updates==16000,"DBHASH real mutual exclusion");
 std::printf("{\"locks\":{\"critical_section\":74,\"initialize_threads\":8,\"recursive_depth\":3,\"locked_updates\":16000,\"partial_release_excludes_worker\":true,\"script_sections_independent\":true}}\n");
}
static void lists(){
 require(!g_freeAssetEntryHead,"initial BSS freehead is null");for(auto h:db_hashTable)require(!h,"initial BSS hash table zero");
 for(unsigned i=0;i<32768;++i)db_hashTable[i]=static_cast<unsigned short>(i^0x6d3b);unsigned cycles=0;
 for(unsigned round=0;round<2;++round)for(bool mod:{false,true,false}){
  Sys_EnterCriticalSection(CRITSECT_DBHASH);std::memset(static_cast<void*>(g_assetEntryPool),0xa5,sizeof(g_assetEntryPool));g_freeAssetEntryHead=nullptr;DB_InitAssetEntryPool(mod);unsigned last=mod?65535:32767,expected=last-15;
  require(g_freeAssetEntryHead==g_assetEntryPool+16,"canonical free list skips reserved0..15");
  const auto*b=reinterpret_cast<const unsigned char*>(g_assetEntryPool);for(unsigned i=0;i<sizeof(XAssetEntryPoolEntry);++i)require(b[i]==0xa5,"sentinel0 untouched");
  for(unsigned i=1;i<65536;++i){XAssetEntryPoolEntry*next;std::memcpy(&next,&g_assetEntryPool[i],sizeof(next));if(i<=last)require(next==(i==last?nullptr:g_assetEntryPool+i+1),"exact initialization pointer chain including reserved1..15");else for(unsigned j=0;j<sizeof(void*);++j)require(b[sizeof(XAssetEntryPoolEntry)*i+j]==0xa5,"inactive upper half untouched");for(unsigned j=sizeof(void*);j<sizeof(XAssetEntryPoolEntry);++j)require(b[sizeof(XAssetEntryPoolEntry)*i+j]==0xa5,"entry initialization changes pointer slot only");}
  unsigned count=0;auto*p=g_freeAssetEntryHead;while(p){require(p==g_assetEntryPool+16+count&&count<expected,"every allocatable node aligned unique ascending");auto index=static_cast<std::size_t>(p-g_assetEntryPool);require(index>=16&&index<=last&&static_cast<unsigned short>(index)==index,"link index fits original16bits");++count;std::memcpy(&p,p,sizeof(p));}require(count==expected,"original full allocatable count");for(unsigned i=0;i<32768;++i)require(db_hashTable[i]==static_cast<unsigned short>(i^0x6d3b),"helper never clears hash buckets");
  Sys_LeaveCriticalSection(CRITSECT_DBHASH);++cycles;
  std::printf("{\"entry_init\":{\"round\":%u,\"mod\":%s,\"head_index\":16,\"last_index\":%u,\"free_nodes\":%u,\"stride\":%zu,\"reserved_sentinel_untouched\":true,\"payload_tails_unchanged\":true,\"hash_buckets_unchanged\":true}}\n",round,mod?"true":"false",last,count,sizeof(XAssetEntryPoolEntry));
 }
 std::memset(db_hashTable,0,sizeof(db_hashTable));require(cycles==6,"six valid quiescent initialization controls");
}
int main(int argc,char**argv){
#if defined(_WIN32)
 _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
#endif
 if(argc==2&&!std::strcmp(argv[1],"--unsupported-section")){Sys_EnterCriticalSection(CRITSECT_COUNT);require(false,"unsupported section must abort");}
 require(argc==1,"known arguments");locks();lists();std::printf("{\"schema\":\"BlackOpsARM.TASK31.entry-init.v1\",\"pointer_bytes\":%zu,\"entry_bytes\":%zu,\"pool_bytes\":%zu,\"hash_bytes\":%zu,\"initializations\":6,\"full_db_init_executed\":false,\"entry_allocation_executed\":false,\"registry_publication_executed\":false}\n",sizeof(void*),sizeof(XAssetEntryPoolEntry),sizeof(g_assetEntryPool),sizeof(db_hashTable));
}
