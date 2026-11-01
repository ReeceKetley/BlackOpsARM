"""Production DYN1/DYN2/DYN3 codec/publication with canonical structs and isolated providers."""
from pathlib import Path
import argparse,hashlib,json
root=Path(__file__).resolve().parents[3];p=argparse.ArgumentParser();p.add_argument('--out',type=Path,required=True);a=p.parse_args()
def extract(path,key):
 s=(root/path).read_text();start=s.index(key);end=s.index('{',start)+1;depth=1
 while depth:depth+=(s[end]=='{')-(s[end]=='}');end+=1
 return s[start:end]
source='src/game_sp/g_sp_dynent_checkpoint.inl'
s=(root/source).read_text();s=s[:s.index('static int s_dynMotionPauseOwner')].replace('#include <client/client.h>','').replace('#include <cgame_mp/cg_local_mp.h>','')
code=r"""
#include <qcommon/common.h>
#include <database/db_registry.h>
#include <gfx_d3d/r_init.h>
#include <qcommon/cm_load.h>
#include <universal/memfile.h>
#include <physics/phys_checkpoint.h>
#include <physics/phys_main.h>
#include <dynentity/DynEntity_load_obj.h>
#include <EffectsCore/fx_system.h>
#include <xanim/xmodel_utils.h>
#include <vector>
#include <string>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <stdexcept>
#include <cstdlib>
#include <limits>
static int checks,created,destroyed,clientLinks,serverLinks,failLink;
#define CHECK(x) do{if(!(x)){fprintf(stderr,"check %i failed line %i\n",checks,__LINE__);abort();}++checks;}while(0)
bool Assert_MyHandler(const char *,int,int,const char *,...){abort();}
void Com_Error(errorParm_t,const char *,...){throw std::runtime_error("rejected");}
void Com_Printf(int,const char *,...){}
void MemFile_ReadData(MemoryFile *f,int n,unsigned char *out){if(n<0||n>f->bufferSize-f->bytesUsed)Com_Error(ERR_DROP,"truncated");memcpy(out,f->buffer+f->bytesUsed,n);f->bytesUsed+=n;}
void MemFile_WriteDataInternal(MemoryFile *,unsigned,unsigned char *){Com_Error(ERR_DROP,"flush");}
char *MemFile_ReadCString(MemoryFile *f){static char s[256];for(int i=0;i<256;++i){MemFile_ReadData(f,1,(unsigned char *)&s[i]);if(!s[i])return s;}Com_Error(ERR_DROP,"long");return nullptr;}
clipMap_t cm{};
int numExtraDynEnts,gNumFadingDynEnts,gNumBurningDynEnts;
unsigned __int8 usedExtraDynEnts[256];
DynEnt_BurnData gDynEntBurnData[16]{};
DynEnt_FadeData gDynEntFadeData[32]{};
struct FixtureCg {int time=100000;};static FixtureCg fixtureCg;
static FixtureCg *FixtureGlobals(int){return &fixtureCg;}
#define CG_GetLocalClientGlobals FixtureGlobals
static FxEffectContainer fixtureBurnEffect{};
static bool fixtureNeedsBolt=true;
FxEffectContainer *UniqueHandleToEffect(int,unsigned handle){return handle&&fixtureBurnEffect.effect.uniqueHandle==handle?&fixtureBurnEffect:nullptr;}
char FX_NeedsBoltUpdate(const FxEffectDef *){return fixtureNeedsBolt;}
void DynEnt_ResetBurning(){gNumBurningDynEnts=0;memset(gDynEntBurnData,0,sizeof(gDynEntBurnData));}
void DynEnt_ResetFading(){gNumFadingDynEnts=0;memset(gDynEntFadeData,0,sizeof(gDynEntFadeData));}
r_global_permanent_t rgp;
static XModel *fixtureAssetModel;
static PhysPreset *fixtureAssetPreset;
static int dbLocks;
static bool missingModel,missingPreset,defaultAsset;
void Sys_EnterCriticalSection(CriticalSection){++dbLocks;}
void Sys_LeaveCriticalSection(CriticalSection){CHECK(dbLocks==1);--dbLocks;}
XAssetEntryPoolEntry *DB_FindXAssetEntry(XAssetType type,const char *name){
 CHECK(dbLocks==1);static XAssetEntryPoolEntry e;e.entry={};e.entry.zoneIndex=defaultAsset?0:1;
 if(type==ASSET_TYPE_XMODEL&&!missingModel&&fixtureAssetModel&&!strcmp(fixtureAssetModel->name,name))e.entry.asset.header.model=fixtureAssetModel;
 else if(type==ASSET_TYPE_PHYSPRESET&&!missingPreset&&fixtureAssetPreset&&!strcmp(fixtureAssetPreset->name,name))e.entry.asset.header.physPreset=fixtureAssetPreset;
 else return nullptr;return &e;
}
static PhysSavedObjectState physics{};
VALIDATOR
bool Phys_CaptureObjectState(int world,int id,PhysSavedObjectState &out){if(world||!id)return false;out=physics;return Phys_IsSavedObjectStateValid(out);}
bool Phys_ApplyObjectState(int world,int id,const PhysSavedObjectState &in){if(world||!id||!Phys_IsSavedObjectStateValid(in))return false;physics=in;return true;}
const char *XModelGetName(const XModel *m){return m->name;}
const DynEntityProps *DynEnt_GetEntityProps(DynEntityType){static DynEntityProps props={"fixture",false,true,true,true};return &props;}
void DynEnt_UnlinkEntity(DynEntityCollType,unsigned short){}
void DynEntSv_LinkModel(unsigned short){++serverLinks;}
void DynEntSv_LinkBrush(unsigned short){++serverLinks;}
void DynEntCl_UnlinkEntity(unsigned short,DynEntityCollType){}
void DynEntCl_LinkModel(unsigned short i){++clientLinks;if(failLink&&!--failLink)Com_Error(ERR_DROP,"link failure");cm.dynEntPoseList[0][i].radius=DynEntCl_GetCurrentXModel(&cm.dynEntDefList[0][i],&cm.dynEntClientList[0][i])->radius;}
double XModelGetRadius(const XModel *m){return m->radius;}
CURRENTMODEL
void DynEntCl_LinkBrush(unsigned short){++clientLinks;}
void DynEnt_DestroyPhysics(PhysWorld,const DynEntityDef *,DynEntityClient *c){++destroyed;c->physObjId=0;}
int DynEntCl_CreateCheckpointPhysObj(const DynEntityDef *,DynEntityClient *c,const GfxPlacement *){++created;return c->physObjId=77;}
void DynEntCl_SP_CleanupSpawnedDynEnts(){
 for(int i=0;i<256;++i)if(usedExtraDynEnts[i]){int id=cm.originalDynEntCount+i;
 CHECK(id<cm.dynEntCount[0]);auto &c=cm.dynEntClientList[0][id];if(c.physObjId)DynEnt_DestroyPhysics(PHYS_WORLD_DYNENT,&cm.dynEntDefList[0][id],&c);
 DynEntCl_UnlinkEntity(id,DYNENT_COLL_CLIENT_MODEL);c.flags=0;c.health=-1;usedExtraDynEnts[i]=0;--numExtraDynEnts;
 }}
PRODUCTION
static std::vector<unsigned char> encode(){static MemoryFile w;w={};SP_DynEntCodec(&w,false);return {w.cacheBuffer,w.cacheBuffer+w.cacheBufferUsed};}
static bool decode(const std::vector<unsigned char>&bytes){MemoryFile f{};f.buffer=const_cast<unsigned char*>(bytes.data());f.bufferSize=(int)bytes.size();try{SP_DynEntCodec(&f,true);CHECK(f.bytesUsed==f.bufferSize);return true;}catch(...){return false;}}
int main(){
 static DynEntityDef defs[259]{};static DynEntityClient clients[259]{};static DynEntityServer servers[3]{};static DynEntityPose poses[259]{};static DynEntityColl colls[259]{};static GfxWorld world{};static XModel model{},destroyedModel{};static PhysPreset preset{};
 rgp.world=&world;world.dpvsDyn.dynEntClientCount[0]=259;cm.dynEntCollList[0]=colls;fixtureAssetModel=&model;fixtureAssetPreset=&preset;preset.name="fixture_preset";
 CHECK(sizeof(void*)==8);CHECK(sizeof(DynEntityClient)==20);CHECK(sizeof(DynEntityPose)==32);
 cm.originalDynEntCount=3;cm.dynEntCount[0]=cm.dynEntCount[2]=3;cm.dynEntDefList[0]=defs;cm.dynEntClientList[0]=clients;cm.dynEntServerList[0]=servers;cm.dynEntPoseList[0]=poses;model.name="fixture_model";model.radius=42;destroyedModel.name="fixture_destroyed";destroyedModel.radius=84;
 for(int i=0;i<3;++i){auto &d=defs[i];d.type=DYNENT_TYPE_CLUTTER;d.xModel=&model;d.health=100;d.physPreset=&preset;d.pose.quat[3]=1;d.pose.origin[0]=float(i)*100;for(auto &x:d.physConstraints)x=511;poses[i].pose=d.pose;poses[i].radius=10;clients[i].flags=3;clients[i].health=100-i*17;clients[i].lightingHandle=9+i;servers[i].flags=1;servers[i].health=90-i*13;}
 clients[2].flags=0;servers[2].flags=0;defs[1].destroyedxModel=&destroyedModel;clients[1].health=0;poses[1].pose.origin[1]=25;clients[1].physObjId=42;
 physics.owner=-1;physics.matrix[0][0]=physics.matrix[1][1]=physics.matrix[2][2]=1;physics.center[0][0]=physics.center[1][1]=physics.center[2][2]=1;physics.m_inv_mass=.25f;physics.mass=4;physics.m_max_delta_t=.033f;physics.geometryCount=1;physics.collisionCount=1;
 CHECK(Phys_IsSavedObjectStateValid(physics));CHECK(SP_DynEntCaptureSupported());SP_CaptureDynEnts();auto wire=encode();CHECK(decode(wire));CHECK(encode()==wire);
 auto saved=s_savedDynEnts;clients[0].health=999;servers[0].health=888;SP_PublishDynEntServer();SP_PublishDynEntClient();CHECK(created==1&&destroyed==1);CHECK(clientLinks==2&&serverLinks==2);CHECK(clients[0].health==100&&servers[0].health==90);CHECK(clients[0].lightingHandle==9);CHECK(clients[1].physObjId==77);CHECK(poses[1].pose.origin[1]==25);CHECK(poses[1].radius==84);SP_PublishDynEntClient();CHECK(created==1);
 for(size_t n=0;n<wire.size();++n){auto cut=wire;cut.resize(n);CHECK(!decode(cut));}
 CHECK(decode(wire));auto bad=wire;bad[0]^=1;CHECK(!decode(bad));bad=wire;int wrong=4;memcpy(bad.data()+4,&wrong,4);CHECK(!decode(bad));
 model.name="changed";CHECK(!decode(wire));model.name="fixture_model";model.radius=42;destroyedModel.name="fixture_destroyed";destroyedModel.radius=84;CHECK(decode(wire));
 auto valid=s_savedDynEnts[0];for(float v:{NAN,INFINITY,-INFINITY}){auto r=valid;r.pose.pose.origin[1]=v;CHECK(!SP_DynStateValid(r));r=valid;r.pose.pose.quat[0]=v;CHECK(!SP_DynStateValid(r));}
 auto r=valid;r.client.physObjId=12;CHECK(!SP_DynStateValid(r));r=valid;r.client.lightingHandle=2;CHECK(!SP_DynStateValid(r));r=valid;r.client.flags|=8;CHECK(!SP_DynStateValid(r));r=valid;r.hasBody=3;CHECK(!SP_DynStateValid(r));
 clients[0].health=777;servers[0].health=777;defs[2].xModel=nullptr;bool rejected=false;try{SP_PublishDynEntServer();}catch(...){rejected=true;}CHECK(rejected&&clients[0].health==777&&servers[0].health==777);defs[2].xModel=&model;
 for(int *v:{&numExtraDynEnts,&gNumBurningDynEnts,&gNumFadingDynEnts}){*v=1;CHECK(!SP_DynEntCaptureSupported());*v=0;}
 CHECK(SP_DynEntCaptureSupported());defs[1].physConstraints[0]=2;CHECK(!SP_DynEntCaptureSupported());CHECK(!SP_DynDefinitionMatches(saved[1]));defs[1].physConstraints[0]=511;
 physics.owner=-2;CHECK(!Phys_IsSavedObjectStateValid(physics));physics.owner=2048;CHECK(!Phys_IsSavedObjectStateValid(physics));physics.owner=-1;CHECK(Phys_IsSavedObjectStateValid(physics));

 // DYN1 archive compatibility retains the exact old wire layout.
 CHECK(decode(wire));s_savedDynSchema=0x314E5944;auto legacy=encode();CHECK(decode(legacy));CHECK(encode()==legacy);CHECK(s_savedDynExtraCount==0);
 // A genuine sparse spawned inventory includes holes and independent bodies.
 for(int slot:{4,200}){int i=3+slot;auto &d=defs[i];d={};d.type=DYNENT_TYPE_CLUTTER;d.xModel=&model;d.physPreset=&preset;d.flags=2;d.contents=model.contents;d.pose.quat[3]=1;d.pose.origin[0]=float(slot);for(auto &c:d.physConstraints)c=511;
  poses[i].pose=d.pose;poses[i].radius=42;clients[i].flags=3;clients[i].physObjId=slot==200?88:0;clients[i].health=-1;usedExtraDynEnts[slot]=1;}
 cm.dynEntCount[0]=204;numExtraDynEnts=2;CHECK(SP_DynEntCaptureSupported());SP_CaptureDynEnts();auto sparse=encode();CHECK(decode(sparse));CHECK(encode()==sparse);CHECK(s_savedDynEnts.size()==5);
 const auto sparseRecords=s_savedDynEnts;CHECK(SP_DynSpawned(sparseRecords[3]));CHECK(sparseRecords[3].index==7&&sparseRecords[4].index==203);CHECK(sparseRecords[4].hasBody==1);
 for(size_t n=0;n<sparse.size();++n){auto cut=sparse;cut.resize(n);CHECK(!decode(cut));}
 CHECK(decode(sparse));const auto staged=s_savedDynEnts;
 // Invalid graph/asset inputs must fail before any native slot mutation.
 CHECK(!SP_DynTopologyValid(260,2,s_savedDynUsed));CHECK(!SP_DynTopologyValid(4,2,s_savedDynUsed));
 auto bitmap=s_savedDynUsed[200];s_savedDynUsed[200]=2;CHECK(!SP_DynTopologyValid(204,2,s_savedDynUsed));s_savedDynUsed[200]=bitmap;
 CHECK(!SP_DynTopologyValid(204,1,s_savedDynUsed));world.dpvsDyn.dynEntClientCount[0]=203;CHECK(!SP_DynTopologyValid(204,2,s_savedDynUsed));world.dpvsDyn.dynEntClientCount[0]=259;
 DynEntityDef constructed{};auto extra=staged[3];CHECK(SP_DynSpawnDefinition(extra,constructed));CHECK(constructed.xModel==&model&&constructed.physPreset==&preset&&constructed.flags==2);
 missingModel=true;CHECK(!decode(sparse));missingModel=false;missingPreset=true;CHECK(!decode(sparse));missingPreset=false;defaultAsset=true;CHECK(!decode(sparse));defaultAsset=false;CHECK(decode(sparse));
 extra.initial.quat[3]=NAN;CHECK(!SP_DynSpawnDefinition(extra,constructed));extra=staged[3];extra.type=DYNENT_TYPE_DESTRUCT;CHECK(!SP_DynSpawnDefinition(extra,constructed));extra=staged[3];extra.serverFlags=0;CHECK(!SP_DynSpawnDefinition(extra,constructed));
 defs[7].targetname=1;CHECK(!SP_DynEntCaptureSupported());defs[7].targetname=0;CHECK(SP_DynEntCaptureSupported());
 // Remove the old extras to model a fresh map; definitions must be recreated.
 DynEntCl_SP_CleanupSpawnedDynEnts();defs[7]={};defs[203]={};cm.dynEntCount[0]=3;
 int bodiesBefore=created;SP_PublishDynEntServer();SP_PublishDynEntTopology();
 CHECK(created==bodiesBefore);CHECK(cm.dynEntCount[0]==204&&numExtraDynEnts==2);CHECK(usedExtraDynEnts[4]&&usedExtraDynEnts[200]&&!usedExtraDynEnts[3]);
 CHECK(clients[203].physObjId==0&&poses[203].pose.origin[0]==200);CHECK(SP_DynSpawnMatchesLive(staged[4]));
 SP_PublishDynEntTopology();CHECK(created==bodiesBefore&&numExtraDynEnts==2);SP_PublishDynEntClient();CHECK(created==bodiesBefore+2);CHECK(clients[203].physObjId==77);
 CHECK(encode()==sparse);SP_PublishDynEntClient();CHECK(created==bodiesBefore+2);
 // Replacement preflight is atomic for a now-missing asset/capacity.
 CHECK(decode(sparse));missingModel=true;int linksBefore=clientLinks;bool topologyRejected=false;try{SP_PublishDynEntTopology();}catch(...){topologyRejected=true;}CHECK(topologyRejected&&numExtraDynEnts==2&&clientLinks==linksBefore);missingModel=false;
 // Repeated capture includes current spawned bodies and preserves exact slots.
 CHECK(SP_DynEntCaptureSupported());SP_CaptureDynEnts();auto recaptured=encode();CHECK(decode(recaptured));CHECK(s_savedDynExtraCount==2&&s_savedDynHighWater==204&&s_savedDynUsed[200]);
 // Partial native publication error leaves only initialized slots owned.
 CHECK(decode(recaptured));failLink=1;bool linkRejected=false;try{SP_PublishDynEntTopology();}catch(...){linkRejected=true;}
 CHECK(linkRejected&&numExtraDynEnts==1&&usedExtraDynEnts[4]&&!usedExtraDynEnts[200]);CHECK(clients[7].physObjId==0);
 DynEntCl_SP_CleanupSpawnedDynEnts();CHECK(numExtraDynEnts==0);CHECK(decode(recaptured));SP_PublishDynEntTopology();CHECK(numExtraDynEnts==2);SP_PublishDynEntClient();
 // A retired high slot can leave a nonzero empty extent, which is distinct from count.
 DynEntCl_SP_CleanupSpawnedDynEnts();CHECK(numExtraDynEnts==0);SP_CaptureDynEnts();auto holes=encode();CHECK(decode(holes));CHECK(s_savedDynHighWater==204&&s_savedDynExtraCount==0);

 // DYN2 retains its original wire layout, without a DYN3 timing suffix.
 CHECK(decode(holes));s_savedDynSchema=0x324E5944;auto dyn2=encode();CHECK(decode(dyn2));CHECK(encode()==dyn2&&s_savedDynBurns.empty()&&s_savedDynFades.empty());
 // Real typed borrowers over placed and sparse spawned owner records. Only
 // FX pool and OS/physics services are isolated providers in this fixture.
 CHECK(SP_DynSpawnDefinition(sparseRecords[3],defs[7]));poses[7]=sparseRecords[3].pose;clients[7]=sparseRecords[3].client;
 clients[7].physObjId=0;clients[7].fadeTime=500;usedExtraDynEnts[4]=1;numExtraDynEnts=1;
 clients[0].flags|=8;clients[0].burnTime=700;
 gNumBurningDynEnts=1;gDynEntBurnData[0]={0,0x10007};gNumFadingDynEnts=1;gDynEntFadeData[0]={7,74500};
 FxEffectDef fire{};fire.name="destructibles/fx_dest_fire_trail_sm";
 fixtureBurnEffect.effect.def=&fire;fixtureBurnEffect.effect.uniqueHandle=0x10007;
 fixtureBurnEffect.effect.msecBegin=99000;fixtureBurnEffect.effect.msecLastUpdate=99700;
 fixtureBurnEffect.effect.boltAndSortOrder._type=2;fixtureBurnEffect.effect.boltAndSortOrder.absDynEntId=0;
 CHECK(SP_DynEntCaptureSupported());SP_CaptureDynEnts();auto timed=encode();CHECK(decode(timed));CHECK(encode()==timed);
 CHECK(s_savedDynSchema==0x334E5944&&s_savedDynQueueClock==100000&&s_savedDynBurns[0].fx==0x10007&&s_savedDynFades[0].startTime==74500);
 for(size_t n=timed.size()-24;n<timed.size();++n){auto cut=timed;cut.resize(n);CHECK(!decode(cut));}CHECK(decode(timed));
 auto rb=s_savedDynBurns;auto rf=s_savedDynFades;const auto rr=s_savedDynEnts;
 auto validQueues=[&]{return SP_DynQueuesValid(rr,100000,s_savedDynBurns,s_savedDynFades);};
 s_savedDynBurns.push_back(rb[0]);CHECK(!validQueues());s_savedDynBurns=rb;
 s_savedDynBurns[0].id=8;CHECK(!validQueues());s_savedDynBurns=rb;
 s_savedDynBurns[0].fx=0;CHECK(!validQueues());s_savedDynBurns=rb;
 s_savedDynBurns[0].fx=0x4010001;CHECK(!validQueues());s_savedDynBurns=rb;
 s_savedDynBurns.clear();CHECK(!validQueues());s_savedDynBurns=rb;
 s_savedDynFades.push_back(rf[0]);CHECK(!validQueues());s_savedDynFades=rf;
 s_savedDynFades[0].id=0;CHECK(!validQueues());s_savedDynFades=rf;
 s_savedDynFades[0].startTime=100001;CHECK(!validQueues());s_savedDynFades=rf;
 s_savedDynFades[0].startTime=-1;CHECK(!validQueues());s_savedDynFades=rf;
 auto badTimed=rr[0];badTimed.client.burnTime=30001;CHECK(!SP_DynStateValid(badTimed,true));badTimed=rr[0];badTimed.client.fadeTime=1001;CHECK(!SP_DynStateValid(badTimed,true));
 CHECK(!SP_DynBurnEffectMatches(rb[0],nullptr));fixtureBurnEffect.effect.boltAndSortOrder.absDynEntId=1;CHECK(!SP_DynBurnEffectMatches(rb[0],&fixtureBurnEffect));fixtureBurnEffect.effect.boltAndSortOrder.absDynEntId=0;
 static FxSystemBuffers fxBuffers{};FxSystemShared shared{};
 fxBuffers.uniqueHandleDb.nodes[0].handle=0x10007;fxBuffers.uniqueHandleDb.nodes[0].effect=0;fxBuffers.uniqueHandleDb.nodes[0].nextNode=-1;
 fxBuffers.effects[0]=fixtureBurnEffect;shared.firstFreeEffect=1;shared.allEffectHandles[0]=0;
 CHECK(SP_DynStagedEffectsValid(fxBuffers,shared));fxBuffers.uniqueHandleDb.nodes[0].handle++;CHECK(!SP_DynStagedEffectsValid(fxBuffers,shared));fxBuffers.uniqueHandleDb.nodes[0].handle--;
 shared.firstFreeEffect=0;CHECK(!SP_DynStagedEffectsValid(fxBuffers,shared));shared.firstFreeEffect=1;
 fxBuffers.effects[0].effect.boltAndSortOrder.absDynEntId=7;CHECK(!SP_DynStagedEffectsValid(fxBuffers,shared));fxBuffers.effects[0]=fixtureBurnEffect;
 // Reconnect leases after native owners, without spawning/retriggering FX.
 CHECK(decode(timed));SP_PublishDynEntClient();CHECK(gNumBurningDynEnts==1&&gNumFadingDynEnts==1);
 CHECK(gDynEntBurnData[0].fx==0x10007&&gDynEntFadeData[0].startTime==74500&&clients[7].fadeTime==500&&clients[0].burnTime==700);
 CHECK(encode()==timed);SP_PublishDynEntClient();CHECK(gNumBurningDynEnts==1&&gNumFadingDynEnts==1);
 printf("production DYN1/DYN2/DYN3 checkpoint %i checks PASS; pointer %zu; client %zu; pose %zu bytes\n",checks,sizeof(void*),sizeof(DynEntityClient),sizeof(DynEntityPose));
}
"""
code=code.replace('CURRENTMODEL',extract('src/dynentity/DynEntity_client.cpp','XModel *__cdecl DynEntCl_GetCurrentXModel(')+'\n'+extract('src/dynentity/DynEntity_client.cpp','bool __cdecl DynEntCl_IsHealthGone('))
code=code.replace('VALIDATOR',extract('src/physics/phys_object_checkpoint.inl','bool Phys_IsSavedObjectStateValid(')).replace('PRODUCTION',s)
a.out.mkdir(parents=True,exist_ok=False);(a.out/'fixture.cpp').write_text(code,encoding='utf-8')
paths=[source,'src/dynentity/DynEntity_client.h','src/dynentity/DynEntity_client.cpp','src/physics/phys_object_checkpoint.inl','src/physics/phys_checkpoint.h','src/game_sp/g_sp_client_checkpoint.inl','src/DynEntity/DynEntity_load_obj.cpp']
(a.out/'source-manifest.json').write_text(json.dumps({x:hashlib.sha256((root/x).read_bytes()).hexdigest() for x in paths},indent=2),encoding='utf-8')
