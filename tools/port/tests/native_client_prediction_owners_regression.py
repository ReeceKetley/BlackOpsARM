"""CPC2/CPC1 actual codecs and binding helpers, with synthetic native owners/IO.

Actual controller components, collision tree and input queries run in the APK.
"""
from pathlib import Path
import argparse, subprocess, sys, json, hashlib
root=Path(__file__).resolve().parents[3]
p=argparse.ArgumentParser();p.add_argument('--out',type=Path,required=True);p.add_argument('--legacy-payload',type=Path);a=p.parse_args()
subprocess.run([sys.executable,str(root/'tools/port/tests/native_client_network_regression.py'),'--out',str(a.out)],check=True)
source=root/'src/game_sp/g_sp_client_prediction_owners.inl';payload=root/'src/game_sp/g_sp_client_pose_payload.inl';s=source.read_text()
def extract(key):
 begin=s.index(key);end=s.index('{',begin)+1;depth=1
 while depth:depth+=(s[end]=='{')-(s[end]=='}');end+=1
 return s[begin:end]
helpers='#include <algorithm>\n'+payload.read_text()+'\n'
if a.legacy_payload:
 helpers+='namespace LegacyCPC {\n'+a.legacy_payload.read_text().split('struct SPClientPredictionHistory')[0]+'\n}\n'
