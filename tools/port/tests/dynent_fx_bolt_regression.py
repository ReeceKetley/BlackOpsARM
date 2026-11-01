"""Exercise actual FX spawn packing/update code with isolated owner providers."""
from pathlib import Path
import argparse, hashlib, json

root = Path(__file__).resolve().parents[3]
p = argparse.ArgumentParser()
p.add_argument('--out', type=Path, required=True)
p.add_argument('--source-dir', type=Path)
a = p.parse_args()
source_dir = a.source_dir or root / 'src/EffectsCore'
spawn = (source_dir / 'fx_system.cpp').read_text()
update = (source_dir / 'fx_update.cpp').read_text()
start = spawn.index('    FxBoltAndSortOrder &bolt = remoteEffect->effect.boltAndSortOrder;')
packing = spawn[start:spawn.index('#endif', start)]
start = update.index('void FX_UpdateEffectBolt(')
end = update.index('{', start) + 1
depth = 1
while depth:
    depth += (update[end] == '{') - (update[end] == '}')
    end += 1
updating = update[start:end]
start = spawn.index('unsigned int __cdecl FX_PlayBoltedEffect_DynEnt(')
end = spawn.index('{', start) + 1
depth = 1
while depth:
    depth += (spawn[end] == '{') - (spawn[end] == '}')
    end += 1
factory = spawn[start:end]

