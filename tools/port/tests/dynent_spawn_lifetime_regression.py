"""Run canonical spawned DynEnt allocation/eviction/cleanup on sparse owners.
Native world services are isolated; no save codec or live game proof is claimed.
"""
from pathlib import Path
import argparse, hashlib, json
root=Path(__file__).resolve().parents[3]
parser=argparse.ArgumentParser()
parser.add_argument('--out',type=Path,required=True)
parser.add_argument('--source',type=Path,default=Path('src/DynEntity/DynEntity_client.cpp'))
a=parser.parse_args()
source=str(a.source)
s=(root/source).read_text()
def extract(key):
    start=s.index(key);end=s.index('{',start)+1;depth=1
    while depth:
        depth+=(s[end]=='{')-(s[end]=='}');end+=1
    return s[start:end]
keys=['void __cdecl DynEntCl_DestroyEntityModel(',
      'unsigned __int16 __cdecl DynEntCl_AddEntityModel()',
      'void DynEntCl_SP_CleanupSpawnedDynEnts()',
      'void DynEntCl_CleanUpOldModels()']
code=r"""
#include <qcommon/cm_load.h>
#include <dynentity/DynEntity_load_obj.h>
#include <cgame_mp/cg_main_mp.h>
#include <client/splitscreen.h>
#include <cgame/cg_compass.h>
#include <cfloat>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <vector>
static int checks;
#define CHECK(x) do{if(!(x)){fprintf(stderr,"check %i failed line %i\n",checks,__LINE__);abort();}++checks;}while(0)
clipMap_t cm;
int numExtraDynEnts;
unsigned __int8 usedExtraDynEnts[256];
static dvar_t limitValue{}, zombieValue{};
const dvar_t *dynEnt_spawnedLimit=&limitValue, *zombietron=&zombieValue;
static cg_s fixtureCg;
cg_s *cgArray=&fixtureCg;
#define cg cgArray[0]
static bool showingZombies, activePlayer;
static DynEntityClient clients[300];
static DynEntityPose poses[300];
static std::vector<int> retired, bodies, fade, burn, unlinked;
bool Assert_MyHandler(const char *,int,int,const char *,...){throw std::runtime_error("assert");}
void Com_Error(errorParm_t,const char *,...){throw std::runtime_error("error");}
bool CG_IsShowingZombieMap(){return showingZombies;}
char CL_LocalClient_IsActive(int){return activePlayer;}
float Vec3DistanceSq(const float *a,const float *b){float r=0;for(int i=0;i<3;++i){float d=a[i]-b[i];r+=d*d;}return r;}
DynEntityPose *DynEnt_GetClientPose(unsigned __int16 id,DynEntityDrawType){CHECK(id<cm.dynEntCount[0]);return &poses[id];}
void Phys_ObjDestroy(int,int id){bodies.push_back(id);}
unsigned __int16 DynEnt_GetClientAbsId(unsigned __int16 id,DynEntityDrawType){return id;}
void DynEntCl_RemoveFromFadeList(unsigned __int16 id){fade.push_back(id);}
void DynEntCl_SetBurning(unsigned __int16 id,DynEntityDrawType,int enabled){CHECK(!enabled);burn.push_back(id);}
void DynEntCl_UnlinkEntity(unsigned __int16 id,DynEntityCollType){CHECK(clients[id].physObjId==0);unlinked.push_back(id);retired.push_back(id);}
PRODUCTION
static void reset(int original=17){
 memset(&cm,0,sizeof(cm));memset(clients,0,sizeof(clients));memset(poses,0,sizeof(poses));
 memset(usedExtraDynEnts,0,sizeof(usedExtraDynEnts));memset(&cg,0,sizeof(cg));
 cm.originalDynEntCount=original;cm.dynEntCount[0]=original;cm.dynEntClientList[0]=clients;
 numExtraDynEnts=0;showingZombies=false;activePlayer=true;limitValue.current.integer=256;zombieValue.current.enabled=false;
 retired.clear();bodies.clear();fade.clear();burn.clear();unlinked.clear();
}
static void spawn(int slot,float distance=1){CHECK(!usedExtraDynEnts[slot]);usedExtraDynEnts[slot]=1;++numExtraDynEnts;
 int id=cm.originalDynEntCount+slot;if(id>=cm.dynEntCount[0])cm.dynEntCount[0]=id+1;
 clients[id].physObjId=1000+slot;clients[id].flags=3;clients[id].health=9;poses[id].pose.origin[0]=distance;
}
static void checkEmpty(){CHECK(numExtraDynEnts==0);for(int i=0;i<256;++i)CHECK(!usedExtraDynEnts[i]);
 CHECK(bodies.size()==retired.size());CHECK(fade==retired);CHECK(burn==retired);CHECK(unlinked==retired);
 for(int id:retired){CHECK(clients[id].flags==0);CHECK(clients[id].health==-1);CHECK(clients[id].physObjId==0);}}
int main(int argc,char **argv){
 // Independent negative-replay cases can also run against a frozen old source.
 if(argc==2){
  if(!strcmp(argv[1],"sparse-cleanup")){reset();spawn(4);spawn(200);DynEntCl_SP_CleanupSpawnedDynEnts();CHECK(retired.size()==2);checkEmpty();}
  else if(!strcmp(argv[1],"hole-eviction")){reset();spawn(1,5);spawn(7,20);poses[17].pose.origin[0]=100000;DynEntCl_CleanUpOldModels();CHECK(retired.size()==1&&retired[0]==24);}
  else if(!strcmp(argv[1],"full-capacity")){reset();for(int i=0;i<256;++i)spawn(i,float(i+1));CHECK(DynEntCl_AddEntityModel()==272);CHECK(numExtraDynEnts==256);}
  else return 2;
  printf("production spawned DynEnt case %s PASS\n",argv[1]);return 0;
 }
 // Dynamic FX validity follows actual ownership, including sparse holes.
 reset();spawn(4);cm.dynEntCount[1]=2;
 for(int id=0;id<65536;++id){bool expected=id<17||id==21||id==273||id==274;CHECK(DynEnt_Valid((unsigned short)id)==expected);}
 // Regression: count2 cannot describe live slots4 and200.
 reset();spawn(4);spawn(200);DynEntCl_SP_CleanupSpawnedDynEnts();CHECK(retired.size()==2);CHECK(retired[0]==21&&retired[1]==217);checkEmpty();
 DynEntCl_SP_CleanupSpawnedDynEnts();CHECK(retired.size()==2);checkEmpty();
 // All single-slot positions, including the final one, plus leading holes.
 for(int slot=0;slot<256;++slot){reset();spawn(slot);DynEntCl_SP_CleanupSpawnedDynEnts();CHECK(retired.size()==1&&retired[0]==17+slot);checkEmpty();}
 // Every other slot + highest slot: destruction mutates high-water/count.
 reset();for(int i=0;i<256;i+=2)spawn(i);spawn(255);DynEntCl_SP_CleanupSpawnedDynEnts();CHECK(retired.size()==129);checkEmpty();
 // Eviction must ignore a stale free-slot pose farther than any live object.
 reset();spawn(1,5);spawn(7,20);poses[17].pose.origin[0]=100000;DynEntCl_CleanUpOldModels();
 CHECK(retired.size()==1&&retired[0]==24);CHECK(usedExtraDynEnts[1]&&!usedExtraDynEnts[7]);CHECK(numExtraDynEnts==1);
 // Full capacity at limit256 previously tried slot256. Evict then reuse safely.
 reset();for(int i=0;i<256;++i)spawn(i,float(i+1));CHECK(DynEntCl_AddEntityModel()==272);
 CHECK(retired.size()==1&&retired[0]==272);CHECK(numExtraDynEnts==256&&cm.dynEntCount[0]==273);CHECK(usedExtraDynEnts[255]);
 // No eviction candidate (all distances zero): bounded refusal, no slot writes.
 reset();for(int i=0;i<256;++i)spawn(i,0);CHECK(DynEntCl_AddEntityModel()==0xFFFF);CHECK(numExtraDynEnts==256&&cm.dynEntCount[0]==273);CHECK(retired.empty());
 // Sparse allocation must reuse the lowest free slot and retain high-water.
 reset();spawn(0);spawn(200);CHECK(DynEntCl_AddEntityModel()==18);CHECK(numExtraDynEnts==3&&cm.dynEntCount[0]==218);
 // Preserve the original configured policy (>limit), including Zombies halving.
 reset();limitValue.current.integer=2;spawn(0,1);spawn(1,2);CHECK(DynEntCl_AddEntityModel()==19);CHECK(retired.empty());
 poses[19].pose.origin[0]=3;CHECK(DynEntCl_AddEntityModel()==19);CHECK(retired.size()==1&&numExtraDynEnts==3);
 reset();showingZombies=true;limitValue.current.integer=2;spawn(0,1);spawn(1,2);CHECK(DynEntCl_AddEntityModel()==18);CHECK(retired.size()==1&&numExtraDynEnts==2);
 // Stress sparse occupancy, compare canonical destruction against owned bitmap.
 unsigned rng=0x18fe55u;
 for(int iteration=0;iteration<1000;++iteration){reset();std::vector<int> expected;
  for(int i=0;i<256;++i){rng=rng*1664525u+1013904223u;if(rng&0x80000000u){spawn(i);expected.push_back(17+i);}}
  DynEntCl_SP_CleanupSpawnedDynEnts();CHECK(retired==expected);checkEmpty();}
 printf("production spawned DynEnt lifetime %i checks PASS; pointer %zu bytes\n",checks,sizeof(void*));
}
"""
valid_source='src/DynEntity/DynEntity_load_obj.cpp'
valid_text=(root/valid_source).read_text();valid_start=valid_text.index('bool __cdecl DynEnt_Valid(');valid_end=valid_text.index('{',valid_start)+1;depth=1
while depth:
    depth+=(valid_text[valid_end]=='{')-(valid_text[valid_end]=='}');valid_end+=1
code=code.replace('PRODUCTION','\n'.join(extract(k) for k in keys)+'\n'+valid_text[valid_start:valid_end])
a.out.mkdir(parents=True,exist_ok=False)
(a.out/'fixture.cpp').write_text(code,encoding='utf-8')
(a.out/'source-manifest.json').write_text(json.dumps({x:hashlib.sha256((root/x).read_bytes()).hexdigest() for x in [source,valid_source]},indent=2),encoding='utf-8')
