"""Actual fake-entity free and collision-tree unlink, with isolated script/render/FX providers."""
from pathlib import Path
import argparse,json,hashlib
p=argparse.ArgumentParser();p.add_argument('--out',type=Path,required=True);p.add_argument('--source',type=Path,default=Path('src/cgame/cg_spawn.cpp'));a=p.parse_args();a.out.mkdir(exist_ok=False)
def extract(path,key):
 s=Path(path).read_text(encoding='utf-8');i=s.index(key);j=s.index('{',i)+1;n=1
 while n:n+=(s[j]=='{')-(s[j]=='}');j+=1
 return s[i:j]
files=[str(a.source),'src/cgame/cg_colltree.cpp','src/cgame/cg_world.cpp']
code=r'''
#include <cgame_mp/cg_local_mp.h>
#include <cgame_mp/cg_main_mp.h>
#include <cgame_mp/cg_snapshot_mp.h>
#include <cgame_mp/cg_animtree_mp.h>
#include <cgame/cg_spawn.h>
#include <cgame/cg_colltree.h>
#include <cgame/cg_world.h>
#include <EffectsCore/fx_system.h>
#include <universal/assertive.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
static unsigned checks;static void check(bool b){if(!b)std::abort();++checks;}
bool Assert_MyHandler(const char*,int,int,const char*,...){std::abort();}
int g_entNumBits=10,g_entNumCscEnd=1536;
static fake_centity_s storage[512];fake_centity_s *cg_fakeEntitiesArray=storage;
centity_s *cg_entitiesArray[1]{};int cg_entArrayCount=MAX_CL_ENTNUM;
bool cg_fakeEntitiesInuseArray[512]{};int cg_fakeEntitiesInuseCount[1]{};
CgEntCollWorld cgEntCollWorld[1]{};static CgEntCollNode nodes[MAX_CL_ENTNUM]{};CgEntCollNode *cgEntCollNodes[1]={nodes};
static void CG_CollTreeOp(int,unsigned int,int,uintptr_t){}
void __cdecl CG_ShutdownEntity(int,centity_s*,bool){} // script/physics shutdown isolated; the tested nonphysics mover still owns its collision node
void __cdecl CG_SafeDObjFree(int,int ent){check(!nodes[ent].sector);check(CG_GetEntity(0,ent)->nextState.number==ent);}
void __cdecl FX_ThroughWithEffect(int,unsigned int,bool){}
UNLINK
WRAPPER
CLEAR
FREE
int main(){
 for(int id:{1024,1327,1328,1329,1330,1534})for(int linked:{0,1})for(int position:{0,1,2}){
  memset(storage,0,sizeof(storage));memset(nodes,0,sizeof(nodes));memset(cgEntCollWorld,0,sizeof(cgEntCollWorld));memset(cg_fakeEntitiesInuseArray,0,sizeof(cg_fakeEntitiesInuseArray));cg_fakeEntitiesInuseCount[0]=1;
  auto &f=storage[id-1024];f.cent.nextState.number=id;f.cent.nextState.eType=ET_SCRIPTMOVER;f.cent.pose.eType=ET_SCRIPTMOVER;f.cent.currentState.useCount=7;f.flags=123;cg_fakeEntitiesInuseArray[id-1024]=true;
  const int first=100,last=140;nodes[first].sector=nodes[last].sector=1;nodes[first].nextEntInSector=last+1;cgEntCollWorld[0].sectors[1].entListHead=first+1;
  if(linked){nodes[id].sector=1;if(position==0){cgEntCollWorld[0].sectors[1].entListHead=id+1;nodes[id].nextEntInSector=first+1;}if(position==1){nodes[first].nextEntInSector=id+1;nodes[id].nextEntInSector=last+1;}if(position==2)nodes[last].nextEntInSector=id+1;}
  CG_Free(0,id);check(!nodes[id].sector);check(cgEntCollWorld[0].sectors[1].entListHead==first+1);check(nodes[first].nextEntInSector==last+1);check(!nodes[last].nextEntInSector);check(nodes[first].sector==1&&nodes[last].sector==1);check(!cg_fakeEntitiesInuseArray[id-1024]&&!cg_fakeEntitiesInuseCount[0]&&!f.flags);
  centity_s zero{};check(!memcmp(&f.cent,&zero,sizeof(zero)));
 }
 printf("canonical CG_Free/collision retirement %u checks PASS pointer %zu\n",checks,sizeof(void*));
}
'''
for name,path,key in [('UNLINK',files[1],'void __cdecl CG_UnlinkEntityColl('),('WRAPPER',files[2],'void __cdecl CG_UnlinkEntity('),('CLEAR',files[0],'void __cdecl CG_ClearFakeEntInUse('),('FREE',files[0],'void __cdecl CG_Free(')]:code=code.replace(name,extract(path,key))
(a.out/'fixture.cpp').write_text(code,encoding='utf-8');(a.out/'source-manifest.json').write_text(json.dumps({f:hashlib.sha256(Path(f).read_bytes()).hexdigest() for f in files},indent=2))
