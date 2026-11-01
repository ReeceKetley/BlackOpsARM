"""CAN2 owner-root/registry guards on actual codec and native declarations.

Fixture IO/assets/locks are synthetic. Native slot publication and canonical
script/playback queries are verified separately on the tablet.
"""
from pathlib import Path
import argparse, subprocess, sys, json, hashlib
root=Path(__file__).resolve().parents[3]
p=argparse.ArgumentParser();p.add_argument('--out',type=Path,required=True);a=p.parse_args()
subprocess.run([sys.executable,str(root/'tools/port/tests/native_client_animation_regression.py'),'--out',str(a.out)],check=True)
tests=r'''
 static SPClientAnimations ownerState{},ownerDecoded{};ownerState=anims;ownerState.schema=0x324E4143;
 ownerState.bindings[ENTNUM_CSC_END]=1;
 ownerState.trees.emplace_back();ownerState.trees[1].instance=SCRIPTINSTANCE_SERVER;
 auto &owners=ownerState.owners;
 owners.roots={{1,0,1},{2,0,1},{6,0,1},{4,MAX_ACTORS-1,2}};
 owners.registrations={{SCRIPTINSTANCE_CLIENT,0,1,0},{SCRIPTINSTANCE_SERVER,1,2,0}};
 owners.genericDefinition=1;owners.viewDefinition=1;owners.viewHandle=ENTNUM_CSC_END;owners.viewHasTree=128;
 owners.hands[0][0]=-1;owners.hands[1][1]=66;owners.partBits[0]=0x80000000u;
 auto ownerEncode=[&]{encoded={};SPNativeCheckpointCodec c{nullptr,&encoded};SP_ClientAnimationsCodec(c,ownerState);};
 auto ownerDecode=[&]{input={};decoded={};input.buffer=encoded.cacheBuffer;input.bufferSize=encoded.cacheBufferUsed;ownerDecoded={};SPNativeCheckpointCodec c{&input,&decoded};SP_ClientAnimationsCodec(c,ownerDecoded);};
 ownerEncode();ownerDecode();check(encoded.cacheBufferUsed==decoded.cacheBufferUsed&&!memcmp(encoded.cacheBuffer,decoded.cacheBuffer,encoded.cacheBufferUsed));
 check(ownerDecoded.schema==0x324E4143&&ownerDecoded.owners.roots.size()==4&&ownerDecoded.owners.registrations.size()==2);
 check(ownerDecoded.trees[1].instance==SCRIPTINSTANCE_SERVER&&ownerDecoded.trees[1].nodes.empty());
 check(ownerDecoded.owners.viewHandle==ENTNUM_CSC_END&&ownerDecoded.owners.hands[0][0]==-1&&ownerDecoded.owners.hands[1][1]==66&&ownerDecoded.owners.partBits[0]==0x80000000u);
 for(int bad:{-1,7,100}){owners.roots[0].kind=bad;rejects(ownerEncode);}owners.roots[0].kind=1;
 for(int bad:{-1,1,MAX_CL_ENTNUM}){owners.roots[0].index=bad;rejects(ownerEncode);}owners.roots[0].index=0;
 for(int bad:{0,-1,3}){owners.roots[0].tree=bad;rejects(ownerEncode);}owners.roots[0].tree=1;
 owners.roots.push_back(owners.roots[0]);rejects(ownerEncode);owners.roots.pop_back();
 auto oldRoot=owners.roots[0];
 for(auto key:std::vector<std::pair<int,int>>{{0,ENTNUM_CSC_END},{0,-1},{0,MAX_CL_ENTNUM},{2,32},{3,4},{4,MAX_ACTORS},{5,MAX_ACTOR_CORPSES_SP},{6,1}}){owners.roots[0]={key.first,key.second,1};rejects(ownerEncode);}owners.roots[0]=oldRoot;
 g_entNumBits=10;owners.roots[0]={0,1537,1};rejects(ownerEncode);g_entNumBits=11;owners.roots[0]=oldRoot;
 auto dormant=owners.roots.back();owners.roots.pop_back();rejects(ownerEncode);owners.roots.push_back(dormant);
 ownerState.bindings[5]=2;rejects(ownerEncode);ownerState.bindings[5]=0;
 auto reg=owners.registrations[0];
 for(int bad:{-1,2,100}){owners.registrations[0].instance=bad;rejects(ownerEncode);}owners.registrations[0]=reg;
 for(int bad:{-1,2}){owners.registrations[0].user=bad;rejects(ownerEncode);}owners.registrations[0]=reg;
 for(int bad:{-1,0,MAX_XANIMTREE_NUM}){owners.registrations[0].index=bad;rejects(ownerEncode);}owners.registrations[0]=reg;
 for(int bad:{-1,1}){owners.registrations[0].definition=bad;rejects(ownerEncode);}owners.registrations[0]=reg;
 owners.registrations.push_back(reg);rejects(ownerEncode);owners.registrations.pop_back();
 for(int bad:{-1,2}){owners.genericDefinition=bad;rejects(ownerEncode);}owners.genericDefinition=1;
 for(int bad:{-1,0,2}){owners.viewDefinition=bad;rejects(ownerEncode);}owners.viewDefinition=1;
 for(int bad:{-2,0,MAX_CL_ENTNUM}){owners.viewHandle=bad;rejects(ownerEncode);}owners.viewHandle=ENTNUM_CSC_END;
 ownerState.bindings[ENTNUM_CSC_END]=0;rejects(ownerEncode);ownerState.bindings[ENTNUM_CSC_END]=1;
 auto viewRoot=owners.roots[2];owners.roots.erase(owners.roots.begin()+2);rejects(ownerEncode);owners.roots.insert(owners.roots.begin()+2,viewRoot);
 ownerState.schema=0x314E4143;rejects(ownerEncode);ownerState.schema=0x324E4143;
 ownerEncode();int ownerLength=encoded.cacheBufferUsed;
 for(int cut=0;cut<ownerLength;++cut){input={};decoded={};input.buffer=encoded.cacheBuffer;input.bufferSize=cut;ownerDecoded={};SPNativeCheckpointCodec c{&input,&decoded};rejects([&]{SP_ClientAnimationsCodec(c,ownerDecoded);});}
 ownerDecode();check(ownerDecoded.owners.roots[0].tree==ownerDecoded.owners.roots[1].tree&&ownerDecoded.owners.roots[3].tree==2);
 printf("actual CAN2 owner roots/definition registrations PASS; ");
'''
f=a.out/'fixture.cpp';s=f.read_text();s=s.replace(' printf("actual SDP2 mixer',tests+'\n printf("actual SDP2 mixer');f.write_text(s,encoding='utf-8')
source=root/'src/game_sp/g_sp_client_animation_owners.inl'
manifest=json.loads((a.out/'source-manifest.json').read_text());manifest.append({'file':str(source),'sha256':hashlib.sha256(source.read_bytes()).hexdigest()})
(a.out/'source-manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
