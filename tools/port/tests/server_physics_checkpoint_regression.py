"""Production independent-body helper/registry tests, using canonical types.

World-list storage, clock, memfile transport and DB lookup are isolated fixture
services. Narrow32 is the real ARM64 provider. Not evidence of a live model
factory or in-game nonzero body restoration.
"""
from pathlib import Path
import argparse, hashlib, json
root = Path(__file__).resolve().parents[3]
p = argparse.ArgumentParser(); p.add_argument('--out', type=Path, required=True); a = p.parse_args()
registry = 'src/game_sp/g_sp_server_physics_checkpoint.inl'
def function(signature):
    text = (root/registry).read_text(encoding='utf-8'); start = text.index(signature); end = text.index('{',start)+1; depth=1
    while depth:
        depth += (text[end]=='{')-(text[end]=='}'); end+=1
    return text[start:end]
code=r'''
#include <physics/phys_main.h>
#include <physics/phys_checkpoint.h>
#include <universal/memfile.h>
#include <database/db_registry.h>
#include <xanim/xmodel_utils.h>
#include <bgame/bg_public.h>
#include <bgame/bg_misc.h>
#include <bgame/bg_local.h>
#include <clientscript/scr_const.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>
#include <string>
static int checks;
static void checkAt(bool value,int line){if(!value){std::fprintf(stderr,"check %i failed at line %i\n",checks,line);std::abort();}++checks;}
#define check(value) checkAt(value,__LINE__)
bool Assert_MyHandler(const char *,int,int,const char *,...){std::abort();}
char _tlAssert(const char *,int,const char *,const char *){std::abort();}
void Com_Error(errorParm_t,const char *,...){throw std::runtime_error("rejected");}
static struct {decltype(PhysGlob::objects_by_world) objects_by_world{};int timeLastUpdate=1000,timeLastSnapshot=950;} fixtureGlob;
#include <physics/physics_system_internal.h>
// Fixture-only environment initialization; no solver step runs in this test.
void environment_rigid_body::set() {}
static physics_system fixturePhysics;
physics_system *g_physics_system=&fixturePhysics;
#define physGlob fixtureGlob
#include <physics/phys_object_checkpoint.inl>
#undef physGlob
scr_const_t scr_const{};
struct SPServerBodyRecord {int number=0,generation=0,model=0;std::string modelName;PhysSavedObjectState state{};};
static std::vector<SPServerBodyRecord> s_serverBodies;
static bool s_serverBodiesStaged,s_serverBodyEntitiesSeen[MAX_GENTITIES];
static int s_serverBodyEntityError;
static int s_serverBodySchema=0x32505053;
static PhysSavedClockState s_serverPhysicsClock{1000,950};
static bool s_serverBodiesPublished;
static XModel model{};
XAssetHeader DB_FindXAssetHeader(XAssetType, char *name, bool, int){XAssetHeader result{};result.model=&model;return result;}
const char *XModelGetName(const XModel *m){return m->name;}
void MemFile_ReadData(MemoryFile *f,int n,unsigned char *out){if(n<0||n>f->bufferSize-f->bytesUsed)Com_Error(ERR_DROP,"truncated");memcpy(out,f->buffer+f->bytesUsed,n);f->bytesUsed+=n;}
void MemFile_WriteDataInternal(MemoryFile *,unsigned,unsigned char *){Com_Error(ERR_DROP,"unexpected fixture flush");}
void MemFile_WriteInt(MemoryFile *f,int n){MemFile_WriteData(f,4,(unsigned char *)&n);}
char *MemFile_ReadCString(MemoryFile *f){static char s[256];for(int i=0;i<256;++i){MemFile_ReadData(f,1,(unsigned char *)&s[i]);if(!s[i])return s;}Com_Error(ERR_DROP,"long name");return nullptr;}
ACTUAL_FUNCTIONS
static PhysObjUserData data[2];static rigid_body bodies[2];static broad_phase_info infos[2];
static void reset(){
 memset(data,0,sizeof(data));memset(bodies,0,sizeof(bodies));memset(infos,0,sizeof(infos));fixtureGlob={};fixtureGlob.timeLastUpdate=1000;fixtureGlob.timeLastSnapshot=950;
 fixtureGlob.objects_by_world[0].m_first=&data[0];fixtureGlob.objects_by_world[0].m_last=&data[1];data[0].m_next_link=&data[1];
 for(int i=0;i<2;++i){auto&d=data[i];auto&b=bodies[i];d.body=&b;d.refcount=1;d.id=756+i;d.m_bpb=&infos[i];infos[i].m_flags=broad_phase_base::FLAG_IS_BPI;d.m_gjk_geom_list.m_geom_count=1;b.m_userdata=&d;
 b.m_mat.x.x=b.m_mat.y.y=b.m_mat.z.z=1;d.cg2rb.x.x=d.cg2rb.y.y=d.cg2rb.z.z=1;
 b.m_inv_mass=0.25f;b.m_max_avel=16;b.m_max_delta_t=0.033f;d.mass=4;d.friction=0.5f;d.bounce=0.2f;
 b.m_mat.w.x=3.25f;b.m_t_vel.y=-27;b.m_a_vel.z=2;b.m_gravity_acc_vec.y=-9.8f;
 d.savedPos[0]=3;d.savedRot[0][0]=d.savedRot[1][1]=d.savedRot[2][2]=1;d.timeBuoyant=800;d.timeRipple=900;
 }
 model.name="fixture_model";scr_const.script_model=7;s_serverBodies.clear();s_serverBodiesStaged=false;s_serverBodyEntityError=-1;
}
int main(){
 reset();check(sizeof(void*)==8);check(sizeof(PhysSavedObjectState)==916);check((uintptr_t)&data[0]>UINT32_MAX);
 PhysSavedClockState clock{};check(Phys_CaptureCheckpointClock(clock));check(clock.update==1000&&clock.snapshot==950);
 check(Phys_ApplyCheckpointClock({2000,1900}));check(fixtureGlob.timeLastUpdate==2000&&fixtureGlob.timeLastSnapshot==1900);
 for(auto invalidClock:{PhysSavedClockState{-1,0},PhysSavedClockState{1,-1},PhysSavedClockState{1,2}}){check(!Phys_ApplyCheckpointClock(invalidClock));check(fixtureGlob.timeLastUpdate==2000&&fixtureGlob.timeLastSnapshot==1900);}
 check(Phys_ApplyCheckpointClock({1000,950}));
 const int first=Phys_GetObjId(&data[0]),second=Phys_GetObjId(&data[1]);check(first!=second);
 PhysSavedObjectState saved{};check(Phys_CaptureObjectState(0,first,saved));check(saved.buoyantOffset==-200&&saved.rippleOffset==-100);
 check(!Phys_ApplyObjectState(0,second,saved));data[1].id=data[0].id;
 for(int k=0;k<128;++k){
  auto state=saved;state.matrix[3][0]=float(k)*0.25f;state.m_t_vel[0]=float(k)-64;state.m_a_vel[2]=float(k)*0.03125f;state.m_force_sum[1]=float(k)*9;
  state.bodyFlags=k%16;state.userFlags=0x80000000u+unsigned(k);state.collisionFlags[0][0]=0x280ec93u^unsigned(k);state.collisionFlags[0][1]=unsigned(k);
  fixtureGlob.timeLastUpdate=2000+k;check(Phys_ApplyObjectState(0,second,state));PhysSavedObjectState actual{};check(Phys_CaptureObjectState(0,second,actual));check(!memcmp(&state,&actual,sizeof(state)));
  check(data[1].timeBuoyant==fixtureGlob.timeLastUpdate-200);check(bodies[1].m_userdata==&data[1]);
 }
 const auto reject=[&](const PhysSavedObjectState &bad){PhysSavedObjectState before{},after{};check(Phys_CaptureObjectState(0,second,before));check(!Phys_ApplyObjectState(0,second,bad));check(Phys_CaptureObjectState(0,second,after));check(!memcmp(&before,&after,sizeof(before)));};
 auto bad=saved;bad.matrix[0][0]=2;reject(bad);bad=saved;bad.center[3][0]=7;reject(bad);bad=saved;bad.geometryCount=2;reject(bad);
 bad=saved;bad.collisionCount=66;reject(bad);bad=saved;bad.collisionFlags[64][0]=1;reject(bad);bad=saved;bad.bodyFlags=16;reject(bad);bad=saved;bad.mass=0;reject(bad);
 for(float value:{std::numeric_limits<float>::infinity(),-std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}){
  bad=saved;bad.m_force_sum[0]=value;reject(bad);bad=saved;bad.mass=value;reject(bad);bad=saved;bad.savedRot[2][1]=value;reject(bad);
 }
 check(!Phys_CaptureObjectState(1,first,bad));check(!Phys_CaptureObjectState(-1,first,bad));check(!Phys_CaptureObjectState(0,123,bad));
 data[0].refcount=2;check(!Phys_CaptureObjectState(0,first,bad));data[0].refcount=1;
 bodies[0].m_partition_node.m_rbc_dist_first=reinterpret_cast<rigid_body_constraint_distance*>(uintptr_t(1));check(!Phys_CaptureObjectState(0,first,bad));bodies[0].m_partition_node.m_rbc_dist_first=nullptr;
 bodies[0].m_flags=0x20;check(!Phys_CaptureObjectState(0,first,bad));bodies[0].m_flags=0;
 // Native contact links remain new owners and are never copied/dereferenced.
 bodies[1].m_partition_node.m_rbc_contact_first=reinterpret_cast<rigid_body_constraint_contact*>(uintptr_t(3));check(Phys_ApplyObjectState(0,second,saved));check((uintptr_t)bodies[1].m_partition_node.m_rbc_contact_first==3);
 // Independent DynEnt factory bodies use logical owner -1. The native
 // allocation/shape validation and exact destination verification still run.
 {const int old0=data[0].id,old1=data[1].id;data[0].id=data[1].id=-1;PhysSavedObjectState dyn{},actual{};
 check(Phys_CaptureObjectState(0,first,dyn));check(dyn.owner==-1);check(Phys_ApplyObjectState(0,second,dyn));check(Phys_CaptureObjectState(0,second,actual));check(!memcmp(&dyn,&actual,sizeof(dyn)));data[0].id=old0;data[1].id=old1;}
 reset();check(Phys_CaptureObjectState(0,first,saved));SPServerBodyRecord record;record.number=756;record.generation=4;record.model=12;record.modelName="fixture_model";record.state=saved;s_serverBodies={record};
 static MemoryFile writer;writer={};SP_WriteServerBodies(&writer);std::vector<unsigned char> wire(writer.cacheBuffer,writer.cacheBuffer+writer.cacheBufferUsed);check(wire.size()==16+12+14+916);
 const auto decode=[&](const std::vector<unsigned char>&bytes,bool accept){MemoryFile reader{};reader.buffer=const_cast<unsigned char*>(bytes.data());reader.bufferSize=(int)bytes.size();bool rejected=false;try{SP_ReadServerBodies(&reader);}catch(...){rejected=true;}check(rejected!=accept);};
 decode(wire,true);check(s_serverBodiesStaged&&s_serverBodies.size()==1);check(!memcmp(&s_serverBodies[0].state,&saved,sizeof(saved)));
 for(size_t n=0;n<wire.size();++n){auto cut=wire;cut.resize(n);decode(cut,false);}
 auto wrong=wire;wrong[0]^=1;decode(wrong,false);wrong=wire;int duplicateCount=2;memcpy(wrong.data()+12,&duplicateCount,4);wrong.insert(wrong.end(),wire.begin()+16,wire.end());decode(wrong,false);
 wrong=wire;int invalid=1022;memcpy(wrong.data()+16,&invalid,4);decode(wrong,false);model.name="wrong_model";decode(wire,false);model.name="fixture_model";decode(wire,true);
 // Old SPP1 remains readable and reserializes without inventing a durable clock.
 auto legacy=wire;legacy.erase(legacy.begin()+4,legacy.begin()+12);int spp1=0x31505053;memcpy(legacy.data(),&spp1,4);decode(legacy,true);
 writer={};SP_WriteServerBodies(&writer);check(std::vector<unsigned char>(writer.cacheBuffer,writer.cacheBuffer+writer.cacheBufferUsed)==legacy);
 for(int offset:{4,8}){wrong=wire;int negative=-1;memcpy(wrong.data()+offset,&negative,4);decode(wrong,false);}
 wrong=wire;int reversed=1001;memcpy(wrong.data()+8,&reversed,4);decode(wrong,false);decode(wire,true);
 static gentity_s entity{};entity.r.inuse=1;entity.s.number=756;entity.useCount=4;entity.model=12;entity.s.eType=ET_SCRIPTMOVER;entity.classname=scr_const.script_model;
 check(SP_ServerBodyMatchesEntity(record,entity));SP_CheckServerBodyEntity(entity);SP_CheckServerBodyEntityInventory();check(s_serverBodyEntityError==-1);
 ++entity.useCount;SP_CheckServerBodyEntity(entity);check(s_serverBodyEntityError==756);--entity.useCount;
 entity.physObjId=first;gentity_s image{};SP_CanonicalServerBodyImage(entity,image);check(image.physObjId==0&&entity.physObjId==first);
 s_serverBodies.clear();bool rejected=false;try{SP_CanonicalServerBodyImage(entity,image);}catch(...){rejected=true;}check(rejected);
 std::printf("production server body checkpoint %i checks PASS; object record %zu bytes; pointer %zu bytes\n",checks,sizeof(saved),sizeof(void*));
}
'''
functions=[function(x) for x in ['static bool SP_ServerBodyMatchesEntity(', 'static void SP_CanonicalServerBodyImage(', 'static void SP_WriteServerBodies(', 'static void SP_ReadServerBodies(', 'static void SP_CheckServerBodyEntity(', 'static void SP_CheckServerBodyEntityInventory(']]
a.out.mkdir(parents=True,exist_ok=False)
(a.out/'fixture.cpp').write_text(code.replace('ACTUAL_FUNCTIONS','\n'.join(functions)),encoding='utf-8')
paths=['src/physics/phys_checkpoint.h','src/physics/phys_object_checkpoint.inl',registry,'src/platform/android/narrow32_android.cpp']
(a.out/'source-manifest.json').write_text(json.dumps([{'file':x,'sha256':hashlib.sha256((root/x).read_bytes()).hexdigest()} for x in paths],indent=2),encoding='utf-8')
