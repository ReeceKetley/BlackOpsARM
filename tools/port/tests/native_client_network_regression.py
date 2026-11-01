"""CNS1 actual typed codecs/native declarations; synthetic IO and registries.

Full snapshot publication/interpolation is separately verified by the APK.
"""
from pathlib import Path
import argparse, subprocess, sys, json, hashlib, re
root=Path(__file__).resolve().parents[3]
p=argparse.ArgumentParser();p.add_argument('--out',type=Path,required=True);a=p.parse_args()
subprocess.run([sys.executable,str(root/'tools/port/tests/native_client_entity_pose_regression.py'),'--out',str(a.out)],check=True)
source=root/'src/game_sp/g_sp_client_network_checkpoint.inl';codecs=root/'src/game_sp/g_sp_client_network_codecs.inl';s=source.read_text()
# Fail if the canonical player declaration gains an uncaptured field. Generic
# value serialization rejects pointers; the one objective pointer is explicit.
header=re.sub(r'//[^\n]*','',(root/'src/bgame/bg_local.h').read_text())
player=header[header.index('struct playerState_s'):];player=player[player.index('{')+1:];player=player[:player.index('struct')]
fields=set(re.findall(r'^\s*[\w\s]+?\s+(\w+)(?:\[\d+\])?;',player,re.M))
player_codec=codecs.read_text().split('static void SP_CNSPlayerCodec(')[1]
captured=set(re.findall(r'SP_CNSValue\(c, v\.(\w+)',player_codec))
assert fields==captured-{'hud'}|{'objective'},(fields-captured,captured-fields)
def extract(key):
 begin=s.index(key);end=s.index('{',begin)+1;depth=1
 while depth:depth+=(s[end]=='{')-(s[end]=='}');end+=1
 return s[begin:end]
