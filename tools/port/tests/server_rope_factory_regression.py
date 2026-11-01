"""Actual server rope factory and JNT1 graph, with isolated entity/pool services.

The canonical CreateConstraint and distance::set functions run over real native
body types, Narrow32 and checkpoint graph validation. No solver step runs here.
--factory-source permits reproducing failures against a frozen source file.
"""
from pathlib import Path
import argparse, hashlib, json, subprocess, sys

root = Path(__file__).resolve().parents[3]
p = argparse.ArgumentParser()
p.add_argument('--out', type=Path, required=True)
p.add_argument('--factory-source', type=Path)
p.add_argument('--case', choices=['all', 'entity-world', 'world-entity', 'entity-entity'], default='all')
a = p.parse_args()
subprocess.run([sys.executable, str(root/'tools/port/tests/physics_joint_checkpoint_regression.py'), '--out', str(a.out)], check=True)

def extract(path, key):
    text = path.read_bytes().decode('latin-1')
    start = text.index(key)
    end = text.index('{', start) + 1
    depth = 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]

factory = a.factory_source or root/'src/game/g_mover.cpp'
providers = r'''
#include <physics/physconstraints_load_obj.h>
static gentity_s ropeEntities[2];
static gentity_s *GetEntity(const unsigned short *name) {
 for(auto &e:ropeEntities)if(e.targetname==*name)return &e;return nullptr;
}
'''
providers += extract(root/'src/physics/phys_collision.cpp', 'PhysObjUserData *__cdecl Phys_GetUserData(')
providers += '\n' + extract(root/'tl/physics/rbc_def_generic.cpp', 'void __thiscall rigid_body_constraint_distance::set(')
providers += '\n' + extract(factory, 'void    CreateConstraint(')
tests = r'''
 reset();
 for(int i=0;i<2;++i){ropeEntities[i]={};ropeEntities[i].targetname=41+i;ropeEntities[i].s.number=756+i;ropeEntities[i].physObjId=Phys_GetObjId(&data[i]);}
 const int ropeIds[2]={ropeEntities[0].physObjId,ropeEntities[1].physObjId};
 // Nonidentity model→body transforms expose stale or swapped anchor vectors.
 data[0].cg2rb.x={0,1,0};data[0].cg2rb.y={-1,0,0};data[0].cg2rb.z={0,0,1};data[0].cg2rb.w={10,20,30};
 data[1].cg2rb.x={0,0,1};data[1].cg2rb.y={0,1,0};data[1].cg2rb.z={-1,0,0};data[1].cg2rb.w={-4,5,6};
 for(int which=0;which<3;++which){
  if(SELECTED>=0&&which!=SELECTED)continue;
  PhysConstraint c{};c.type=CONSTRAINT_ROPE;c.distance=128;
  c.attach_point_type1=which==1?ATTACH_POINT_WORLD:ATTACH_POINT_ENT;
  c.attach_point_type2=which==0?ATTACH_POINT_WORLD:ATTACH_POINT_ENT;
  c.target_ent1=41;c.target_ent2=42;
  c.pos[0]=1;c.pos[1]=2;c.pos[2]=3;c.pos2[0]=4;c.pos2[1]=5;c.pos2[2]=6;
  // Independent expected transforms: A=(8,21,33), B=(-10,10,10).
  const float firstExpected[3]={which==1?-10.f:8.f,which==1?10.f:21.f,which==1?10.f:33.f};
  const float secondExpected[3]={which==0?4.f:which==1?1.f:-10.f,which==0?5.f:which==1?2.f:10.f,which==0?6.f:which==1?3.f:10.f};
  std::printf("server rope factory case %i begin\n",which);std::fflush(stdout);
  CreateConstraint(&c);check(c.constraintHandle&&fixtureJointAllocations==1);
  PhysSavedConstraintState j;check(Phys_CaptureConstraintState(0,CONSTRAINT_DISTANCE,(unsigned)c.constraintHandle,ropeIds,2,j));
  check(j.first==(which==1?1:0)&&j.second==(which==2?1:-1));
  for(int n=0;n<3;++n){check(j.vectors[0][n]==firstExpected[n]);check(j.vectors[1][n]==secondExpected[n]);}
  check(j.values[0]==0&&j.values[1]==128&&j.values[2]==128&&j.flags==1);
  check(c.target_index1==(which==1?0:756));check(c.target_index2==(which==0?0:757));
  PhysSavedObjectState states[2];PhysSavedConstraintState joint;
  PhysCheckpointJointRef ref{CONSTRAINT_DISTANCE,(unsigned)c.constraintHandle};
  check(Phys_CaptureObjectGraph(0,ropeIds,2,&ref,1,states,&joint));
  check(states[0].owner==756&&states[1].owner==757);
  check(Phys_DestroyCheckpointConstraint(ref.kind,ref.handle)&&!fixtureJointAllocations);
  fixtureJointFail=true;c.constraintHandle=0;CreateConstraint(&c);fixtureJointFail=false;check(!c.constraintHandle&&!fixtureJointAllocations);
 }
 // The native graph helper can preserve a server/DynEnt cohort when a caller
 // owns both registries. This does not enable the public registry yet.
 data[1].id=-1;
 PhysSavedConstraintState mixed{};mixed.kind=CONSTRAINT_DISTANCE;mixed.first=0;mixed.second=1;
 mixed.values[1]=mixed.values[2]=64;mixed.flags=1;
 unsigned mixedHandle=Phys_CreateSavedConstraint(0,ropeIds,2,mixed);check(mixedHandle);
 PhysCheckpointJointRef mixedRef{CONSTRAINT_DISTANCE,mixedHandle};PhysSavedObjectState mixedBodies[2];
 PhysSavedConstraintState mixedState;check(Phys_CaptureObjectGraph(0,ropeIds,2,&mixedRef,1,mixedBodies,&mixedState));
 check(mixedBodies[0].owner==756&&mixedBodies[1].owner==-1);
 PhysObjUserData freshData[2];rigid_body freshBodies[2];broad_phase_info freshInfos[2];int freshIds[2];
 for(int i=0;i<2;++i){freshData[i]=data[i];freshBodies[i]=bodies[i];freshInfos[i]=infos[i];
  freshData[i].body=&freshBodies[i];freshData[i].m_bpb=&freshInfos[i];freshBodies[i].m_userdata=&freshData[i];
  freshBodies[i].m_partition_node={};freshBodies[i].m_t_vel.x+=19;
  freshData[i].m_next_link=fixtureGlob.objects_by_world[0].m_first;fixtureGlob.objects_by_world[0].m_first=&freshData[i];
  freshIds[i]=Phys_GetObjId(&freshData[i]);check(freshIds[i]!=ropeIds[i]);}
 unsigned freshJoint=0;check(Phys_ApplyObjectGraph(0,freshIds,2,mixedBodies,&mixedState,1,&freshJoint));
 PhysCheckpointJointRef freshRef{CONSTRAINT_DISTANCE,freshJoint};PhysSavedObjectState freshState[2];PhysSavedConstraintState freshConstraint;
 check(Phys_CaptureObjectGraph(0,freshIds,2,&freshRef,1,freshState,&freshConstraint));
 check(!memcmp(mixedBodies,freshState,sizeof(mixedBodies))&&!memcmp(&mixedState,&freshConstraint,sizeof(mixedState)));
 check(Phys_DestroyCheckpointConstraint(freshRef.kind,freshRef.handle));
 check(Phys_DestroyCheckpointConstraint(mixedRef.kind,mixedRef.handle)&&!fixtureJointAllocations);
 fixtureGlob.objects_by_world[0].m_first=&data[0];
 std::printf("canonical server rope endpoints/transformed anchors and fresh mixed-owner native graph PASS; ");
'''.replace('SELECTED', str({'all':-1,'entity-world':0,'world-entity':1,'entity-entity':2}[a.case]))
f = a.out/'fixture.cpp'
s = f.read_text(encoding='utf-8')
s = s.replace('int main(){', providers + '\nint main(){', 1)
marker = ' std::printf("production server body checkpoint'
assert s.count(marker) == 1
s = s.replace(marker, tests + marker, 1)
f.write_text(s, encoding='utf-8')
m = json.loads((a.out/'source-manifest.json').read_text(encoding='utf-8'))
for path in [factory, root/'src/physics/phys_local.h', root/'src/physics/phys_collision.cpp', Path(__file__).resolve()]:
    m.append({'file':str(path), 'sha256':hashlib.sha256(path.read_bytes()).hexdigest()})
(a.out/'source-manifest.json').write_text(json.dumps(m, indent=2), encoding='utf-8')
