"""CPM1 actual typed replay record; IO/assets and movement execution are separate.

Actual Pmove/collision/services must additionally pass installed APK tests.
"""
from pathlib import Path
import argparse,subprocess,sys,json,hashlib,re
root=Path(__file__).resolve().parents[3]
p=argparse.ArgumentParser();p.add_argument('--out',type=Path,required=True);a=p.parse_args()
subprocess.run([sys.executable,str(root/'tools/port/tests/native_client_character_regression.py'),'--out',str(a.out)],check=True)
source=root/'src/game_sp/g_sp_client_movement_checkpoint.inl';text=source.read_text()
helpers='#include <cgame_mp/cg_predict_mp.h>\n'+text[:text.index('static void SP_CPMCheckWeapons')]
begin=text.index('static void SP_AbortClientMovement()');end=text.index('static bool SP_CPMReadPendingCommand',begin)
helpers+=r'''
namespace movement_cleanup_fixture {
static int epilogs,destroys,resets,mantles,dogs;static unsigned restoredSeed;
struct Visitor {void reset(){++resets;}};
struct Movement {const gjkcc_input_t *m_gjkcc_input{};playerState_s *ps{};Visitor proximity_data;} s_clientMovementPmove;
struct Database {void gjkcc_info_destroy(uintptr_t,bool){++destroys;}} g_gjkcc_info_client_database;
static bgs_t *bgs,*s_previousMovementBgs;
static clientInfo_t s_previousMovementClients[36];static int s_previousMovementProne[32];
static std::vector<float> s_previousMovementAnimRates;
static unsigned s_previousMovementSeed;static bool s_clientMovementActive,s_clientMovementServices;
static clientInfo_t *SP_CRCClientSlot(int i){return i<32?&cpcFixtureCg.bgs.clientinfo[i]:&cpcFixtureCgs.corpseinfo[i-32];}
static cg_s *CG_GetLocalClientGlobals(int){return &cpcFixtureCg;}
static void setup_gjkcc_input(Movement *,gjkcc_input_t *){}
static void gjkcc_epilog(gjkcc_input_t *,float *){++epilogs;}
static void Mantle_ShutdownAnims(){++mantles;}
static void Dog_ShutdownAnims(){++dogs;}
namespace bo1::android {static void CrtSrand(unsigned seed){restoredSeed=seed;}}
'''+text[begin:end]+'\n}\n'
begin=text.index('static bool SP_CPMReadPendingCommand');end=text.index('static void SP_CPMReplay',begin)
helpers+=r'''
namespace movement_pending_fixture {
static usercmd_s ring[128];static int current=256,missing=-1;
static bool CL_GetUserCmd(int,int n,usercmd_s *cmd){
 if(n<=current-128 || n>current || n==missing)return false;
 *cmd=ring[n-(current-127)];return true;
}
'''+text[begin:end]+'\n}\n'
decl=(root/'src/cgame_mp/cg_predict_mp.h').read_text().split('struct pmove_t',1)[1].split('pmove_t();',1)[0]
decl=re.sub(r'//[^\n]*','',decl)
for field in re.findall(r'^\s*(?:struct\s+)?[\w:]+(?:\s+\w+)?\s*\*?\s+(\w+)\s*(?:\[[^;]*\])?;',decl,re.M):
 assert re.search(r'\b'+field+r'\b',text), 'Unclassified pmove field '+field
