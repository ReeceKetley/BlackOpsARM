"""Actual JNT1 records/native helpers; isolated allocator and world providers."""
from pathlib import Path
import argparse, subprocess, sys, hashlib, json
root = Path(__file__).resolve().parents[3]
p = argparse.ArgumentParser()
p.add_argument('--out', type=Path, required=True)
p.add_argument('--legacy-object', type=Path)
a = p.parse_args()
subprocess.run([sys.executable, str(root/'tools/port/tests/server_physics_checkpoint_regression.py'), '--out', str(a.out)], check=True)
f = a.out/'fixture.cpp'
s = f.read_text()
providers = r'''
#include <physics/physics_system.h>
#include <physics/physics_system_internal.h>
static int fixtureJointAllocations;
static bool fixtureJointFail;
static int fixtureJointFailAfter=-1;
template<class T> static T *FixtureJointAdd(phys_free_list<T> &p, environment_rigid_body *a, environment_rigid_body *b) {
 if(fixtureJointFail)return nullptr;
 if(fixtureJointFailAfter==0)return nullptr;
 if(fixtureJointFailAfter>0)--fixtureJointFailAfter;
 auto *n=new typename phys_free_list<T>::T_internal{};
 n->m_prev_T_internal=&p.m_dummy_head;n->m_next_T_internal=p.m_dummy_head.m_next_T_internal;
 n->m_next_T_internal->m_prev_T_internal=n;p.m_dummy_head.m_next_T_internal=n;
 ++p.m_list_count;p.debug_add(n);n->m_data.b1=a;n->m_data.b2=b;++fixtureJointAllocations;return &n->m_data;
}
template<class T> static void FixtureJointRemove(phys_free_list<T> &p,T *j) {
 using N=typename phys_free_list<T>::T_internal;
 auto *n=reinterpret_cast<N *>(reinterpret_cast<char *>(j)-offsetof(N,m_data));
 p.debug_remove(n);n->m_prev_T_internal->m_next_T_internal=n->m_next_T_internal;
 n->m_next_T_internal->m_prev_T_internal=n->m_prev_T_internal;--p.m_list_count;--fixtureJointAllocations;delete n;
}
environment_rigid_body *phys_sys::get_environment_rigid_body(){return &g_physics_system->m_environment_rigid_body;}
rigid_body_constraint_point *phys_sys::create_rbc_point(environment_rigid_body *a,environment_rigid_body *b,int){return FixtureJointAdd(g_physics_system->m_list_rbc_point,a,b);}
rigid_body_constraint_distance *phys_sys::create_rbc_dist(environment_rigid_body *a,environment_rigid_body *b,int){return FixtureJointAdd(g_physics_system->m_list_rbc_dist,a,b);}
rigid_body_constraint_hinge *phys_sys::create_rbc_hinge(environment_rigid_body *a,environment_rigid_body *b,int){return FixtureJointAdd(g_physics_system->m_list_rbc_hinge,a,b);}
void phys_sys::destroy(rigid_body_constraint_point *j){FixtureJointRemove(g_physics_system->m_list_rbc_point,j);}
void phys_sys::destroy(rigid_body_constraint_distance *j){FixtureJointRemove(g_physics_system->m_list_rbc_dist,j);}
void phys_sys::destroy(rigid_body_constraint_hinge *j){FixtureJointRemove(g_physics_system->m_list_rbc_hinge,j);}
'''
if not a.legacy_object:
    providers += '#include <physics/phys_constraint_checkpoint.inl>\n'
else:
    s = s.replace('#include <physics/phys_object_checkpoint.inl>', '#include "'+str(a.legacy_object.resolve()).replace('\\','/')+'"')
