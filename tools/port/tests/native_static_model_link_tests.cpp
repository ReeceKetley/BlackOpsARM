#include <qcommon/cm_world.h>
#include <qcommon/clipmap_state.h>
#include <qcommon/common.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstdarg>
#include <vector>
#include <cstddef>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include <csignal>
extern cm_world_t cm_world;
clipMap_t cm{}; // Isolated actual metadata; no map/collision-ready claim.
static unsigned checks;static int *fatalMarker;
static void Check(bool ok,const char *why){if(!ok){std::fprintf(stderr,"FAIL: %s\n",why);std::abort();}++checks;}
void Com_Error(errorParm_t,const char *format,...){if(fatalMarker)*fatalMarker=1;va_list args;va_start(args,format);std::vfprintf(stderr,format,args);va_end(args);std::fputc('\n',stderr);} // Deliberate return requires native fail-closed validation.
static cStaticModel_s models[8]{};
static void Expected(const std::vector<unsigned short> &values){unsigned short link=cm_world.sectors[1].contents.staticModels;for(auto expected:values){Check(link==expected,"actual sorted modelnum+1 linked value");link=models[link-1].writable.nextModelInWorldSector;}Check(!link,"actual zero terminal sentinel");}
static void Insert(unsigned index,const std::vector<unsigned short> &expected){
    const cm_world_t beforeWorld=cm_world;const auto beforeCount=cm.numStaticModels;
    unsigned char beforeModels[sizeof(models)];std::memcpy(beforeModels,models,sizeof(models));
    CM_AddStaticModelToNode(&models[index],1);Expected(expected);Check(cm.numStaticModels==beforeCount,"insertion preserves real model count");
    const auto *before=reinterpret_cast<const unsigned char *>(&beforeWorld),*after=reinterpret_cast<const unsigned char *>(&cm_world);
    const std::size_t head=offsetof(cm_world_t,sectors)+sizeof(worldSector_s)+offsetof(worldSector_s,contents)+offsetof(worldContents_s,staticModels);
    bool preserved=true;for(std::size_t i=0;i<sizeof(cm_world);++i)if((i<head || i>=head+sizeof(unsigned short)) && before[i]!=after[i])preserved=false;
    Check(preserved,"only actual two-byte world head may change, all neighbor/tree/sector canaries retained");
    const auto *rows=reinterpret_cast<const unsigned char *>(models);preserved=true;for(std::size_t i=0;i<sizeof(models);++i){const auto within=i%sizeof(cStaticModel_s);if(within>=sizeof(cStaticModelWritable) && rows[i]!=beforeModels[i])preserved=false;}Check(preserved,"only genuine per-model writable link slots change");
}
static void Fault(unsigned which){*fatalMarker=0;pid_t child=fork();Check(child>=0,"actual malformed link child");if(!child){
    if(which==0)CM_AddStaticModelToNode(models,0);
    if(which==1)CM_AddStaticModelToNode(models,1024);
    if(which==2)CM_AddStaticModelToNode(models+8,1);
    if(which==3)CM_AddStaticModelToNode(reinterpret_cast<cStaticModel_s *>(reinterpret_cast<unsigned char *>(models)+2),1);
    if(which==4){cm.numStaticModels=65536;CM_AddStaticModelToNode(models,1);}
    if(which==5){cm_world.sectors[1].contents.staticModels=9;CM_AddStaticModelToNode(models,1);}
    if(which==6){cm_world.sectors[1].contents.staticModels=5;models[4].writable.nextModelInWorldSector=5;CM_AddStaticModelToNode(models,1);}
    if(which==7)CM_AddStaticModelToNode(models,1); // Existing destination index must not create a self cycle.
    _exit(95);
}int status{};Check(waitpid(child,&status,0)==child && WIFSIGNALED(status) && WTERMSIG(status)==SIGABRT && *fatalMarker,"actual invalid argument/list invokes fatal and aborts after return");}
int main(){
    alarm(15);static_assert(sizeof(worldContents_s)==12 && sizeof(worldSector_s)==24 && sizeof(cStaticModelWritable)==2);
    cm.staticModelList=models;cm.numStaticModels=8;std::memset(&cm_world,0xA5,sizeof(cm_world));cm_world.sectors[1].contents.staticModels=0;
    models[4].writable.nextModelInWorldSector=8;Insert(4,{5});Insert(0,{1,5});Insert(7,{1,5,8});Insert(2,{1,3,5,8});Insert(6,{1,3,5,7,8});
    models[1].writable.nextModelInWorldSector=4;Insert(1,{1,2,3,5,7,8}); // A real transfer may carry a nonzero parent next.
    fatalMarker=static_cast<int *>(mmap(nullptr,4096,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0));Check(fatalMarker!=MAP_FAILED,"actual shared fatal marker");for(unsigned which=0;which<8;++which)Fault(which);Expected({1,2,3,5,7,8});
    std::vector<cStaticModel_s> boundary(65535);cm.staticModelList=boundary.data();cm.numStaticModels=static_cast<unsigned>(boundary.size());cm_world.sectors[1].contents.staticModels=0;CM_AddStaticModelToNode(&boundary.back(),1);Check(cm_world.sectors[1].contents.staticModels==65535 && !boundary.back().writable.nextModelInWorldSector,"actual maximum fixed16 index+1 representable without truncation");
    std::printf("PASS: %u actual static-model insertion/link/canary/bound observations\n",checks);
}
