// Controlled byte fixtures only. Real expand/stream/alias bodies; no game assets.
#include <database/db_stream.h>
#include <database/db_stream_load.h>
#include <database/db_expand.hpp>
#include <database/db_rawfile_types.h>
#include <universal/com_stringtable.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <initializer_list>
#include <array>

namespace {
alignas(16) unsigned char storage[7][4096]{};
XBlock blocks[7]{},detached[7]{};
const unsigned char* source{};unsigned available{};bool partialRead{};
uintptr_t external{};unsigned cases{},shortReads{},matrixCases{};
void check(bool condition,const char* label){if(!condition){std::fprintf(stderr,"FAIL TEMP write: %s\n",label);std::exit(1);}}
void fresh(){DB_InitStreams(blocks);source=nullptr;available=0;partialRead=false;std::memset(storage,0,sizeof(storage));}
void input(const void* data,unsigned bytes){source=static_cast<const unsigned char*>(data);available=bytes;}
uintptr_t marker(unsigned offset){return reinterpret_cast<uintptr_t>(storage[6]+offset);}
void convert(unsigned block,unsigned offset,uintptr_t expected){uintptr_t field=(block<<29)+offset+1;DB_ConvertOffsetToAlias(&field);check(field==expected,"actual checked alias converter value");}
#if BO1_NATIVE_64BIT
DB_StreamAliasResult state(unsigned block,unsigned offset,uintptr_t expected,DB_StreamAliasResult wanted){uintptr_t value=0;auto result=DB_StreamResolveAliasValue(block,offset,&value);if(result!=wanted){std::fprintf(stderr,"Alias state block=%u offset=%u expected=%d actual=%d cases=%u matrix=%u\n",block,offset,static_cast<int>(wanted),static_cast<int>(result),cases,matrixCases);}check(result==wanted,"alias state");if(result==DB_StreamAliasResult::Active||result==DB_StreamAliasResult::Retained)check(value==expected,"resolved native alias value");return result;}
#endif
void stringHeader(){
    const std::uint32_t disk[5]={0xffffffffu,1,4,0xffffffffu,0xffffffffu};input(disk,sizeof(disk));DB_PushStreamPos(0);
#if BO1_NATIVE_64BIT
    auto* header=reinterpret_cast<StringTable*>(DB_GetStreamPos());DB_ExpandLoad_StringTable(header);
#else
    auto* header=reinterpret_cast<StringTable*>(DB_GetStreamPos());Load_Stream(true,reinterpret_cast<unsigned char*>(header),20);
#endif
    header->name=reinterpret_cast<const char*>(marker(16));header->values=reinterpret_cast<StringTableCell*>(marker(32));header->cellIndex=reinterpret_cast<__int16*>(marker(48));
    DB_PopStreamPos();check(DB_GetStreamPos()==storage[0],"first header workspace rewind");
}
void rawHeader(){const std::uint32_t disk[3]={0xfffffffeu,9,0xffffffffu};input(disk,sizeof(disk));DB_PushStreamPos(0);
#if BO1_NATIVE_64BIT
    DB_ExpandLoad_RawFile(reinterpret_cast<RawFile*>(DB_GetStreamPos()));
#else
    Load_Stream(true,DB_GetStreamPos(),12);
#endif
    auto* header=reinterpret_cast<RawFile*>(storage[0]);check(header->len==9,"actual RawFile signed len");header->name=reinterpret_cast<const char*>(marker(64));header->buffer=reinterpret_cast<const char*>(marker(80));DB_PopStreamPos();}
void headers(){fresh();stringHeader();
    // Snapshot actual UPDATED values, rather than the first disk tags.
    auto* first=reinterpret_cast<StringTable*>(storage[0]);first->values=reinterpret_cast<StringTableCell*>(marker(96));
#if BO1_NATIVE_64BIT
    external=marker(112);DB_StreamRegisterAliasSlot(0,4,&external);
    auto* child=reinterpret_cast<uintptr_t*>(storage[4]+128);*child=marker(128);DB_StreamRegisterAliasSlot(4,16,child);
#endif
    const std::uint32_t childBytes[2]={0x11223344u,0x55667788u};input(childBytes,sizeof(childBytes));
    DB_PushStreamPos(0);DB_PushStreamPos(4);Load_Stream(true,DB_GetStreamPos(),sizeof(childBytes));DB_PopStreamPos();DB_PopStreamPos();
#if BO1_NATIVE_64BIT
    state(0,12,marker(96),DB_StreamAliasResult::Active);
#endif
    rawHeader();convert(0,0,marker(64));
#if BO1_NATIVE_64BIT
    state(0,0,marker(64),DB_StreamAliasResult::Active);state(0,8,marker(80),DB_StreamAliasResult::Active);
    state(0,12,marker(96),DB_StreamAliasResult::Retained);state(0,16,marker(48),DB_StreamAliasResult::Active);
    convert(0,12,marker(96));convert(0,16,marker(48));
    state(0,4,0,DB_StreamAliasResult::External);state(4,16,0,DB_StreamAliasResult::Missing);convert(4,16,marker(128));
    check(!DB_StreamExactLookup(0,12)&&!DB_StreamMapLookup(0,12),"TEMP expanded interior has no drift fallback");
#else
    // Win32 ordinary aliases read the original four-byte slot directly.
    convert(0,8,marker(80));convert(0,12,marker(96));convert(0,16,marker(48));
#endif
    ++cases;
}
void shortThenGood(){for(bool partial:{false,true}){fresh();stringHeader();const std::uint32_t raw[3]={0xffffffffu,9,0xffffffffu};input(raw,11);partialRead=partial;bool failed=false;DB_PushStreamPos(0);
    std::array<unsigned char,sizeof(StringTable)> snapshot{};std::memcpy(snapshot.data(),storage[0],snapshot.size());const auto cursor=DB_GetStreamPos();const auto stack=g_streamPosStackIndex;
    try{
#if BO1_NATIVE_64BIT
        DB_ExpandLoad_RawFile(reinterpret_cast<RawFile*>(DB_GetStreamPos()));
#else
        Load_Stream(true,DB_GetStreamPos(),12);
#endif
    }catch(const std::runtime_error&){failed=true;}check(failed,"controlled short read throws");
#if BO1_NATIVE_64BIT
    check(std::memcmp(snapshot.data(),storage[0],snapshot.size())==0,"scratch read failure leaves complete native header unchanged");state(0,0,marker(16),DB_StreamAliasResult::Active);state(0,12,marker(32),DB_StreamAliasResult::Active);state(0,16,marker(48),DB_StreamAliasResult::Active);check(DB_StreamWin32Pos(0)==0,"scratch read leaves shadow cursor unchanged");check(DB_StreamExactLookup(0,0)==storage[0],"scratch exact mapping unchanged before pop");
#endif
    check(DB_GetStreamPos()==cursor&&g_streamPosStackIndex==stack,"failed read preserves cursor/stack before pop");DB_PopStreamPos();
    convert(0,12,marker(32));
#if BO1_NATIVE_64BIT
    state(0,12,marker(32),DB_StreamAliasResult::Active);check(DB_StreamExactLookup(0,0)==storage[0],"short read keeps old exact map");
#endif
    ++shortReads;rawHeader();
#if BO1_NATIVE_64BIT
    state(0,8,marker(80),DB_StreamAliasResult::Active);check(DB_StreamWin32Pos(0)==0,"successful read clears private accumulated disk width before paired pop");
#endif
    }++cases;
}
void directMatrix(){
#if BO1_NATIVE_64BIT
    // One byte-segment with a fully initialized pointer slot. Each matrix row
    // has a separate lifetime; cursor placement is explicit fixture setup.
    for(unsigned size:{8u,16u,32u,48u}){
        fresh();DB_PushStreamPos(0);DB_StreamPrepareWrite(1);std::memset(storage[0],0,32);auto* slot=reinterpret_cast<uintptr_t*>(storage[0]+8);*slot=marker(144);DB_StreamRegisterAliasSlot(0,8,slot);DB_IncStreamPos(32);DB_PopStreamPos();
        DB_PushStreamPos(0);DB_StreamPrepareWrite();std::memset(storage[0],0,size);DB_IncStreamPos(static_cast<int>(size));DB_PopStreamPos();
        state(0,8,marker(144),size<=8?DB_StreamAliasResult::Active:DB_StreamAliasResult::Invalid);++matrixCases;
    }
    // Physical overlap alone retains the saved pointer while the original
    // logical alias word lies outside the newly consumed four disk bytes.
    fresh();DB_PushStreamPos(0);DB_StreamPrepareWrite(1);auto* nativeSlot=reinterpret_cast<uintptr_t*>(storage[0]+8);*nativeSlot=marker(152);DB_StreamRegisterAliasSlot(0,8,nativeSlot);DB_IncStreamPos(24);DB_PopStreamPos();
    DB_PushStreamPos(0);DB_StreamPrepareWrite();std::memset(storage[0],0,16);DB_SetExpandWin32Size(4);DB_IncStreamPos(16);DB_PopStreamPos();state(0,8,marker(152),DB_StreamAliasResult::Retained);++matrixCases;
    // Partial pointer writes are invalid; no unsafe read of the damaged slot.
    fresh();DB_PushStreamPos(0);DB_StreamPrepareWrite(1);auto* slot=reinterpret_cast<uintptr_t*>(storage[0]+8);*slot=marker(160);DB_StreamRegisterAliasSlot(0,8,slot);DB_IncStreamPos(24);DB_PopStreamPos();
    DB_PushStreamPos(0);DB_StreamPrepareWrite();std::memset(storage[0],0,12);DB_IncStreamPos(12);DB_PopStreamPos();state(0,8,0,DB_StreamAliasResult::Invalid);++matrixCases;
    // Expanded array element starts survive outside the actual native overwrite.
    fresh();DB_PushStreamPos(0);DB_StreamPrepareWrite();std::memset(storage[0],0,32);DB_SetExpandElem(4,8);DB_SetExpandWin32Size(16);DB_IncStreamPos(32);DB_PopStreamPos();
    DB_PushStreamPos(0);DB_StreamPrepareWrite();std::memset(storage[0],0,8);DB_SetExpandWin32Size(4);DB_IncStreamPos(8);DB_PopStreamPos();check(DB_StreamExactLookup(0,8)==storage[0]+16,"untouched expanded-array start retained");++matrixCases;
    // Cancel(false) leaves current values/maps intact; touched cancellation
    // poisons owned TEMP resolution and never the external-envelope slot.
    fresh();stringHeader();external=marker(176);DB_StreamRegisterAliasSlot(0,4,&external);DB_StreamPrepareWrite();DB_StreamCancelWrite(false);state(0,12,marker(32),DB_StreamAliasResult::Active);
    DB_StreamPrepareWrite();storage[0][0]=0;DB_StreamCancelWrite(true);state(0,12,0,DB_StreamAliasResult::Invalid);state(0,4,0,DB_StreamAliasResult::External);check(!DB_StreamMapLookup(0,0),"touched cancellation fails TEMP maps closed");++matrixCases;
    // No-write scope retains history, and Init discards it for next lifetime.
    fresh();stringHeader();DB_PushStreamPos(0);DB_PopStreamPos();state(0,12,marker(32),DB_StreamAliasResult::Active);fresh();state(0,12,0,DB_StreamAliasResult::Missing);++matrixCases;
    // Actual direct-reader partial copy poisons TEMP, while scratch expansion
    // above cancels atomically. Never dereference damaged pointer slots.
    fresh();stringHeader();external=marker(184);DB_StreamRegisterAliasSlot(0,4,&external);auto* child=reinterpret_cast<uintptr_t*>(storage[4]+128);*child=marker(192);DB_StreamRegisterAliasSlot(4,16,child);
    std::array<unsigned char,sizeof(StringTable)> beforeDirect{};std::memcpy(beforeDirect.data(),storage[0],beforeDirect.size());const std::uint32_t bytes[3]={1,2,3};input(bytes,7);partialRead=true;bool failed=false;DB_PushStreamPos(0);try{Load_Stream(true,DB_GetStreamPos(),12);}catch(const std::runtime_error&){failed=true;}check(failed,"actual direct reader fails after partial native write");check(std::memcmp(beforeDirect.data(),storage[0],beforeDirect.size())!=0,"direct partial copy changes bytes; poisoning is not rollback");state(0,12,0,DB_StreamAliasResult::Invalid);check(!DB_StreamExactLookup(0,0),"direct partial copy poisons exact map before reset");state(0,4,0,DB_StreamAliasResult::External);convert(4,16,marker(192));DB_PopStreamPos();++matrixCases;
    // A retained old value must not be recaptured from its reused physical
    // slot when a subsequent, physically unrelated fixture write occurs.
    fresh();stringHeader();reinterpret_cast<StringTable*>(storage[0])->values=reinterpret_cast<StringTableCell*>(marker(96));rawHeader();state(0,12,marker(96),DB_StreamAliasResult::Retained);DB_PushStreamPos(0);g_streamPos=storage[0]+64;DB_StreamPrepareWrite();std::memset(DB_GetStreamPos(),0,8);DB_SetExpandWin32Size(4);DB_IncStreamPos(8);DB_PopStreamPos();state(0,12,marker(96),DB_StreamAliasResult::Retained);convert(0,12,marker(96));++matrixCases;
    // Explicit fixture cursor placement separates physical/logical overlap.
    // A partial write straddles two expanded elements; only complete untouched
    // elements remain addressable. There is no corresponding genuine asset.
    fresh();DB_StreamPrepareWrite();std::memset(storage[0],0,32);DB_SetExpandElem(4,8);DB_SetExpandWin32Size(16);DB_IncStreamPos(32);DB_PushStreamPos(0);g_streamPos=storage[0]+7;DB_StreamPrepareWrite();std::memset(DB_GetStreamPos(),0,2);DB_SetExpandWin32Size(2);DB_IncStreamPos(2);DB_PopStreamPos();check(!DB_StreamExactLookup(0,0)&&!DB_StreamExactLookup(0,4),"partial array elements retired");check(DB_StreamExactLookup(0,8)==storage[0]+16&&DB_StreamExactLookup(0,12)==storage[0]+24,"whole untouched array elements survive");++matrixCases;
    fresh();DB_StreamPrepareWrite();std::memset(storage[0],0,48);DB_IncStreamPos(48);DB_PushStreamPos(0);g_streamPos=storage[0]+8;DB_StreamPrepareWrite();std::memset(DB_GetStreamPos(),0,8);DB_IncStreamPos(8);DB_PopStreamPos();check(DB_StreamExactLookup(0,4)==storage[0]+4&&DB_StreamExactLookup(0,24)==storage[0]+24,"byte segment preserves untouched prefix and suffix");check(!DB_StreamExactLookup(0,12),"overwritten byte interior retired");++matrixCases;
#endif
    ++cases;
}
void opaqueControls(){
#if BO1_NATIVE_64BIT
    const uintptr_t bits[]={0,UINTPTR_MAX,UINTPTR_MAX-1,0x80001235u,0x60000011u,static_cast<uintptr_t>(0x100000123ull)};
    for(uintptr_t value:bits){fresh();DB_PushStreamPos(0);DB_StreamPrepareWrite(1);auto* slot=reinterpret_cast<uintptr_t*>(storage[0]+8);*slot=value;DB_StreamRegisterAliasSlot(0,8,slot);DB_IncStreamPos(24);DB_PopStreamPos();DB_PushStreamPos(0);DB_StreamPrepareWrite();std::memset(storage[0],0,16);DB_SetExpandWin32Size(4);DB_IncStreamPos(16);DB_PopStreamPos();state(0,8,value,DB_StreamAliasResult::Retained);convert(0,8,value);}
    fresh();DB_PushStreamPos(0);DB_StreamPrepareWrite(1);auto* slot=reinterpret_cast<uintptr_t*>(storage[0]+8);*slot=marker(200);DB_StreamRegisterAliasSlot(0,8,slot);DB_IncStreamPos(24);DB_PopStreamPos();DB_PushStreamPos(0);DB_StreamPrepareWrite(1);std::memset(storage[0],0,12);DB_StreamRegisterAliasSlot(0,8,slot);DB_SetExpandWin32Size(4);DB_IncStreamPos(12);DB_PopStreamPos();state(0,8,0,DB_StreamAliasResult::Invalid);
    // Real C-string write hooks, backed only by bounded controlled byte input.
    fresh();stringHeader();const unsigned char terminated[]={0x61,0x62,0};input(terminated,sizeof(terminated));DB_PushStreamPos(0);const char* text=reinterpret_cast<const char*>(DB_GetStreamPos());Load_XStringCustom(&text);check(std::memcmp(storage[0],terminated,sizeof(terminated))==0,"actual terminated direct string bytes");check(DB_StreamWin32Pos(0)==3,"actual C-string commit width");state(0,12,marker(32),DB_StreamAliasResult::Active);DB_PopStreamPos();
    fresh();stringHeader();external=marker(208);DB_StreamRegisterAliasSlot(0,4,&external);auto* child=reinterpret_cast<uintptr_t*>(storage[4]+128);*child=marker(216);DB_StreamRegisterAliasSlot(4,16,child);const unsigned char unterminated[]={0x63,0x64,0x65};input(unterminated,sizeof(unterminated));DB_PushStreamPos(0);text=reinterpret_cast<const char*>(DB_GetStreamPos());bool failed=false;try{Load_XStringCustom(&text);}catch(const std::runtime_error&){failed=true;}check(failed&&std::memcmp(storage[0],unterminated,sizeof(unterminated))==0,"actual C-string partial write exception changes bytes");state(0,12,0,DB_StreamAliasResult::Invalid);check(!DB_StreamExactLookup(0,0),"C-string exception poisons TEMP map");state(0,4,0,DB_StreamAliasResult::External);convert(4,16,marker(216));DB_PopStreamPos();DB_InitStreams(detached);
    std::printf("{\"schema\":\"BlackOpsARM.TASK-0028.opaque-controls.v1\",\"pointer_bytes\":%zu,\"opaque_retained_tests\":6,\"latest_tombstone_tests\":1,\"c_string_tests\":2,\"cleanup_scope\":\"controlled_C++_throw_adapter_only\",\"production_longjmp_recovery_certified\":false}\n",sizeof(void*));
#else
    std::fputs("Native-only typed retained-value API\n",stderr);std::exit(8);
#endif
}
}
void __cdecl DB_LoadXFileData(unsigned char* destination,int size){
    if(!source||size<=0){std::fputs("Unsupported controlled decoder callback\n",stderr);std::abort();}
    if(static_cast<unsigned>(size)>available){if(partialRead&&available)std::memcpy(destination,source,available);throw std::runtime_error("controlled short read");}
    std::memcpy(destination,source,static_cast<unsigned>(size));source+=size;available-=static_cast<unsigned>(size);
}
int DB_LoadXFileDataNullTerminated(unsigned char* destination){if(!source||!available){std::fputs("Unsupported controlled C-string decoder callback\n",stderr);std::abort();}for(unsigned i=0;i<available;++i){destination[i]=source[i];if(!source[i]){source+=i+1;available-=i+1;return static_cast<int>(i+1);}}throw std::runtime_error("controlled unterminated input");}
int main(int argc,char**argv){for(unsigned b=0;b<7;++b){blocks[b].data=storage[b];blocks[b].size=sizeof(storage[b]);}
#if defined(_WIN32)
    _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
#endif
    if(argc==2&&std::strcmp(argv[1],"--opaque-controls")==0){opaqueControls();return 0;}
    if(argc==2&&std::strcmp(argv[1],"--invalid-alias")==0){fresh();
#if BO1_NATIVE_64BIT
        uintptr_t field=129;DB_ConvertOffsetToAlias(&field);std::fputs("Unexpected invalid alias success\n",stderr);return 9;
#else
        std::fputs("Unsafe Win32 unknown alias intentionally not invoked\n",stderr);return 8;
#endif
    }
    if(argc==2&&(std::strcmp(argv[1],"--invalid-slot")==0||std::strcmp(argv[1],"--external-alias")==0)){
#if BO1_NATIVE_64BIT
        fresh();stringHeader();uintptr_t field;
        if(std::strcmp(argv[1],"--external-alias")==0){external=marker(16);DB_StreamRegisterAliasSlot(0,4,&external);field=5;}
        else{DB_PushStreamPos(0);DB_StreamPrepareWrite(1);std::memset(storage[0],0,20);DB_StreamRegisterAliasSlot(0,12,reinterpret_cast<uintptr_t*>(storage[0]+16));DB_SetExpandWin32Size(4);DB_IncStreamPos(20);DB_PopStreamPos();state(0,12,0,DB_StreamAliasResult::Invalid);field=13;}
        DB_ConvertOffsetToAlias(&field);std::fputs("Unexpected invalid alias success\n",stderr);return 9;
#else
        return 8;
#endif
    }
    check(argc==1,"supported invocation");for(unsigned round=0;round<6;++round){headers();shortThenGood();directMatrix();}DB_InitStreams(detached);
    std::printf("{\"schema\":\"BlackOpsARM.TASK-0028.temp-write.v1\",\"pointer_bytes\":%zu,\"native64\":%s,\"cycles\":6,\"cases\":%u,\"short_reads\":%u,\"matrix_cases\":%u,\"controlled_fixtures\":true,\"genuine_assets_loaded\":false,\"payload_children_loaded\":false}\n",sizeof(void*),BO1_NATIVE_64BIT?"true":"false",cases,shortReads,matrixCases);
}
