"""DYN5/RPE1 production codec/publication over real checkpoint graph helpers.

Uses the joint fixture's canonical body types, real Narrow32, live pool lists,
and graph validators. Map/DB/render/physics storage services are isolated.
"""
from pathlib import Path
import argparse, subprocess, sys, hashlib, json
root = Path(__file__).resolve().parents[3]
p = argparse.ArgumentParser(); p.add_argument('--out', type=Path, required=True); p.add_argument('--bindings-source', type=Path); a = p.parse_args()
subprocess.run([sys.executable, str(root/'tools/port/tests/physics_joint_checkpoint_regression.py'), '--out', str(a.out)], check=True)
f = a.out/'fixture.cpp'; s = f.read_text()
s = s.replace('void Com_Error(errorParm_t,const char *,...){throw std::runtime_error("rejected");}',
    '#include <cstdarg>\nvoid Com_Error(errorParm_t,const char *fmt,...){char message[1024];va_list args;va_start(args,fmt);vsnprintf(message,sizeof(message),fmt,args);va_end(args);throw std::runtime_error(message);}')
def extract(path, key):
    source = path.read_text(); start = source.index(key); end = source.index('{', start)+1; depth=1
    while depth:
        depth += (source[end]=='{')-(source[end]=='}'); end += 1
    return source[start:end]
