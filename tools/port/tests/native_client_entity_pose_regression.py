"""CET1 actual typed codec with native declarations; fixture IO is synthetic.

Real pose slot publication and canonical consumer queries use the APK probe.
"""
from pathlib import Path
import argparse, subprocess, sys, json, hashlib
root = Path(__file__).resolve().parents[3]
p = argparse.ArgumentParser(); p.add_argument('--out', type=Path, required=True); a = p.parse_args()
subprocess.run([sys.executable, str(root/'tools/port/tests/native_client_animation_owners_regression.py'), '--out', str(a.out)], check=True)
source = root/'src/game_sp/g_sp_client_entity_checkpoint.inl'
s = source.read_text()
def extract(key):
    begin=s.index(key); end=s.index('{',begin)+1; depth=1
    while depth: depth+=(s[end]=='{')-(s[end]=='}'); end+=1
    return s[begin:end]
helpers='\n'.join(extract(k)+';' for k in ['struct SPClientEntityPose','struct SPClientEntityPoses'])
helpers+='\n'+'\n'.join(extract(k) for k in ['static bool SP_ClientPoseSlot(', 'static void SP_ClientMoverCodec(', 'static void SP_ClientEntityPoseCodec(', 'static void SP_ClientEntityPosesCodec(', 'static void SP_ApplyClientPoseScalars('])
helpers+='\n'+extract('struct SPClientPoseLease')+';\n'
helpers+=r'''
static std::vector<SPClientPoseLease> s_clientPoseLeases;
struct PoseFixtureFree { unsigned char *p; int size; scriptInstance_t instance; };
static std::vector<PoseFixtureFree> poseFixtureFrees;
void MT_Free(unsigned char *p,int size,scriptInstance_t instance) { poseFixtureFrees.push_back({p,size,instance}); }
'''
helpers+=extract('void G_SP_NotifyClientCheckpointMoverFree(')+'\n'
helpers+=extract('void G_SP_NotifyClientCheckpointLinkFree(')+'\n'
helpers+=extract('static void SP_DetachClientEntityPoses(')+'\n'
tests=r'''
 static SPClientEntityPoses poses{},posesDecoded{};poses.bits=11;poses.end=g_entNumCscEnd;
 poses.owners.resize(4);
 poses.owners[0].kind=1;poses.owners[0].number=0;
 poses.owners[1].index=poses.owners[1].number=1024;
 poses.owners[2].index=poses.owners[2].number=1025;
 poses.owners[3].index=poses.owners[3].number=1537;
 auto &po=poses.owners[2];po.linked=true;po.link.linkEnt=1024;po.link.linkTag=1;po.tag="tag_attach";
 po.link.axis[0][0]=po.link.axis[1][1]=po.link.axis[2][2]=1;
 po.link.angles_set=true;po.moving=true;po.type=ET_SCRIPTMOVER;po.poseType=6;
 po.origin[0]=123;po.angles[1]=270;po.wetness=.5f;po.burnTime=-1;po.currentUse=21;po.nextUse=22;
 po.mover.decelTime=2;po.mover.aDecelTime=3;po.mover.speed=4;po.mover.aSpeed=-5;po.mover.midTime=6;po.mover.aMidTime=7;
 po.mover.pos1[2]=8;po.mover.pos2[1]=9;po.mover.pos3[0]=10;po.mover.apos1[1]=11;po.mover.apos2[2]=12;po.mover.apos3[0]=13;
 auto poseEncode=[&]{encoded={};SPNativeCheckpointCodec c{nullptr,&encoded};SP_ClientEntityPosesCodec(c,poses);};
 auto poseDecode=[&]{input={};decoded={};input.buffer=encoded.cacheBuffer;input.bufferSize=encoded.cacheBufferUsed;posesDecoded={};SPNativeCheckpointCodec c{&input,&decoded};SP_ClientEntityPosesCodec(c,posesDecoded);};
 poseEncode();poseDecode();check(encoded.cacheBufferUsed==decoded.cacheBufferUsed&&!memcmp(encoded.cacheBuffer,decoded.cacheBuffer,encoded.cacheBufferUsed));
 check(posesDecoded.owners[2].mover.aSpeed==-5&&posesDecoded.owners[2].link.angles_set&&posesDecoded.owners[2].nextUse==22);
 for(int bad:{-1,0,10,12}){poses.bits=bad;rejects(poseEncode);}poses.bits=11;
 for(int bad:{-1,1024,1279,1536}){poses.end=bad;rejects(poseEncode);}poses.end=g_entNumCscEnd;
 auto old=po;
 for(int bad:{-1,2,100}){po.kind=bad;rejects(poseEncode);}po=old;
 for(int bad:{-1,ENTNUM_CSC_END,MAX_CL_ENTNUM,1279}){po.index=bad;rejects(poseEncode);}po=old;
 for(int bad:{-1,ENTNUM_CSC_END,MAX_CL_ENTNUM,1024}){po.number=bad;rejects(poseEncode);}po=old;
 po.type=static_cast<entityType_t>(-1);rejects(poseEncode);po=old;
 for(int bad:{-1,ENTNUM_CSC_END,MAX_CL_ENTNUM,1025}){po.link.linkEnt=bad;rejects(poseEncode);}po=old;
 for(int bad:{-2,0,2,65535}){po.link.linkTag=bad;rejects(poseEncode);}po=old;
 po.tag.clear();rejects(poseEncode);po=old;
 po.link.linkTag=0;po.tag.clear();poseEncode();poseDecode();check(posesDecoded.owners[2].link.linkTag==0);po=old;
 po.link.linkTag=-1;po.tag.clear();poseEncode();poseDecode();check(posesDecoded.owners[2].link.linkTag==-1);po=old;
 for(float *v:{&po.origin[0],&po.angles[1],&po.mins[2],&po.maxs[0],&po.wetness,&po.link.axis[3][2],&po.link.relative_angles[1],
 &po.mover.decelTime,&po.mover.aDecelTime,&po.mover.speed,&po.mover.aSpeed,&po.mover.midTime,&po.mover.aMidTime,
 &po.mover.pos1[0],&po.mover.pos2[1],&po.mover.pos3[2],&po.mover.apos1[0],&po.mover.apos2[1],&po.mover.apos3[2]}){
 float value=*v;for(float bad:{NAN,INFINITY,-INFINITY}){*v=bad;rejects(poseEncode);}*v=value;
 }
 poses.owners[1].linked=true;poses.owners[1].link.linkEnt=1025;rejects(poseEncode);poses.owners[1].linked=false;
 poses.owners.push_back(poses.owners[1]);rejects(poseEncode);poses.owners.pop_back();
 poses.owners[0].kind=0;rejects(poseEncode);poses.owners[0].kind=1;
 g_entNumBits=10;poses.bits=10;rejects(poseEncode);g_entNumBits=11;poses.bits=11;
 poseEncode();int poseLength=encoded.cacheBufferUsed;
 for(int cut=0;cut<poseLength;++cut){input={};decoded={};input.buffer=encoded.cacheBuffer;input.bufferSize=cut;posesDecoded={};SPNativeCheckpointCodec c{&input,&decoded};rejects([&]{SP_ClientEntityPosesCodec(c,posesDecoded);});}
 poseDecode();check(posesDecoded.owners[2].origin[0]==123&&posesDecoded.owners[3].index==1537);
 // Actual apply function must touch only its explicit logical fields. Native
 // owners, physics, caches, trajectory/union data and flags remain byte exact.
 centity_s poseCent{};memset(&poseCent,0x35,sizeof(poseCent));
 centity_s expectedCent=poseCent;
 expectedCent.nextState.number=po.number;expectedCent.currentState.useCount=po.currentUse;
 expectedCent.nextState.lerp.useCount=po.nextUse;expectedCent.nextState.eType=po.type;
 expectedCent.pose.eType=po.poseType;expectedCent.pose.eTypeUnion=po.poseUnion;
 memcpy(expectedCent.pose.origin,po.origin,sizeof(po.origin));memcpy(expectedCent.pose.angles,po.angles,sizeof(po.angles));
 memcpy(expectedCent.pose.absmin,po.mins,sizeof(po.mins));memcpy(expectedCent.pose.absmax,po.maxs,sizeof(po.maxs));
 expectedCent.pose.wetness=po.wetness;expectedCent.pose.startBurnTime=po.burnTime;
 SP_ApplyClientPoseScalars(&poseCent,po);check(!memcmp(&poseCent,&expectedCent,sizeof(poseCent)));
 // Actual cleanup body, with synthetic MT retirement, covers publication,
 // absent saved link, and allocation tracked before a later failure publishes.
 cLinkInfo_s priorLink{},newLink{};cgScriptMover_s newMover{};
 for(int phase=0;phase<3;++phase){
  poseFixtureFrees.clear();poseCent={};SPClientEntityPose before{};before.number=7;before.origin[0]=-37;
  SPClientPoseLease lease{};lease.cent=&poseCent;lease.before=before;lease.previousLink=&priorLink;
  if(phase!=1)lease.link=&newLink;
  if(phase==0){lease.mover=&newMover;poseCent.linkInfo=&newLink;poseCent.cScriptMover=&newMover;}
  SP_ApplyClientPoseScalars(&poseCent,po);s_clientPoseLeases.push_back(lease);SP_DetachClientEntityPoses();
  check(s_clientPoseLeases.empty()&&poseCent.linkInfo==&priorLink&&poseCent.cScriptMover==nullptr);
  check(poseCent.nextState.number==7&&poseCent.pose.origin[0]==-37);
  check(poseFixtureFrees.size()==(phase==0?2:phase==1?0:1));
  for(auto &freed:poseFixtureFrees)check(freed.p!=reinterpret_cast<unsigned char *>(&priorLink)&&freed.instance==SCRIPTINSTANCE_SERVER);
  if(phase!=1)check(poseFixtureFrees[0].size==sizeof(cLinkInfo_s));
  if(phase==0)check(poseFixtureFrees[1].size==sizeof(cgScriptMover_s));
  auto freeCount=poseFixtureFrees.size();SP_DetachClientEntityPoses();check(poseFixtureFrees.size()==freeCount);
 }
 // Canonical snapshot retirement ends the saved allocation's lease before
 // freeing it. Address reuse must not make a later detach free a new owner.
 for(int reuse=0;reuse<2;++reuse){
  poseFixtureFrees.clear();poseCent={};SPClientPoseLease lease{};lease.cent=&poseCent;
  lease.mover=&newMover;poseCent.cScriptMover=&newMover;s_clientPoseLeases.push_back(lease);
  cgScriptMover_s unrelated{};G_SP_NotifyClientCheckpointMoverFree(&unrelated);check(s_clientPoseLeases[0].mover==&newMover);
  G_SP_NotifyClientCheckpointMoverFree(&newMover);MT_Free((unsigned char*)&newMover,sizeof(newMover),SCRIPTINSTANCE_SERVER);poseCent.cScriptMover=nullptr;
  check(s_clientPoseLeases[0].mover==nullptr);
  if(reuse)poseCent.cScriptMover=&newMover;
  SP_DetachClientEntityPoses();check(poseFixtureFrees.size()==1);check(poseCent.cScriptMover==(reuse?&newMover:nullptr));
 }
 // A mover allocated before publication still belongs to abort cleanup.
 poseFixtureFrees.clear();poseCent={};SPClientPoseLease partial{};partial.cent=&poseCent;partial.mover=&newMover;s_clientPoseLeases.push_back(partial);SP_DetachClientEntityPoses();check(poseFixtureFrees.size()==1&&poseFixtureFrees[0].size==sizeof(newMover));
 printf("actual CET1 pose/link/mover codec and retirement PASS; ");
'''
f=a.out/'fixture.cpp';text=f.read_text();text=text.replace('static void check(bool b)',helpers+'\nstatic void check(bool b)')
text=text.replace(' printf("actual SDP2 mixer',tests+'\n printf("actual SDP2 mixer');f.write_text(text,encoding='utf-8')
manifest=json.loads((a.out/'source-manifest.json').read_text());manifest.append({'file':str(source),'sha256':hashlib.sha256(source.read_bytes()).hexdigest()})
(a.out/'source-manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