helpers='#include <cgame_mp/cg_local_mp.h>\n'+codecs.read_text()+'\n'
helpers+=s[s.index('#define SP_CNS_CG_FIELDS'):s.index('struct SPClientNetworkClock')]
helpers+='\n'.join(extract(k)+';' for k in ['struct SPClientNetworkClock','struct SPClientSnapshotImage','struct SPClientNetworkEntity','struct SPClientNetworkState'])
helpers+='\n'+'\n'.join(extract(k) for k in ['static void SP_CNSSnapshotCodec(','static void SP_CNSNetworkEntityCodec(','static void SP_CNSClockCodec(','static void SP_ClientNetworkCodec('])
tests=r'''
 static SPClientNetworkState network{},networkDecoded{};
 network.clock.frameInterpolation=.5f;network.clock.time=90;network.clock.client.serverTime=95;
 network.snapshots.resize(1);auto &ns=network.snapshots[0];ns.value.serverTime=100;
 ns.value.numEntities=2;ns.value.entities[0].number=0;ns.value.entities[1].number=1537;
 ns.value.entities[1].lerp.pos.trBase[1]=37;ns.value.numClients=1;ns.value.clients[0].clientIndex=0;
 network.predicted.value.commandTime=95;network.predicted.value.origin[0]=123;
 network.predicted.value.objective[1].alt_3D_text=reinterpret_cast<const char *>(0xdeadbeef);
 network.predicted.textPresent[1]=true;network.predicted.text[1]="objective text";
 network.predicted.textPresent[2]=true;network.predicted.text[2]="";
 network.entities.resize(1);network.entities[0].kind=1;network.entities[0].target="script_target";
 network.entities[0].next.targetname=523;network.entities[0].next.weapon=17;
 network.entities[0].next.lerp.pos.trTime=500;network.entities[0].next.events[0]=23;
 network.entities[0].flags=0x1fffffff;network.entities[0].flagState=331;
 auto networkEncode=[&]{encoded={};SPNativeCheckpointCodec c{nullptr,&encoded};SP_ClientNetworkCodec(c,network);};
 auto networkDecode=[&]{input={};decoded={};input.buffer=encoded.cacheBuffer;input.bufferSize=encoded.cacheBufferUsed;networkDecoded={};SPNativeCheckpointCodec c{&input,&decoded};SP_ClientNetworkCodec(c,networkDecoded);};
 networkEncode();networkDecode();check(encoded.cacheBufferUsed==decoded.cacheBufferUsed&&!memcmp(encoded.cacheBuffer,decoded.cacheBuffer,encoded.cacheBufferUsed));
 check(networkDecoded.predicted.value.commandTime==95&&networkDecoded.predicted.value.origin[0]==123);
 check(!strcmp(networkDecoded.predicted.value.objective[1].alt_3D_text,"objective text"));
 check(networkDecoded.predicted.value.objective[2].alt_3D_text&&!*networkDecoded.predicted.value.objective[2].alt_3D_text);
 check(networkDecoded.predicted.value.objective[0].alt_3D_text==nullptr);
 check(networkDecoded.entities[0].next.targetname==0&&networkDecoded.entities[0].target=="script_target"&&networkDecoded.entities[0].next.events[0]==23);
 for(int bad:{-1,1,2}){network.current=bad;rejects(networkEncode);}network.current=0;
 for(int bad:{-1,1,2}){network.next=bad;rejects(networkEncode);}network.next=0;
 for(int bad:{-1,1025}){ns.value.numEntities=bad;rejects(networkEncode);}ns.value.numEntities=2;
 for(int bad:{-1,33}){ns.value.numClients=bad;rejects(networkEncode);}ns.value.numClients=1;
 for(int bad:{0,1024,ENTNUM_CSC_END,MAX_CL_ENTNUM}){ns.value.entities[1].number=bad;rejects(networkEncode);}ns.value.entities[1].number=1537;
 ns.value.clients[0].clientIndex=32;rejects(networkEncode);ns.value.clients[0].clientIndex=0;
 ns.targets[1]="invalid script string";rejects(networkEncode);ns.targets[1].clear();
 network.predicted.value.clientNum=1;rejects(networkEncode);network.predicted.value.clientNum=0;
 network.predicted.text[1]=std::string(256,'x');rejects(networkEncode);network.predicted.text[1]="objective text";
 network.predicted.text[1]=std::string("a\0b",3);rejects(networkEncode);network.predicted.text[1]="objective text";
 ns.player.value.eFlags=0x4000;rejects(networkEncode);ns.player.value.eFlags=0;
 for(float *v:{&network.clock.frameInterpolation,&network.predicted.value.origin[1],&network.predicted.value.velocity[2],
 &network.predicted.value.fWeaponPosFrac,&network.predicted.value.objective[1].origin[2],&network.predicted.value.hud.current[3].x,
 &ns.value.entities[1].lerp.pos.trBase[2],&ns.value.clients[0].maxSprintTimeMultiplier}){
 float before=*v;for(float bad:{NAN,INFINITY,-INFINITY}){*v=bad;rejects(networkEncode);}*v=before;
 }
 network.entities[0].flags=0xe0000000;rejects(networkEncode);network.entities[0].flags=0x1fffffff;
 networkEncode();int networkLength=encoded.cacheBufferUsed;
 for(int cut=0;cut<networkLength;cut+=97){input={};decoded={};input.buffer=encoded.cacheBuffer;input.bufferSize=cut;networkDecoded={};SPNativeCheckpointCodec c{&input,&decoded};rejects([&]{SP_ClientNetworkCodec(c,networkDecoded);});}
 for(int cut=networkLength-32;cut<networkLength;++cut){input={};decoded={};input.buffer=encoded.cacheBuffer;input.bufferSize=cut;networkDecoded={};SPNativeCheckpointCodec c{&input,&decoded};rejects([&]{SP_ClientNetworkCodec(c,networkDecoded);});}
 networkDecode();check(networkDecoded.snapshots[0].value.entities[1].lerp.pos.trBase[1]==37&&networkDecoded.entities[0].flagState==331);
 // Two full snapshot bodies fit the fixture's canonical32KiB IO buffer; the
 // APK additionally exercises their sharing and live current/next publication.
 encoded={};{SPNativeCheckpointCodec c{nullptr,&encoded};SP_CNSSnapshotCodec(c,network.snapshots[0]);SP_CNSSnapshotCodec(c,network.snapshots[0]);}
 static SPClientSnapshotImage pairDecoded[2];input={};decoded={};input.buffer=encoded.cacheBuffer;input.bufferSize=encoded.cacheBufferUsed;
 {SPNativeCheckpointCodec c{&input,&decoded};SP_CNSSnapshotCodec(c,pairDecoded[0]);SP_CNSSnapshotCodec(c,pairDecoded[1]);}
 check(input.bytesUsed==encoded.cacheBufferUsed&&encoded.cacheBufferUsed==decoded.cacheBufferUsed&&!memcmp(encoded.cacheBuffer,decoded.cacheBuffer,encoded.cacheBufferUsed));
 static playerState_s objectiveSource{};char objectiveSourceText[]="owned text";objectiveSource.objective[0].alt_3D_text=objectiveSourceText;
 SPClientPlayerImage ownedObjective;SP_CNSReadPlayer(objectiveSource,ownedObjective);objectiveSourceText[0]='X';SP_CNSBindPlayer(ownedObjective);
 check(!strcmp(ownedObjective.value.objective[0].alt_3D_text,"owned text")&&ownedObjective.value.objective[1].alt_3D_text==nullptr);
 printf("actual CNS1 snapshot/entity/prediction codec PASS; ");
'''
f=a.out/'fixture.cpp';text=f.read_text().replace('static void check(bool b)',helpers+'\nstatic void check(bool b)')
text=text.replace(' printf("actual SDP2 mixer',tests+'\n printf("actual SDP2 mixer');f.write_text(text,encoding='utf-8')
manifest=json.loads((a.out/'source-manifest.json').read_text());manifest.extend({'file':str(p),'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} for p in [source,codecs])
(a.out/'source-manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
