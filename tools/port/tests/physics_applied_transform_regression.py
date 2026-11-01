"""Actual transform verifier/math and native types, with isolated body storage."""
from pathlib import Path
import argparse, hashlib, json
root = Path(__file__).resolve().parents[3]
p = argparse.ArgumentParser(); p.add_argument('--out', type=Path, required=True); a = p.parse_args()
def body(path, key):
    s = (root/path).read_text(); start = s.index(key); end = s.index('{', start)+1; depth = 1
    while depth:
        depth += (s[end] == '{') - (s[end] == '}'); end += 1
    return s[start:end]
physics = 'src/physics/phys_checkpoint.inl'
code = r'''
#include <physics/phys_main.h>
#include <physics/phys_checkpoint.h>
#include <universal/com_math.h>
#include <universal/com_math_anglevectors.h>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#define BO1_MATH_ELEMENT(rows,index) ((rows)[(index)/3][(index)%3])
static rigid_body liveBody;
static PhysObjUserData liveData;
static unsigned checks;
bool Assert_MyHandler(const char*,int,int,const char*,...){std::abort();}
PhysObjUserData *Phys_GetUserData(int id){return id==1?&liveData:nullptr;}
void Phys_ObjGetVelocities(int,float*v,float*w){Phys_NitrousVecToVec3(&liveBody.m_t_vel,v);Phys_NitrousVecToVec3(&liveBody.m_a_vel,w);}
'''
for path, key in [
    ('src/universal/com_math_anglevectors.cpp','void __cdecl QuatToAxis('),
    ('src/universal/com_math.cpp','void __cdecl AxisTransformVec3('),
    (physics,'bool Phys_IsSavedBodyStateValid('),
    (physics,'bool Phys_VerifyAppliedBodyState(')]:
    code += '\n' + body(path, key)
code += r'''
static void check(bool v){if(!v){fprintf(stderr,"check %u failed\n",checks);std::abort();}++checks;}
static void install(const PhysSavedBodyState&s){
 memset(&liveBody,0,sizeof(liveBody));liveData={};liveData.body=&liveBody;
 float offset[3]={1.75f,-2.5f,3.125f},axis[3][3],rotated[3],center[3];
 Phys_Vec3ToNitrousVec(offset,&liveData.cg2rb.w);QuatToAxis(s.quaternion,axis);AxisTransformVec3(axis,offset,rotated);
 for(int i=0;i<3;++i)center[i]=s.origin[i]-rotated[i];
 Phys_AxisToNitrousMat(axis,&liveBody.m_mat);Phys_Vec3ToNitrousVec(center,&liveBody.m_mat.w);
 Phys_Vec3ToNitrousVec(s.linearVelocity,&liveBody.m_t_vel);Phys_Vec3ToNitrousVec(s.angularVelocity,&liveBody.m_a_vel);
 Phys_Vec3ToNitrousVec(s.gravity,&liveBody.m_gravity_acc_vec);
}
int main(){
 PhysSavedBodyState s{};s.origin[0]=28.2968426f;s.origin[1]=-1116.75f;s.origin[2]=-5.20343924f;
 s.quaternion[0]=.153976709f;s.quaternion[1]=-.646842718f;s.quaternion[2]=-.533849239f;s.quaternion[3]=.54146558f;
 float norm=0;for(float q:s.quaternion)norm+=q*q;for(float&q:s.quaternion)q/=sqrtf(norm);
 s.linearVelocity[0]=13.75f;s.angularVelocity[2]=-.125f;s.gravity[2]=-800;
 install(s);check(Phys_VerifyAppliedBodyState(1,&s));check(!Phys_VerifyAppliedBodyState(0,&s));check(!Phys_VerifyAppliedBodyState(2,&s));
 check(!Phys_VerifyAppliedBodyState(1,nullptr));auto bad=s;bad.quaternion[3]=100;check(!Phys_VerifyAppliedBodyState(1,&bad));
 // Every real destination component rejects even a one-ULP mutation: no epsilon.
 for(int group=0;group<7;++group)for(int c=0;c<3;++c){install(s);
  phys_vec3* fields[]={&liveBody.m_mat.x,&liveBody.m_mat.y,&liveBody.m_mat.z,&liveBody.m_mat.w,&liveBody.m_t_vel,&liveBody.m_a_vel,&liveBody.m_gravity_acc_vec};
  float* value=c==0?&fields[group]->x:c==1?&fields[group]->y:&fields[group]->z;
  *value=std::nextafter(*value,INFINITY);check(!Phys_VerifyAppliedBodyState(1,&s));
 }
 // A correct pose with wrong model/COM offset must also fail.
 install(s);liveData.cg2rb.w.x+=.25f;check(!Phys_VerifyAppliedBodyState(1,&s));
 for(int i=0;i<100;++i){s.origin[0]=i*.03125f-10;s.origin[2]=i*17.5f;install(s);check(Phys_VerifyAppliedBodyState(1,&s));}
 printf("actual canonical physics destination verifier %u checks PASS, no tolerance\n",checks);
}
'''
a.out.mkdir(parents=True, exist_ok=False)
(a.out/'fixture.cpp').write_text(code, encoding='utf-8')
(a.out/'source-manifest.json').write_text(json.dumps({path:hashlib.sha256((root/path).read_bytes()).hexdigest() for path in [physics,'src/physics/phys_checkpoint.h','src/universal/com_math_anglevectors.cpp','src/universal/com_math.cpp','src/vehicle/nitrous_vehicle.cpp']},indent=2),encoding='utf-8')
