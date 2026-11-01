"""CAN1 typed codec and actual free-chain query; synthetic IO/assets/locks.

Native XAnim definition/tree/node lifetime is exercised by the tablet probe.
"""
from pathlib import Path
import argparse, subprocess, sys, json, hashlib
root=Path(__file__).resolve().parents[3]
p=argparse.ArgumentParser();p.add_argument('--out',type=Path,required=True);a=p.parse_args()
subprocess.run([sys.executable,str(root/'tools/port/tests/native_client_models_regression.py'),'--out',str(a.out)],check=True)
source=root/'src/game_sp/g_sp_client_animation_checkpoint.inl'
pool=root/'src/xanim/xanim.cpp'

def extract(s,key):
    begin=s.index(key);end=s.index('{',begin)+1;depth=1
    while depth:depth+=(s[end]=='{')-(s[end]=='}');end+=1
    return s[begin:end]

s=source.read_text()
helpers=r'''
int g_xAnimInfoCount=64;
int g_entNumBits=11;
int g_maxActors=32;
static XAnimInfo fixtureNodePool[64]{};
#define g_xAnimInfo fixtureNodePool
static int lockBalance;
void Sys_EnterCriticalSection(CriticalSection){++lockBalance;}
void Sys_LeaveCriticalSection(CriticalSection){--lockBalance;}
'''
helpers+=extract(pool.read_text(),'int XAnimGetFreeInfoCount()')+'\n'
helpers+='\n'.join(extract(s,k)+';' for k in ['struct SPClientAnimEntry','struct SPClientAnimDefinition','struct SPClientAnimNode','struct SPClientAnimTree','struct SPClientAnimRoot','struct SPClientAnimRegistration','struct SPClientAnimOwners','struct SPClientAnimations'])
helpers+='\n'.join(extract(s,k) for k in ['static int SP_ClientAnimationCount(','static void SP_ClientAnimationDefinition(','static void SP_ClientAnimationTree(','static bool SP_ClientAnimRootIndex(','static void SP_ClientAnimOwnersCodec(','static void SP_ClientAnimationsCodec('])
tests=r'''
 fixturePart.name=const_cast<char *>("fixture_anim");fixturePart.boneCount[9]=2;
 static SPClientAnimations anims{},animDecoded{};
 anims.definitions.resize(1);auto &def=anims.definitions[0];def.entries.resize(2);def.params.resize(1);
 def.entries[0].count=1;def.entries[0].children=1;def.entries[0].first=1;def.entries[0].values=1;
 def.entries[1].created=1;def.entries[1].hasParts=true;def.entries[1].asset="fixture_anim";
 memcpy(def.params[0].strName,"sixteen-byte-key",16);def.params[0].fValue=2.5f;
 anims.trees.resize(1);auto &tree=anims.trees[0];tree.instance=SCRIPTINSTANCE_CLIENT;tree.nodes.resize(2);
 tree.nodes[1].parent=1;tree.nodes[1].anim=1;tree.nodes[1].notify="fire";tree.nodes[1].notifyType=1;
 tree.nodes[1].boneMap.resize(23);tree.nodes[1].boneMap[20]=2;tree.nodes[1].boneMap[21]=1;tree.nodes[1].boneMap[22]=159;
 anims.bindings[0]=anims.bindings[MAX_CL_ENTNUM-1]=1;
 auto animEncode=[&]{encoded={};SPNativeCheckpointCodec c{nullptr,&encoded};SP_ClientAnimationsCodec(c,anims);};
 auto animDecode=[&]{input={};decoded={};input.buffer=encoded.cacheBuffer;input.bufferSize=encoded.cacheBufferUsed;animDecoded={};SPNativeCheckpointCodec c{&input,&decoded};SP_ClientAnimationsCodec(c,animDecoded);};
 animEncode();animDecode();check(encoded.cacheBufferUsed==decoded.cacheBufferUsed&&!memcmp(encoded.cacheBuffer,decoded.cacheBuffer,encoded.cacheBufferUsed));
 check(animDecoded.bindings[0]==animDecoded.bindings[MAX_CL_ENTNUM-1]&&animDecoded.trees.size()==1);
 for(int bad:{-1,2}){anims.bindings[0]=bad;rejects(animEncode);}anims.bindings[0]=1;
 anims.bindings[0]=anims.bindings[MAX_CL_ENTNUM-1]=0;rejects(animEncode);anims.bindings[0]=anims.bindings[MAX_CL_ENTNUM-1]=1;
 for(int bad:{-1,1,10}){tree.definition=bad;rejects(animEncode);}tree.definition=0;
 tree.instance=SCRIPTINSTANCE_SERVER;rejects(animEncode);tree.instance=SCRIPTINSTANCE_CLIENT;
 for(int bad:{-1,0,2,100}){tree.nodes[1].parent=bad;rejects(animEncode);}tree.nodes[1].parent=1;
 for(int bad:{-1,2,65535}){tree.nodes[1].anim=bad;rejects(animEncode);}tree.nodes[1].anim=1;
 tree.nodes[1].notifyChild=2;rejects(animEncode);tree.nodes[1].notifyChild=0;
 tree.nodes[1].notifyIndex=-2;rejects(animEncode);tree.nodes[1].notifyIndex=-1;
 for(float *v:{&tree.nodes[0].state.currentAnimTime,&tree.nodes[1].state.oldTime,&tree.nodes[1].state.goalTime,&tree.nodes[1].state.goalWeight,&tree.nodes[1].state.weight,&tree.nodes[1].state.rate,&def.params[0].fValue}){
  float old=*v;for(float bad:{NAN,INFINITY,-INFINITY}){*v=bad;rejects(animEncode);}*v=old;
 }
 auto map=tree.nodes[1].boneMap;
 for(int size:{1,20,21,22,24,277}){tree.nodes[1].boneMap.resize(size);rejects(animEncode);tree.nodes[1].boneMap=map;}
 for(int bad:{160,254,255}){tree.nodes[1].boneMap[22]=bad;rejects(animEncode);}tree.nodes[1].boneMap=map;
 tree.nodes[0].boneMap=map;rejects(animEncode);tree.nodes[0].boneMap.clear();
 tree.nodes[1].boneMap.clear();animEncode();animDecode();check(animDecoded.trees[0].nodes[1].boneMap.empty());tree.nodes[1].boneMap=map;
 def.entries[1].asset="missing";rejects(animEncode);def.entries[1].asset="fixture_anim";
 def.entries[1].defaultAsset=true;rejects(animEncode);def.entries[1].defaultAsset=false;
 def.entries[0].children=0;rejects(animEncode);def.entries[0].children=1;
 def.entries[0].count=2;rejects(animEncode);def.entries[0].count=1;
 def.entries[1].parent=1;rejects(animEncode);def.entries[1].parent=0;
 def.entries[0].values=2;rejects(animEncode);def.entries[0].values=1;
 // Empty default leaf identity is distinct from an absent/uncreated entry.
 def.entries[1].asset.clear();def.entries[1].defaultAsset=true;animEncode();animDecode();check(animDecoded.definitions[0].entries[1].hasParts&&animDecoded.definitions[0].entries[1].asset.empty());
 def.entries[1].hasParts=false;rejects(animEncode);def.entries[1].hasParts=true;
 def.entries[1].asset="fixture_anim";def.entries[1].defaultAsset=false;
 animEncode();int animLength=encoded.cacheBufferUsed;
 for(int cut=0;cut<animLength;++cut){input={};decoded={};input.buffer=encoded.cacheBuffer;input.bufferSize=cut;SPNativeCheckpointCodec c{&input,&decoded};animDecoded={};rejects([&]{SP_ClientAnimationsCodec(c,animDecoded);});}
 // Actual free-chain query must release its lock even when rejecting corruption.
 fixtureNodePool[0].next=1;fixtureNodePool[1].next=2;fixtureNodePool[2].next=0;
 check(XAnimGetFreeInfoCount()==2&&lockBalance==0);
 fixtureNodePool[2].next=1;rejects([&]{XAnimGetFreeInfoCount();});check(lockBalance==0);
 fixtureNodePool[2].next=64;rejects([&]{XAnimGetFreeInfoCount();});check(lockBalance==0);
 fixtureNodePool[0].next=0;check(XAnimGetFreeInfoCount()==0&&lockBalance==0);
 printf("actual CAN1 animation codec and free-chain guards PASS; ");
'''
f=a.out/'fixture.cpp';text=f.read_text()
text=text.replace('#include <string>','#include <string>\n#include <xanim/xanim.h>\n#include <game_mp/actor_mp.h>\n#include <clientscript/cscr_animtree.h>\n#include <win32/win_common.h>\nstatic XAnimParts fixturePart{};\nstatic XAnimParts fixtureEmptyPart{};')
text=text.replace('XAssetHeader DB_FindXAssetHeader(XAssetType, char *name, bool, int)', 'XAssetHeader DB_FindXAssetHeader(XAssetType type, char *name, bool, int)')
text=text.replace('XAssetHeader h{}; if (!strcmp(name,"fixture_model")) h.model=&fixtureModel; return h;', 'XAssetHeader h{}; if(type==ASSET_TYPE_XANIMPARTS){fixtureEmptyPart.name=const_cast<char *>("");fixtureEmptyPart.boneCount[9]=2;if(!*name)h.parts=&fixtureEmptyPart;else if(!strcmp(name,"fixture_anim"))h.parts=&fixturePart;return h;} if (!strcmp(name,"fixture_model")) h.model=&fixtureModel; return h;')
text=text.replace('bool DB_IsXAssetDefault(XAssetType,const char *) { return false; }', 'bool DB_IsXAssetDefault(XAssetType type,const char *name) { return type==ASSET_TYPE_XANIMPARTS&&!*name; }')
text=text.replace('static void check(bool b)',helpers+'\nstatic void check(bool b)')
text=text.replace(' printf("actual SDP2 mixer',tests+'\n printf("actual SDP2 mixer')
f.write_text(text,encoding='utf-8')
manifest=json.loads((a.out/'source-manifest.json').read_text())
manifest.extend({'file':str(p),'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} for p in [source,pool])
(a.out/'source-manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
