"""Canonical native joint retirement must clear matching map bindings only.

Actual physics_system destroy/pool paths, native types and checkpoint validators;
heap/world services are isolated. No natural sleep or solver claim is made.
"""
from pathlib import Path
import argparse, hashlib, json, subprocess, sys

root = Path(__file__).resolve().parents[3]
p = argparse.ArgumentParser()
p.add_argument('--out', type=Path, required=True)
p.add_argument('--physics-source', type=Path)
p.add_argument('--case', choices=['all', 'direct', 'body', 'bulk', 'all-bodies'], default='all')
a = p.parse_args()
subprocess.run([sys.executable, str(root/'tools/port/tests/physics_joint_checkpoint_regression.py'), '--out', str(a.out)], check=True)
source = a.physics_source or root/'src/physics/physics_system.cpp'
text = source.read_bytes().decode('latin-1')

def extract(key):
    start = text.index(key)
    end = text.index('{', start) + 1
    depth = 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]

providers = r'''
#include <qcommon/cm_load.h>
#include <physics/physconstraints_load_obj.h>
clipMap_t cm{};
void PMM_VALIDATE(char *p,unsigned size,unsigned alignment){check(p&&size&&alignment&&uintptr_t(p)%alignment==0);}
void PMM_FREE(unsigned char *p,unsigned size,unsigned alignment){check(p&&size&&alignment&&uintptr_t(p)%alignment==0);--fixtureJointAllocations;::operator delete(p);}
// No wheel or contact owners are part of these model-body cases.
void phys_sys::fixup_wheel_constraints(rigid_body *){check(!g_physics_system->m_list_rbc_wheel.m_list_count);}
rigid_body_constraint_contact::~rigid_body_constraint_contact(){check(false);}
'''
if 'template<class T> static void Phys_RetireMapConstraint(' in text:
    providers += extract('template<class T> static void Phys_RetireMapConstraint(') + '\n'
    providers += extract('template<class T> static void Phys_RetireMapConstraintPool(') + '\n'
f = a.out/'fixture.cpp'
s = f.read_text(encoding='utf-8')
for cls, pool in [('point','point'), ('distance','dist'), ('hinge','hinge')]:
    old = f'void phys_sys::destroy(rigid_body_constraint_{cls} *j){{FixtureJointRemove(g_physics_system->m_list_rbc_{pool},j);}}'
    assert old in s
    s = s.replace(old, '')
    providers += extract(f'void __cdecl phys_sys::destroy(rigid_body_constraint_{cls} *const rbc)') + '\n'
for key in ['void __cdecl phys_sys::destroy_all_constraint(', 'void __cdecl phys_sys::destroy_all_rbc_point(',
            'void __cdecl phys_sys::destroy_all_rbc_hinge(', 'void __cdecl phys_sys::destroy_all_rbc_dist(',
            'void __cdecl phys_sys::destroy_all_rigid_body(']:
    providers += extract(key) + '\n'
# Definitions must precede the checkpoint helper's uses of destroy overloads.
s = s.replace('#include <physics/phys_constraint_checkpoint.inl>', providers+'\n#include <physics/phys_constraint_checkpoint.inl>', 1)
tests = r'''
 reset();static PhysConstraint bindings[8];cm.isInUse=1;cm.num_constraints=8;cm.constraints=bindings;
 for(int mode=0;mode<4;++mode)for(int kind:{1,2,3}){
  if(SELECTED>=0&&mode!=SELECTED)continue;
  PhysSavedConstraintState j{};j.kind=kind;j.first=0;j.second=-1;
  if(kind==2)j.values[1]=j.values[2]=4;
  if(kind==3){for(int n:{2,3})j.vectors[n][2]=1;for(int n:{4,6,7,8})j.vectors[n][0]=1;j.vectors[5][1]=1;}
  const int ids[]={first,second};unsigned doomed=Phys_CreateSavedConstraint(0,ids,2,j);check(doomed);
  j.first=1;unsigned survivor=Phys_CreateSavedConstraint(0,ids,2,j);check(survivor&&survivor!=doomed);
  for(auto &b:bindings){b={};b.type=static_cast<ConstraintType>(kind);b.rope_index=7;b.centity_num[0]=78;b.pos[0]=3.25f;}
  bindings[0].constraintHandle=static_cast<int>(doomed);
  bindings[1].type=static_cast<ConstraintType>(kind%3+1);bindings[1].constraintHandle=static_cast<int>(doomed); // Wrong class, leave untouched.
  bindings[2].type=CONSTRAINT_ROPE;bindings[2].constraintHandle=static_cast<int>(doomed);
  bindings[3].constraintHandle=static_cast<int>(survivor);
  bindings[4].type=CONSTRAINT_LIGHT;bindings[4].constraintHandle=1;
  bindings[5].constraintHandle=0;
  bindings[6].constraintHandle=static_cast<int>(doomed); // All matching aliases must retire.
  bindings[7].constraintHandle=1;
  PhysConstraint expected[8];memcpy(expected,bindings,sizeof(bindings));
  expected[0].constraintHandle=expected[6].constraintHandle=0;if(kind==2)expected[2].constraintHandle=0;
  if(mode>=2)expected[3].constraintHandle=0;
  printf("retirement mode %i kind %i begin\n",mode,kind);fflush(stdout);
  if(mode==0)check(Phys_DestroyCheckpointConstraint(kind,doomed));
  if(mode==1)phys_sys::destroy_all_constraint(&bodies[0]);
  if(mode==2){if(kind==1)phys_sys::destroy_all_rbc_point();if(kind==2)phys_sys::destroy_all_rbc_dist();if(kind==3)phys_sys::destroy_all_rbc_hinge();}
  if(mode==3){phys_sys system;system.destroy_all_rigid_body();}
  check(!memcmp(expected,bindings,sizeof(bindings)));
  PhysSavedConstraintState out;check(!Phys_CaptureConstraintState(0,kind,doomed,ids,2,out));
  check(fixtureJointAllocations==(mode>=2?0:1));
  // Remove intentionally malformed unrelated fixture refs, then verify the
  // remaining real map joint graph is still capturable after retirement.
  bindings[1].constraintHandle=bindings[2].constraintHandle=bindings[4].constraintHandle=bindings[7].constraintHandle=0;
  PhysSavedObjectState states[2];PhysSavedConstraintState state;
  PhysCheckpointJointRef ref{kind,survivor};
  check(Phys_CaptureObjectGraph(0,ids,2,mode>=2?nullptr:&ref,mode>=2?0:1,states,mode>=2?nullptr:&state));
  if(mode<2){check(Phys_DestroyCheckpointConstraint(kind,survivor));check(!bindings[3].constraintHandle);}
  check(!fixtureJointAllocations);
 }
 cm.constraints=nullptr;cm.num_constraints=0;cm.isInUse=0;
 printf("canonical direct/body/pool retirement clears exact map handles, retains rope/owner metadata and preserves unrelated graph PASS; ");
'''.replace('SELECTED', str({'all':-1,'direct':0,'body':1,'bulk':2,'all-bodies':3}[a.case]))
marker = ' std::printf("production server body checkpoint'
assert s.count(marker) == 1
s = s.replace(marker, tests+marker, 1)
f.write_text(s, encoding='utf-8')
m = json.loads((a.out/'source-manifest.json').read_text(encoding='utf-8'))
for path in [source, root/'src/qcommon/cm_load.h', Path(__file__).resolve()]:
    m.append({'file':str(path),'sha256':hashlib.sha256(path.read_bytes()).hexdigest()})
(a.out/'source-manifest.json').write_text(json.dumps(m,indent=2),encoding='utf-8')