code = r'''
#include <EffectsCore/fx_system.h>
#include <EffectsCore/fx_update.h>
#include <dynentity/DynEntity_load_obj.h>
#include <dynentity/DynEntity_client.h>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#if defined(_WIN32)
// Windows canonical headers initialize unrelated wire-field lookup tables.
// This isolated FX fixture does not run transport or inspect these providers.
#define UNUSED_NETFIELDS(n) decltype(n) n{};
UNUSED_NETFIELDS(eventEntityStateFields) UNUSED_NETFIELDS(actorStateFields)
UNUSED_NETFIELDS(vehicleEntityStateFields) UNUSED_NETFIELDS(planeStateFields)
UNUSED_NETFIELDS(helicopterEntityStateFields) UNUSED_NETFIELDS(turretEntityStateFields)
UNUSED_NETFIELDS(loopFxEntityStateFields) UNUSED_NETFIELDS(fxStateFields)
UNUSED_NETFIELDS(soundBlendEntityStateFields) UNUSED_NETFIELDS(scriptMoverStateFields)
UNUSED_NETFIELDS(missileEntityStateFields) UNUSED_NETFIELDS(itemEntityStateFields)
UNUSED_NETFIELDS(corpseEntityStateFields) UNUSED_NETFIELDS(playerEntityStateFields)
UNUSED_NETFIELDS(archivedEntityFields) UNUSED_NETFIELDS(entityStateFields)
UNUSED_NETFIELDS(clientStateFields) UNUSED_NETFIELDS(playerStateFields)
UNUSED_NETFIELDS(objectiveFields) UNUSED_NETFIELDS(matchStateFields)
#undef UNUSED_NETFIELDS
#endif
static unsigned checks, seenId, wantedId, kills, stops, bones, temporalCalls;
static bool present=true, temporal=true, bonePresent=true, killImmediate=true;
static DynEntityPose ownerPose{};
#define CHECK(x) do {++checks;if(!(x)){fprintf(stderr,"check %u failed line %i: %s\n",checks,__LINE__,#x);abort();}}while(0)
#undef iassert
#define iassert(x) CHECK(x)
bool DynEnt_Valid(unsigned short id){seenId=id;return present && id==wantedId;}
const DynEntityPose *DynEnt_GetClientPose(unsigned short id){CHECK(id==wantedId);seenId=id;return &ownerPose;}
static bool FixtureTemporal(int,unsigned){++temporalCalls;return temporal;}
static char FixtureBone(int,unsigned,int,orientation_t *out){++bones;*out={};out->origin[0]=77;return bonePresent;}
static void FixtureKill(FxSystem *,FxEffectContainer *){++kills;}
static void FixtureStop(FxSystem *,FxEffectContainer *){++stops;}
static bool FixtureVisuals(){return false;}
static char FixtureImmediate(const FxEffectDef *){return killImmediate;}
static void FixtureConcat(const orientation_t *,const orientation_t *b,orientation_t *out){*out=*b;}
static void FixtureQuat(const float (*)[3],float *out){out[0]=out[1]=out[2]=0;out[3]=1;}
static void FixturePrint(int,const char *,...){}
#define FX_GetBoltTemporalBits FixtureTemporal
#define FX_GetBoneOrientation FixtureBone
#define FX_KillEffect FixtureKill
#define FX_StopEffect FixtureStop
#define FX_CanKillImmediate FixtureImmediate
#define CG_SP_VisualsEnabled FixtureVisuals
#define OrientationConcatenate FixtureConcat
#define AxisToQuat FixtureQuat
#define Com_Printf FixturePrint
static void Pack(FxSystem *system,FxEffectContainer *remoteEffect,int dobjHandle,int boneIndex,int markEntnum,int runnerSortOrder){
PACKING
}
UPDATING

static FxSystemContainer factorySystem{};static FxEffectContainer factoryEffect{};
static bool factoryNeedsBolt=true;static unsigned factorySpawns,factoryReleases;
struct FixtureEnable {struct {bool enabled=true;}current;};static FixtureEnable factoryEnable;static FixtureEnable *fixtureEnable=&factoryEnable;
static orientation_t fixtureIdentity{};
static FxSystemContainer *FixtureSystem(int){return &factorySystem;}
static char FixtureNeedsBolt(const FxEffectDef *){return factoryNeedsBolt;}
static void FixtureAxis(const float *,float (*)[3]){}
static unsigned FixtureSpawn(FxSystem *system,const FxEffectDef *def,int,const float *,const float (*)[3],unsigned dobj,int bone,int sort,unsigned short,unsigned mark,const orientation_t *){
 ++factorySpawns;factoryEffect={};factoryEffect.effect.def=def;Pack(system,&factoryEffect,dobj,bone,mark,sort);return 0x10001;
}
static FxEffectContainer *FixtureUnique(FxSystem *,unsigned h){return h==0x10001?&factoryEffect:nullptr;}
static void FixtureDelRef(FxSystem *,FxEffectContainer *,int){++factoryReleases;}
#define fx_enable fixtureEnable
#define orIdentity fixtureIdentity
#define FX_NeedsBoltUpdate FixtureNeedsBolt
#define FX_GetSystem FixtureSystem
#define UnitQuatToAxis FixtureAxis
#define FX_SpawnEffect FixtureSpawn
#define UniqueHandleToEffect FixtureUnique
#define FX_DelRefToEffect FixtureDelRef
FACTORY
static unsigned Raw(const FxBoltAndSortOrder &bolt){unsigned raw;memcpy(&raw,&bolt,4);return raw;}
int main(int argc,char **argv){
 const bool packing=argc==1||!strcmp(argv[1],"pack");
 const bool updating=argc==1||!strcmp(argv[1],"update");
 const bool factory=argc==1||!strcmp(argv[1],"factory");
 FxSystem system{};FxEffectContainer fx{};FxEffectDef def{};def.name="fixture";
 if(packing){
  // All21 owner bits, including IDs beyond the 11-bit DObj range. 511 is
  // the existing factory's no-bone sentinel and cannot encode a dynamic bolt.
  for(int id=0;id<0x200000;++id){if(id==511)continue;
   fx={};Pack(&system,&fx,2047,id,1023,0xa7);
   CHECK(Raw(fx.effect.boltAndSortOrder)==(0x400000a7u|(unsigned(id)<<8)));
   CHECK(fx.effect.boltAndSortOrder.type==2&&fx.effect.boltAndSortOrder.absDynEntId==unsigned(id));
  }
  for(int id:{0,1,2046,2047})for(int bone:{0,255,256,511})for(bool t:{false,true}){
   temporal=t;fx={};Pack(&system,&fx,id,bone,1023,27);
   CHECK(fx.effect.boltAndSortOrder.type==unsigned(id==2047&&bone!=511?2:id==2047?0:1));
   if(id!=2047||bone==511){CHECK(fx.effect.boltAndSortOrder.dobjHandle==unsigned(id));CHECK(fx.effect.boltAndSortOrder.boneIndex==unsigned(bone));CHECK(fx.effect.boltAndSortOrder.temporalBits==t);}
  }
  for(int mark:{0,1022,2046}){fx={};Pack(&system,&fx,2047,511,mark,99);CHECK(fx.effect.boltAndSortOrder.type==3&&fx.effect.boltAndSortOrder.dobjHandle==unsigned(mark)&&fx.effect.boltAndSortOrder.sortOrder==99);}
 }
 if(updating){
  // Independently construct a valid type2 bolt so the old update truncation
  // fails even when spawn packing is separately broken.
  for(unsigned id=2048;id<65535;++id){
   wantedId=id;seenId=~0u;fx={};fx.effect.def=&def;
   fx.effect.boltAndSortOrder.absDynEntId=id;fx.effect.boltAndSortOrder._type=2;
   ownerPose.pose.origin[0]=float(id);ownerPose.pose.origin[1]=-float(id);ownerPose.pose.quat[3]=1;
   unsigned beforeBones=bones,beforeKills=kills,beforeTemporal=temporalCalls;
   FX_UpdateEffectBolt(&system,&fx.effect,&fx);
   CHECK(seenId==id&&kills==beforeKills&&bones==beforeBones&&temporalCalls==beforeTemporal);
   CHECK(!memcmp(fx.effect.frameNow.origin,ownerPose.pose.origin,12)&&!memcmp(fx.effect.frameNow.quat,ownerPose.pose.quat,16));
   present=false;FX_UpdateEffectBolt(&system,&fx.effect,&fx);CHECK(kills==beforeKills+1&&seenId==id);present=true;
  }
  // Keep the normal DObj branch, generation rejection, detach/stop/kill and
  // no-bone/unbound branches under the same actual production update function.
  for(bool canKill:{false,true}){
   wantedId=17;temporal=true;bonePresent=true;fx={};fx.effect.def=&def;Pack(&system,&fx,17,256,1023,1);
   FX_UpdateEffectBolt(&system,&fx.effect,&fx);CHECK(fx.effect.frameNow.origin[0]==77&&fx.effect.frameNow.quat[3]==1);
   unsigned oldKills=kills,oldStops=stops;temporal=false;killImmediate=canKill;
   FX_UpdateEffectBolt(&system,&fx.effect,&fx);CHECK(kills==oldKills+canKill&&stops==oldStops+!canKill);
   CHECK(fx.effect.boltAndSortOrder.type==0&&fx.effect.boltAndSortOrder.rewindKill&&fx.effect.boltAndSortOrder.boneIndex==511&&fx.effect.boltAndSortOrder.dobjHandle==2047);
  }
  fx={};unsigned before=bones+kills+stops;FX_UpdateEffectBolt(&system,&fx.effect,&fx);CHECK(bones+kills+stops==before);
 }

 if(factory){
  for(int id:{0,510,511,512,2048,65534})for(bool needs:{false,true}){
   wantedId=id;present=true;factoryNeedsBolt=needs;factoryEnable.current.enabled=true;
   unsigned before=factorySpawns,releases=factoryReleases;
   CHECK(FX_PlayBoltedEffect_DynEnt(0,&def,1000,id)==0x10001);
   CHECK(factorySpawns==before+1&&factoryReleases==releases+1);
   CHECK(factoryEffect.effect.boltAndSortOrder.type==unsigned(needs?2:0));
   if(needs)CHECK(factoryEffect.effect.boltAndSortOrder.absDynEntId==unsigned(id));
   CHECK((factoryEffect.atomics.status&0x8000)!=0);
  }
  unsigned before=factorySpawns;wantedId=0;
  for(int invalid:{-1,65535,65536,0x200000})CHECK(!FX_PlayBoltedEffect_DynEnt(0,&def,1000,invalid));
  CHECK(factorySpawns==before);present=false;CHECK(!FX_PlayBoltedEffect_DynEnt(0,&def,1000,0));present=true;
  factoryEnable.current.enabled=false;CHECK(!FX_PlayBoltedEffect_DynEnt(0,&def,1000,0));CHECK(factorySpawns==before);
 }
 printf("actual dynamic FX spawn packing and owner update %u checks PASS; pointer %zu\n",checks,sizeof(void*));
}
'''.replace('PACKING', packing).replace('UPDATING', updating).replace('FACTORY', factory)
a.out.mkdir(parents=True, exist_ok=False)
(a.out / 'fixture.cpp').write_text(code, encoding='utf-8')
paths = [source_dir / 'fx_system.cpp', source_dir / 'fx_update.cpp', root / 'src/gfx_d3d/fxprimitives.h']
(a.out / 'source-manifest.json').write_text(json.dumps({str(x): hashlib.sha256(x.read_bytes()).hexdigest() for x in paths}, indent=2))
