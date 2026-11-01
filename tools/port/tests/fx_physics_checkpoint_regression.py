"""Production FX physical-owner staging/rebinding with isolated physics services.
Actual model geometry, body allocation and integration require APK acceptance.
"""
from pathlib import Path
import argparse,json,hashlib
root=Path(__file__).resolve().parents[3];p=argparse.ArgumentParser();p.add_argument('--out',type=Path,required=True);a=p.parse_args()
def body(path,key):
 s=(root/path).read_text(encoding='utf-8');start=s.index(key);end=s.index('{',start)+1;depth=1
 while depth:depth+=(s[end]=='{')-(s[end]=='}');end+=1
 return s[start:end]
owners='src/game_sp/g_sp_native_owners_checkpoint.inl';helper='src/game_sp/g_sp_fx_physics_checkpoint.inl';physics='src/physics/phys_checkpoint.inl'
decl=body(owners,'struct SPFxCheckpointControls')+';\n'+body(owners,'struct SPFxCheckpointStage')+';\n'+body(owners,'static int SP_NativeHandleIndex(')+'\n'+body(physics,'bool Phys_IsSavedBodyStateValid(')
code=r'''
#include <EffectsCore/fx_system.h>
#include <EffectsCore/fx_archive.h>
#include <physics/phys_checkpoint.h>
#include <xanim/xmodel.h>
#include <qcommon/common.h>
#include <map>
#include <stdexcept>
#include <cstring>
#include <cmath>
#include <cstdio>
#include <cstdlib>
int fx_elemLimit=16;
static unsigned checks,creates,applies;static int failAt=-1,nextId=100;static bool corrupt,missingModel;
static XModel model;static char preset[256];static std::map<int,PhysSavedBodyState> bodies;
void Com_Error(errorParm_t,const char*,...){throw std::runtime_error("rejected");}
void Com_Printf(int,const char*,...){}
bool Assert_MyHandler(const char*,int,int,const char*,...){std::abort();}
ACTUAL_DECLARATIONS
FxEffectContainer *FX_EffectFromHandle(FxSystem*s,unsigned short h){return &s->effects[h/(sizeof(FxEffectContainer)/4)];}
FxElemVisuals FX_GetElemVisuals(const FxElemDef*,int seed){if(seed<0||seed>=479)std::abort();FxElemVisuals v{};v.model=missingModel?nullptr:&model;return v;}
bool FX_SpawnModelPhysics(FxSystem*,FxEffect*e,const FxElemDef*d,int seed,FxElem*elem,int sequence){
 if(d->elemType!=7||!(d->flags&0x8000000)||elem->physObjId||sequence!=elem->sequence||seed<0||seed>=479||!e->def)std::abort();
 if(static_cast<int>(creates++)==failAt)return false;
 elem->physObjId=nextId++;bodies[elem->physObjId]={};return true;
}
void Phys_ApplyBodyState(int id,const PhysSavedBodyState*s){if(!bodies.count(id)||!Phys_IsSavedBodyStateValid(s))std::abort();bodies[id]=*s;++applies;}
void Phys_CaptureBodyState(int id,PhysSavedBodyState*s){if(!bodies.count(id))std::abort();*s=bodies[id];if(corrupt)s->origin[0]+=1;}
bool Phys_VerifyAppliedBodyState(int id,const PhysSavedBodyState*s){return !corrupt&&bodies.count(id)&&!memcmp(&bodies[id],s,sizeof(*s));}
#include <game_sp/g_sp_fx_physics_checkpoint.inl>
static void check(bool b){if(!b){fprintf(stderr,"check %u failed\n",checks);std::abort();}++checks;}
template<class F>static void rejects(F f){bool b=false;try{f();}catch(const std::runtime_error&){b=true;}check(b);}
static SPFxCheckpointStage stage;static FxSystemBuffers live;static FxSystem system;static FxEffectDef effectDef;static FxElemDef elemDef;
static void setup(){
 memset(&stage,0,sizeof(stage));memset(&live,0,sizeof(live));memset(&system,0,sizeof(system));memset(&effectDef,0,sizeof(effectDef));memset(&elemDef,0,sizeof(elemDef));
 bodies.clear();creates=applies=0;failAt=-1;nextId=100;corrupt=missingModel=false;fx_elemLimit=16;
 model={};model.physPreset=reinterpret_cast<PhysPreset*>(preset);
 effectDef.elemDefCountOneShot=1;effectDef.elemDefsEA=&elemDef;elemDef.elemType=7;elemDef.flags=0x8000000;elemDef.visualCount=1;
 stage.buffers.effects[0].effect.def=&effectDef;stage.buffers.effects[0].effect.randomSeed=200;
 for(int slot:{0,7}){auto&o=stage.physical[slot];o.present=true;o.effectHandle=0;o.body.quaternion[3]=1;o.body.origin[0]=11.25f+slot;o.body.linearVelocity[1]=7.5f;o.body.gravity[2]=-800;
  auto&e=stage.buffers.elems[slot].item.elem;e.msecBegin=1234+slot;e.sequence=slot+1;}
 stage.physicalParticles=2;memcpy(&live,&stage.buffers,sizeof(live));system.effects=live.effects;system.elems=live.elems;
}
static void retire(){for(int i=0;i<fx_elemLimit;++i){int&id=system.elems[i].item.elem.physObjId;if(id){check(bodies.erase(id)==1);id=0;}}check(bodies.empty());}
int main(){
 setup();SP_PreflightClientEffectsPhysics(stage);SP_PublishClientEffectsPhysics(stage,&system);
 check(creates==2&&applies==2&&bodies.size()==2);check(live.elems[0].item.elem.physObjId!=live.elems[7].item.elem.physObjId);
 for(int slot:{0,7}){check(stage.buffers.elems[slot].item.elem.physObjId==0);check(!memcmp(&bodies[live.elems[slot].item.elem.physObjId],&stage.physical[slot].body,sizeof(PhysSavedBodyState)));}
 retire();
 // Every bad shape/owner must fail before any factory publication.
 for(int fault=0;fault<11;++fault){setup();
  if(fault==0)stage.physicalParticles=1;if(fault==1)stage.physical[7].effectHandle=0xFFFF;
  if(fault==2)stage.buffers.effects[0].effect.def=nullptr;if(fault==3)stage.buffers.elems[7].item.elem.defIndex=1;
  if(fault==4)elemDef.flags=0;if(fault==5)stage.buffers.elems[7].item.elem.physObjId=123;
  if(fault==6)model.physPreset=nullptr;if(fault==7)missingModel=true;if(fault==8)stage.physical[7].body.quaternion[3]=0;
  if(fault==9)elemDef.visualCount=0;if(fault==10){elemDef.visualCount=2;elemDef.visuals.array=nullptr;}
  rejects([]{SP_PreflightClientEffectsPhysics(stage);});check(creates==0&&bodies.empty());
 }
 setup();stage.physical[16]=stage.physical[7];stage.physicalParticles=3;rejects([]{SP_PreflightClientEffectsPhysics(stage);});check(creates==0);
 // A partial factory failure leaves every allocated object owned by a live
 // element so the engine's normal FX retirement can destroy it.
 setup();SP_PreflightClientEffectsPhysics(stage);failAt=1;rejects([]{SP_PublishClientEffectsPhysics(stage,&system);});
 check(bodies.size()==1&&live.elems[0].item.elem.physObjId&&live.elems[7].item.elem.physObjId==0);retire();
 setup();SP_PreflightClientEffectsPhysics(stage);corrupt=true;rejects([]{SP_PublishClientEffectsPhysics(stage,&system);});check(bodies.size()==1&&live.elems[0].item.elem.physObjId);retire();
 setup();memset(stage.physical,0,sizeof(stage.physical));stage.physicalParticles=0;SP_PreflightClientEffectsPhysics(stage);SP_PublishClientEffectsPhysics(stage,&system);check(creates==0&&bodies.empty());
 printf("actual FX physics checkpoint %u checks PASS, native pointers %zu bytes, model preset offset %zu\n",checks,sizeof(void*),offsetof(XModel,physPreset));
}
'''
code=code.replace('static FxSystem system','static FxSystem fixtureSystem').replace('&system','&fixtureSystem').replace('sizeof(system)','sizeof(fixtureSystem)').replace('system.effects','fixtureSystem.effects').replace('system.elems','fixtureSystem.elems')
a.out.mkdir(parents=True,exist_ok=False);(a.out/'fixture.cpp').write_text(code.replace('ACTUAL_DECLARATIONS',decl),encoding='utf-8')
(a.out/'source-manifest.json').write_text(json.dumps({f:hashlib.sha256((root/f).read_bytes()).hexdigest() for f in [owners,helper,physics,'src/effectscore/fx_system.cpp']},indent=2),encoding='utf-8')