providers = r'''
#include <qcommon/cm_load.h>
#include <gfx_d3d/r_init.h>
#include <dynentity/DynEntity_load_obj.h>
#include <dynentity/DynEntity_client.h>
#include <EffectsCore/fx_system.h>
#include <physics/physconstraints_load_obj.h>
#include <clientscript/cscr_stringlist.h>
#include <gfx_d3d/r_material.h>
#include <xanim/dobj.h>
#include <xanim/dobj_utils.h>
#include <qcommon/dobj_management.h>
#include <game/g_mover.h>
#include <cgame_mp/cg_local_mp.h>
#include <cgame_mp/cg_main_mp.h>
clipMap_t cm{};r_global_permanent_t rgp{};
int g_entNumBits=10,g_entNumCscEnd=1536;
static fake_centity_s dynFakePool[512]{};
fake_centity_s *cg_fakeEntitiesArray=dynFakePool;
centity_s *cg_entitiesArray[1]{};
bool cg_fakeEntitiesInuseArray[512]{};
gentity_s g_entities[MAX_GENTITIES_SV]{};
static DObj dynServerDObj{};
DObj *Com_GetServerDObj(unsigned){return &dynServerDObj;}
XModel *DObjGetModel(const DObj *,int){return &model;}
int numExtraDynEnts,gNumFadingDynEnts,gNumBurningDynEnts;
unsigned __int8 usedExtraDynEnts[256]{};
DynEnt_BurnData gDynEntBurnData[16]{};DynEnt_FadeData gDynEntFadeData[32]{};
struct FixtureDynCg {int time=1000;};static FixtureDynCg dynCg;
static FixtureDynCg *DynGlobals(int){return &dynCg;}
#define CG_GetLocalClientGlobals DynGlobals
static FxEffectContainer dynBurnEffect;
FxEffectContainer *UniqueHandleToEffect(int,unsigned handle){return dynBurnEffect.effect.uniqueHandle==handle?&dynBurnEffect:nullptr;}
char FX_NeedsBoltUpdate(const FxEffectDef *){return true;}
void DynEnt_ResetBurning(){gNumBurningDynEnts=0;memset(gDynEntBurnData,0,sizeof(gDynEntBurnData));}
void DynEnt_ResetFading(){gNumFadingDynEnts=0;memset(gDynEntFadeData,0,sizeof(gDynEntFadeData));}
void Sys_EnterCriticalSection(CriticalSection){}
void Sys_LeaveCriticalSection(CriticalSection){}
static Material dynRopeMaterial{},dynFreshRopeMaterial{};static Material *dynLiveRopeMaterial=&dynRopeMaterial;static bool dynMissingRopeMaterial;
XAssetEntryPoolEntry *DB_FindXAssetEntry(XAssetType type,const char *name){static XAssetEntryPoolEntry e{};if(type!=ASSET_TYPE_MATERIAL||dynMissingRopeMaterial||strcmp(name,"rope_material"))return nullptr;e.entry.asset.header.material=dynLiveRopeMaterial;return &e;}
const char *Material_GetName(Material *m){return m->info.name;}
static bool dynDifferentString;
char *SL_ConvertToString(unsigned handle,scriptInstance_t inst){check(handle&&inst==SCRIPTINSTANCE_SERVER);return const_cast<char *>(dynDifferentString?"different_constraint":"fixture_constraint");}
void Com_Printf(int,const char *fmt,...){va_list args;va_start(args,fmt);vprintf(fmt,args);va_end(args);fflush(stdout);}
const DynEntityProps *DynEnt_GetEntityProps(DynEntityType){static DynEntityProps props={"fixture",false,true,true,true};return &props;}
void DynEnt_UnlinkEntity(DynEntityCollType,unsigned short){}
void DynEntSv_LinkModel(unsigned short){}
void DynEntSv_LinkBrush(unsigned short){}
void DynEntCl_UnlinkEntity(unsigned short,DynEntityCollType){}
double XModelGetRadius(const XModel *m){return m->radius;}
CURRENTMODEL
void DynEntCl_LinkModel(unsigned short i){cm.dynEntPoseList[0][i].radius=DynEntCl_GetCurrentXModel(&cm.dynEntDefList[0][i],&cm.dynEntClientList[0][i])->radius;}
void DynEntCl_LinkBrush(unsigned short){}
void DynEntCl_SP_CleanupSpawnedDynEnts(){check(!numExtraDynEnts);}
struct FixtureDynAllocation {PhysObjUserData data;rigid_body body;broad_phase_info info;};
static PhysObjUserData dynTemplateData;static rigid_body dynTemplateBody;static broad_phase_info dynTemplateInfo;
static std::vector<FixtureDynAllocation *> dynAllocations;
static int dynFactoryCount,dynDestroyCount;
int DynEntCl_CreateCheckpointPhysObj(const DynEntityDef *,DynEntityClient *client,const GfxPlacement *){
 auto *a=new FixtureDynAllocation{};a->data=dynTemplateData;a->body=dynTemplateBody;a->info=dynTemplateInfo;
 a->data.body=&a->body;a->body.m_userdata=&a->data;a->body.m_partition_node={};a->data.m_bpb=&a->info;
 a->data.m_next_link=fixtureGlob.objects_by_world[0].m_first;fixtureGlob.objects_by_world[0].m_first=&a->data;
 if(!fixtureGlob.objects_by_world[0].m_last)fixtureGlob.objects_by_world[0].m_last=&a->data;
 dynAllocations.push_back(a);++dynFactoryCount;return client->physObjId=Phys_GetObjId(&a->data);
}
static int dynServerFactoryCount;
void G_CreateCheckpointPhysicsObject(gentity_s *ent){
 DynEntityClient local{};ent->physObjId=DynEntCl_CreateCheckpointPhysObj(nullptr,&local,nullptr);
 dynAllocations.back()->data.id=ent->s.number;++dynServerFactoryCount;
}
void G_CreatePhysicsObject(gentity_s *){check(false);} // No default joints during publication.
void DynEnt_DestroyPhysics(PhysWorld,const DynEntityDef *,DynEntityClient *client){
 auto &list=fixtureGlob.objects_by_world[0];PhysObjUserData *previous=nullptr;
 for(auto *d=list.m_first;d;d=d->m_next_link){if(Phys_GetObjId(d)!=client->physObjId){previous=d;continue;}
 if(previous)previous->m_next_link=d->m_next_link;else list.m_first=d->m_next_link;
 if(list.m_last==d)list.m_last=previous;
 for(auto it=dynAllocations.begin();it!=dynAllocations.end();++it)if(&(*it)->data==d){delete *it;dynAllocations.erase(it);break;}
 client->physObjId=0;++dynDestroyCount;return;}check(false);
}
'''
providers = providers.replace('CURRENTMODEL', extract(root/'src/DynEntity/DynEntity_client.cpp','XModel *__cdecl DynEntCl_GetCurrentXModel(')+'\n'+extract(root/'src/DynEntity/DynEntity_client.cpp','bool __cdecl DynEntCl_IsHealthGone('))
ropeProviders = r'''
#include <physics/rope.h>
#include <physics/rope_checkpoint.h>
rope_t *g_ropes;int g_max_ropes,g_rope_sys_time,g_ropesWithEntsAnchorsCount,g_ropeCount,g_totalRopeCount,g_mapRopeCount,g_num_contacts,g_update_count;
int g_ropesWithEntsAnchors[10]{};
struct {Material *ropeMaterial;} dynRopeMedia;
#define cgMedia dynRopeMedia
const Material *R_Rope_GetMaterialFromGfxWorld(){return dynLiveRopeMaterial;}
unsigned SL_FindString(const char *name,scriptInstance_t inst){check(inst==SCRIPTINSTANCE_SERVER);return !strcmp(name,"fixture_constraint")?2117:0;}
void Com_PrintWarning(int,const char *,...){}
'''
ropeKeys=['bool __cdecl Rope_IsValid(', 'void __cdecl Rope_Reset(int ', 'void __cdecl Rope_SetMaterial(', 'void __cdecl Rope_AddAnchor(', 'void __cdecl Rope_Activate(', 'void __cdecl Rope_AddDynEntAnchor(', 'void __cdecl Rope_AddEntAnchor(', 'void __cdecl Rope_RemoveAnchor(', 'void __cdecl Rope_Create(int ', 'void __cdecl Rope_Init(', 'void __cdecl Rope_AddParticle(']
ropeProviders += extract(root/'src/universal/com_math.cpp','float __cdecl Vec3Length(')+'\n'
ropeProviders += '\n'.join(extract(root/'src/physics/rope.cpp',key) for key in ropeKeys)
ropeProviders += '\n#include <physics/rope_checkpoint.inl>\n'
providers += ropeProviders
for key in ['static bool SP_ReadLiveServerBody(', 'static bool SP_CollectLiveServerBodies(', 'static bool SP_ServerBodiesSupported(', 'static void SP_CaptureServerBodies(', 'static void SP_PublishServerBodies(']:
    providers += '\n'+extract(root/'src/game_sp/g_sp_server_physics_checkpoint.inl', key)
