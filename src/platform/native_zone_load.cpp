#include "native_zone_load.h"
#include "fastfile_assetlist.h"
#include "fastfile_asset_directory.h"
#include "fastfile_stringtable_header.h"
#include "fastfile_stringtable_cells.h"
#include "fastfile_stringtable_indices.h"
#include "fastfile_raw_string.h"
#include "fastfile_rawfile_header.h"
#include "fastfile_rawfile_payload.h"
#include <database/db_native_zone_load.h>
#include <database/db_registry_add_api.h>
#include <database/db_stream.h>
#include <database/db_stream_load.h>
#include <database/db_expand.hpp>
#include <algorithm>
#include <cstring>
#include <climits>
#include <cstdlib>

namespace bo1::fastfile {
namespace {
// Serial binding. No async replacement or fake worker service is provided.
NativeZoneLoad *active;
// DB streams retain their descriptor-array address. Detached storage must live
// beyond the loader instance, including after its successful removal/destruction.
XBlock detachedBlocks[7]{};
struct StringProof { std::uint64_t disk,native; std::size_t source,bytes; };
std::uint32_t u32(const unsigned char *p) { std::uint32_t x; std::memcpy(&x,p,4); return x; }
bool ascii(const unsigned char *p,std::size_t n) { for(std::size_t i=0;i<n;++i) if(p[i]>=128)return false; return true; }
bool services(unsigned zone) {
    // g_modLazyModels defaults true and only affects top-level XModel loads in
    // DB_IsModDeferringLoad. Neither supported type enters that policy path.
    return zone && zone<33 && DB_NativeZonePolicyReady(zone) && g_zoneIndex==zone && db_xassetdebug && db_xassetdebugtype
        && useFastFile && useFastFile->current.enabled && !g_isRecoveringLostDevice
        && !g_mayRecoverLostAssets && !g_modDeferredCount && !g_modDeferredUsed;
}
std::size_t ownedCapacity(unsigned char *p) {
    if (!active || !active->File() || !g_streamBlocks || p!=DB_GetStreamPos()) return 0;
    const auto &block=g_streamBlocks[g_streamPosIndex];
    const auto address=reinterpret_cast<std::uintptr_t>(p),base=reinterpret_cast<std::uintptr_t>(block.data);
    if(!block.data||address<base||address-base>block.size)return 0;
    return block.size-static_cast<std::size_t>(address-base);
}
struct NativeReadFault {};
// Canonical expanders can own temporary vectors while invoking these reads.
// Unwind those frames before entering the engine's longjmp recovery route.
[[noreturn]] void readFailure() { throw NativeReadFault{}; }
}
NativeZoneLoad::~NativeZoneLoad() {
    // Silently discarding a live binding would lose its removal/recovery state.
    if(file_ || active==this)std::abort();
}
void NativeZoneReadData(unsigned char *destination,int bytes) {
    if(!active||!active->File()||bytes<0)readFailure();
    // Scratch callers are canonical expanders with their own exact n-byte stack
    // or vector allocation. Owned writes additionally check the actual XBlock.
    std::size_t capacity=static_cast<std::size_t>(bytes);
    const auto address=reinterpret_cast<std::uintptr_t>(destination);
    for(const auto &block:active->File()->Blocks()) {
        const auto base=reinterpret_cast<std::uintptr_t>(block.data);
        if(block.data&&address>=base&&address-base<=block.size) {
            capacity=block.size-static_cast<std::size_t>(address-base);break;
        }
    }
    if(!active->File()->Read(destination,capacity,bytes))readFailure();
}
int NativeZoneReadCString(unsigned char *destination) {
    int bytes;
    if(!active||!active->File()||g_streamPosIndex!=4)readFailure();
    const auto *source=active->File()->Current();
#if BO1_NATIVE_64BIT
    const auto disk=DB_StreamWin32Pos(4);
#else
    const auto disk=static_cast<unsigned>(destination-g_streamBlocks[4].data);
#endif
    if(!active->File()->ReadCString(destination,ownedCapacity(destination),bytes)||bytes<1
        ||destination[bytes-1]||std::memcmp(destination,source,static_cast<std::size_t>(bytes)))readFailure();
    active->initialized_.push_back({g_streamBlocks[4].data,destination,active->generation_,disk,
        static_cast<std::uint64_t>(destination-g_streamBlocks[4].data),static_cast<std::uint64_t>(bytes)});
    return bytes;
}
void NativeZoneValidatePointerOffset(void *field) {
    if(!active||!active->File()||!active->File()->IsOpen())readFailure();
    std::uintptr_t value;std::memcpy(&value,field,sizeof(value));
    const auto tag=static_cast<std::uint32_t>(value);
    const auto &blocks=active->File()->Blocks();
    const InitializedStringOwner owner{blocks[4].data,active->File()->Plan().file.blockSize[4],blocks[4].size,active->generation_};
    InitializedStringOffsetPlan plan;
    if(PlanInitializedStringOffset(tag,owner,active->initialized_,plan)!=InitializedStringResult::Ok)readFailure();
#if BO1_NATIVE_64BIT
    if(DB_StreamExactLookup(4,plan.diskStart)!=plan.address||DB_StreamMapLookup(4,plan.diskStart)!=plan.address)readFailure();
#else
    if(blocks[4].data+plan.diskStart!=plan.address)readFailure();
#endif
}
NativeLoadResult NativeZoneLoad::Prepare(NativeZoneFile &file,unsigned zone) {
    if(file_||active||file.childrenRetained_)return NativeLoadResult::Occupied;
    if(!file.IsOpen()||file.Position()||!services(zone)||g_streamPosStackIndex)return NativeLoadResult::CallerState;
    const auto &data=file.Decoded().decoded;const auto &plan=file.Plan();
    AssetListPlan envelope;
    if(PlanAssetList(data.data(),data.size(),data.size(),plan.limits,4096,envelope)!=AssetListResult::Ok)return NativeLoadResult::Malformed;
    if(envelope.disk.stringCount||envelope.disk.stringsTag||!envelope.disk.assetsTag)return NativeLoadResult::Unsupported;
    AssetDirectoryPlan dir;
    if(PlanAssetDirectory(envelope.disk.assetCount,envelope.disk.assetsTag,data.data()+52,data.size()-52,
        plan.file.blockSize[4],plan.logical[4],0,0,dir)!=AssetDirectoryResult::Ok)return NativeLoadResult::Malformed;
    // Bound metadata/planning work by the actual supported retail header pools,
    // before allocating per-root records or performing name comparisons.
    unsigned plannedTables=0,plannedRaws=0;
    for(int i=0;i<dir.count;++i) {
        const auto type=u32(data.data()+52+8*static_cast<std::size_t>(i));
        if(u32(data.data()+56+8*static_cast<std::size_t>(i))!=0xffffffffu)return NativeLoadResult::Unsupported;
        if(type==ASSET_TYPE_RAWFILE) {
            if(++plannedRaws>POOLSIZE_RAWFILE)return NativeLoadResult::Capacity;
        } else if(type==ASSET_TYPE_STRINGTABLE) {
            if(++plannedTables>POOLSIZE_STRINGTABLE)return NativeLoadResult::Capacity;
        } else return NativeLoadResult::Unsupported;
    }
    std::vector<NativeRootRecord> roots;std::vector<StringProof> strings;
    auto source=52+static_cast<std::size_t>(dir.diskBytes);auto disk=dir.diskEnd,native=dir.nativeEnd;
    unsigned tables=0,raws=0;
    for(int i=0;i<dir.count;++i) {
        NativeRootRecord row;row.type=static_cast<XAssetType>(u32(data.data()+52+8*static_cast<std::size_t>(i)));
        const auto tag=u32(data.data()+56+8*static_cast<std::size_t>(i));row.sourceStart=source;
        if(tag!=0xffffffffu)return NativeLoadResult::Unsupported;
        if(row.type==ASSET_TYPE_RAWFILE) {
            RawFileHeaderPlan h;RawFilePayloadPlan p;
            if(PlanRawFileHeader(36,tag,data.data()+source,data.size()-source,data.size()-source,
                plan.file.blockSize[0],plan.logical[0],0,0,h)!=RawFileHeaderResult::Ok)return NativeLoadResult::Malformed;
            source+=12;
            if(PlanRawFilePayload(h.nameTag,h.length,h.bufferTag,data.data()+source,data.size()-source,
                plan.file.blockSize[4],plan.logical[4],disk,native,p)!=RawFilePayloadResult::Ok)return NativeLoadResult::Malformed;
            if(!p.name.present)return NativeLoadResult::Unsupported;
            row.nameSource=source;row.nameBytes=static_cast<std::size_t>(p.name.bytes);row.nativeName=p.name.nativeStart;
            source+=static_cast<std::size_t>(p.sourceBytes);row.bufferBytes=static_cast<std::size_t>(p.bufferBytes);
            row.bufferSource=source-row.bufferBytes;row.length=h.length;disk=p.diskEnd;native=p.nativeEnd;++raws;
        } else if(row.type==ASSET_TYPE_STRINGTABLE) {
            StringTableHeaderPlan h;RawStringPlan name;StringTableCellsPlan cells;StringTableIndicesPlan indices;
            if(PlanStringTableHeader(37,tag,data.data()+source,data.size()-source,data.size()-source,
                plan.file.blockSize[0],plan.logical[0],0,0,h)!=StringTableHeaderResult::Ok)return NativeLoadResult::Malformed;
            source+=20;
            if(PlanRawString(h.nameTag,data.data()+source,data.size()-source,plan.file.blockSize[4],plan.logical[4],disk,native,name)!=RawStringResult::Ok||!name.present)return NativeLoadResult::Malformed;
            row.nameSource=source;row.nameBytes=static_cast<std::size_t>(name.bytes);row.nativeName=name.nativeStart;
            source+=row.nameBytes;disk=name.diskEnd;native=name.nativeEnd;
            if(PlanStringTableCells(h.columns,h.rows,h.valuesTag,data.data()+source,data.size()-source,
                plan.file.blockSize[4],plan.logical[4],disk,native,cells)!=StringTableCellsResult::Ok)return NativeLoadResult::Malformed;
            if(!cells.present||!h.indexTag)return NativeLoadResult::Unsupported;
            const auto cellSource=source;source+=static_cast<std::size_t>(cells.diskBytes);disk=cells.diskEnd;native=cells.nativeEnd;
            row.nativeValues=cells.nativeStart;row.columns=h.columns;row.rows=h.rows;
            for(int cell=0;cell<cells.count;++cell) {
                const auto value=u32(data.data()+cellSource+8*static_cast<std::size_t>(cell));
                if(value==0xffffffffu) {
                    RawStringPlan str;
                    if(PlanRawString(value,data.data()+source,data.size()-source,plan.file.blockSize[4],plan.logical[4],disk,native,str)!=RawStringResult::Ok)return NativeLoadResult::Malformed;
                    if(!ascii(data.data()+source,static_cast<std::size_t>(str.bytes)))return NativeLoadResult::Unsupported;
                    strings.push_back({str.diskStart,str.nativeStart,source,static_cast<std::size_t>(str.bytes)});
                    source+=static_cast<std::size_t>(str.bytes);disk=str.diskEnd;native=str.nativeEnd;
                } else {
                    if(!value||((value-1)>>29)!=4)return NativeLoadResult::Unsupported;
                    const auto within=(value-1)&0x1fffffffu;
                    const auto found=std::find_if(strings.begin(),strings.end(),[&](const StringProof &proof){return proof.disk==within;});
                    if(found==strings.end())return NativeLoadResult::Malformed;
                }
            }
            if(PlanStringTableIndices(h.columns,h.rows,h.indexTag,data.data()+source,data.size()-source,
                plan.file.blockSize[4],plan.logical[4],disk,native,indices)!=StringTableIndicesResult::Ok)return NativeLoadResult::Malformed;
            row.nativeIndices=indices.nativeStart;source+=static_cast<std::size_t>(indices.diskBytes);disk=indices.diskEnd;native=indices.nativeEnd;++tables;
        } else return NativeLoadResult::Unsupported;
        if(row.nameBytes<2||data[row.nameSource]==','||!ascii(data.data()+row.nameSource,row.nameBytes))return NativeLoadResult::Unsupported;
        for(const auto &prior:roots)if(prior.type==row.type&&!I_stricmp(reinterpret_cast<const char*>(data.data()+prior.nameSource),reinterpret_cast<const char*>(data.data()+row.nameSource)))return NativeLoadResult::NameCollision;
        row.sourceEnd=source;row.diskEnd=disk;row.nativeEnd=native;roots.push_back(row);
    }
    if(source!=data.size())return NativeLoadResult::Consumption;
    Sys_EnterCriticalSection(CRITSECT_DBHASH);
    for(const auto bucket:db_hashTable)for(unsigned index=bucket,steps=0;index;index=g_assetEntryPool[index].entry.nextHash) {
        if(index<16||index>=32768||++steps>32768){Sys_LeaveCriticalSection(CRITSECT_DBHASH);return NativeLoadResult::CallerState;}
        if(g_assetEntryPool[index].entry.zoneIndex==zone){Sys_LeaveCriticalSection(CRITSECT_DBHASH);return NativeLoadResult::CallerState;}
    }
    bool collision=false;for(const auto &row:roots)if(DB_FindXAssetEntry(row.type,reinterpret_cast<const char*>(data.data()+row.nameSource)))collision=true;
    unsigned freeEntries=0;for(auto *p=g_freeAssetEntryHead;p&&freeEntries<65536;p=p->next)++freeEntries;
    unsigned freeTables=0;for(auto *p=g_StringTablePool.freeHead;p&&freeTables<POOLSIZE_STRINGTABLE_MOD;p=p->next)++freeTables;
    unsigned freeRaws=0;for(auto *p=g_RawFilePool.freeHead;p&&freeRaws<POOLSIZE_RAWFILE_MOD;p=p->next)++freeRaws;
    Sys_LeaveCriticalSection(CRITSECT_DBHASH);
    if(collision)return NativeLoadResult::NameCollision;
    if(freeEntries<roots.size()||freeTables<tables||freeRaws<raws)return NativeLoadResult::Capacity;
    if(generation_==UINT64_MAX)return NativeLoadResult::CallerState;
    initialized_.clear();initialized_.reserve(strings.size()+roots.size());++generation_;
    if(!file.RetainChildren())return NativeLoadResult::Occupied;
    roots_=std::move(roots);file_=&file;zone_=zone;return NativeLoadResult::Ok;
}
NativeLoadResult NativeZoneLoad::Run() {
    if(!file_||active||file_->Position()||!services(zone_))return NativeLoadResult::CallerState;
    Sys_EnterCriticalSection(CRITSECT_DBHASH);
    for(const auto bucket:db_hashTable)for(unsigned index=bucket,steps=0;index;index=g_assetEntryPool[index].entry.nextHash) {
        if(index<16||index>=32768||++steps>32768||g_assetEntryPool[index].entry.zoneIndex==zone_){Sys_LeaveCriticalSection(CRITSECT_DBHASH);return NativeLoadResult::CallerState;}
    }
    for(const auto &row:roots_)if(DB_FindXAssetEntry(row.type,reinterpret_cast<const char*>(file_->Decoded().decoded.data()+row.nameSource))){Sys_LeaveCriticalSection(CRITSECT_DBHASH);return NativeLoadResult::NameCollision;}
    Sys_LeaveCriticalSection(CRITSECT_DBHASH);
    bool readFault=false;
    NativeLoadResult result=NativeLoadResult::Ok;
    try {
    active=this;DB_InitStreams(file_->Blocks());
    XFile header;NativeZoneReadData(reinterpret_cast<unsigned char*>(&header),36);
#if BO1_NATIVE_64BIT
    DB_ExpandLoad_XAssetList(&list_);
#else
    NativeZoneReadData(reinterpret_cast<unsigned char*>(&list_),16);
#endif
    DB_PushStreamPos(4);
    list_.assets=AllocLoad_FxElemVisStateSample();varXAsset=list_.assets;
#if BO1_NATIVE_64BIT
    DB_ExpandLoad_XAssetArray(varXAsset,list_.assetCount);
#else
    Load_Stream(true,reinterpret_cast<unsigned char*>(varXAsset),8*list_.assetCount);
#endif
    for(std::size_t i=0;i<roots_.size();++i) {
        auto &row=roots_[i];if(file_->Position()!=row.sourceStart)readFailure();
        varXAsset=list_.assets+i;g_dbTopAssetType=varXAsset->type;
        Load_XAsset(false);row.registered=varXAsset->header;
        if(file_->Position()!=row.sourceEnd)readFailure();
    }
    DB_PopStreamPos();g_dbTopAssetType=-1;
    if(file_->Remaining())result=NativeLoadResult::Consumption;
    } catch(const NativeReadFault &) {
        readFault=true;
    } catch(...) {
        Detach();
        throw;
    }
    Detach();
    // The catch has ended: neither a loader vector nor a live C++ exception
    // handler is bypassed by Com_Error's engine recovery transfer.
    if(readFault) {
        Com_Error(ERR_DROP,"Invalid bounded native zone read");
        std::abort();
    }
    return result;
}
void NativeZoneLoad::Detach() {
    if(active&&active!=this)return;
    initialized_.clear();
    if(active==this){active=nullptr;DB_InitStreams(detachedBlocks);}
    varXAsset=nullptr;varXAssetHeader=nullptr;varRawFile=nullptr;varRawFilePtr=nullptr;
    varStringTable=nullptr;varStringTablePtr=nullptr;varStringTableCell=nullptr;
    varXString=nullptr;varConstChar=nullptr;varshort=nullptr;g_dbTopAssetType=-1;
}
NativeLoadResult NativeZoneLoad::Remove() {
    if(active||!file_)return NativeLoadResult::CallerState;
    Sys_EnterCriticalSection(CRITSECT_DBHASH);
    // Validate every membership before any mutation. Scoped primary-only removal
    // has no render/mod/default/override behavior; full DB_Unload remains intact.
    for(const auto bucket:db_hashTable)for(unsigned index=bucket,steps=0;index;index=g_assetEntryPool[index].entry.nextHash) {
        if(index<16||index>=32768||++steps>32768){Sys_LeaveCriticalSection(CRITSECT_DBHASH);return NativeLoadResult::Referenced;}
        const auto &entry=g_assetEntryPool[index].entry;
        if(entry.nextOverride){Sys_LeaveCriticalSection(CRITSECT_DBHASH);return NativeLoadResult::Referenced;}
        if(entry.zoneIndex==zone_&&(entry.inuse||entry.nextOverride||(entry.asset.type!=ASSET_TYPE_RAWFILE&&entry.asset.type!=ASSET_TYPE_STRINGTABLE))) {
            Sys_LeaveCriticalSection(CRITSECT_DBHASH);return NativeLoadResult::Referenced;
        }
        if(entry.zoneIndex==zone_) {
            const auto *name=DB_GetXAssetName(&entry.asset);
            const auto &data=file_->Decoded().decoded;
            const auto found=std::find_if(roots_.begin(),roots_.end(),[&](const NativeRootRecord &row){return row.type==entry.asset.type&&!I_stricmp(name,reinterpret_cast<const char*>(data.data()+row.nameSource));});
            if(found==roots_.end()||(found->registered.data&&found->registered.data!=entry.asset.header.data)) {
                Sys_LeaveCriticalSection(CRITSECT_DBHASH);return NativeLoadResult::Referenced;
            }
        }
    }
    for(auto &bucket:db_hashTable) {
        auto *link=&bucket;
        while(*link) {auto *entry=g_assetEntryPool+*link;
            if(entry->entry.zoneIndex!=zone_){link=&entry->entry.nextHash;continue;}
            *link=entry->entry.nextHash;entry->entry.nextHash=0;DB_FreeXAssetEntry(entry);
        }
    }
    Sys_LeaveCriticalSection(CRITSECT_DBHASH);
    initialized_.clear();list_={};roots_.clear();file_->ReleaseChildren();file_=nullptr;zone_=0;return NativeLoadResult::Ok;
}
}
unsigned long long DB_XFileBytesLoaded() {
    return bo1::fastfile::active&&bo1::fastfile::active->File()?bo1::fastfile::active->File()->Position():0;
}
