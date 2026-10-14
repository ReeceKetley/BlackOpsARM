#include <gfx_d3d/r_staticmodel.h>
#include <gfx_d3d/r_init.h>
#include <qcommon/common.h>
#include <xanim/xmodel.h>
#include <cstdio>
#include <cstdlib>
#include <csignal>
#include <sys/wait.h>
#include <unistd.h>
r_global_permanent_t rgp;
void Com_Error(errorParm_t,const char*,...) {} // Deliberately returns: native invalid ownership must abort.
static void Check(bool ok,const char *message) { if(!ok) { std::fprintf(stderr,"FAIL %s\n",message); std::abort(); } }
static GfxWorld world;static GfxCell cells[3];static GfxAabbTree tree[2];static GfxStaticModelDrawInst instances[3];static XModel target,other;static unsigned short firstIndices[]{0,1,0},secondIndices[]{2};
static void Setup() { world={};cells[0]={};cells[1]={};cells[2]={};tree[0]={};tree[1]={};world.cells=cells;world.dpvsPlanes.cellCount=3;world.dpvs.smodelCount=3;world.dpvs.smodelDrawInsts=instances;instances[0].model=&target;instances[1].model=&other;instances[2].model=&target;cells[0].aabbTreeCount=1;cells[0].aabbTree=&tree[0];cells[1].aabbTreeCount=1;cells[1].aabbTree=&tree[1];tree[0].smodelIndexCount=3;tree[0].smodelIndexes=firstIndices;tree[1].smodelIndexCount=1;tree[1].smodelIndexes=secondIndices;rgp.world=&world; }
static void Reject(int kind) { pid_t pid=fork();Check(pid>=0,"fork");if(!pid) { int count{};switch(kind) {case 0: R_StaticModelGetMemoryUsage(&target,nullptr);break;case 1: world.dpvsPlanes.cellCount=-1;break;case 2: cells[0].aabbTree=nullptr;break;case 3: tree[0].smodelIndexes=nullptr;break;case 4: firstIndices[0]=3;break;case 5: world.dpvs.smodelDrawInsts=nullptr;break;}if(kind)R_StaticModelGetMemoryUsage(&target,&count);_Exit(9); }int status{};Check(waitpid(pid,&status,0)==pid && WIFSIGNALED(status) && WTERMSIG(status)==SIGABRT,"malformed owner/index cannot return a false memory report"); }
#undef main
int main() { int count=-1;rgp.world=nullptr;Check(R_StaticModelGetMemoryUsage(&target,&count)==0 && count==0,"canonical no-world output");Setup();int bytes=R_StaticModelGetMemoryUsageInst();Check(bytes==sizeof(GfxStaticModelDrawInst),"native actual instance width");Check(R_StaticModelGetMemoryUsage(&target,&count)==3*bytes && count==3,"original repeated per-cell references counted");Check(R_StaticModelGetMemoryUsage(&other,&count)==bytes && count==1,"identity selection");XModel absent{};Check(R_StaticModelGetMemoryUsage(&absent,&count)==0 && count==0,"no matching references");Check(cells[2].aabbTreeCount==0 && cells[2].aabbTree==nullptr,"genuine empty-cell case");for(int kind=0;kind<6;++kind)Reject(kind);std::printf("PASS actual static model owner: native instance %i bytes, borrowed world/cells/root identities and duplicate matches, empty/no world, six malformed-input fatal-return guards; no loaded asset/GPU proof\n",bytes);return 0; }
