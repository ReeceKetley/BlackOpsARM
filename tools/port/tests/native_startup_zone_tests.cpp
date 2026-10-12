// Controlled caller inputs are test-only. Production loader never initializes
// Dvars/Cmd or claims Com_Init closure from this executable.
#include <platform/native_zone_load.h>
#include <database/db_registry_add_api.h>
#include <database/db_stream.h>
#include <universal/physicalmemory.h>
#include <zlib/zlib.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <bitset>
#if defined(BO1_NATIVE_THREAD_INTEGRATION)
#include <platform/android/thread_services.h>
#include <universal/thread_data.h>
#endif
#if defined(__ANDROID__)
#include <platform/android/hunk_services.h>
#include <platform/android/virtual_memory.h>
#include <platform/android/zone_memory.h>
bool Bo1_TestZoneRegistrationFailure(){return false;}
#else
#include <Windows.h>
#endif
void InitializeEntryOwnerInput();
void InitializeRegistryAddInputs();
using namespace bo1::fastfile;
static void require(bool value,const char *why){if(!value){std::fprintf(stderr,"FAIL %s\n",why);std::exit(1);}}
static unsigned freeEntries(){unsigned n=0;for(auto *p=g_freeAssetEntryHead;p&&n<65536;p=p->next)++n;return n;}
template<class T,int N>static unsigned freeHeaders(const XAssetPool<T,N> &pool){
    std::bitset<N> seen;unsigned count=0;
    const auto base=reinterpret_cast<std::uintptr_t>(pool.entries);
    for(auto *p=pool.freeHead;p;p=p->next){
        const auto address=reinterpret_cast<std::uintptr_t>(p);
        require(address>=base&&(address-base)%sizeof(pool.entries[0])==0,"free header is an exact pool slot");
        const auto index=(address-base)/sizeof(pool.entries[0]);
        require(index<N&&!seen[index],"free header chain conserves unique slots");seen[index]=true;++count;
    }return count;
}
static unsigned long checksum(const void *p,std::size_t n){return static_cast<unsigned long>(adler32(1,static_cast<const Bytef*>(p),static_cast<unsigned>(n)));}
int main(int argc,char **argv){
    require(argc==2,"genuine retail file required");
#if defined(BO1_NATIVE_THREAD_INTEGRATION)
    Sys_InitMainThread();
    require(Sys_GetThreadContext()==0&&Sys_IsMainThread(),"actual native main role");
    require(Sys_GetValue(1)==&va_info[0]&&Sys_GetValue(2)==g_com_error[0]&&
        Sys_GetValue(3)==&g_traceThreadInfo[0]&&Sys_GetValue(4)==&g_cmd_args[0],"actual canonical main TLS slots");
    g_cmd_args[0].nesting=17;g_cmd_args[0].totalUsedArgvPool=19;g_cmd_args[0].totalUsedTextPool=23;
    Cmd_ResetArgs(&g_cmd_args[0]);
    require(g_cmd_args[0].nesting==-1&&g_cmd_args[0].totalUsedArgvPool==0&&g_cmd_args[0].totalUsedTextPool==0,"actual Cmd_ResetArgs");
#if defined(__ANDROID__)
    require(bo1::android::IsHunkMainThread(),"native main is actual hunk owner");
#endif
    std::printf("{\"startup_thread_integration\":{\"pointer_bytes\":%zu,\"thread_context\":0,\"canonical_slots\":4,\"actual_cmd_reset\":true,\"com_init_executed\":false}}\n",sizeof(void*));
#endif
#if defined(__ANDROID__)
    bo1::android::RegisterHunkMainThread();const auto page=bo1::android::VirtualPageSize();
#else
    PMem_Init();SYSTEM_INFO info{};GetSystemInfo(&info);const auto page=info.dwPageSize;
#endif
    Sys_InitializeCriticalSections();InitializeEntryOwnerInput();InitializeRegistryAddInputs();
    DB_InitAssetEntryPool(false);DB_InitPool<decltype(g_StringTablePool)>(&g_StringTablePool,POOLSIZE_STRINGTABLE);
    DB_InitPool<decltype(g_RawFilePool)>(&g_RawFilePool,POOLSIZE_RAWFILE);
    XBlock detached[7]{};DB_InitStreams(detached);const auto baseline=freeEntries();require(baseline==32752,"actual free-entry baseline");
    for(unsigned cycle=0;cycle<4;++cycle){
        NativeZoneFile file;NativeZoneLoad load;
        ZoneLimits limits{64u*1024u*1024u,INT32_MAX-8192u,4096,cycle%2,0};Bo1InflateLimits inflate;inflate.maxDecodedBytes=1024*1024;
        require(file.Open(argv[1],"production-native-startup-zone",inflate,limits,page,8*1024*1024)==NativeZoneResult::Ok,"actual full read/codec/owner");
        const auto prepared=load.Prepare(file,1);
        if(prepared!=NativeLoadResult::Ok)std::fprintf(stderr,"PREPARE result=%u\n",static_cast<unsigned>(prepared));
        require(prepared==NativeLoadResult::Ok,"complete production schema preflight");
        require(file.Close()==NativeZoneResult::BorrowedChildren,"prepared production context retains owner before registration");
        NativeZoneLoad competing;
        require(competing.Prepare(file,1)==NativeLoadResult::Occupied,"same owner cannot acquire a second production context");
        require(load.Roots().size()==71,"genuine MP all71 directory roots");
        require(load.Run()==NativeLoadResult::Ok&&file.Position()==150108&&XAssetEntryHighCount==71,"actual stock root dispatch complete consumption/registration");
        const auto contextAddress=reinterpret_cast<std::uintptr_t>(&load);
        const auto detachedAddress=reinterpret_cast<std::uintptr_t>(g_streamBlocks);
        require(detachedAddress<contextAddress||detachedAddress-contextAddress>=sizeof(load),"detached descriptors outlive the loader instance");
        require(file.Close()==NativeZoneResult::BorrowedChildren,"detached production streams do not release registered child ownership");
        std::memset(file.Blocks()[0].data,0xa5,(std::min<unsigned>)(file.Blocks()[0].size,128));
        unsigned queries=0;
        for(std::size_t i=0;i<load.Roots().size();++i){const auto &row=load.Roots()[i];XAsset asset{row.type,row.registered};const auto *name=DB_GetXAssetName(&asset);
            require(name==reinterpret_cast<const char*>(file.Blocks()[4].data+row.nativeName),"name remains zone-owned after TEMP overwrite");
            require(!std::memcmp(name,file.Decoded().decoded.data()+row.nameSource,row.nameBytes),"name exact source bytes");
            Sys_EnterCriticalSection(CRITSECT_DBHASH);auto *entry=DB_FindXAssetEntry(row.type,name);require(entry&&entry->entry.asset.header.data==row.registered.data&&entry->entry.zoneIndex==1,"real registered lookup identity");Sys_LeaveCriticalSection(CRITSECT_DBHASH);
            std::printf("{\"startup_zone_asset\":{\"cycle\":%u,\"index\":%zu,\"type\":%u,\"source_start\":%zu,\"source_end\":%zu,\"name_bytes\":%zu,\"name_adler32\":%lu,\"disk_end\":%llu,\"native_end\":%llu",cycle,i,static_cast<unsigned>(row.type),row.sourceStart,row.sourceEnd,row.nameBytes,checksum(name,row.nameBytes),static_cast<unsigned long long>(row.diskEnd),static_cast<unsigned long long>(row.nativeEnd));
            if(row.type==ASSET_TYPE_RAWFILE){const auto *raw=row.registered.rawfile;require(raw->len==row.length&&row.bufferBytes==static_cast<std::size_t>(raw->len)+1,"actual RawFile length");require(!std::memcmp(raw->buffer,file.Decoded().decoded.data()+row.bufferSource,row.bufferBytes),"full raw buffer survives TEMP reuse");std::printf(",\"length\":%d,\"buffer_bytes\":%zu,\"buffer_adler32\":%lu",raw->len,row.bufferBytes,checksum(raw->buffer,row.bufferBytes));}
            else {const auto *table=row.registered.stringTable;require(table->rowCount==row.rows&&table->columnCount==row.columns&&reinterpret_cast<unsigned char*>(table->values)==file.Blocks()[4].data+row.nativeValues&&reinterpret_cast<unsigned char*>(table->cellIndex)==file.Blocks()[4].data+row.nativeIndices,"actual table child ownership/dimensions");
                const auto cells=table->rowCount*table->columnCount;uLong hashes=1,strings=1;std::printf(",\"rows\":%d,\"columns\":%d,\"cells\":%d,\"query_rows\":[",table->rowCount,table->columnCount,cells);
                for(int cell=0;cell<cells;++cell){const auto &value=table->values[cell];hashes=adler32(hashes,reinterpret_cast<const Bytef*>(&value.hash),4);strings=adler32(strings,reinterpret_cast<const Bytef*>(value.string),static_cast<unsigned>(std::strlen(value.string)+1));require(!std::strcmp(StringTable_GetColumnValueForRow(table,cell/table->columnCount,cell%table->columnCount),value.string),"real table column lookup");const auto found=StringTable_LookupRowNumForValue(table,cell%table->columnCount,value.string);require(found>=0&&found<table->rowCount&&!std::strcmp(table->values[found*table->columnCount+cell%table->columnCount].string,value.string),"real table row lookup");std::printf("%s%d",cell?",":"",found);queries+=2;}
                std::printf("],\"cell_hash_adler32\":%lu,\"cell_string_adler32\":%lu,\"normalized_index_adler32\":%lu",static_cast<unsigned long>(hashes),static_cast<unsigned long>(strings),checksum(table->cellIndex,static_cast<std::size_t>(cells)*2));
            }std::printf("}}\n");
        }
        Sys_EnterCriticalSection(CRITSECT_DBHASH);
        const auto &firstRoot=load.Roots()[0];XAsset firstAsset{firstRoot.type,firstRoot.registered};
        auto *firstEntry=DB_FindXAssetEntry(firstRoot.type,DB_GetXAssetName(&firstAsset));
        require(firstEntry!=nullptr,"live primary entry for reference-removal control");firstEntry->entry.inuse=1;
        Sys_LeaveCriticalSection(CRITSECT_DBHASH);
        require(load.Remove()==NativeLoadResult::Referenced&&XAssetEntryHighCount==71&&freeEntries()==baseline-71,"referenced-zone removal refuses before changing any entry");
        require(file.Close()==NativeZoneResult::BorrowedChildren,"refused removal retains all child ownership");
        Sys_EnterCriticalSection(CRITSECT_DBHASH);firstEntry->entry.inuse=0;Sys_LeaveCriticalSection(CRITSECT_DBHASH);
        require(load.Remove()==NativeLoadResult::Ok&&!XAssetEntryHighCount&&freeEntries()==baseline,"real scoped removal/header frees restore entry baseline");
        require(freeHeaders(g_StringTablePool)==POOLSIZE_STRINGTABLE&&freeHeaders(g_RawFilePool)==POOLSIZE_RAWFILE,"all real header pool slots restored");
        require(file.Close()==NativeZoneResult::Ok,"all registered references removed before owner release");
#if defined(__ANDROID__)
        require(!bo1::android::ZoneMemoryCount()&&!bo1::android::ZoneMemoryRequestedBytes(),"real named zone ownership restored");
#else
        require(!g_mem.prim[0].allocListCount&&!g_mem.prim[1].allocListCount&&!g_mem.prim[0].pos&&g_mem.prim[1].pos==g_mem.size,"real PMem ownership/cursors restored");
#endif
        std::printf("{\"startup_zone_cycle\":{\"cycle\":%u,\"assets\":71,\"decoded_consumed\":150108,\"queries\":%u,\"high_water\":%d,\"free_entries\":%u,\"controlled_caller_inputs\":true,\"com_init_executed\":false}}\n",cycle,queries,XAssetEntryHighWaterMark,freeEntries());
    }return 0;
}
