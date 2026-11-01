"""RPE1 and canonical rope factories/trace, with isolated asset/transport services."""
from pathlib import Path
import argparse, hashlib, json
root=Path(__file__).resolve().parents[3]
p=argparse.ArgumentParser();p.add_argument('--out',type=Path,required=True);p.add_argument('--legacy-trace',type=Path);a=p.parse_args()
def function(path,key):
 s=path.read_text();start=s.index(key);end=s.index('{',start)+1;depth=1
 while depth:depth+=(s[end]=='{')-(s[end]=='}');end+=1
 return s[start:end]
code=r'''
#include <physics/rope.h>
#include <physics/rope_checkpoint.h>
#include <qcommon/cm_load.h>
#include <DynEntity/DynEntity_client.h>
#include <qcommon/common.h>
#include <universal/com_math.h>
#include <universal/memfile.h>
#include <clientscript/cscr_stringlist.h>
#include <database/db_registry.h>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <limits>
static int checks;
#define CHECK(v) do{if(!(v)){fprintf(stderr,"failed check %i line %i\n",checks,__LINE__);abort();}++checks;}while(0)
bool Assert_MyHandler(const char *,int,int,const char *,...){abort();}
void Com_Error(errorParm_t,const char *,...){throw std::runtime_error("rejected");}
void Com_PrintWarning(int,const char *,...){}
void MemFile_ReadData(MemoryFile *f,int n,unsigned char *out){if(n<0||n>f->bufferSize-f->bytesUsed)Com_Error(ERR_DROP,"truncated");memcpy(out,f->buffer+f->bytesUsed,n);f->bytesUsed+=n;}
void MemFile_WriteDataInternal(MemoryFile *,unsigned,unsigned char *){Com_Error(ERR_DROP,"unexpected flush");}
clipMap_t cm{};rope_t *g_ropes;int g_max_ropes,g_rope_sys_time,g_ropesWithEntsAnchorsCount,g_ropeCount,g_totalRopeCount,g_mapRopeCount,g_num_contacts,g_update_count;
int g_ropesWithEntsAnchors[10]{};unsigned __int8 usedExtraDynEnts[256]{};
static Material firstMaterial{},secondMaterial{};static Material *liveMaterial=&firstMaterial;
static bool missingMaterial,missingBone;static int materialCalls,dbLocks,deleted;
struct {Material *ropeMaterial;} fixtureMedia;
#define cgMedia fixtureMedia
const Material *R_Rope_GetMaterialFromGfxWorld(){++materialCalls;return liveMaterial;}
const char *Material_GetName(Material *m){return m->info.name;}
void Sys_EnterCriticalSection(CriticalSection){++dbLocks;}
void Sys_LeaveCriticalSection(CriticalSection){CHECK(dbLocks==1);--dbLocks;}
XAssetEntryPoolEntry *DB_FindXAssetEntry(XAssetType t,const char *name){CHECK(dbLocks==1);static XAssetEntryPoolEntry e{};if(missingMaterial||t!=ASSET_TYPE_MATERIAL||strcmp(name,"rope_test"))return nullptr;e.entry.asset.header.material=liveMaterial;return &e;}
char *SL_ConvertToString(unsigned value,scriptInstance_t inst){CHECK((value==1234||value==4321)&&inst==SCRIPTINSTANCE_SERVER);return const_cast<char *>("tag_rope");}
unsigned SL_FindString(const char *name,scriptInstance_t inst){CHECK(inst==SCRIPTINSTANCE_SERVER);return !missingBone&&!strcmp(name,"tag_rope")?4321:0;}
void DeleteConstraint(int){++deleted;}
CSTRING
MATH
FACTORIES
#undef PROF_SCOPED
#define PROF_SCOPED(...)
static const float radius_0=5.0f,local_scale=20.0f;
static int sphereCalls,hitVertex=-1;static bool checkFrame=true;
int trace_point_through_sphere(const float *,const float *,const float *point,float,float *t,float *){
 if(checkFrame)CHECK(point[0]==2000.0f+sphereCalls);int n=sphereCalls++;*t=1;return n==hitVertex;
}
TRACE
#include <physics/rope_checkpoint.inl>
static std::vector<unsigned char> encode(const RopeSavedSystem &s){MemoryFile w{};Rope_WriteCheckpoint(&w,s);return {w.cacheBuffer,w.cacheBuffer+w.cacheBufferUsed};}
static bool decode(const std::vector<unsigned char> &bytes,RopeSavedSystem &out){MemoryFile r{};r.buffer=const_cast<unsigned char *>(bytes.data());r.bufferSize=(int)bytes.size();try{Rope_ReadCheckpoint(&r,out);CHECK(r.bytesUsed==r.bufferSize);return true;}catch(...){return false;}}
int main(){
 CHECK(sizeof(void*)==8);static rope_t storage[4]{};g_ropes=storage;cm.ropes=storage;g_max_ropes=cm.max_ropes=4;g_mapRopeCount=g_totalRopeCount=2;g_ropeCount=3;
 firstMaterial.info.name=secondMaterial.info.name="rope_test";fixtureMedia.ropeMaterial=&firstMaterial;
 static DynEntityPose ownerPoses[4]{};static DynEntityClient ownerClients[4]{};cm.originalDynEntCount=4;cm.dynEntCount[0]=4;cm.dynEntPoseList[0]=ownerPoses;cm.dynEntClientList[0]=ownerClients;
 const float start[3]={0,0,0},end[3]={600,0,0};
 Rope_Create(0);Rope_Init(0,4,start,end,75,2,1,1,1);Rope_Create(2);Rope_Init(2,25,start,end,600,3,.9f,2,1);
 Rope_AddAnchor(0,0,start,-1,-1,ROPE_WORLD_CONSTRAINT);Rope_AddDynEntAnchor(0,3,end,2);
 Rope_AddEntAnchor(2,0,start,5,1234);Rope_AddEntAnchor(2,12,start,7,0);Rope_AddAnchor(2,24,end,-1,-1,ROPE_WORLD_CONSTRAINT);
 for(int slot:{0,2}){auto &r=storage[slot];r.m_min[0]=r.m_min[1]=r.m_min[2]=-10;r.m_max[0]=r.m_max[1]=r.m_max[2]=3000;r.m_visible=1;r.m_flags|=4;r.m_frame=29;r.m_stable_count=7;r.m_lightingHandle=63;r.m_client_verts.frame_index=1;r.m_num_draw_verts=r.m_client_verts.frame_verts[1].num_verts=49;
  for(int n=0;n<50;++n){r.m_client_verts.frame_verts[1].v[n][0]=2000.0f+n;r.m_client_verts.frame_verts[0].v[n][0]=1000.0f+n;}
  for(int n=0;n<r.m_num_particles;++n){r.m_particles[n].p0[1]=float(n)*.25f;r.m_particles[n].p_prev[2]=-float(n)*.125f;r.m_particles[n].flags=n%4;}
 }
 g_rope_sys_time=12345;RopeSavedSystem saved;CHECK(Rope_CaptureCheckpoint(saved));CHECK(Rope_IsSavedCheckpointValid(saved));CHECK(saved.entityRopes==std::vector<int>{2});CHECK(saved.ropes[2].constraint[24].bone=="tag_rope");
 auto wire=encode(saved);CHECK(!memcmp(wire.data(),"RPE1",4));RopeSavedSystem decoded;CHECK(decode(wire,decoded)&&decoded==saved);CHECK(encode(decoded)==wire);
 for(size_t n=0;n<wire.size();++n){auto cut=wire;cut.resize(n);auto before=decoded;CHECK(!decode(cut,decoded));CHECK(decoded==before);}CHECK(decode(wire,decoded));
 auto invalid=saved;invalid.entityRopes.push_back(2);CHECK(!Rope_IsSavedCheckpointValid(invalid));invalid=saved;invalid.ropes[2].entityAnchor[1]=24;CHECK(!Rope_IsSavedCheckpointValid(invalid));
 invalid=saved;invalid.ropes[2].constraint[0].second=25;CHECK(!Rope_IsSavedCheckpointValid(invalid));invalid=saved;invalid.ropes[0].constraint[0].entity=5;CHECK(!Rope_IsSavedCheckpointValid(invalid));
 for(float bad:{NAN,INFINITY,-INFINITY})for(int field=0;field<9;++field){invalid=saved;float *v=field<3?invalid.ropes[2].particle[24].p:field<6?invalid.ropes[2].particle[24].previous:invalid.ropes[2].particle[24].drawPrevious;v[field%3]=bad;CHECK(!Rope_IsSavedCheckpointValid(invalid));}
 invalid=saved;invalid.ropes[2].curveFrame=2;CHECK(!Rope_IsSavedCheckpointValid(invalid));invalid=saved;invalid.ropes[2].curveSegments=50;CHECK(!Rope_IsSavedCheckpointValid(invalid));
 // Resource/owner failure must not change even native cache or unused bytes.
 std::vector<unsigned char> raw(sizeof(storage));memcpy(raw.data(),storage,sizeof(storage));int calls=materialCalls;
 missingMaterial=true;CHECK(!Rope_ApplyCheckpoint(saved));missingMaterial=false;CHECK(!memcmp(raw.data(),storage,sizeof(storage))&&materialCalls==calls);
 missingBone=true;CHECK(!Rope_ApplyCheckpoint(saved));missingBone=false;CHECK(!memcmp(raw.data(),storage,sizeof(storage))&&materialCalls==calls);
 invalid=saved;invalid.ropes[0].constraint[4].entity=99;CHECK(!Rope_ApplyCheckpoint(invalid));CHECK(!memcmp(raw.data(),storage,sizeof(storage)));
 liveMaterial=&secondMaterial;CHECK(Rope_ApplyCheckpoint(saved));CHECK(storage[2].m_material==&secondMaterial&&storage[2].m_constraints[24].bone_name_hash==4321);CHECK(storage[2].m_lightingHandle==0);RopeSavedSystem actual;CHECK(Rope_CaptureCheckpoint(actual)&&actual==saved);
 // Current, bounded drawable geometry is the only curve consumed by tracing.
 // RPE1 may discard the inactive buffer because BuildCurve overwrites it.
 g_ropeCount=3;storage[0].m_in_use=0;const float rayStart[3]={0,0,0},rayEnd[3]={2500,0,0};sphereCalls=0;Rope_Trace(rayStart,rayEnd);CHECK(sphereCalls==50);
 const auto previous=storage[2].m_particles[24].p[0];sphereCalls=0;hitVertex=49;Rope_Trace(rayStart,rayEnd);CHECK(sphereCalls==50&&storage[2].m_particles[24].p[0]!=previous);CHECK(storage[2].m_particles[16].p[0]==saved.ropes[2].particle[16].p[0]);
 // Adaptive short curves must still map their endpoint to the last particle.
 CHECK(Rope_ApplyCheckpoint(saved));storage[0].m_in_use=0;storage[2].m_client_verts.frame_verts[1].num_verts=6;sphereCalls=0;hitVertex=6;Rope_Trace(rayStart,rayEnd);CHECK(sphereCalls==7&&storage[2].m_particles[24].p[0]!=saved.ropes[2].particle[24].p[0]);CHECK(storage[2].m_particles[2].p[0]==saved.ropes[2].particle[2].p[0]);
 storage[2].m_client_verts.frame_verts[1].num_verts=0;sphereCalls=0;Rope_Trace(rayStart,rayEnd);CHECK(sphereCalls==0);
 storage[2].m_client_verts.frame_verts[1].num_verts=50;bool rejected=false;try{Rope_Trace(rayStart,rayEnd);}catch(...){rejected=true;}CHECK(rejected&&sphereCalls==0);
 // The cut branch uses actual constraint ownership, not a particle-array alias.
 CHECK(Rope_ApplyCheckpoint(saved));storage[0].m_in_use=0;storage[2].m_health=1;storage[2].m_constraints[24].type=ROPE_WORLD_CONSTRAINT;storage[2].m_constraints[24].entity_index=-1;storage[2].m_constraints[24].bone_name_hash=0;storage[2].m_entity_anchors[0]=25;storage[2].m_num_entity_anchors=1;sphereCalls=0;hitVertex=0;Rope_Trace(rayStart,rayEnd);CHECK(deleted==1&&storage[2].m_health==0);
 printf("production RPE1/factories/current-curve trace %i checks PASS; pointer %zu; wire %zu bytes\n",checks,sizeof(void*),wire.size());
}
'''
code=code.replace('CSTRING','static char buffer[1024];\n'+function(root/'src/universal/memfile.cpp','char *__cdecl MemFile_ReadCString('))
rope=root/'src/physics/rope.cpp'
keys=['bool __cdecl Rope_IsValid(', 'void __cdecl Rope_Reset(int ', 'void __cdecl Rope_SetMaterial(', 'void __cdecl Rope_AddAnchor(', 'void __cdecl Rope_Activate(', 'void __cdecl Rope_AddDynEntAnchor(', 'void __cdecl Rope_AddEntAnchor(', 'void __cdecl Rope_RemoveAnchor(', 'void __cdecl Rope_Create(int ', 'void __cdecl Rope_Init(', 'void __cdecl Rope_AddParticle(']
code=code.replace('FACTORIES','\n'.join(function(rope,key) for key in keys))
code=code.replace('MATH','\n'.join(function(root/'src/universal/com_math.cpp',key) for key in ['float __cdecl Vec3Length(', 'void __cdecl Vec3Min(', 'void __cdecl Vec3Max(']))
code=code.replace('TRACE',function(a.legacy_trace or rope,'void __cdecl Rope_Trace('))
if a.legacy_trace:
 code=code.replace('sphereCalls=0;Rope_Trace(rayStart,rayEnd);','sphereCalls=0;checkFrame=false;Rope_Trace(rayStart,rayEnd);fprintf(stderr,\"legacy trace sampled %i vertices against 50-vertex storage limit\\n\",sphereCalls);',1)
a.out.mkdir(parents=True,exist_ok=False);(a.out/'fixture.cpp').write_text(code,encoding='utf-8')
files=[rope,root/'src/physics/rope.h',root/'src/physics/rope_checkpoint.h',root/'src/physics/rope_checkpoint.inl',root/'src/universal/com_math.cpp',root/'src/universal/memfile.cpp',Path(__file__)]
if a.legacy_trace:files.append(a.legacy_trace)
(a.out/'source-manifest.json').write_text(json.dumps({str(x):hashlib.sha256(x.read_bytes()).hexdigest() for x in files},indent=2))
