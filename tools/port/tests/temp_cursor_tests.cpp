// Bounded stream cursor/alias prerequisite. Owned scratch arrays are not zones.
#include <database/db_stream.h>
#include <platform/bo1_native_width.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
alignas(16) unsigned char storage[7][4096]{};
XBlock blocks[7]{};
XBlock detached[7]{};
uintptr_t externalSlots[160]{};
unsigned cases{}, aliasChecks{};
void require(bool ok, const char* why) {
    if (!ok) { std::fprintf(stderr,"FAIL TEMP cursor: %s\n",why); std::exit(1); }
}
unsigned native(unsigned disk, unsigned wide) { return BO1_NATIVE_64BIT ? wide : disk; }
void advance(unsigned disk, unsigned wide) {
#if BO1_NATIVE_64BIT
    DB_SetExpandWin32Size(static_cast<int>(disk));
#endif
    DB_IncStreamPos(static_cast<int>(native(disk,wide)));
}
void position(unsigned index,unsigned disk,unsigned wide) {
    require(g_streamPosIndex==index&&DB_GetStreamPos()==storage[index]+native(disk,wide),"physical cursor");
#if BO1_NATIVE_64BIT
    require(DB_StreamWin32Pos(index)==disk,"paired shadow cursor");
#endif
}
void fresh() { DB_InitStreams(blocks); }
void nestedTemp() {
    fresh(); advance(4,8); position(0,4,8);
    DB_PushStreamPos(0); advance(8,16); position(0,12,24);
    DB_PushStreamPos(0); advance(4,8); position(0,16,32);
    DB_PopStreamPos(); position(0,12,24);
    DB_PopStreamPos(); position(0,4,8);
    require(g_streamPosStackIndex==0,"nested0 stack restored");
#if BO1_NATIVE_64BIT
    require(DB_StreamExactLookup(0,12)==storage[0]+24,"no-write pop retains exact commit");
#endif
    ++cases;
}
void parentFour() {
    fresh(); advance(8,16); DB_PushStreamPos(4); advance(12,16);
    DB_PushStreamPos(0); position(0,8,16); advance(12,24);
    DB_PushStreamPos(4); position(4,12,16); advance(4,8);
    DB_PopStreamPos(); position(0,20,40);
    DB_PopStreamPos(); position(4,16,24);
    require(g_streamPosArray[0]==storage[0]+native(8,16),"TEMP target entry restored independently of parent4");
#if BO1_NATIVE_64BIT
    require(DB_StreamWin32Pos(0)==8,"inactive TEMP shadow restored");
#endif
    DB_PopStreamPos(); position(0,8,16);
    require(g_streamPosStackIndex==0,"4->0->4 stack restored"); ++cases;
}
void mapsAndSlots() {
    fresh(); DB_PushStreamPos(0); advance(8,16);
#if BO1_NATIVE_64BIT
    DB_StreamRegisterAliasSlot(0,4,&externalSlots[0]);
#endif
    DB_PopStreamPos(); position(0,0,0);
#if BO1_NATIVE_64BIT
    require(DB_StreamExactLookup(0,0)==storage[0],"TEMP no-write pop preserves mapping");
    require(DB_StreamAliasSlotLookup(0,4)==&externalSlots[0],"TEMP no-write pop preserves external-envelope alias");
#endif
    // Reset between writes: overlapping TEMP commits are deliberately outside scope.
    fresh(); advance(4,8); DB_PushStreamPos(0);
    const void** inserted=DB_InsertPointer();
    require(reinterpret_cast<const unsigned char*>(inserted)==storage[4],"actual stable block4 inserted slot");
    const void* marker=storage[6]+32; *inserted=marker;
    advance(8,16); DB_PopStreamPos(); position(0,4,8);
    require(*inserted==marker,"inserted pointer survives TEMP pop");
    require(g_streamPosArray[4]==storage[4]+sizeof(void*),"block4 reservation retained");
#if BO1_NATIVE_64BIT
    require(DB_StreamWin32Pos(4)==4,"inserted slot keeps four-byte disk footprint");
#endif
    const void** next=DB_InsertPointer();
    require(reinterpret_cast<const unsigned char*>(next)==storage[4]+sizeof(void*),"next inserted pointer has independent slot");
    require(*inserted==marker,"second reservation preserves first pointer"); ++cases;
}
void deterministicAliases() {
#if BO1_NATIVE_64BIT
    fresh();
    // Forty equal keys exceed std::sort's small partitions; lookup creates a
    // merged prefix. Later groups exercise large unsorted tails and ties.
    for(unsigned i=0;i<40;++i)DB_StreamRegisterAliasSlot(0,24,&externalSlots[i]);
    require(DB_StreamAliasSlotLookup(0,24)==&externalSlots[39],"latest among40 equal keys"); ++aliasChecks;
    for(unsigned i=40;i<93;++i){DB_StreamRegisterAliasSlot(0,24,&externalSlots[i]);if(i%7==0)DB_StreamRegisterAliasSlot(0,8,&externalSlots[i]);}
    require(DB_StreamAliasSlotLookup(0,24)==&externalSlots[92],"latest prefix/tail equal key"); ++aliasChecks;
    require(DB_StreamAliasSlotLookup(0,8)==&externalSlots[91],"mixed-key latest alias"); ++aliasChecks;
    for(unsigned i=93;i<150;++i)DB_StreamRegisterAliasSlot(0,24,&externalSlots[i]);
    for(unsigned repeat=0;repeat<10;++repeat){require(DB_StreamAliasSlotLookup(0,24)==&externalSlots[149],"deterministic repeated equal-key lookup");++aliasChecks;}
    require(!DB_StreamAliasSlotLookup(0,23),"unknown alias absent"); ++aliasChecks;
#endif
    ++cases;
}
void reset() {
    fresh();
#if BO1_NATIVE_64BIT
    DB_SetExpandElem(4,8);advance(8,16);
    require(DB_StreamExactLookup(0,4)==storage[0]+8,"stride seeded");
    DB_StreamRegisterAliasSlot(0,0,&externalSlots[0]);
    DB_SetExpandWin32Size(4);DB_SetExpandElem(4,8);
#else
    advance(8,16);
#endif
    DB_PushStreamPos(4);g_streamDelayIndex=3;fresh();position(0,0,0);
    require(g_streamPosStackIndex==0&&g_streamDelayIndex==0&&g_streamPosArray[0]==nullptr,"init clears stack/delay/current slot");
#if BO1_NATIVE_64BIT
    for(unsigned b=0;b<7;++b){require(DB_StreamWin32Pos(b)==0,"init resets all shadows");require(!DB_StreamExactLookup(b,0)&&!DB_StreamAliasSlotLookup(b,0),"init clears exact/alias maps");}
#endif
    DB_IncStreamPos(16);
#if BO1_NATIVE_64BIT
    require(DB_StreamWin32Pos(0)==16&&DB_StreamExactLookup(0,4)==storage[0]+4,"init clears pending width/stride");
    DB_SetExpandElem(4,8); DB_SetExpandWin32Size(4);fresh();
    DB_SetExpandWin32Size(8);DB_IncStreamPos(16);
    require(!DB_StreamExactLookup(0,4),"pending stride reset independently");
#endif
    ++cases;
}
}
int main() {
    for(unsigned b=0;b<7;++b){blocks[b].data=storage[b];blocks[b].size=sizeof(storage[b]);}
    for(unsigned round=0;round<8;++round){nestedTemp();parentFour();mapsAndSlots();deterministicAliases();reset();}
    DB_InitStreams(detached);
    require(g_streamBlocks==detached&&g_streamPosStackIndex==0&&DB_GetStreamPos()==nullptr,"detach stable descriptor set");
    std::printf("{\"schema\":\"BlackOpsARM.TASK-0027.temp-cursor.v1\",\"pointer_bytes\":%zu,\"native64\":%s,\"cycles\":8,\"cases\":%u,\"alias_checks\":%u,\"paired_restore\":true,\"insert_pointer\":true,\"no_write_maps_retained\":true,\"reset\":true,\"production_zone_allocation_claimed\":false,\"asset2_loaded\":false}\n",sizeof(void*),BO1_NATIVE_64BIT?"true":"false",cases,aliasChecks);
}
