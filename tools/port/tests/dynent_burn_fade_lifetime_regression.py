"""Canonical DynEnt burn/fade queues with isolated FX, map and retirement services."""
from pathlib import Path
import argparse,json,hashlib
root=Path(__file__).resolve().parents[3];p=argparse.ArgumentParser();p.add_argument('--out',type=Path,required=True);p.add_argument('--source',type=Path,default=Path('src/DynEntity/DynEntity_client.cpp'));a=p.parse_args()
s=(root/a.source).read_text();start=s.index('void __cdecl DynEntCl_RemoveFromFadeList(');end=s.index('void __cdecl DynEntCl_FlameDamage(',start);production=s[start:end]
def extract(path,key):
 t=(root/path).read_text();i=t.index(key);j=t.index('{',i)+1;depth=1
 while depth:depth+=(t[j]=='{')-(t[j]=='}');j+=1
 return t[i:j]
code=r"""
#include <dynentity/DynEntity_load_obj.h>
#include <cgame_mp/cg_main_mp.h>
#include <EffectsCore/fx_system.h>
#include <EffectsCore/fx_unique_handle.h>
#include <EffectsCore/fx_load_obj.h>
#include <physics/phys_main.h>
#include <qcommon/cm_load.h>
#include <gfx_d3d/r_dvars.h>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <stdexcept>
#include <limits>
static int checks,createdBodies,registered;
#define CHECK(x) do{if(!(x)){fprintf(stderr,"check %i failed line %i\n",checks,__LINE__);abort();}++checks;}while(0)
bool Assert_MyHandler(const char *,int,int,const char *,...){throw std::runtime_error("assert");}
void Com_Error(errorParm_t,const char *,...){throw std::runtime_error("error");}
static cg_s fixtureCg;cg_s *cgArray=&fixtureCg;
clipMap_t cm;int numExtraDynEnts;unsigned __int8 usedExtraDynEnts[256];
static dvar_t reflection;const dvar_t *r_reflectionProbeGenerate=&reflection;
static DynEntityColl colls[1100];
void DynEnt_ClearCollWorld(DynEntityCollType){}
void DynEnt_CalcPhysPreset(const DynEntityDef *){}
DynEntityColl *DynEnt_GetEntityColl(DynEntityCollType draw,unsigned short id){return &colls[draw?1000+id:id];}
unsigned short DynEnt_GetEntityCount(DynEntityCollType type){return cm.dynEntCount[type];}
void DynEntCl_UnlinkEntity(unsigned short,DynEntityCollType){}
void DynEntCl_LinkModel(unsigned short){}
void DynEntCl_LinkBrush(unsigned short){}
void DynEnt_DestroyPhysics(PhysWorld,const DynEntityDef *,DynEntityClient *c){c->physObjId=0;}
unsigned short DynEntCl_AddEntityModel(){return 2;}
void Phys_ObjAddCustomForce(int,const float *,const float *,int,PhysPreset *){}
int gNumBurningDynEnts,gNumFadingDynEnts;
DynEnt_BurnData gDynEntBurnData[16];DynEnt_FadeData gDynEntFadeData[32];
OLD_ALIAS
const FxEffectDef *fxdefsm;
static FxEffectDef fireDef;
static DynEntityClient clients[1100];static DynEntityDef defs[1100];static DynEntityPose poses[1100];static bool ownerAlive[1100];
static FxEffectContainer effects[128];static bool active[128];static unsigned nextHandle,keys[128];static unsigned effectOwner[128];
static bool failCreate,initialized=true;static std::vector<unsigned> killed;static std::vector<int> destroyed;
static int absid(unsigned short id,DynEntityDrawType draw){return draw?1000+id:id;}
DynEntityClient *DynEnt_GetClientEntity(unsigned short id,DynEntityDrawType draw){CHECK(ownerAlive[absid(id,draw)]);return &clients[absid(id,draw)];}
DynEntityClient *DynEnt_GetClientEntity(unsigned short id){CHECK(id<1100&&ownerAlive[id]);return &clients[id];}
const DynEntityDef *DynEnt_GetEntityDef(unsigned short id,DynEntityDrawType draw){return &defs[absid(id,draw)];}
const DynEntityDef *DynEnt_GetEntityDef(unsigned short id){return &defs[id];}
DynEntityPose *DynEnt_GetClientPose(unsigned short id,DynEntityDrawType draw){return &poses[absid(id,draw)];}
const DynEntityPose *DynEnt_GetClientPose(unsigned short id){return &poses[id];}
unsigned short DynEnt_GetClientAbsId(unsigned short id,DynEntityDrawType draw){return absid(id,draw);}
void DynEnt_GetClientIdDrawType(unsigned short absolute,unsigned short *id,DynEntityDrawType *draw){*draw=absolute>=1000?DYNENT_DRAW_BRUSH:DYNENT_DRAW_MODEL;*id=absolute>=1000?absolute-1000:absolute;}
bool DynEnt_Valid(unsigned short id){return id<1100&&ownerAlive[id];}
const DynEntityProps *DynEnt_GetEntityProps(DynEntityType type){static DynEntityProps p={"fixture",false,true,true,true},noPhys={"nonphysical",false,true,false,true};return type==DYNENT_TYPE_DESTRUCT?&noPhys:&p;}
int DynEntCl_CreatePhysObj(const DynEntityDef *,DynEntityClient *,const GfxPlacement *){return ++createdBodies;}
FxEffectContainer *UniqueHandleToEffect(int local,unsigned h){CHECK(local==0);unsigned slot=h&127;return h&&active[slot]&&keys[slot]==h?&effects[slot]:nullptr;}
const FxEffectDef *FX_Register(char *){++registered;return &fireDef;}
unsigned DynEntCl_PlayBoltedFX(const FxEffectDef *,unsigned short owner){
 if(failCreate)return 0;
 for(unsigned i=0;i<128;++i)if(!active[i]){unsigned h=(++nextHandle<<7)|i;active[i]=true;keys[i]=h;effectOwner[i]=owner;effects[i]={};effects[i].effect.msecLastUpdate=1000;return h;}return 0;
}
void FX_ThroughWithEffect(int local,unsigned h,bool force){CHECK(local==0&&force);if(!initialized)return;if(UniqueHandleToEffect(0,h)){active[h&127]=false;killed.push_back(h);}}
PLAYED_TIME
PRODUCTION
INIT_AND_CREATE
void DynEntCl_DestroyEntityModel(unsigned short id){CHECK(id<1000&&ownerAlive[id]);destroyed.push_back(id);clients[id].flags=0;clients[id].health=-1;clients[id].physObjId=0;DynEntCl_RemoveFromFadeList(id);DynEntCl_SetBurning(id,DYNENT_DRAW_MODEL,false);ownerAlive[id]=false;}
static void reset(){gNumBurningDynEnts=gNumFadingDynEnts=0;memset(gDynEntBurnData,0,sizeof(gDynEntBurnData));memset(gDynEntFadeData,0,sizeof(gDynEntFadeData));memset(clients,0,sizeof(clients));memset(active,0,sizeof(active));memset(ownerAlive,1,sizeof(ownerAlive));nextHandle=0;killed.clear();destroyed.clear();createdBodies=0;registered=0;failCreate=false;initialized=true;_S2_5=0;fxdefsm=nullptr;fixtureCg.time=100000;
 for(auto &c:clients)c.flags=3;for(auto &d:defs)d.type=DYNENT_TYPE_CLUTTER;
}
static int activeCount(){int n=0;for(bool b:active)n+=b;return n;}
static void invariant(){CHECK(gNumBurningDynEnts>=0&&gNumBurningDynEnts<=16);bool seen[1100]{};
 for(int i=0;i<gNumBurningDynEnts;++i){auto r=gDynEntBurnData[i];CHECK(r.id<1100&&!seen[r.id]);seen[r.id]=true;CHECK(clients[r.id].flags&8);CHECK(UniqueHandleToEffect(0,r.fx));CHECK(effectOwner[r.fx&127]==r.id);}
 for(int i=0;i<1100;++i)CHECK(bool(clients[i].flags&8)==seen[i]);CHECK(activeCount()==gNumBurningDynEnts);
}
static void retention(){reset();DynEntCl_SetBurning(0,DYNENT_DRAW_MODEL,true);CHECK(gNumBurningDynEnts==1);unsigned h=gDynEntBurnData[0].fx;CHECK(UniqueHandleToEffect(0,h));DynEnt_UpdateBurning();CHECK(gNumBurningDynEnts==1&&(clients[0].flags&8));CHECK(clients[0].burnTime==1000);invariant();}
int main(int argc,char **argv){
 if(argc==2&&!strcmp(argv[1],"full-queue")){reset();for(int i=0;i<17;++i)DynEntCl_SetBurning(i,DYNENT_DRAW_MODEL,true);CHECK(!(clients[0].flags&8));CHECK(activeCount()==16);puts("production full queue PASS");return 0;}
 if(argc==2&&!strcmp(argv[1],"reset")){reset();DynEntCl_SetBurning(1,DYNENT_DRAW_MODEL,true);DynEnt_ResetBurning();CHECK(activeCount()==0&&!(clients[1].flags&8));puts("production reset PASS");return 0;}
 if(argc==2&&!strcmp(argv[1],"expiry")){reset();for(int i=0;i<8;++i)DynEntCl_SetBurning(i,DYNENT_DRAW_MODEL,true);ALIAS_SYNC for(int i=0;i<8;++i)effects[gDynEntBurnData[i].fx&127].effect.msecLastUpdate=21001;DynEnt_UpdateBurning();CHECK(gNumBurningDynEnts==0);puts("production expiry PASS");return 0;}
 if(argc==2&&!strcmp(argv[1],"fade-wrap")){reset();DynEntCl_SetFadeOut(1,DYNENT_DRAW_MODEL);DynEnt_UpdateFading(190536);CHECK(destroyed.size()==1);puts("production fade wrap PASS");return 0;}
 if(argc==2&&!strcmp(argv[1],"fade-swap")){reset();for(int i=0;i<4;++i)DynEntCl_SetFadeOut(i,DYNENT_DRAW_MODEL);DynEnt_UpdateFading(126001);CHECK(destroyed.size()==4);puts("production fade swapped expiry PASS");return 0;}
 if(argc==2&&!strcmp(argv[1],"retention")){retention();puts("production burn retention PASS");return 0;}
 retention();
 // New owners must not inherit scorch/fade/constraint clocks from a retired slot.
 reset();static XModel model;static PhysPreset preset;model.physPreset=&preset;float quat[4]={0,0,0,1},origin[3]={1,2,3},force[3]={1,0,0};clients[2].burnTime=5000;clients[2].fadeTime=800;clients[2].physicsStartTime=90000;
 CHECK(DynEntCl_CreateEntityModel(&model,quat,origin,origin,force,0,nullptr)==2);CHECK(clients[2].burnTime==0&&clients[2].fadeTime==0&&clients[2].physicsStartTime==0);
 // Map init retires old burn consumers and resets fresh logical appearance clocks.
 reset();cm={};cm.originalDynEntCount=2;cm.dynEntCount[0]=3;cm.dynEntCount[1]=1;clients[0].burnTime=5000;clients[0].fadeTime=800;clients[0].physicsStartTime=90000;clients[1000]=clients[0];DynEntCl_SetBurning(0,DYNENT_DRAW_MODEL,true);DynEntCl_InitEntities(0);
 CHECK(activeCount()==0&&gNumBurningDynEnts==0);CHECK(cm.dynEntCount[0]==2);for(int id:{0,1000})CHECK(clients[id].burnTime==0&&clients[id].fadeTime==0&&clients[id].physicsStartTime==0);
 // The repeated-burning wake path must respect nonphysical object types.
 reset();defs[5].type=DYNENT_TYPE_DESTRUCT;clients[5].health=-80;DynEntCl_SetBurning(5,DYNENT_DRAW_MODEL,true);DynEntCl_SetBurning(5,DYNENT_DRAW_MODEL,true);CHECK(createdBodies==0);DynEnt_ResetBurning();invariant();
 // Normal extinguish must kill precisely its own reference, then retain moved owner.
 reset();for(int i=0;i<3;++i)DynEntCl_SetBurning(i,DYNENT_DRAW_MODEL,true);auto first=gDynEntBurnData[0];auto last=gDynEntBurnData[2];DynEntCl_SetBurning(0,DYNENT_DRAW_MODEL,false);CHECK(killed.size()==1&&killed[0]==first.fx);CHECK(gDynEntBurnData[0].id==last.id&&gDynEntBurnData[0].fx==last.fx);invariant();
 // A full queue replaces its first owner and clears that old owner's burning flag.
 reset();for(int i=0;i<17;++i)DynEntCl_SetBurning(i,DYNENT_DRAW_MODEL,true);CHECK(gNumBurningDynEnts==16);CHECK(!(clients[0].flags&8)&&(clients[16].flags&8));CHECK(killed.size()==1);invariant();
 failCreate=true;DynEntCl_SetBurning(20,DYNENT_DRAW_MODEL,true);CHECK(gNumBurningDynEnts==16&&killed.size()==1&&!(clients[20].flags&8));failCreate=false;invariant();
 // All expired entries, including swapped-in tails, retire in this one update.
 for(int i=0;i<16;++i)effects[gDynEntBurnData[i].fx&127].effect.msecLastUpdate=21001;DynEnt_UpdateBurning();CHECK(gNumBurningDynEnts==0&&activeCount()==0);invariant();
 // Clamp before narrowing to the 16-bit burn timer after a large time jump.
 reset();DynEntCl_SetBurning(0,DYNENT_DRAW_MODEL,true);effects[gDynEntBurnData[0].fx&127].effect.msecLastUpdate=70000;DynEnt_UpdateBurning();CHECK(clients[0].burnTime==30000);CHECK(gNumBurningDynEnts==0);invariant();
 // A stale generation must not kill a new FX allocation that reused its slot.
 reset();DynEntCl_SetBurning(0,DYNENT_DRAW_MODEL,true);auto stale=gDynEntBurnData[0].fx;active[stale&127]=false;auto unrelated=DynEntCl_PlayBoltedFX(&fireDef,50);CHECK((unrelated&127)==(stale&127)&&unrelated!=stale);DynEnt_UpdateBurning();CHECK(gNumBurningDynEnts==0&&!(clients[0].flags&8));CHECK(UniqueHandleToEffect(0,unrelated));FX_ThroughWithEffect(0,unrelated,true);invariant();
 // Actual absolute IDs include brushes. Reset must retire owners and references.
 reset();DynEntCl_SetBurning(0,DYNENT_DRAW_BRUSH,true);DynEntCl_SetBurning(2,DYNENT_DRAW_MODEL,true);DynEnt_ResetBurning();CHECK(gNumBurningDynEnts==0&&activeCount()==0);invariant();DynEnt_ResetBurning();CHECK(killed.size()==2);
 CHECK(registered==1);DynEntCl_SetBurning(4,DYNENT_DRAW_MODEL,true);CHECK(registered==2);clients[4].burnTime=1234;DynEntCl_SetBurning(4,DYNENT_DRAW_MODEL,false);CHECK(clients[4].burnTime==1234);invariant();
 reset();DynEntCl_SetBurning(0,DYNENT_DRAW_MODEL,true);initialized=false;memset(active,0,sizeof(active));DynEnt_ResetBurning();initialized=true;invariant();
 reset();DynEntCl_SetBurning(4,DYNENT_DRAW_MODEL,true);ownerAlive[4]=false;clients[4].flags=0;DynEnt_UpdateBurning();CHECK(gNumBurningDynEnts==0&&activeCount()==0);invariant();
 // A normally retired owner removes a still-active burn consumer first.
 reset();DynEntCl_SetBurning(2,DYNENT_DRAW_MODEL,true);DynEntCl_DestroyEntityModel(2);CHECK(gNumBurningDynEnts==0&&activeCount()==0);invariant();
 // Fading removes all eligible swapped entries, with no ushort time wrap.
 reset();for(int i=0;i<4;++i)DynEntCl_SetFadeOut(i,DYNENT_DRAW_MODEL);CHECK(gNumFadingDynEnts==4);DynEnt_UpdateFading(125500);for(int i=0;i<4;++i)CHECK(clients[i].fadeTime==500);DynEnt_UpdateFading(126001);CHECK(gNumFadingDynEnts==0&&destroyed.size()==4);
 reset();DynEntCl_SetFadeOut(1,DYNENT_DRAW_MODEL);DynEnt_UpdateFading(190536);CHECK(destroyed.size()==1&&gNumFadingDynEnts==0);
 reset();DynEntCl_SetFadeOut(1,DYNENT_DRAW_MODEL);ownerAlive[1]=false;clients[1].flags=0;DynEnt_UpdateFading(125500);CHECK(gNumFadingDynEnts==0&&destroyed.empty());
 // Repeated scheduling is a single owner, not a second destruction lease.
 reset();DynEntCl_SetFadeOut(0xFFFF,DYNENT_DRAW_MODEL);CHECK(gNumFadingDynEnts==0);DynEntCl_SetFadeOut(1,DYNENT_DRAW_MODEL);DynEntCl_SetFadeOut(1,DYNENT_DRAW_MODEL);CHECK(gNumFadingDynEnts==1);DynEnt_UpdateFading(126001);CHECK(destroyed.size()==1&&gNumFadingDynEnts==0);
 reset();for(int i=0;i<33;++i){fixtureCg.time=100000+i;DynEntCl_SetFadeOut(i,DYNENT_DRAW_MODEL);}CHECK(gNumFadingDynEnts==32&&destroyed.size()==1&&destroyed[0]==0);DynEnt_UpdateFading(126100);CHECK(gNumFadingDynEnts==0&&destroyed.size()==33);
 // Mixed repeat/replace/extinguish stress exercises generation reuse and ownership.
 reset();unsigned rng=0xb17f001u;for(int i=0;i<1500;++i){rng=rng*1664525u+1013904223u;int id=(rng>>16)%32;DynEntCl_SetBurning(id,DYNENT_DRAW_MODEL,bool(rng&1));invariant();if(i%41==0)DynEnt_UpdateBurning();}DynEnt_ResetBurning();invariant();
 printf("production burn/fade lifetime %i checks PASS; pointer %zu; queue entry %zu bytes\n",checks,sizeof(void*),sizeof(DynEnt_BurnData));
}
"""
init_create=extract(a.source,'void __cdecl DynEntCl_InitEntities(')+'\n'+extract(a.source,'unsigned __int16 __cdecl DynEntCl_CreateEntityModel(')
code=code.replace('ALIAS_SYNC','for(int i=0;i<gNumBurningDynEnts;++i)hEffect[2*i]=gDynEntBurnData[i].fx;' if 'unsigned int hEffect[31];' in s else '').replace('INIT_AND_CREATE',init_create).replace('OLD_ALIAS','unsigned int hEffect[31];' if 'unsigned int hEffect[31];' in s else '').replace('PLAYED_TIME',extract('src/EffectsCore/fx_system.cpp','int __cdecl FX_GetEffectPlayedTime(')).replace('PRODUCTION',production)
a.out.mkdir(parents=True,exist_ok=False);(a.out/'fixture.cpp').write_text(code,encoding='utf-8');(a.out/'source-manifest.json').write_text(json.dumps({str(a.source):hashlib.sha256((root/a.source).read_bytes()).hexdigest(),'src/EffectsCore/fx_system.cpp':hashlib.sha256((root/'src/EffectsCore/fx_system.cpp').read_bytes()).hexdigest()},indent=2),encoding='utf-8')
