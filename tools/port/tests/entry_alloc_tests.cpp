#include <database/db_entry_alloc_api.h>
#include <universal/assertive.h>
#include <atomic>
#include <bitset>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <vector>
#if defined(_WIN32)
#include <crtdbg.h>
#endif
static dvar_t fastFile{};
const dvar_t *useFastFile=&fastFile;
static const char *mode;
static std::bitset<65536> live;
static void require(bool ok,const char *why){if(!ok){std::fprintf(stderr,"FAIL %s\n",why);std::abort();}}
bool __cdecl Assert_MyHandler(const char*,int,int,const char*,...){require(false,"engine assertion");return false;}
char *va(const char *format,...){static thread_local char text[1024];va_list args;va_start(args,format);std::vsnprintf(text,sizeof(text),format,args);va_end(args);return text;}
void Com_Printf(int,const char *format,...){va_list args;va_start(args,format);std::vfprintf(stderr,format,args);va_end(args);}
void Com_PrintError(int,const char *format,...){va_list args;va_start(args,format);std::vfprintf(stderr,format,args);va_end(args);}
static unsigned index(const XAssetEntryPoolEntry*p){auto a=reinterpret_cast<uintptr_t>(p),b=reinterpret_cast<uintptr_t>(g_assetEntryPool);return !p||a<b||(a-b)%sizeof(*p)||(a-b)/sizeof(*p)>=65536?65536:unsigned((a-b)/sizeof(*p));}
static unsigned headerFree(void *head){unsigned count=0;while(head){require(++count<=2048,"finite header chain");std::memcpy(&head,head,sizeof(head));}return count;}
static void initialized(){DB_InitAssetEntryPool(false);DB_InitPool<decltype(g_RawFilePool)>(&g_RawFilePool,POOLSIZE_RAWFILE);DB_InitPool<decltype(g_StringTablePool)>(&g_StringTablePool,POOLSIZE_STRINGTABLE);fastFile.current.enabled=true;}
static XAssetEntryPoolEntry* allocate(XAssetType type,unsigned char zone){auto *p=DB_AllocXAssetEntry(type,zone);auto i=index(p);require(i>=16&&i<32768&&!live[i],"real entry unique owned node");live[i]=true;return p;}
static bool release(XAssetEntryPoolEntry*p){auto i=index(p);if(i<16||i>=32768||!live[i])return false;DB_FreeXAssetEntry(p);live[i]=false;return true;}
static void fatalUnlocked(){std::atomic<bool> got{false};std::thread worker([&]{bool ok=Sys_TryEnterCriticalSection(CRITSECT_DBHASH);if(ok)Sys_LeaveCriticalSection(CRITSECT_DBHASH);got.store(ok,std::memory_order_release);});worker.join();require(got.load(std::memory_order_acquire),"fatal branch releases DBHASH before reporting");}
void Com_Error(errorParm_t code,const char *format,...){
 require(code==ERR_DROP,"actual expected error class");fatalUnlocked();bool empty=!std::strcmp(mode,"--empty-entry");unsigned expected=empty?0:(!std::strcmp(mode,"--raw-limit")?1026:578);
 require(XAssetEntryHighCount==static_cast<int>(expected)&&XAssetEntryHighWaterMark==static_cast<int>(expected),"actual fatal count/highwater progress");
 require(empty?!g_freeAssetEntryHead:index(g_freeAssetEntryHead)==16+expected,"fatal freelist position");
 require(empty?headerFree(g_RawFilePool.freeHead)==1024:(!std::strcmp(mode,"--raw-limit")?!g_RawFilePool.freeHead:!g_StringTablePool.freeHead),"fatal header pool state");
 std::printf("{\"fatal_state\":{\"empty_head_injected\":%s,\"entry_high_count\":%u,\"lock_released\":true,\"atomic_rollback\":false}}\n",empty?"true":"false",expected);std::fflush(stdout);
 va_list args;va_start(args,format);std::vfprintf(stderr,format,args);va_end(args);std::fprintf(stderr,"\nFixture fatal endpoint; production longjmp recovery not certified\n");std::fflush(stderr);std::abort();
}
void __cdecl DB_EnumXAssets(XAssetType,void(__cdecl *)(XAssetHeader,void*),void*,bool){std::fprintf(stderr,"Explicit unsupported standalone LoadObj diagnostic\n");std::fflush(stderr);std::abort();}
static void setHeader(XAssetEntryPoolEntry*p,const char*name){
 if(p->entry.asset.type==ASSET_TYPE_RAWFILE){static const char bytes[]={0x41,0,0x7f};RawFile value{name,2,bytes};std::memcpy(p->entry.asset.header.data,&value,sizeof(value));}
 else{StringTable value{};value.name=name;std::memcpy(p->entry.asset.header.data,&value,sizeof(value));}
 require(DB_GetXAssetHeaderName(p->entry.asset.type,&p->entry.asset.header)==name,"actual name getter preserves native pointer");
}
static void hashInputs(XAssetEntryPoolEntry*a,XAssetEntryPoolEntry*b,XAssetEntryPoolEntry*other){std::strcpy(g_zoneNames[1].name,"zone-one");std::strcpy(g_zoneNames[2].name,"zone-two");a->entry.zoneIndex=1;b->entry.zoneIndex=2;db_hashTable[3]=static_cast<unsigned short>(index(a));a->entry.nextOverride=static_cast<unsigned short>(index(b));a->entry.nextHash=static_cast<unsigned short>(index(other));}
static void normal(){
 initialized();Sys_EnterCriticalSection(CRITSECT_DBHASH);std::vector<XAssetEntryPoolEntry*>entries;unsigned count=0;
 for(auto type:{ASSET_TYPE_RAWFILE,ASSET_TYPE_STRINGTABLE}){unsigned cap=type==ASSET_TYPE_RAWFILE?1024:576;
  for(unsigned i=0;i<cap;++i){auto*next=g_freeAssetEntryHead;next->entry.usageFrame=0x5aa5;auto*p=allocate(type,static_cast<unsigned char>(i%33));require(p==next&&p->entry.asset.type==type&&p->entry.asset.header.data&&p->entry.zoneIndex==i%33&&!p->entry.inuse&&!p->entry.nextHash&&!p->entry.nextOverride&&p->entry.usageFrame==0x5aa5,"real entry allocation preserves exact field policy");setHeader(p,type==ASSET_TYPE_RAWFILE?"raw-control":"table-control");entries.push_back(p);++count;}
 }
 require(count==1600&&XAssetEntryHighCount==1600&&XAssetEntryHighWaterMark==1600&&!g_RawFilePool.freeHead&&!g_StringTablePool.freeHead,"real header and entry capacities/counts");
 for(auto it=entries.rbegin();it!=entries.rend();++it)require(release(*it),"real entry release header before node");
 require(!live.any()&&!XAssetEntryHighCount&&XAssetEntryHighWaterMark==1600&&index(g_freeAssetEntryHead)==16&&headerFree(g_RawFilePool.freeHead)==1024&&headerFree(g_StringTablePool.freeHead)==576,"real free restores owned baselines without resetting highwater");
 auto*a=allocate(ASSET_TYPE_RAWFILE,32),*b=allocate(ASSET_TYPE_STRINGTABLE,0);require(release(a)&&release(b),"LIFO release");auto*c=allocate(ASSET_TYPE_RAWFILE,1),*d=allocate(ASSET_TYPE_STRINGTABLE,2);require(c==b&&d==a,"entry LIFO reuse across asset types");require(release(c)&&release(d)&&index(g_freeAssetEntryHead)==16,"LIFO baseline");
 unsigned rejected=0;XAssetEntryPoolEntry foreign;for(auto*p:{static_cast<XAssetEntryPoolEntry*>(nullptr),&foreign,g_assetEntryPool,g_assetEntryPool+15,reinterpret_cast<XAssetEntryPoolEntry*>(reinterpret_cast<unsigned char*>(g_assetEntryPool+16)+1),a}){auto *before=g_freeAssetEntryHead;require(!release(p)&&before==g_freeAssetEntryHead&&!XAssetEntryHighCount,"fixture invalid caller never invokes unchecked free");++rejected;}
 unsigned n=0;for(auto*p=g_freeAssetEntryHead;p;p=p->next){require(p==g_assetEntryPool+16+n&&++n<=32752,"actual recovered full entry chain");}require(n==32752,"full entry baseline");Sys_LeaveCriticalSection(CRITSECT_DBHASH);
 std::printf("{\"schema\":\"BlackOpsARM.TASK32.entry-alloc.v1\",\"pointer_bytes\":%zu,\"header_entry_allocations\":1600,\"invalid_caller_guards\":%u,\"high_count_after_release\":0,\"high_water_retained\":1600,\"free_entry_nodes\":32752,\"raw_free\":1024,\"table_free\":576,\"lifo_verified\":true,\"registry_publication_executed\":false}\n",sizeof(void*),rejected);
}
int main(int argc,char**argv){
#if defined(_WIN32)
 _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
#endif
 require(argc==2,"known mode required");mode=argv[1];Sys_InitializeCriticalSections();
 if(!std::strcmp(mode,"normal")){normal();return 0;}
 initialized();if(!std::strcmp(mode,"--loadobj-unavailable")){fastFile.current.enabled=false;DB_PrintXAssetsForType(ASSET_TYPE_RAWFILE,DB_PrintAssetName,nullptr,true);require(false,"no silent LoadObj diagnostic");}
 Sys_EnterCriticalSection(CRITSECT_DBHASH);
 if(!std::strcmp(mode,"--empty-entry")){g_freeAssetEntryHead=nullptr;DB_AllocXAssetEntry(ASSET_TYPE_RAWFILE,1);require(false,"empty entry never returns");}
 if(!std::strcmp(mode,"diagnostic")){auto*a=allocate(ASSET_TYPE_RAWFILE,1),*b=allocate(ASSET_TYPE_RAWFILE,2),*other=allocate(ASSET_TYPE_STRINGTABLE,3);setHeader(a,"name-one");setHeader(b,"name-two");setHeader(other,"other-type");hashInputs(a,b,other);Sys_LeaveCriticalSection(CRITSECT_DBHASH);int type=ASSET_TYPE_RAWFILE;DB_PrintXAssetsForType(ASSET_TYPE_RAWFILE,DB_PrintAssetName,&type,false);DB_PrintXAssetsForType(ASSET_TYPE_RAWFILE,DB_PrintAssetName,&type,true);DB_PrintAssetName(a->entry.asset.header,&type);Sys_EnterCriticalSection(CRITSECT_DBHASH);db_hashTable[3]=0;require(release(other)&&release(b)&&release(a),"diagnostic owned cleanup");Sys_LeaveCriticalSection(CRITSECT_DBHASH);std::printf("{\"diagnostic\":{\"primary_override_nonmatching_inputs\":true,\"actual_ff_traversal\":true,\"typed_callback_executed\":true,\"logical_baseline_restored\":true}}\n");return 0;}
 bool raw=!std::strcmp(mode,"--raw-limit");require(raw||!std::strcmp(mode,"--table-limit"),"known fatal mode");auto type=raw?ASSET_TYPE_RAWFILE:ASSET_TYPE_STRINGTABLE;unsigned cap=raw?1024:576;std::vector<XAssetEntryPoolEntry*>entries;for(unsigned i=0;i<cap;++i){auto*p=allocate(type,1);setHeader(p,i==0?"name-one":i==1?"name-two":"remaining-control");entries.push_back(p);}auto*other=allocate(raw?ASSET_TYPE_STRINGTABLE:ASSET_TYPE_RAWFILE,3);setHeader(other,"other-type");hashInputs(entries[0],entries[1],other);DB_AllocXAssetEntry(type,1);require(false,"header exhaustion never returns");
}