production = (root/'src/game_sp/g_sp_dynent_checkpoint.inl').read_text().split('static int s_dynMotionPauseOwner')[0]
production = production.replace('#include <client/client.h>','').replace('#include <cgame_mp/cg_local_mp.h>','')
production = production.replace('#include "g_sp_dynent_joints_checkpoint.inl"','#include <game_sp/g_sp_dynent_joints_checkpoint.inl>')
if a.bindings_source:
    production = production.replace('#include <game_sp/g_sp_dynent_joints_checkpoint.inl>', '#include "'+str(a.bindings_source.resolve()).replace('\\','/')+'"')
legacy = extract(root/'build-android-audit/zombie-save-20261019/public-ui-source-v129-r2/src/game_sp/g_sp_dynent_checkpoint.inl','static void SP_DynEntCodec(').replace('SP_DynEntCodec(', 'SP_DynLegacyCodec(',1)
legacy4Root=root/'build-android-audit/zombie-save-20261019/rope-v137-before/src/game_sp'
legacy += '\n'+extract(legacy4Root/'g_sp_dynent_joints_checkpoint.inl','static void SP_DynJointBindingsCodec(').replace('SP_DynJointBindingsCodec(', 'SP_DynLegacy4BindingsCodec(',1)
legacy += '\n'+extract(legacy4Root/'g_sp_dynent_checkpoint.inl','static void SP_DynEntCodec(').replace('SP_DynEntCodec(', 'SP_DynLegacy4Codec(',1).replace('SP_DynJointBindingsCodec(file, reading)','SP_DynLegacy4BindingsCodec(file, reading)')
legacy5Root=root/'build-android-audit/zombie-save-20261019/mixed-v139-before/src/game_sp'
legacy += '\n'+extract(legacy5Root/'g_sp_dynent_joints_checkpoint.inl','static void SP_DynJointBindingsCodec(').replace('SP_DynJointBindingsCodec(', 'SP_DynLegacy5BindingsCodec(',1)
legacy += '\n'+extract(legacy5Root/'g_sp_dynent_checkpoint.inl','static void SP_DynEntCodec(').replace('SP_DynEntCodec(', 'SP_DynLegacy5Codec(',1).replace('SP_DynJointBindingsCodec(file, reading)','SP_DynLegacy5BindingsCodec(file, reading)')
helpers=r'''
static std::vector<unsigned char> dynEncode(bool legacy=false){MemoryFile w{};if(legacy){if(s_savedDynSchema==0x354E5944)SP_DynLegacy5Codec(&w,false);else if(s_savedDynSchema==0x344E5944)SP_DynLegacy4Codec(&w,false);else SP_DynLegacyCodec(&w,false);}else SP_DynEntCodec(&w,false);return {w.cacheBuffer,w.cacheBuffer+w.cacheBufferUsed};}
static bool dynDecode(const std::vector<unsigned char> &bytes){MemoryFile r{};r.buffer=const_cast<unsigned char *>(bytes.data());r.bufferSize=(int)bytes.size();try{SP_DynEntCodec(&r,true);check(r.bytesUsed==r.bufferSize);return true;}catch(...){return false;}}
'''
s=s.replace('int main(){', providers+'\n'+production+'\n'+legacy+'\n'+helpers+'\nint main(){',1)
tests=r'''
 // Two map bodies joined to each other and the environment, with an inactive
 // third binding. Actual graph helpers validate all native allocations.
 reset();for(auto &d:data)d.id=-1;s_serverBodySchema=0x33505053;s_serverBodiesStaged=true;
 dynTemplateData=data[0];dynTemplateBody=bodies[0];dynTemplateInfo=infos[0];
 static DynEntityDef dynDefs[2]{};static DynEntityClient dynClients[2]{};static DynEntityPose dynPoses[2]{};static DynEntityServer dynServers[2]{};static DynEntityColl dynColls[2]{};static PhysPreset dynPreset{};static PhysConstraint mapJoints[3]{};
 cm.originalDynEntCount=2;cm.dynEntCount[0]=cm.dynEntCount[2]=2;cm.dynEntDefList[0]=dynDefs;cm.dynEntClientList[0]=dynClients;cm.dynEntPoseList[0]=dynPoses;cm.dynEntServerList[0]=dynServers;cm.dynEntCollList[0]=dynColls;cm.num_constraints=3;cm.constraints=mapJoints;model.radius=16;
 for(int i=0;i<2;++i){auto &d=dynDefs[i];d.type=DYNENT_TYPE_CLUTTER;d.xModel=&model;d.physPreset=&dynPreset;d.pose.quat[3]=1;d.health=100;for(auto &v:d.physConstraints)v=511;dynPoses[i].pose=d.pose;dynPoses[i].radius=16;dynClients[i].flags=3;dynClients[i].health=100-i;dynClients[i].physicsStartTime=500;dynClients[i].physObjId=Phys_GetObjId(&data[i]);dynServers[i].flags=1;dynServers[i].health=100-i;}
 const int nativeIds[]={dynClients[0].physObjId,dynClients[1].physObjId};
 for(int i=0;i<3;++i){auto &c=mapJoints[i];c.type=i==1?CONSTRAINT_DISTANCE:CONSTRAINT_POINT;c.attach_point_type1=ATTACH_POINT_DYNENT;c.target_index1=i==1?1:0;c.attach_point_type2=i==0?ATTACH_POINT_DYNENT:ATTACH_POINT_WORLD;c.target_index2=i==0?1:-1;c.rope_index=-1;c.timeout=2000;c.min_health=3;for(auto &v:c.centity_num)v=-1;}
 for(auto &c:mapJoints){c.targetname=7;c.target_ent1=17;}
 // Canonical DynEnt_SetupConstraints swaps a first environment endpoint to
 // second before calling the native joint factory. Map declaration order and
 // native solver endpoint order therefore differ for this distance joint.
 mapJoints[1].attach_point_type1=ATTACH_POINT_WORLD;mapJoints[1].target_index1=-1;mapJoints[1].target_ent1=0;
 mapJoints[1].attach_point_type2=ATTACH_POINT_DYNENT;mapJoints[1].target_index2=1;mapJoints[1].target_ent2=17;
 dynDefs[0].physConstraints[0]=0;dynDefs[0].physConstraints[1]=2;dynDefs[1].physConstraints[0]=0;dynDefs[1].physConstraints[1]=1;
 PhysSavedConstraintState nativeJoints[2];nativeJoints[0].kind=1;nativeJoints[0].first=0;nativeJoints[0].second=1;nativeJoints[0].values[0]=7;nativeJoints[0].pulses[2]=.375f;
 nativeJoints[1].kind=2;nativeJoints[1].first=1;nativeJoints[1].second=-1;nativeJoints[1].values[1]=nativeJoints[1].values[2]=5;nativeJoints[1].pulses[1]=-.25f;
 for(int i=0;i<2;++i){unsigned h=Phys_CreateSavedConstraint(0,nativeIds,2,nativeJoints[i]);check(h);mapJoints[i].constraintHandle=static_cast<int>(h);}
 // DYN4 keeps the DYN3 timed/FX borrower validation and publication suffix.
 FxEffectDef dynFire{};dynFire.name="destructibles/fx_dest_fire_trail_sm";
 dynBurnEffect.effect.def=&dynFire;dynBurnEffect.effect.uniqueHandle=0x10007;
 dynBurnEffect.effect.msecBegin=300;dynBurnEffect.effect.msecLastUpdate=1000;
 dynBurnEffect.effect.boltAndSortOrder._type=2;dynBurnEffect.effect.boltAndSortOrder.absDynEntId=0;
 dynClients[0].flags|=8;dynClients[0].burnTime=700;gNumBurningDynEnts=1;gDynEntBurnData[0]={0,0x10007};
 check(SP_DynEntCaptureSupported());SP_CaptureDynEnts();check(s_savedDynSchema==0x364E5944&&s_savedDynBindings.size()==3&&s_savedDynBindings[0].active&&!s_savedDynBindings[2].active);
 auto dyn5=dynEncode();check(dynDecode(dyn5));check(dynEncode()==dyn5);
 check(s_savedDynBurns.size()==1&&s_savedDynBurns[0].fx==0x10007&&s_savedDynQueueClock==1000);
 static FxSystemBuffers dynFxBuffers{};FxSystemShared dynFxShared{};
 dynFxBuffers.uniqueHandleDb.nodes[0].handle=0x10007;dynFxBuffers.uniqueHandleDb.nodes[0].effect=0;dynFxBuffers.uniqueHandleDb.nodes[0].nextNode=-1;
 dynFxBuffers.effects[0]=dynBurnEffect;dynFxShared.firstFreeEffect=1;dynFxShared.allEffectHandles[0]=0;
 check(SP_DynStagedEffectsValid(dynFxBuffers,dynFxShared));dynFxBuffers.uniqueHandleDb.nodes[0].handle++;check(!SP_DynStagedEffectsValid(dynFxBuffers,dynFxShared));dynFxBuffers.uniqueHandleDb.nodes[0].handle--;
 // New processes can assign different SL handles to the same target names.
 for(auto &c:mapJoints){c.targetname+=100;if(c.target_ent1)c.target_ent1+=100;if(c.target_ent2)c.target_ent2+=100;}
 check(dynDecode(dyn5));check(dynEncode()==dyn5);
 dynDifferentString=true;check(!dynDecode(dyn5));dynDifferentString=false;check(dynDecode(dyn5));
 // Truncation cannot reach native allocation or map binding mutation.
 const int sourceHandle=mapJoints[0].constraintHandle;
 for(size_t n=0;n<dyn5.size();++n){auto cut=dyn5;cut.resize(n);check(!dynDecode(cut));check(dynFactoryCount==0&&mapJoints[0].constraintHandle==sourceHandle);}
 check(dynDecode(dyn5));mapJoints[0].timeout++;check(!dynDecode(dyn5));mapJoints[0].timeout--;check(dynDecode(dyn5));
 auto savedBindings=s_savedDynBindings;auto savedRecords=s_savedDynEnts;
 s_savedDynBindings[0].joint.second=-1;check(!SP_DynBindingsValid(s_savedDynEnts,s_savedDynBindings));s_savedDynBindings=savedBindings;
 s_savedDynBindings[0].centities[0]=1024;check(!SP_DynBindingsValid(s_savedDynEnts,s_savedDynBindings));s_savedDynBindings=savedBindings;
 mapJoints[2].constraintHandle=1;check(!SP_DynEntCaptureSupported());mapJoints[2].constraintHandle=0;
 mapJoints[2].rope_index=0;check(!SP_DynEntCaptureSupported());mapJoints[2].rope_index=-1;
 // Real production publication retires old bindings before bodies, creates
 // distinct target allocations, restores the graph, then publishes fresh map
 // handles. Native graph/readback checks execute inside the implementation.
 check(dynDecode(dyn5));SP_PublishDynEntServer();SP_PublishDynEntClient();check(dynFactoryCount==2&&dynDestroyCount==2);
 check(dynClients[0].physObjId!=nativeIds[0]&&dynClients[1].physObjId!=nativeIds[1]);
 check(mapJoints[0].constraintHandle&&mapJoints[1].constraintHandle&&!mapJoints[2].constraintHandle);check(dynEncode()==dyn5);
 check(gNumBurningDynEnts==1&&gDynEntBurnData[0].fx==0x10007&&dynClients[0].burnTime==700);
 auto copied=s_savedDynEnts;std::vector<SPDynJointBinding> liveBindings;check(SP_DynCaptureJointGraph(copied,liveBindings));
 check(!memcmp(liveBindings.data(),savedBindings.data(),savedBindings.size()*sizeof(savedBindings[0])));
 for(size_t i=0;i<copied.size();++i)check(!memcmp(&copied[i].body,&savedRecords[i].body,sizeof(copied[i].body)));
 SP_PublishDynEntClient();check(dynFactoryCount==2);
 // Allocation failure after one joint must leave zero new bindings/joints;
 // target body owners remain in the map for ordinary recovery cleanup.
 check(dynDecode(dyn5));fixtureJointFailAfter=1;bool publishRejected=false;
 try{SP_PublishDynEntClient();}catch(...){publishRejected=true;}fixtureJointFailAfter=-1;
 check(publishRejected&&!fixtureJointAllocations&&!mapJoints[0].constraintHandle&&!mapJoints[1].constraintHandle);
 check(dynDecode(dyn5));SP_PublishDynEntClient();check(fixtureJointAllocations==2);
 // Old-world retirement is not another save. An unlisted old joint must still
 // block closed-graph capture, while named map joints can retire safely and
 // canonical body destruction later retires the remaining old joint.
 const int retiringIds[]={dynClients[0].physObjId,dynClients[1].physObjId};
 PhysSavedConstraintState oldExtra{};oldExtra.kind=1;oldExtra.first=0;oldExtra.second=1;
 const unsigned unlistedOldJoint=Phys_CreateSavedConstraint(0,retiringIds,2,oldExtra);check(unlistedOldJoint);
 PhysCheckpointJointRef retiringRefs[]={{1,static_cast<unsigned>(mapJoints[0].constraintHandle)},{2,static_cast<unsigned>(mapJoints[1].constraintHandle)}};
 PhysSavedObjectState notSaveable[2]{};PhysSavedConstraintState retiringStates[2]{};
 check(!Phys_CaptureObjectGraph(0,retiringIds,2,retiringRefs,2,notSaveable,retiringStates));
 check(Phys_CaptureRetiringJoints(0,retiringIds,2,retiringRefs,2,retiringStates));
 const auto heldRetiring=std::vector<PhysSavedConstraintState>(retiringStates,retiringStates+2);
 const int duplicatedBodies[]={retiringIds[0],retiringIds[0]};check(!Phys_CaptureRetiringJoints(0,duplicatedBodies,2,retiringRefs,2,retiringStates));
 PhysCheckpointJointRef badRetiringRefs[]={{1,retiringRefs[0].handle},{1,retiringRefs[0].handle}};
 check(!Phys_CaptureRetiringJoints(0,retiringIds,2,badRetiringRefs,2,retiringStates));
 badRetiringRefs[1]={2,1};check(!Phys_CaptureRetiringJoints(0,retiringIds,2,badRetiringRefs,2,retiringStates));
 check(!Phys_CaptureRetiringJoints(0,retiringIds,1,retiringRefs,2,retiringStates));
 check(!memcmp(retiringStates,heldRetiring.data(),sizeof(retiringStates)));
 check(Phys_CaptureRetiringJoints(0,retiringIds,2,nullptr,0,nullptr));
 SP_RetireDynJointBindings();check(fixtureJointAllocations==1);PhysSavedConstraintState remainingOld{};
 check(Phys_CaptureConstraintState(0,1,unlistedOldJoint,retiringIds,2,remainingOld));
 // This fixture isolates body storage. The unlisted joint stayed with its old
 // owner; retire it through the canonical joint API before releasing storage.
 check(Phys_DestroyCheckpointConstraint(1,unlistedOldJoint));
for(int i=0;i<2;++i)DynEnt_DestroyPhysics(PHYS_WORLD_DYNENT,&dynDefs[i],&dynClients[i]);check(dynAllocations.empty()&&!fixtureJointAllocations);
 // Two logical rope slots with real native factories, a dynamic anchor,
 // entity/bone anchor and an active map-rope native distance constraint.
 DynEnt_ResetBurning();dynClients[0].flags&=~8;dynClients[0].burnTime=0;
 static rope_t dynRopeStorage[4]{};cm.max_ropes=g_max_ropes=4;cm.ropes=g_ropes=dynRopeStorage;
 g_ropeCount=g_mapRopeCount=g_totalRopeCount=2;g_rope_sys_time=975;
 dynRopeMaterial.info.name=dynFreshRopeMaterial.info.name="rope_material";dynRopeMedia.ropeMaterial=&dynRopeMaterial;
 const float ropeStart[3]={0,0,0},ropeEnd[3]={100,0,0};
 for(int slot=0;slot<2;++slot){Rope_Create(slot);Rope_Init(slot,4,ropeStart,ropeEnd,100,2,1,1,1);for(int k=0;k<3;++k){dynRopeStorage[slot].m_min[k]=-100;dynRopeStorage[slot].m_max[k]=100;}}
 Rope_AddDynEntAnchor(0,0,ropeStart,0);Rope_AddAnchor(0,3,ropeEnd,-1,-1,ROPE_WORLD_CONSTRAINT);Rope_AddEntAnchor(0,2,ropeStart,7,7);
 Rope_AddAnchor(1,0,ropeStart,-1,-1,ROPE_WORLD_CONSTRAINT);Rope_AddAnchor(1,3,ropeEnd,-1,-1,ROPE_WORLD_CONSTRAINT);
 for(int i=0;i<2;++i){mapJoints[i].type=CONSTRAINT_ROPE;mapJoints[i].rope_index=i;mapJoints[i].attach_point_type1=i?ATTACH_POINT_WORLD:ATTACH_POINT_DYNENT;mapJoints[i].target_index1=i?-1:0;mapJoints[i].attach_point_type2=ATTACH_POINT_WORLD;mapJoints[i].target_index2=-1;}
 for(auto &d:dynDefs)for(auto &v:d.physConstraints)v=511;dynDefs[0].physConstraints[0]=0;
 for(int i=0;i<2;++i)DynEntCl_CreateCheckpointPhysObj(&dynDefs[i],&dynClients[i],&dynPoses[i].pose);
 const int ropeNativeIds[]={dynClients[0].physObjId,dynClients[1].physObjId};auto ropeJoint=nativeJoints[1];ropeJoint.first=0;
 mapJoints[0].constraintHandle=static_cast<int>(Phys_CreateSavedConstraint(0,ropeNativeIds,2,ropeJoint));check(mapJoints[0].constraintHandle);
 check(SP_DynEntCaptureSupported());SP_CaptureDynEnts();auto ropeWire=dynEncode();check(dynDecode(ropeWire)&&dynEncode()==ropeWire);
 check(s_savedDynBindings[0].rope==0&&s_savedDynBindings[1].rope==1&&s_savedDynBindings[0].joint.kind==2);check(s_savedDynRopes.entityRopes==std::vector<int>{0});
 auto ropesBefore=s_savedDynRopes;auto ropeBindings=s_savedDynBindings;
 s_savedDynBindings[1].rope=0;check(!SP_DynBindingsValid(s_savedDynEnts,s_savedDynBindings,&s_savedDynRopes));s_savedDynBindings=ropeBindings;
 s_savedDynRopes.ropes[0].constraint[3].entity=99;check(!SP_DynBindingsValid(s_savedDynEnts,s_savedDynBindings,&s_savedDynRopes));s_savedDynRopes=ropesBefore;
 const int destroyBefore=dynDestroyCount,factoryBefore=dynFactoryCount,oldRopeHandle=mapJoints[0].constraintHandle;
 for(size_t n=0;n<ropeWire.size();++n){auto cut=ropeWire;cut.resize(n);check(!dynDecode(cut));check(dynDestroyCount==destroyBefore&&dynFactoryCount==factoryBefore&&mapJoints[0].constraintHandle==oldRopeHandle);}
 check(dynDecode(ropeWire));dynMissingRopeMaterial=true;bool resourceRejected=false;try{SP_PublishDynEntClient();}catch(...){resourceRejected=true;}dynMissingRopeMaterial=false;
 check(resourceRejected&&dynDestroyCount==destroyBefore&&dynFactoryCount==factoryBefore&&mapJoints[0].constraintHandle==oldRopeHandle);
 check(dynDecode(ropeWire));
 // Real CLIENT teardown clears visual high-water storage while the map still
 // owns its native distance joint and rope slot bindings. Those joints must
 // retire through the native graph, not through another visual save capture.
 g_ropeCount=0;g_ropesWithEntsAnchorsCount=0;
 RopeSavedSystem retiredVisual;check(Rope_CaptureCheckpoint(retiredVisual));check(!SP_DynBindingsValid(s_savedDynEnts,s_savedDynBindings,&retiredVisual));
 dynLiveRopeMaterial=&dynFreshRopeMaterial;SP_PublishDynEntClient();
 check(dynRopeStorage[0].m_material==&dynFreshRopeMaterial&&dynRopeStorage[0].m_constraints[5].bone_name_hash==2117);
 check(s_savedDynRopes==ropesBefore&&dynEncode()==ropeWire);RopeSavedSystem ropeReadback;check(Rope_CaptureCheckpoint(ropeReadback)&&ropeReadback==ropesBefore);
 check(mapJoints[0].constraintHandle&&mapJoints[0].rope_index==0&&mapJoints[1].rope_index==1);
 SP_RetireDynJointBindings();for(int i=0;i<2;++i)DynEnt_DestroyPhysics(PHYS_WORLD_DYNENT,&dynDefs[i],&dynClients[i]);check(!fixtureJointAllocations&&dynAllocations.empty());
 cm.max_ropes=g_max_ropes=g_ropeCount=g_mapRopeCount=g_totalRopeCount=g_rope_sys_time=g_ropesWithEntsAnchorsCount=0;cm.ropes=g_ropes=nullptr;
 // Empty cohorts and legacy layouts: compare current encoders directly with
 // frozen production v129 DYN1/2/3, including their exact timing suffix rules.
 cm.num_constraints=0;for(auto &d:dynDefs)for(auto &v:d.physConstraints)v=511;
 DynEnt_ResetBurning();dynClients[0].flags&=~8;dynClients[0].burnTime=0;
 SP_CaptureDynEnts();auto emptyDyn4=dynEncode();check(dynDecode(emptyDyn4));SP_PublishDynEntClient();
 s_serverBodySchema=0x32505053;
 for(int magic:{0x314E5944,0x324E5944,0x334E5944,0x344E5944,0x354E5944}){s_savedDynSchema=magic;auto wire=dynEncode();check(wire==dynEncode(true));check(dynDecode(wire));check(dynEncode()==wire);}
 s_savedDynSchema=0x334E5944;s_savedDynEnts[0].client.flags|=8;s_savedDynEnts[0].client.burnTime=700;s_savedDynBurns={{0,0x10007}};
 auto legacyTimed=dynEncode();check(legacyTimed==dynEncode(true));check(dynDecode(legacyTimed));check(dynEncode()==legacyTimed);
 // Unified SPP3/DYN6 registry: real server/world and server/DynEnt native
 // joints, fresh entity body ownership, separate map slots and visual ropes.
 // The cross-registry joint is deliberately allocated to test closure; this
 // fixture is not a claim that a retail map factory naturally creates it.
 DynEnt_ResetBurning();dynClients[0].flags&=~8;dynClients[0].burnTime=0;
 cm.num_constraints=3;s_serverBodies.clear();s_serverBodiesPublished=false;dynServerDObj.numModels=1;
 for(auto &d:dynDefs)for(auto &v:d.physConstraints)v=511;
 dynDefs[0].physConstraints[0]=0;dynDefs[1].physConstraints[0]=2;
 for(int i=0;i<2;++i){dynClients[i].flags=3;DynEntCl_CreateCheckpointPhysObj(&dynDefs[i],&dynClients[i],&dynPoses[i].pose);
  auto &e=g_entities[756+i];e={};e.r.inuse=1;e.s.number=756+i;e.useCount=24+i;e.model=12;e.s.eType=ET_SCRIPTMOVER;e.classname=scr_const.script_model;e.targetname=17+i;G_CreateCheckpointPhysicsObject(&e);}
 for(auto &c:mapJoints){c={};c.rope_index=-1;for(auto &n:c.centity_num)n=-1;c.attach_point_type2=ATTACH_POINT_WORLD;}
 mapJoints[0].type=CONSTRAINT_POINT;mapJoints[0].attach_point_type1=ATTACH_POINT_DYNENT;mapJoints[0].target_index1=0;
 mapJoints[1].type=CONSTRAINT_ROPE;mapJoints[1].attach_point_type1=ATTACH_POINT_ENT;mapJoints[1].target_index1=756;mapJoints[1].target_ent1=17;mapJoints[1].rope_index=0;
 mapJoints[2].type=CONSTRAINT_DISTANCE;mapJoints[2].attach_point_type1=ATTACH_POINT_DYNENT;mapJoints[2].target_index1=1;mapJoints[2].attach_point_type2=ATTACH_POINT_ENT;mapJoints[2].target_index2=757;mapJoints[2].target_ent2=18;
 cm.max_ropes=g_max_ropes=4;cm.ropes=g_ropes=dynRopeStorage;g_mapRopeCount=g_totalRopeCount=g_ropeCount=1;g_rope_sys_time=975;
 Rope_Create(0);Rope_Init(0,4,ropeStart,ropeEnd,100,2,1,1,1);for(int k=0;k<3;++k){dynRopeStorage[0].m_min[k]=-100;dynRopeStorage[0].m_max[k]=100;}
 Rope_AddEntAnchor(0,0,ropeStart,756,0);Rope_AddAnchor(0,3,ropeEnd,-1,-1,ROPE_WORLD_CONSTRAINT);
 const int mixedIds[]={dynClients[0].physObjId,dynClients[1].physObjId,g_entities[756].physObjId,g_entities[757].physObjId};
 PhysSavedConstraintState mixedJoints[3]{};
 for(int i=0;i<3;++i){auto &j=mixedJoints[i];j.kind=i?CONSTRAINT_DISTANCE:CONSTRAINT_POINT;j.first=i==1?2:i==2?1:0;j.second=i==2?3:-1;if(i)j.values[1]=j.values[2]=100;
  mapJoints[i].constraintHandle=static_cast<int>(Phys_CreateSavedConstraint(0,mixedIds,4,j));check(mapJoints[i].constraintHandle);}
 check(!SP_ServerBodiesSupported());check(SP_ServerBodiesSupported(true));SP_CaptureServerBodies(true);SP_CaptureDynEnts();
 check(s_serverBodies.size()==2&&s_savedDynBindings[1].server[0]==0&&s_savedDynBindings[2].server[1]==1);
 auto mixedEncode=[&](){MemoryFile w{};SP_WriteServerBodies(&w);SP_DynEntCodec(&w,false);return std::vector<unsigned char>(w.cacheBuffer,w.cacheBuffer+w.cacheBufferUsed);};
 auto mixedDecode=[&](const std::vector<unsigned char>&bytes){MemoryFile r{};r.buffer=const_cast<unsigned char*>(bytes.data());r.bufferSize=(int)bytes.size();try{SP_ReadServerBodies(&r);SP_DynEntCodec(&r,true);return r.bytesUsed==r.bufferSize;}catch(...){return false;}};
 auto mixedWire=mixedEncode();check(mixedDecode(mixedWire)&&mixedEncode()==mixedWire);
 const int mixedAllocationCount=dynFactoryCount;const int mixedHandle=mapJoints[1].constraintHandle;
 for(size_t n=0;n<mixedWire.size();++n){auto cut=mixedWire;cut.resize(n);check(!mixedDecode(cut));check(dynFactoryCount==mixedAllocationCount&&mapJoints[1].constraintHandle==mixedHandle);}
 check(mixedDecode(mixedWire));auto mixedBindings=s_savedDynBindings;
 s_savedDynBindings[1].server[0]=2;check(!SP_DynBindingsValid(s_savedDynEnts,s_savedDynBindings,&s_savedDynRopes,false,&s_serverBodies));s_savedDynBindings=mixedBindings;
 ++g_entities[756].useCount;bool badGeneration=false;try{SP_PublishDynEntClient();}catch(...){badGeneration=true;}--g_entities[756].useCount;
 check(badGeneration&&mapJoints[1].constraintHandle==mixedHandle&&dynFactoryCount==mixedAllocationCount);
 ++g_entities[756].targetname;check(!SP_MixedServerTargetsValid(s_savedDynBindings,s_serverBodies));--g_entities[756].targetname;
 // Retire before canonical ShutdownRopes can discard handles or old server
 // entity teardown can destroy their joints. Simulate only those outer services.
 SP_RetireDynJointBindings();check(!fixtureJointAllocations);
 for(int i=0;i<2;++i){DynEntityClient local{};local.physObjId=g_entities[756+i].physObjId;DynEnt_DestroyPhysics(PHYS_WORLD_DYNENT,nullptr,&local);g_entities[756+i].physObjId=0;}
 g_ropeCount=g_ropesWithEntsAnchorsCount=0;mapJoints[1].target_index1=999;mapJoints[2].target_index2=998;
 const int serverFactoriesBefore=dynServerFactoryCount;
 SP_PublishDynEntClient();check(dynServerFactoryCount==serverFactoriesBefore+2&&s_serverBodiesPublished);
 check(g_entities[756].physObjId!=mixedIds[2]&&g_entities[757].physObjId!=mixedIds[3]);
 check(mapJoints[1].target_index1==756&&mapJoints[2].target_index2==757&&mixedEncode()==mixedWire);
 auto mixedRecords=s_savedDynEnts;auto mixedServers=s_serverBodies;std::vector<SPDynJointBinding> mixedReadback;
 check(SP_DynCaptureJointGraph(mixedRecords,mixedReadback,&ropeReadback,false,&mixedServers));
 check(!memcmp(mixedReadback.data(),mixedBindings.data(),mixedBindings.size()*sizeof(mixedBindings[0])));
 SP_RetireDynJointBindings();for(int i=0;i<2;++i){DynEntityClient local{};local.physObjId=g_entities[756+i].physObjId;DynEnt_DestroyPhysics(PHYS_WORLD_DYNENT,nullptr,&local);g_entities[756+i]={};DynEnt_DestroyPhysics(PHYS_WORLD_DYNENT,&dynDefs[i],&dynClients[i]);}
 check(dynAllocations.empty()&&!fixtureJointAllocations);
 printf("production DYN6/SPP3/RPE1 mixed native registry codec/publication, rollback and frozen DYN1/2/3/4 comparison PASS; ");
'''
marker=' std::printf("production server body checkpoint'
assert s.count(marker)==1;s=s.replace(marker,tests+marker,1);f.write_text(s,encoding='utf-8')
m=json.loads((a.out/'source-manifest.json').read_text())
for name in ['src/game_sp/g_sp_dynent_checkpoint.inl','src/game_sp/g_sp_dynent_joints_checkpoint.inl','src/game_sp/g_sp_client_checkpoint.inl','src/game_sp/g_sp_save_animation.inl','src/physics/rope_checkpoint.h','src/physics/rope_checkpoint.inl','src/physics/rope.cpp','src/physics/rope.h','tools/port/tests/dynent_joint_checkpoint_regression.py']:
    m.append({'file':name,'sha256':hashlib.sha256((root/name).read_bytes()).hexdigest()})
if a.bindings_source:
    m.append({'file':str(a.bindings_source.resolve()),'sha256':hashlib.sha256(a.bindings_source.read_bytes()).hexdigest()})
(a.out/'source-manifest.json').write_text(json.dumps(m,indent=2))
