"""Actual logical light borrower predicates and staged FDO1/CNS preflight; no world publication."""
from pathlib import Path
import argparse,json,hashlib
p=argparse.ArgumentParser();p.add_argument('--out',type=Path,required=True);a=p.parse_args();a.out.mkdir(exist_ok=False)
def extract(path,key):
 s=Path(path).read_text(encoding='utf-8');i=s.index(key);j=s.index('{',i)+1;n=1
 while n:n+=(s[j]=='{')-(s[j]=='}');j+=1
 return s[i:j]
js='src/game_sp/g_sp_dynent_joints_checkpoint.inl';ns='src/game_sp/g_sp_client_network_checkpoint.inl';ms='src/game_sp/g_sp_client_models_checkpoint.inl'
code=r'''
#include <qcommon/cm_load.h>
#include <physics/physconstraints_load_obj.h>
#include <physics/phys_checkpoint.h>
#include <client/client.h>
#include <cgame_mp/cg_local_mp.h>
#include <qcommon/common.h>
#include <vector>
#include <string>
#include <stdexcept>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
clipMap_t cm{};int g_entNumBits=10,g_entNumCscEnd=1536;
void __cdecl Com_Error(errorParm_t,const char*,...){throw std::runtime_error("rejected");}
STRUCTS
static SPClientEntityRegistry s_clientEntityRegistry;
static std::vector<SPDynJointBinding> s_savedDynBindings;
static struct {std::vector<SPClientNetworkEntity> entities;} s_clientNetworkState;
HELPERS
static unsigned checks;static void check(bool b){if(!b)std::abort();++checks;}
int main(){
 PhysConstraint c{};c.type=CONSTRAINT_LIGHT;c.target_index2=7;
 for(int type:{CONSTRAINT_POINT,CONSTRAINT_ROPE,CONSTRAINT_LIGHT})for(int client:{0,1,2,3})for(int id:{-2,-1,0,1023,1024,1098,1534,1536,2000}){
  c.type=static_cast<ConstraintType>(type);const bool expected=id>=-1&&(id<1024||(type==CONSTRAINT_LIGHT&&client==0&&id<1536));check(SP_DynCentitySlotValid(c,client,id)==expected);
 }
 c.type=CONSTRAINT_LIGHT;entityState_s state{};state.number=1098;state.eType=ET_PRIMARY_LIGHT;state.index.primaryLight=7;check(SP_DynLightOwnerValid(c,1098,true,&state));
 check(!SP_DynLightOwnerValid(c,1098,false,&state));check(!SP_DynLightOwnerValid(c,1098,true,nullptr));state.index.primaryLight=8;check(!SP_DynLightOwnerValid(c,1098,true,&state));state.index.primaryLight=7;state.number=1099;check(!SP_DynLightOwnerValid(c,1098,true,&state));state.number=1098;state.eType=ET_GENERAL;check(!SP_DynLightOwnerValid(c,1098,true,&state));
 cm.num_constraints=1;cm.constraints=&c;
 for(int fault=0;fault<11;++fault){
  s_clientEntityRegistry={};s_savedDynBindings.assign(1,SPDynJointBinding{});s_savedDynBindings[0].centities[0]=1098;
  auto &slot=s_clientEntityRegistry.slots[74];slot.used=true;slot.type=ET_PRIMARY_LIGHT;slot.currentUse=3;slot.nextUse=5;
  s_clientNetworkState.entities.assign(1,SPClientNetworkEntity{});auto &owner=s_clientNetworkState.entities[0];owner.index=1098;owner.current.useCount=3;owner.next.number=1098;owner.next.eType=ET_PRIMARY_LIGHT;owner.next.lerp.useCount=5;owner.next.index.primaryLight=7;
  if(fault==1)slot.used=false;if(fault==2)slot.type=ET_GENERAL;if(fault==3)s_clientNetworkState.entities.clear();if(fault==4)owner.kind=1;if(fault==5)owner.next.number=1099;if(fault==6)owner.next.index.primaryLight=8;if(fault==7)slot.currentUse=4;if(fault==8)slot.nextUse=6;if(fault==9)s_savedDynBindings[0].centities[0]=1536;
  bool accepted=true;try{SP_PreflightClientDynLights(fault==10?7:14);}catch(const std::runtime_error&){accepted=false;}check(accepted==(fault==0));
 }
 printf("canonical light slot/identity/preflight %u checks PASS pointer %zu\n",checks,sizeof(void*));
}
'''
structs='\n'.join(extract(ms,k)+';' for k in ['struct SPClientModelPart','struct SPClientModelOwner','struct SPClientEntityRegistry'])+'\n'+extract(ns,'struct SPClientNetworkEntity')+';\n'+extract(js,'struct SPDynJointBinding')+';'
helpers=extract(js,'static bool SP_DynCentitySlotValid(')+'\n'+extract(js,'static bool SP_DynLightOwnerValid(')+'\n'+extract(ns,'static void SP_PreflightClientDynLights(')
code=code.replace('STRUCTS',structs).replace('HELPERS',helpers);(a.out/'fixture.cpp').write_text(code,encoding='utf-8');(a.out/'source-manifest.json').write_text(json.dumps({f:hashlib.sha256(Path(f).read_bytes()).hexdigest() for f in [js,ns,ms]},indent=2))