tests=r'''
 static SPClientMovement movement{},movementDecoded{};
 movement.base.value.clientNum=movement.result.value.clientNum=0;
 movement.base.value.commandTime=100;movement.result.value.commandTime=132;movement.steps=2;
 movement.weapons={"defaultweapon","m1911_zm"};movement.seed=123;movement.finalSeed=456;
 movement.scalars.pitchHistory[29]=15;movement.scalars.tracemask=0x2818011;
 auto movementEncode=[&]{encoded={};SPNativeCheckpointCodec c{nullptr,&encoded};SP_ClientMovementCodec(c,movement);};
 auto movementDecode=[&]{input={};decoded={};input.buffer=crcFixtureBuffers[&encoded].data();input.bufferSize=encoded.cacheBufferUsed;movementDecoded={};SPNativeCheckpointCodec c{&input,&decoded};SP_ClientMovementCodec(c,movementDecoded);};
 movementEncode();auto movementBytes=crcFixtureBuffers[&encoded];movementDecode();check(movementBytes==crcFixtureBuffers[&decoded]);
 check(movementDecoded.steps==2&&movementDecoded.finalSeed==456&&movementDecoded.scalars.pitchHistory[29]==15);
 for(int bad:{0,129,-1}){movement.steps=bad;rejects(movementEncode);}movement.steps=2;
 for(int bad:{99,100,1101}){movement.result.value.commandTime=bad;rejects(movementEncode);}movement.result.value.commandTime=132;
 movement.result.value.clientNum=1;rejects(movementEncode);movement.result.value.clientNum=0;
 movement.scalars.numtouch=33;rejects(movementEncode);movement.scalars.numtouch=0;
 movement.scalars.numGlassTouch=-1;rejects(movementEncode);movement.scalars.numGlassTouch=0;
 movement.scalars.spDisabledStances=1;rejects(movementEncode);movement.scalars.spDisabledStances=0;
 movement.scalars.vehAnimState=static_cast<pmoveVehAnimState_t>(3);rejects(movementEncode);movement.scalars.vehAnimState=static_cast<pmoveVehAnimState_t>(0);
 movement.scalars.pitchHistory[29]=NAN;rejects(movementEncode);movement.scalars.pitchHistory[29]=15;
 movement.weapons.clear();rejects(movementEncode);movement.weapons={"defaultweapon","m1911_zm"};
 movement.weapons[0]="";rejects(movementEncode);movement.weapons[0]="defaultweapon";
 movementEncode();auto allMovementBytes=crcFixtureBuffers[&encoded];
 for(size_t cut:{size_t(0),size_t(4),size_t(8),allMovementBytes.size()/2,allMovementBytes.size()-1}){
  crcFixtureBuffers[&encoded].assign(allMovementBytes.begin(),allMovementBytes.begin()+cut);encoded.cacheBufferUsed=cut;rejects(movementDecode);
 }
 movement.steps=0;movement.result=movement.base;movementEncode();movementDecode();check(movementDecoded.steps==0&&movementDecoded.result.value.commandTime==100);
 printf("actual CPM1 movement scalar/player/weapon/replay codec PASS; ");
 {
  using namespace movement_pending_fixture;
  for(auto &cmd:ring){cmd={};cmd.serverTime=133;cmd.weapon=4;cmd.button_bits.array[0]=7;}
  usercmd_s cmd{},previous{};int cursor=100,selected=0,first=-1;
  for(int n=129;n<=256;++n)if(SP_CPMReadPendingCommand(n,133,cursor,cmd,previous)){
   check(previous.serverTime==133 && previous.weapon==4 && previous.button_bits.array[0]==7);
   cursor=cmd.serverTime;++selected;if(first<0)first=n;
  }
  check(selected==1 && first==130 && cursor==133);
  check(!SP_CPMReadPendingCommand(130,133,133,cmd,previous));
  check(!SP_CPMReadPendingCommand(130,100,100,cmd,previous));
  missing=130;rejects([&]{SP_CPMReadPendingCommand(130,133,100,cmd,previous);});missing=-1;
  ring[1].serverTime=1101;rejects([&]{SP_CPMReadPendingCommand(130,1101,100,cmd,previous);});
  for(int i=0;i<128;++i)ring[i].serverTime=100+i;
  cursor=100;selected=0;
  for(int n=129;n<=256;++n)if(SP_CPMReadPendingCommand(n,227,cursor,cmd,previous)){cursor=cmd.serverTime;++selected;}
  check(selected==127 && cursor==227);
 }
 printf("actual CPM1 canonical pending selection/long-pause duplicate history PASS; ");
 {
  using namespace movement_cleanup_fixture;
  playerState_s state{};gjkcc_input_t input{};static bgs_t oldBgs;static bgsAnim_s rateOwner;
  cpcFixtureCg.bgs.animData=&rateOwner;
  for(bool inPmove:{false,true})for(bool services:{false,true}){
   epilogs=destroys=resets=mantles=dogs=0;
   s_clientMovementPmove.ps=&state;s_clientMovementPmove.m_gjkcc_input=inPmove?&input:nullptr;
   s_clientMovementActive=true;s_clientMovementServices=services;
   s_previousMovementAnimRates={1.25f};rateOwner.animScriptData.animations[0].forceAnimRate=-1;
   s_previousMovementBgs=&oldBgs;s_previousMovementSeed=12345;
   s_previousMovementClients[0].health=73;s_previousMovementClients[35].health=39;
   s_previousMovementProne[31]=421;cpcFixtureCg.bgs.proneTime[31]=0;
   SP_AbortClientMovement();
   check(!s_clientMovementActive&&!s_clientMovementServices&&!s_clientMovementPmove.ps&&!s_clientMovementPmove.m_gjkcc_input);
   check(rateOwner.animScriptData.animations[0].forceAnimRate==1.25f&&s_previousMovementAnimRates.empty());
   check(epilogs==int(inPmove)&&destroys==1&&resets==1&&mantles==int(services)&&dogs==int(services));
   check(restoredSeed==12345&&movement_cleanup_fixture::bgs==&oldBgs&&cpcFixtureCg.bgs.proneTime[31]==421);
   check(cpcFixtureCg.bgs.clientinfo[0].health==73&&cpcFixtureCgs.corpseinfo[3].health==39);
   SP_AbortClientMovement();check(destroys==1&&resets==1);
  }
 }
 printf("actual CPM1 abort/normal cleanup idempotence PASS (synthetic services); ");
'''
f=a.out/'fixture.cpp';s=f.read_text().replace('static void check(bool b)',helpers+'\nstatic void check(bool b)')
tests=tests.replace('MemFile_WriteData(', 'CRCFixtureWriteData(').replace('MemFile_WriteCString(', 'CRCFixtureWriteCString(')
# Actual codec calls use the same synthetic stream as the CCR1 phase.
begin=s.index('// CPM1:');end=s.index('static void check(bool b)',begin)
s=s[:begin]+s[begin:end].replace('MemFile_WriteData(', 'CRCFixtureWriteData(').replace('MemFile_WriteCString(', 'CRCFixtureWriteCString(')+s[end:]
s=s.replace(' printf("actual SDP2 mixer',tests+'\n printf("actual SDP2 mixer');f.write_text(s,encoding='utf-8')
manifest=json.loads((a.out/'source-manifest.json').read_text());manifest.extend({'file':str(p),'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} for p in [source,root/'src/cgame_mp/cg_predict_mp.h'])
(a.out/'source-manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