helpers+=extract('struct SPClientPredictionOwners')+';\n'
helpers+='\n'.join(extract(k) for k in ['static void SP_CPCHitsCodec(','static void SP_ClientPredictionOwnersCodec('])
helpers+=r'''
static cg_s cpcFixtureCg{};
static cgs_t cpcFixtureCgs{};
static cg_s *CPCFixtureGlobals(int){return &cpcFixtureCg;}
static cgs_t *CPCFixtureStatics(int){return &cpcFixtureCgs;}
#define CG_GetLocalClientGlobals CPCFixtureGlobals
#define CG_GetLocalClientStaticGlobals CPCFixtureStatics
'''
helpers+=extract('static clientControllers_t *SP_CPCControlOwner(')+'\n'+extract('static void SP_CPCApplyPose(')+'\n'
helpers+='#undef CG_GetLocalClientGlobals\n#undef CG_GetLocalClientStaticGlobals\n'
helpers+=r'''
static DObj *s_clientControllerScratch;
static DSkel s_previousControllerSkel;
static bool s_clientControllerScratchLocked;
static unsigned cpcFixtureUnlocks;
static void CPCFixtureUnlock(const DObj *) { ++cpcFixtureUnlocks; }
#define DObjUnlock CPCFixtureUnlock
'''
helpers+=extract('static void SP_RestoreClientControllerScratch(')+'\n#undef DObjUnlock\n'
tests=r'''
 static SPClientPredictionOwners components{},componentsDecoded{};
 components.poses.resize(7);auto &cp=components.poses;
 cp[0].kind=1;cp[0].index=0;cp[0].type=1;cp[0].control=0;
 cp[1].kind=2;cp[1].type=21;
 cp[2].index=5;cp[2].type=ET_MG42;cp[2].payload.turret.playerUsing=true;
 cp[2].payload.turret.viewAngles=reinterpret_cast<const float *>(0xdeadbeef);
 cp[3].index=6;cp[3].type=ET_VEHICLE;cp[3].payload.vehicle.wheelHeight[5]=31;
 cp[4].index=7;cp[4].type=ET_FX;cp[4].payload.fx.effect=0xfedc1234;
 cp[5].index=8;cp[5].type=ET_ACTOR;cp[5].payload.actor.height=15;cp[5].queried=true;cp[5].reference.quat[3]=1;
 cp[6].index=9;cp[6].type=ET_GENERAL;cp[6].payload.general.linked=true;cp[6].linked=true;cp[6].linkMaxs[0]=32;
 for(auto &o:cp)o.payloadKind=SP_CPCPayloadKind(o.type);
 cp[0].payload.player.control=reinterpret_cast<clientControllers_t *>(0x12345678);
 components.controls[0].tag_origin_offset[2]=7;components.controls[35].angles[5][2]=90;
 components.worldMins[0]=-100;components.worldMaxs[0]=100;components.worldHits={5,9};components.nearHits={9};
 components.history.number=129;components.history.commands.resize(128);components.history.archiveIndex=255;
 components.history.commands[0].forwardmove=-127;components.history.commands[127].weapon=17;
 components.history.commands[127].button_bits.array[1]=0x7ffff;
 components.history.archive[254].serverTime=777;components.history.archive[254].origin[0]=123;
 auto componentEncode=[&]{encoded={};SPNativeCheckpointCodec c{nullptr,&encoded};SP_ClientPredictionOwnersCodec(c,components);};
 auto componentDecode=[&]{input={};decoded={};input.buffer=encoded.cacheBuffer;input.bufferSize=encoded.cacheBufferUsed;componentsDecoded={};SPNativeCheckpointCodec c{&input,&decoded};SP_ClientPredictionOwnersCodec(c,componentsDecoded);};
 componentEncode();componentDecode();check(encoded.cacheBufferUsed==decoded.cacheBufferUsed&&!memcmp(encoded.cacheBuffer,decoded.cacheBuffer,encoded.cacheBufferUsed));
 check(componentsDecoded.poses[0].control==0&&componentsDecoded.poses[0].payload.player.control==nullptr);
 check(componentsDecoded.poses[2].payload.turret.playerUsing&&componentsDecoded.poses[2].payload.turret.viewAngles==nullptr);
 check(componentsDecoded.poses[3].payload.vehicle.wheelHeight[5]==31&&componentsDecoded.poses[4].payload.fx.effect==0xfedc1234);
 check(componentsDecoded.history.commands[0].forwardmove==-127&&componentsDecoded.history.commands[127].weapon==17&&componentsDecoded.history.archive[254].origin[0]==123);
 for(int bad:{-2,36,100}){cp[0].control=bad;rejects(componentEncode);}cp[0].control=0;
 for(int bad:{-1,3,100}){cp[0].kind=bad;rejects(componentEncode);}cp[0].kind=1;
 for(int bad:{-1,1,MAX_CL_ENTNUM}){cp[1].index=bad;rejects(componentEncode);}cp[1].index=0;
 cp[1].type=ET_GENERAL;rejects(componentEncode);cp[1].type=21;
 cp[0].linked=true;rejects(componentEncode);cp[0].linked=false;
 cp[6].linkMins[0]=33;rejects(componentEncode);cp[6].linkMins[0]=0;
 cp[5].type=ET_FX;rejects(componentEncode);cp[5].type=ET_ACTOR;
 for(float *v:{&components.controls[0].angles[1][2],&cp[0].payload.player.waterHeight,&cp[2].payload.turret.firingOffset,
 &cp[3].payload.vehicle.time,&cp[3].payload.vehicle.wheelHeight[4],&cp[5].payload.actor.height,&cp[5].reference.quat[3],
 &cp[6].payload.general.mins[0],&cp[6].linkMaxs[0],&components.history.commands[0].rollmove,&components.history.archive[0].origin[0]}){
 float old=*v;for(float bad:{NAN,INFINITY,-INFINITY}){*v=bad;rejects(componentEncode);}*v=old;
 }
 auto hits=components.worldHits;
 for(auto bad:std::vector<std::vector<int>>{{5,5},{9,5},{ENTNUM_CSC_END},{-1},{MAX_CL_ENTNUM}}){components.worldHits=bad;rejects(componentEncode);}components.worldHits=hits;
 for(int bad:{-1,0,0x7fffffff}){components.history.number=bad;rejects(componentEncode);}components.history.number=129;
 for(int bad:{-1,256}){components.history.archiveIndex=bad;rejects(componentEncode);}components.history.archiveIndex=255;
 components.history.commands[127].button_bits.array[1]=0x80000;rejects(componentEncode);components.history.commands[127].button_bits.array[1]=0x7ffff;
 componentEncode();int componentLength=encoded.cacheBufferUsed;
 for(int cut=0;cut<componentLength;cut+=71){input={};decoded={};input.buffer=encoded.cacheBuffer;input.bufferSize=cut;componentsDecoded={};SPNativeCheckpointCodec c{&input,&decoded};rejects([&]{SP_ClientPredictionOwnersCodec(c,componentsDecoded);});}
 componentDecode();
 // Actual binding helper targets the actual canonical native structures in a
 // synthetic context, proving aliases and pointer-free payload restoration.
 cpose_t applied{};SP_CPCApplyPose(&applied,componentsDecoded.poses[0]);check(applied.player.control==&cpcFixtureCg.bgs.clientinfo[0].control);
 componentsDecoded.poses[0].control=35;SP_CPCApplyPose(&applied,componentsDecoded.poses[0]);check(applied.player.control==&cpcFixtureCgs.corpseinfo[3].control);
 componentsDecoded.poses[0].control=-1;SP_CPCApplyPose(&applied,componentsDecoded.poses[0]);check(applied.player.control==nullptr);
 SP_CPCApplyPose(&applied,componentsDecoded.poses[2]);check(applied.turret.viewAngles==cpcFixtureCg.predictedPlayerState.viewangles&&applied.turret.playerUsing);
 SP_CPCApplyPose(&applied,componentsDecoded.poses[3]);check(applied.vehicle.wheelHeight[5]==31);
 SP_CPCApplyPose(&applied,componentsDecoded.poses[4]);check(applied.fx.effect==0xfedc1234);
 SP_CPCApplyPose(&applied,componentsDecoded.poses[5]);check(applied.actor.height==15);
 SP_CPCApplyPose(&applied,componentsDecoded.poses[1]);check(applied.eType==21&&applied.eTypeUnion==0);
 // Actual recoverable-error skeleton restoration, including an error before
 // lock publication and repeated cleanup. Only the unlock service is synthetic.
 DObj borrowedObj{};DObjAnimMat priorMat{};s_previousControllerSkel={};s_previousControllerSkel.mat=&priorMat;
 s_previousControllerSkel.timeStamp=43;s_previousControllerSkel.partBits.anim[0]=1;
 for(bool locked:{false,true}){
  borrowedObj.skel={};s_clientControllerScratch=&borrowedObj;s_clientControllerScratchLocked=locked;
  auto before=cpcFixtureUnlocks;SP_RestoreClientControllerScratch();
  check(s_clientControllerScratch==nullptr&&borrowedObj.skel.mat==&priorMat&&borrowedObj.skel.timeStamp==43&&borrowedObj.skel.partBits.anim[0]==1);
  check(cpcFixtureUnlocks==before+locked&&!s_clientControllerScratchLocked);
  SP_RestoreClientControllerScratch();check(cpcFixtureUnlocks==before+locked);
 }

 // Preserve the legacy CPC1 stream where its original selector was sufficient.
 encoded={};{SPNativeCheckpointCodec c{nullptr,&encoded};c.Int(0x31435043);
 for(auto &owner:components.controls)SP_CPCControllerCodec(c,owner);
 c.Int((int)cp.size());for(auto &o:cp)SP_CPCPoseCodec(c,o,true);
 SP_CNSValue(c,components.worldMins);SP_CNSValue(c,components.worldMaxs);
 SP_CNSValue(c,components.nearMins);SP_CNSValue(c,components.nearMaxs);
 SP_CPCHitsCodec(c,components.worldHits);SP_CPCHitsCodec(c,components.nearHits);SP_CPCHistoryCodec(c,components.history);}
 componentDecode();check(componentsDecoded.poses[4].payload.fx.effect==0xfedc1234);
 // Actual Five capture had pose type0/union0 but logical ET_FX. The UID's
 // low byte14 must remain a UID byte, never become a collision boolean.
 SPClientPoseComponent fakeFx{};fakeFx.index=1209;fakeFx.payload.fx.triggerTime=95178;
 fakeFx.payload.fx.effect=0x018b000e;fakeFx.payloadKind=SP_CPCOwnerPayloadKind(0,1209,0,ET_FX);
 check(fakeFx.payloadKind==CPC_FX&&fakeFx.type==0&&fakeFx.unionType==0);
 encoded={};{SPNativeCheckpointCodec c{nullptr,&encoded};SP_CPCPoseCodec(c,fakeFx);}
 input={};decoded={};input.buffer=encoded.cacheBuffer;input.bufferSize=encoded.cacheBufferUsed;
 SPClientPoseComponent fakeDecoded{};{SPNativeCheckpointCodec c{&input,&decoded};SP_CPCPoseCodec(c,fakeDecoded);}
 check(fakeDecoded.payloadKind==CPC_FX&&fakeDecoded.payload.fx.effect==0x018b000e&&fakeDecoded.payload.fx.triggerTime==95178);
 applied={};SP_CPCApplyPose(&applied,fakeDecoded);check(applied.fx.effect==0x018b000e&&applied.eType==0&&applied.eTypeUnion==0);
 check(SP_CPCOwnerPayloadKind(0,1210,0,ET_LOOP_FX)==CPC_FX);
 check(SP_CPCOwnerPayloadKind(0,1209,0,ET_GENERAL)==CPC_GENERAL);
 check(SP_CPCOwnerPayloadKind(0,8,0,ET_FX)==CPC_GENERAL);
 check(SP_CPCOwnerPayloadKind(1,0,0,ET_FX)==CPC_GENERAL);
 for(unsigned char bad:{(unsigned char)CPC_PLAYER,(unsigned char)CPC_ACTOR,(unsigned char)255}){
 fakeFx.payloadKind=bad;rejects([&]{SPNativeCheckpointCodec c{nullptr,&encoded};SP_CPCPoseCodec(c,fakeFx);});}
 fakeFx.payloadKind=CPC_FX;fakeFx.index=8;rejects([&]{SPNativeCheckpointCodec c{nullptr,&encoded};SP_CPCPoseCodec(c,fakeFx);});
 printf("actual CPC2/CPC1 pose/controller/collision/input codecs, fake FX UID and native borrower bindings PASS; ");
'''
if a.legacy_payload:
 tests+=r'''
 LegacyCPC::SPClientPoseComponent oldFake{};oldFake.index=1209;oldFake.payload.fx.triggerTime=95178;oldFake.payload.fx.effect=0x018b000e;
 rejects([&]{SPNativeCheckpointCodec c{nullptr,&encoded};LegacyCPC::SP_CPCPoseCodec(c,oldFake);});
 printf("frozen CPC1 Five bool14 rejection reproduced; ");
'''
f=a.out/'fixture.cpp';text=f.read_text().replace('static void check(bool b)',helpers+'\nstatic void check(bool b)')
text=text.replace(' printf("actual SDP2 mixer',tests+'\n printf("actual SDP2 mixer');f.write_text(text,encoding='utf-8')
manifest=json.loads((a.out/'source-manifest.json').read_text());manifest.extend({'file':str(p),'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} for p in [source,payload])
(a.out/'source-manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
