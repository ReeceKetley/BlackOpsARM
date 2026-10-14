#include <flame/flame_system.h>
#include <flame/flame_class_chunk.h>
#include <flame/flame_class_fire.h>
#include <flame/flame_class_smoke.h>
#include <flame/flame_class_stream.h>
#include <flame/flame_class_drips.h>
#include <qcommon/common.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstdarg>
#include <vector>
#include <sys/wait.h>
#include <sys/mman.h>
#include <unistd.h>
#include <csignal>
#include <cstddef>
#include <type_traits>
static unsigned checks;static int *fatalMarker;
static void Check(bool ok,const char *why){if(!ok){std::fprintf(stderr,"FAIL: %s\n",why);std::abort();}++checks;}
bool Assert_MyHandler(const char *file,int line,int,const char *format,...){std::fprintf(stderr,"assert %s:%d ",file,line);va_list args;va_start(args,format);std::vfprintf(stderr,format,args);va_end(args);std::abort();}
void Com_Error(errorParm_t,const char *format,...){if(fatalMarker)*fatalMarker=1;va_list args;va_start(args,format);std::vfprintf(stderr,format,args);va_end(args);std::fputc('\n',stderr);} // Deliberately returning reporter: guards must stop.
extern flameChunk_s flameChunks[800],sv_flameChunks[800];extern flameChunk_s *flameChunksFree,*flameChunksUsed,*sv_flameChunksFree,*sv_flameChunksUsed;
extern flameFire_t flameFire[2400];extern flameFire_t *flameFireFree,*flameFireUsed;
extern flameSmoke_t flameSmoke[250];extern flameSmoke_t *flameSmokeFree,*flameSmokeUsed;
extern flameDrips_t flameDrips[250],sv_flameDrips[250];extern flameDrips_t *flameDripsFree,*flameDripsUsed,*sv_flameDripsFree,*sv_flameDripsUsed;
extern flameStream_s flameStreams[64],sv_flameStreams[64];extern flameStream_s *flameStreamsFree,*flameStreamsUsed,*sv_flameStreamsFree,*sv_flameStreamsUsed;
extern flameRenderList_s flameStreamRenderList[64];extern flameSource_t flameSources[64],sv_flameSources[64];extern unsigned char flameSourceLookup[1024],sv_flameSourceLookup[1024];
template<class T,std::size_t N> void List(T (&pool)[N],T *head){
    static_assert(offsetof(T,gen)==0);
    Check(head==pool,"real initial free head");
    for(std::size_t i=0;i<N;++i){auto &item=pool[i];Check(reinterpret_cast<std::uintptr_t>(&item)%alignof(T)==0,"actual row native alignment");Check(item.gen.listGlobal.prev==(i?&pool[i-1].gen:nullptr) && item.gen.listGlobal.next==(i+1<N?&pool[i+1].gen:nullptr),"actual links address typed neighbors");bool zeroed=true;const auto *bytes=reinterpret_cast<const unsigned char *>(&item);for(std::size_t byte=0;byte<sizeof(T);++byte){bool pointer=byte>=offsetof(flameGeneric_s,listGlobal) && byte<offsetof(flameGeneric_s,listGlobal)+sizeof(flameList_t);if constexpr(std::is_same_v<T,flameStream_s>)pointer=pointer || (byte>=offsetof(flameStream_s,renderList) && byte<offsetof(flameStream_s,renderList)+sizeof(void *));if(!pointer && bytes[byte])zeroed=false;}Check(zeroed,"actual class Init clears complete native row from poisoned storage");}
}
template<class T> void LocalCanary(){
    struct Storage {unsigned char before[16];T items[3];unsigned char after[16];} owned;
    std::memset(&owned,0xA5,sizeof(owned));Flame_List_Init(&owned.items[0].gen,sizeof(T),3);
    for(auto byte:owned.before)Check(byte==0xA5,"list prefix canary");for(auto byte:owned.after)Check(byte==0xA5,"list suffix canary");
    T &item=owned.items[1];item.gen.listLocal.prev=&owned.items[0].gen;item.gen.listLocal.next=&owned.items[2].gen;
    const auto global=item.gen.listGlobal,local=item.gen.listLocal;
    Flame_Item_Init(&item.gen,sizeof(T));Check(!std::memcmp(&global,&item.gen.listGlobal,sizeof(global)) && !std::memcmp(&local,&item.gen.listLocal,sizeof(local)),"actual item reset preserves both native lists");
    const auto *bytes=reinterpret_cast<const unsigned char *>(&item);bool cleared=true;
    for(std::size_t i=0;i<sizeof(T);++i){const bool links=(i>=offsetof(T,gen)+offsetof(flameGeneric_s,listGlobal) && i<offsetof(T,gen)+offsetof(flameGeneric_s,listLocal)+sizeof(flameList_t));if(!links && bytes[i])cleared=false;}
    Check(cleared,"actual item reset clears complete native row including tail");
    for(auto byte:owned.before)Check(byte==0xA5,"item prefix canary");for(auto byte:owned.after)Check(byte==0xA5,"item suffix canary");
}
template<class T,std::size_t N,class Allocate,class Free> void Cycle(T (&pool)[N],T *&free,T *&used,Allocate allocate,Free release){
    std::vector<T *> owners;
    for(std::size_t i=0;i<N;++i){auto *item=allocate();Check(item==&pool[i],"actual class allocation walks owned typed pool");owners.push_back(item);}
    Check(!allocate() && !free,"actual class exhaustion returns null");
    for(auto *item:owners)release(item);
    Check(!used && free,"actual class release retires used list");
    unsigned count=0;for(auto *item=free;item;item=reinterpret_cast<T *>(item->gen.listGlobal.next)){Check(count++<N,"released list has no cycle");}
    Check(count==N,"released list retains exact actual pool ownership");
}
int main(){
    alarm(15);
    std::memset(flameChunks,0xA5,sizeof(flameChunks));std::memset(sv_flameChunks,0xA5,sizeof(sv_flameChunks));std::memset(flameFire,0xA5,sizeof(flameFire));std::memset(flameSmoke,0xA5,sizeof(flameSmoke));std::memset(flameDrips,0xA5,sizeof(flameDrips));std::memset(sv_flameDrips,0xA5,sizeof(sv_flameDrips));std::memset(flameStreams,0xA5,sizeof(flameStreams));std::memset(sv_flameStreams,0xA5,sizeof(sv_flameStreams));
    Flame_Init_Classes();
    std::printf("native sizes generic %zu chunk %zu fire %zu smoke %zu stream %zu drips %zu\n",sizeof(flameGeneric_s),sizeof(flameChunk_s),sizeof(flameFire_t),sizeof(flameSmoke_t),sizeof(flameStream_s),sizeof(flameDrips_t));
    List(flameChunks,flameChunksFree);List(sv_flameChunks,sv_flameChunksFree);List(flameFire,flameFireFree);List(flameSmoke,flameSmokeFree);List(flameStreams,flameStreamsFree);List(sv_flameStreams,sv_flameStreamsFree);List(flameDrips,flameDripsFree);List(sv_flameDrips,sv_flameDripsFree);
    for(unsigned i=0;i<64;++i)Check(flameStreams[i].renderList==&flameStreamRenderList[i] && !sv_flameStreams[i].renderList,"real client render-list ownership/server absence");
    LocalCanary<flameChunk_s>();LocalCanary<flameFire_t>();LocalCanary<flameSmoke_t>();LocalCanary<flameDrips_t>();LocalCanary<flameStream_s>();
    Cycle(flameFire,flameFireFree,flameFireUsed,[]{return Flame_Class_Fire_Alloc();},[](auto *p){Flame_Class_Fire_Free(false,p);});
    Cycle(flameSmoke,flameSmokeFree,flameSmokeUsed,[]{return Flame_Class_Smoke_Alloc();},[](auto *p){Flame_Class_Smoke_Free(p);});
    Cycle(flameChunks,flameChunksFree,flameChunksUsed,[]{return Flame_Class_Chunk_Alloc(false);},[](auto *p){Flame_Class_Chunk_Free(false,p);});
    Cycle(sv_flameChunks,sv_flameChunksFree,sv_flameChunksUsed,[]{return Flame_Class_Chunk_Alloc(true);},[](auto *p){Flame_Class_Chunk_Free(true,p);});
    Cycle(flameDrips,flameDripsFree,flameDripsUsed,[]{return Flame_Class_Drips_Alloc(false);},[](auto *p){Flame_Class_Drips_Free(false,p);});
    Cycle(sv_flameDrips,sv_flameDripsFree,sv_flameDripsUsed,[]{return Flame_Class_Drips_Alloc(true);},[](auto *p){Flame_Class_Drips_Free(true,p);});
    // Real isolated source lookup storage supplies Stream_Free's actual source owner.
    flameSourceLookup[0]=1;sv_flameSourceLookup[0]=1;
    Cycle(flameStreams,flameStreamsFree,flameStreamsUsed,[]{return Flame_Class_Stream_Alloc(false);},[](auto *p){flameSources[0].currentStream=p;Flame_Class_Stream_Free(false,p);Check(!flameSources[0].currentStream,"actual stream source ownership retirement");});
    Cycle(sv_flameStreams,sv_flameStreamsFree,sv_flameStreamsUsed,[]{return Flame_Class_Stream_Alloc(true);},[](auto *p){sv_flameSources[0].currentStream=p;Flame_Class_Stream_Free(true,p);Check(!sv_flameSources[0].currentStream,"actual server stream source ownership retirement");});
    alignas(flameGeneric_s) unsigned char storage[sizeof(flameStream_s)+16]{};
    fatalMarker=static_cast<int *>(mmap(nullptr,4096,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0));Check(fatalMarker!=MAP_FAILED,"actual shared failure publication");
    for(int fault=0;fault<5;++fault){*fatalMarker=0;pid_t pid=fork();Check(pid>=0,"real malformed pool child");if(!pid){if(fault==0)Flame_List_Init(reinterpret_cast<flameGeneric_s *>(storage),100,2);if(fault==1)Flame_List_Init(reinterpret_cast<flameGeneric_s *>(storage),sizeof(flameStream_s),-1);if(fault==2)Flame_Item_Init(reinterpret_cast<flameGeneric_s *>(storage+1),sizeof(flameStream_s));if(fault==4)Flame_List_Init(reinterpret_cast<flameGeneric_s *>(storage),-128,2);if(fault==3)Flame_Item_Init(reinterpret_cast<flameGeneric_s *>(storage),sizeof(flameGeneric_s)-1);_exit(90);}int status;Check(waitpid(pid,&status,0)==pid && WIFSIGNALED(status) && WTERMSIG(status)==SIGABRT && *fatalMarker==1,"malformed native storage fails after returning error reporter");}
    std::printf("PASS: %u actual class-pool/list/item ownership and canary observations\n",checks);
}
