#pragma once
#include <database/db_asset_pool.h>
#include <bitset>
namespace rawfile_owner_fixture {
static_assert(sizeof(XAssetPoolEntry<RawFile>)==sizeof(RawFile));
static_assert(offsetof(XAssetPoolEntry<RawFile>,entry)==0&&offsetof(XAssetPoolEntry<RawFile>,next)==0);
static std::bitset<POOLSIZE_RAWFILE_MOD> live;
static unsigned active;
static RawFile* persistent;
static std::array<unsigned char,sizeof(RawFile)> saved;
static unsigned index(const void*p){auto n=reinterpret_cast<uintptr_t>(p),b=reinterpret_cast<uintptr_t>(&g_RawFilePool.entries[0]);return !p||n<b||(n-b)%sizeof(RawFile)||(n-b)/sizeof(RawFile)>=active?POOLSIZE_RAWFILE_MOD:unsigned((n-b)/sizeof(RawFile));}
static bool init(int n){if(n<1||n>POOLSIZE_RAWFILE_MOD||live.any()||persistent)return false;DB_InitPool<decltype(g_RawFilePool)>(&g_RawFilePool,n);active=unsigned(n);return true;}
static unsigned freeCount(bool ascending=false){std::bitset<POOLSIZE_RAWFILE_MOD> seen;auto*p=g_RawFilePool.freeHead;unsigned n=0;while(p){auto i=index(p);require(i<active&&!seen[i]&&!live[i],"RawFile unique owned free node");if(ascending)require(i==n,"RawFile ascending baseline");seen[i]=true;++n;decltype(p) next;std::memcpy(&next,p,sizeof(next));p=next;}require(n+live.count()==active,"RawFile free/live conservation");return n;}
static XAssetHeader alloc(){auto h=DB_AllocXAsset<RawFile>(&g_RawFilePool);if(h.data){auto i=index(h.data);require(i<active&&!live[i],"RawFile actual allocation unique");live[i]=true;}return h;}
static bool release(XAssetHeader h){auto i=index(h.data);if(i>=active||!live[i]||h.data==persistent)return false;::DB_FreeXAssetHeader_EmblemSet_(reinterpret_cast<XAssetPoolEntry<EmblemSet>**>(&g_RawFilePool.freeHead),h);live[i]=false;return true;}
static bool clone(const XAsset&from,XAsset&to,std::size_t sb,std::size_t db){
 auto a=reinterpret_cast<uintptr_t>(from.header.data),b=reinterpret_cast<uintptr_t>(to.header.data);auto i=index(to.header.data);
 if(from.type!=ASSET_TYPE_RAWFILE||to.type!=ASSET_TYPE_RAWFILE||i>=active||!live[i]||!a||!b||a%alignof(RawFile)||b%alignof(RawFile)||sb<sizeof(RawFile)||db<sizeof(RawFile)||a>UINTPTR_MAX-sizeof(RawFile)||b>UINTPTR_MAX-sizeof(RawFile)||(a<b+sizeof(RawFile)&&b<a+sizeof(RawFile)))return false;
 DB_CloneXAssetInternal(&from,&to);return true;
}
static void fixtures(){
 require(!persistent&&!live.any()&&DB_GetXAssetTypeSize(36)==sizeof(RawFile)&&DB_SizeofXAsset_RawFile_()==sizeof(RawFile),"RawFile actual type36 size dispatch");unsigned allocated=0,invalid=0;
 for(int n:{1,2,POOLSIZE_RAWFILE,POOLSIZE_RAWFILE_MOD}){
  require(init(n)&&freeCount(true)==unsigned(n),"RawFile actual initialization policy");std::vector<XAssetHeader> h;
  for(int i=0;i<n;++i){auto x=alloc();require(index(x.data)==unsigned(i),"RawFile ascending stride");h.push_back(x);++allocated;}
  std::vector<unsigned char> before(sizeof(g_RawFilePool));std::memcpy(before.data(),&g_RawFilePool,before.size());require(!alloc().data&&!std::memcmp(before.data(),&g_RawFilePool,before.size())&&!freeCount(),"RawFile null exhaustion no writes");
  for(auto i=h.rbegin();i!=h.rend();++i){std::memset(i->data,0x35,sizeof(RawFile));require(release(*i),"RawFile release valid");auto*p=static_cast<unsigned char*>(i->data);for(unsigned j=sizeof(void*);j<sizeof(RawFile);++j)require(p[j]==0x35,"RawFile free preserves payload tail");}
  require(!live.any()&&freeCount(true)==unsigned(n),"RawFile complete baseline without reset");
 }
 require(init(2),"RawFile LIFO setup");auto a=alloc(),b=alloc();require(release(a)&&release(b),"RawFile LIFO free");auto c=alloc(),d=alloc();require(c.data==b.data&&d.data==a.data&&release(c)&&release(d)&&freeCount(true)==2,"RawFile actual LIFO reuse");
 auto reject=[&](auto fn){std::array<unsigned char,sizeof(g_RawFilePool)> before;std::memcpy(before.data(),&g_RawFilePool,before.size());auto was=live;require(!fn()&&was==live&&!std::memcmp(before.data(),&g_RawFilePool,before.size()),"RawFile invalid caller rejected before unchecked leaf");++invalid;};
 for(int n:{0,-1,POOLSIZE_RAWFILE_MOD+1,INT32_MAX})reject([&]{return init(n);});a=alloc();reject([&]{return init(2);});reject([&]{return release(XAssetHeader{});});RawFile foreign{};reject([&]{return release(XAssetHeader(&foreign));});reject([&]{return release(XAssetHeader(&g_StringTablePool.entries[0]));});reject([&]{return release(XAssetHeader(static_cast<unsigned char*>(a.data)+1));});require(release(a),"RawFile valid guarded release");reject([&]{return release(a);});
 a=alloc();XAsset from{},to{};from.type=to.type=ASSET_TYPE_RAWFILE;from.header.data=&foreign;to.header=a;
 auto bad=from;bad.type=ASSET_TYPE_STRINGTABLE;reject([&]{return clone(bad,to,sizeof(RawFile),sizeof(RawFile));});bad=from;bad.header.data=nullptr;reject([&]{return clone(bad,to,sizeof(RawFile),sizeof(RawFile));});
 reject([&]{return clone(from,to,sizeof(RawFile)-1,sizeof(RawFile));});reject([&]{return clone(from,to,sizeof(RawFile),sizeof(RawFile)-1);});bad=from;bad.header=a;reject([&]{return clone(bad,to,sizeof(RawFile),sizeof(RawFile));});bad=from;bad.header.data=reinterpret_cast<unsigned char*>(&foreign)+1;reject([&]{return clone(bad,to,sizeof(RawFile),sizeof(RawFile));});bad=from;bad.header.data=reinterpret_cast<void*>(UINTPTR_MAX-(alignof(RawFile)-1));reject([&]{return clone(bad,to,sizeof(RawFile),sizeof(RawFile));});
 auto wrong=to;wrong.type=ASSET_TYPE_STRINGTABLE;reject([&]{return clone(from,wrong,sizeof(RawFile),sizeof(RawFile));});wrong=to;wrong.header.data=&foreign;reject([&]{return clone(from,wrong,sizeof(RawFile),sizeof(RawFile));});
 require(clone(from,to,sizeof(RawFile),sizeof(RawFile))&&!std::memcmp(a.data,&foreign,sizeof(RawFile))&&release(a),"RawFile actual shallow clone");reject([&]{return clone(from,to,sizeof(RawFile),sizeof(RawFile));});require(freeCount(true)==2&&init(POOLSIZE_RAWFILE)&&freeCount(true)==POOLSIZE_RAWFILE,"RawFile normal production baseline");
 std::printf("{\"rawfile_owner_fixtures\":{\"active_sizes\":[1,2,1024,2048],\"allocated_nodes\":%u,\"exhaustions\":4,\"invalid_caller_preconditions\":%u,\"header_bytes\":%zu,\"pool_bytes\":%zu,\"baseline_restored\":true,\"lifo_verified\":true,\"full_registry_executed\":false}}\n",allocated,invalid,sizeof(RawFile),sizeof(g_RawFilePool));
}
static void prepare(RawFile*temp){require(!persistent&&!live.any()&&freeCount(true)==POOLSIZE_RAWFILE,"RawFile genuine pool starts baseline");XAsset from{},to{};from.type=to.type=ASSET_TYPE_RAWFILE;from.header.data=temp;
#if defined(BO1_ENTRY_GENUINE_GATE)
 #if defined(BO1_REGISTRY_ADD_GATE)
 to.header=entry_owner_fixture::registerHeader(ASSET_TYPE_RAWFILE,temp);
#else
 to.header=entry_owner_fixture::acquire(ASSET_TYPE_RAWFILE);
#endif
const auto id=index(to.header.data);require(id<active&&!live[id],"actual RawFile entry header unique in pool");live[id]=true;
#else
 to.header=alloc();
#endif
#if !defined(BO1_REGISTRY_ADD_GATE)
require(clone(from,to,sizeof(RawFile),sizeof(RawFile)),"RawFile actual clone before TEMP pop");
#endif
persistent=static_cast<RawFile*>(to.header.data);std::memcpy(saved.data(),temp,sizeof(RawFile));require(persistent!=temp&&!std::memcmp(persistent,temp,sizeof(RawFile)),"RawFile persistent exact native header");}
static void verify(RawFile*temp,XBlock(&blocks)[7],const RawFilePayloadPlan&p,const StringTablePayloadPlan&first,bool measurement){
 require(persistent&&live[index(persistent)]&&!std::memcmp(persistent,saved.data(),sizeof(RawFile)),"RawFile persistent survives Pop");auto state=frontend_fixture::streamState();auto owners=frontend_fixture::counts();std::vector<unsigned char> children(static_cast<std::size_t>(p.nativeEnd));std::memcpy(children.data(),blocks[4].data,children.size());
 require(persistent->name==reinterpret_cast<char*>(blocks[4].data+p.name.nativeStart)&&persistent->buffer==reinterpret_cast<char*>(blocks[4].data+p.bufferNativeStart)&&persistent->len==140&&p.name.bytes==17&&p.bufferBytes==141,"RawFile genuine child ownership and original length");
 std::memset(temp,0xa5,sizeof(RawFile));require(!std::memcmp(persistent,saved.data(),sizeof(RawFile))&&!std::memcmp(children.data(),blocks[4].data,children.size()),"RawFile complete child bytes survive TEMP overwrite");executeLookups(first,owner_fixture::persistent,false);require(!lastLookup.rowMisses&&!lastLookup.rowDisagreements&&!lastLookup.columnDisagreements&&state==frontend_fixture::streamState()&&owners==frontend_fixture::counts(),"RawFile TEMP overwrite preserves first-table queries/cursors/owners");
 if(measurement)std::printf("{\"rawfile_owner_measurements\":{\"header_bytes\":%zu,\"name_bytes\":17,\"buffer_bytes\":141,\"temp_overwritten_bytes\":%zu,\"persistent_header_unchanged\":true,\"children_unchanged\":true,\"first_table_queries\":%u,\"full_registry_executed\":false}}\n",sizeof(RawFile),sizeof(RawFile),lastLookup.queries);
}
static void finish(){const bool had=persistent!=nullptr;std::printf("{\"rawfile_owner_release\":{\"allocated\":%s,\"baseline_restored\":true,\"before_zone_release\":true}}\n",had?"true":"false");if(!persistent){require(!live.any()&&freeCount(true)==POOLSIZE_RAWFILE,"deferred asset has no RawFile owner");return;}XAssetHeader h(persistent);persistent=nullptr;
#if defined(BO1_ENTRY_GENUINE_GATE)
 auto id=index(h.data);require(id<active&&live[id],"actual RawFile entry-owned release precondition");entry_owner_fixture::release(h.data);live[id]=false;require(!live.any()&&freeCount(true)==POOLSIZE_RAWFILE,
#else
 require(release(h)&&!live.any()&&freeCount(true)==POOLSIZE_RAWFILE,
#endif
"RawFile header released before zone children; baseline restored");saved={};}
}