s = s.replace('#undef physGlob', '#undef physGlob\n'+providers)
guard = r'''
 // A live joint before partition generation, and a non-head island member,
 // both have empty local m_rbc_* lists. Neither body is independent.
 auto *testJoint=phys_sys::create_rbc_point(reinterpret_cast<environment_rigid_body *>(&bodies[0]),reinterpret_cast<environment_rigid_body *>(&bodies[1]),1);
 bodies[0].m_partition_node={};bodies[1].m_partition_node={};
 bodies[1].m_partition_node.m_partition_head=&bodies[0];
 PhysSavedObjectState independent{};
 check(!Phys_CaptureObjectState(0,first,independent));
 check(!Phys_CaptureObjectState(0,second,independent));
 phys_sys::destroy(testJoint);check(!fixtureJointAllocations);
 check(Phys_CaptureObjectState(0,first,independent));
'''
tests = r'''
 const int ids[2]={first,second};
 for(int kind:{1,2,3})for(bool worldAnchor:{false,true}){
  PhysSavedConstraintState joint;joint.kind=kind;joint.first=0;joint.second=worldAnchor?-1:1;
  joint.vectors[0][0]=1.25f;joint.vectors[1][2]=-2.5f;
  if(kind==1){joint.values[0]=7;joint.values[1]=3;joint.values[2]=.25f;joint.spring=1;}
  if(kind==2){joint.values[0]=1;joint.values[1]=5;joint.values[2]=6;joint.values[3]=-2;joint.values[4]=.125f;joint.flags=3;}
  if(kind==3){joint.values[0]=.25f;joint.flags=19;
   for(int n:{2,3})joint.vectors[n][2]=1;
   for(int n:{4,6,7,8})joint.vectors[n][0]=1;joint.vectors[5][1]=1;
  }
  for(int n=0;n<(kind==3?8:3);++n)joint.pulses[n]=float(n-2)*.25f;
  check(Phys_IsSavedConstraintStateValid(joint,2));
  const int beforeAlloc=fixtureJointAllocations;
  unsigned handle=Phys_CreateSavedConstraint(0,ids,2,joint);check(handle&&fixtureJointAllocations==beforeAlloc+1);
  PhysSavedConstraintState actual;check(Phys_CaptureConstraintState(0,kind,handle,ids,2,actual));check(!memcmp(&joint,&actual,sizeof(joint)));
  check(!Phys_CaptureObjectState(0,first,independent));
  check(!Phys_CaptureConstraintState(0,kind,1,ids,2,actual));
  check(!Phys_CaptureConstraintState(0,kind%3+1,handle,ids,2,actual));
  check(!Phys_CaptureConstraintState(1,kind,handle,ids,2,actual));
  int duplicateIds[2]={first,first};check(!Phys_CaptureConstraintState(0,kind,handle,duplicateIds,2,actual));
  check(Phys_DestroyCheckpointConstraint(kind,handle));check(fixtureJointAllocations==beforeAlloc);
  check(!Phys_CaptureConstraintState(0,kind,handle,ids,2,actual));check(!Phys_DestroyCheckpointConstraint(kind,handle));
  fixtureJointFail=true;check(!Phys_CreateSavedConstraint(0,ids,2,joint));fixtureJointFail=false;
  // Production JNT1 codec with isolated MemFile transport; all truncations.
  MemoryFile wireWriter{};Phys_WriteConstraintState(&wireWriter,joint,2);
  std::vector<unsigned char> wireBytes(wireWriter.cacheBuffer,wireWriter.cacheBuffer+wireWriter.cacheBufferUsed);
  check(wireBytes.size()==196&&!memcmp(wireBytes.data(),"JNT1",4));
  MemoryFile wireReader{};wireReader.buffer=wireBytes.data();wireReader.bufferSize=(int)wireBytes.size();
  PhysSavedConstraintState decoded;Phys_ReadConstraintState(&wireReader,decoded,2);check(!memcmp(&joint,&decoded,sizeof(joint)));
  for(size_t n=0;n<wireBytes.size();++n){
   MemoryFile truncated{};truncated.buffer=wireBytes.data();truncated.bufferSize=(int)n;
   auto prior=decoded;bool rejected=false;try{Phys_ReadConstraintState(&truncated,decoded,2);}catch(...){rejected=true;}
   check(rejected&&!memcmp(&prior,&decoded,sizeof(prior)));
  }
  auto invalid=joint;invalid.first=invalid.second;check(!Phys_IsSavedConstraintStateValid(invalid,2));
  invalid=joint;invalid.second=2;check(!Phys_CreateSavedConstraint(0,ids,2,invalid));
  invalid=joint;invalid.flags=~0u;check(!Phys_CreateSavedConstraint(0,ids,2,invalid));
  for(int field=0;field<43;++field){
   for(float badFloat:{NAN,INFINITY,-INFINITY}){
    invalid=joint;float *target=field<27?&invalid.vectors[field/3][field%3]:field<35?&invalid.values[field-27]:&invalid.pulses[field-35];
    *target=badFloat;check(!Phys_IsSavedConstraintStateValid(invalid,2));check(!Phys_CreateSavedConstraint(0,ids,2,invalid));
   }
  }
  check(fixtureJointAllocations==beforeAlloc);
 }
 check(!fixtureJointAllocations);
 // A body graph must be closed over all native joint classes. No output may
 // publish if a reference is missing, duplicated or crosses the owner cohort.
 const int graphIds[]={first,second};
 PhysSavedConstraintState graphJoints[2];
 graphJoints[0].kind=1;graphJoints[0].first=0;graphJoints[0].second=1;
 graphJoints[1].kind=2;graphJoints[1].first=1;graphJoints[1].second=-1;
 graphJoints[1].values[1]=graphJoints[1].values[2]=4;
 unsigned originals[2];PhysCheckpointJointRef refs[2];
 for(int i=0;i<2;++i){originals[i]=Phys_CreateSavedConstraint(0,graphIds,2,graphJoints[i]);check(originals[i]);refs[i]={graphJoints[i].kind,originals[i]};}
 PhysSavedObjectState graphBodies[2],untouchedBodies[2];
 PhysSavedConstraintState capturedJoints[2],untouchedJoints[2];
 memcpy(untouchedBodies,graphBodies,sizeof(graphBodies));memcpy(untouchedJoints,capturedJoints,sizeof(capturedJoints));
 const auto unchanged=[&](){check(!memcmp(graphBodies,untouchedBodies,sizeof(graphBodies)));check(!memcmp(capturedJoints,untouchedJoints,sizeof(capturedJoints)));};
 check(!Phys_CaptureObjectGraph(0,graphIds,2,nullptr,0,graphBodies,nullptr));unchanged();
 check(!Phys_CaptureObjectGraph(0,graphIds,2,refs,1,graphBodies,capturedJoints));unchanged();
 check(!Phys_CaptureObjectGraph(0,graphIds,1,refs,2,graphBodies,capturedJoints));unchanged();
 auto duplicateRefs=refs[1];refs[1]=refs[0];check(!Phys_CaptureObjectGraph(0,graphIds,2,refs,2,graphBodies,capturedJoints));unchanged();refs[1]=duplicateRefs;
 auto *unsupported=FixtureJointAdd(g_physics_system->m_list_rbc_wheel,reinterpret_cast<environment_rigid_body *>(&bodies[0]),phys_sys::get_environment_rigid_body());
 check(!Phys_CaptureObjectGraph(0,graphIds,2,refs,2,graphBodies,capturedJoints));unchanged();FixtureJointRemove(g_physics_system->m_list_rbc_wheel,unsupported);
 check(Phys_CaptureObjectGraph(0,graphIds,2,refs,2,graphBodies,capturedJoints));check(!memcmp(graphJoints,capturedJoints,sizeof(graphJoints)));
 for(int i=0;i<2;++i)check(Phys_DestroyCheckpointConstraint(refs[i].kind,refs[i].handle));
 // Restore full state first, then joints. A failure after one allocation must
 // retire it and restore both bodies, without exposing even one output handle.
 bodies[0].m_t_vel.x+=2;bodies[1].m_t_vel.z-=3;
 PhysSavedObjectState prior[2];for(int i=0;i<2;++i)check(Phys_CaptureObjectState(0,graphIds[i],prior[i]));
 unsigned restored[2]={0x12345678u,0x87654321u};fixtureJointFailAfter=1;
 check(!Phys_ApplyObjectGraph(0,graphIds,2,graphBodies,graphJoints,2,restored));fixtureJointFailAfter=-1;
 check(restored[0]==0x12345678u&&restored[1]==0x87654321u&&!fixtureJointAllocations);
 for(int i=0;i<2;++i){PhysSavedObjectState now;check(Phys_CaptureObjectState(0,graphIds[i],now));check(!memcmp(&prior[i],&now,sizeof(now)));}
 auto invalidBody=graphBodies[1];graphBodies[1].center[3][0]+=1;
 check(!Phys_ApplyObjectGraph(0,graphIds,2,graphBodies,graphJoints,2,restored));check(!fixtureJointAllocations);graphBodies[1]=invalidBody;
 check(Phys_ApplyObjectGraph(0,graphIds,2,graphBodies,graphJoints,2,restored));
 for(int i=0;i<2;++i)refs[i]={graphJoints[i].kind,restored[i]};
 PhysSavedObjectState restoredBodies[2];PhysSavedConstraintState restoredJoints[2];
 check(Phys_CaptureObjectGraph(0,graphIds,2,refs,2,restoredBodies,restoredJoints));
 check(!memcmp(restoredBodies,graphBodies,sizeof(graphBodies)));check(!memcmp(restoredJoints,graphJoints,sizeof(graphJoints)));
 for(int i=0;i<2;++i)check(Phys_DestroyCheckpointConstraint(refs[i].kind,refs[i].handle));check(!fixtureJointAllocations);
 printf("production JNT1 point/distance/hinge records and live-pool guards (isolated allocator) PASS; ");
'''
marker = ' std::printf("production server body checkpoint'
assert s.count(marker) == 1
s = s.replace(marker, guard + (tests if not a.legacy_object else '') + marker, 1)
f.write_text(s, encoding='utf-8')
m = json.loads((a.out/'source-manifest.json').read_text())
for name in ['src/physics/phys_constraint_checkpoint.inl','src/physics/physics_system.cpp','src/physics/rigid_body.h','tl/physics/rbc_def_generic.cpp']:
    m.append({'file':name,'sha256':hashlib.sha256((root/name).read_bytes()).hexdigest()})
(a.out/'source-manifest.json').write_text(json.dumps(m,indent=2))
