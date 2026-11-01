"""Canonical animation/DObj bodies and types; isolate the two VM string owners."""
from pathlib import Path
import argparse
import hashlib
import json

root = Path(__file__).resolve().parents[3]
p = argparse.ArgumentParser()
p.add_argument('--out', type=Path, required=True)
a = p.parse_args()

def body(path, signature):
    s = (root / path).read_bytes().decode('cp1252')
    start = s.index(signature)
    end = s.index('{', start) + 1
    depth = 1
    while depth:
        depth += (s[end] == '{') - (s[end] == '}')
        end += 1
    return s[start:end]

sources = [
    ('src/xanim/xanim.cpp', 'void __cdecl XAnimCloneAnimInfo('),
    ('src/xanim/xanim.cpp', 'void __cdecl XAnimClearServerNotify('),
    ('src/xanim/dobj.cpp', 'void __cdecl DObjSetTree('),
    ('src/game_sp/g_sp_render_probe.inl', 'static void SP_ClearServerNotificationsFromClientPose('),
]
fixture = r'''
#include <xanim/xanim.h>
#include <clientscript/cscr_stringlist.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
static XAnimInfo nodes[8];
static unsigned refs[2][128];
static unsigned checks;
static void check(bool ok){if(!ok)std::abort();++checks;}
bool Assert_MyHandler(const char*,int,int,const char*,...){std::abort();}
XAnimInfo *GetAnimInfo(int i){check(i>0 && i<8);return &nodes[i];}
void SL_AddRefToString(unsigned id,scriptInstance_t vm){check(id<128);++refs[vm][id];}
void SL_RemoveRefToString(scriptInstance_t vm,unsigned id){
 if(!refs[vm][id])throw std::runtime_error("wrong VM or missing owner");
 --refs[vm][id];
}
bool DObjIsServer(const DObj *obj){return (obj->flags&1)!=0 || obj->localClientIndex<0;}
void XAnimResetAnimMap(const DObj*,unsigned int){}
ACTUAL_BODIES
int main(){
 for(unsigned id=1;id<128;++id){
  std::memset(nodes,0,sizeof(nodes));std::memset(refs,0,sizeof(refs));
  XAnimInfo original{};original.animIndex=3;original.notifyName=id;
  original.notifyIndex=2;original.notifyChild=7;original.notifyType=1;
  original.state.currentAnimTime=.6f;original.state.oldTime=.55f;
  original.state.weight=.4f;original.state.goalWeight=.8f;original.state.rate=.9f;
  refs[SCRIPTINSTANCE_SERVER][id]=1;
  XAnimTree_s pose{};pose.inst=SCRIPTINSTANCE_SERVER;pose.children=1;
  nodes[1].animIndex=nodes[2].animIndex=3;
  XAnimCloneAnimInfo(&pose,&original,&nodes[1]);
  // This is the v21/v23 bug: DObj handoff changes the VM of copied notify IDs.
  DObj client{};client.localClientIndex=0;DObjSetTree(&client,&pose);
  check(pose.inst==SCRIPTINSTANCE_CLIENT && client.localTree==&pose);
  bool rejected=false;
  try{XAnimClearServerNotify(&nodes[1],(scriptInstance_t)pose.inst);}
  catch(const std::runtime_error&){rejected=true;}
  check(rejected && refs[SCRIPTINSTANCE_CLIENT][id]==0);
  // Reclaim the known SERVER-owned test clone, then exercise the repaired
  // conversion with both a child and a sibling and verify exact pose bytes.
  XAnimClearServerNotify(&nodes[1],SCRIPTINSTANCE_SERVER);
  pose.inst=SCRIPTINSTANCE_SERVER;
  for(int n=1;n<=3;++n){nodes[n].animIndex=3;XAnimCloneAnimInfo(&pose,&original,&nodes[n]);}
  nodes[1].children=2;nodes[2].next=3;
  SP_ClearServerNotificationsFromClientPose(&pose,pose.children);
  check(refs[SCRIPTINSTANCE_SERVER][id]==1 && original.notifyName==id);
  DObjSetTree(&client,&pose);
  for(int n=1;n<=3;++n){
   check(!std::memcmp(&nodes[n].state,&original.state,sizeof(XAnimState)));
   check(!nodes[n].notifyName && !nodes[n].notifyType && !nodes[n].notifyChild && nodes[n].notifyIndex==-1);
   XAnimClearServerNotify(&nodes[n],(scriptInstance_t)pose.inst);
  }
  check(refs[SCRIPTINSTANCE_SERVER][id]==1 && refs[SCRIPTINSTANCE_CLIENT][id]==0);
 }
 std::printf("corpse pose VM ownership %u checks PASS (original handoff rejected)\n",checks);
}
'''
a.out.mkdir(parents=True, exist_ok=False)
(a.out / 'fixture.cpp').write_text(fixture.replace('ACTUAL_BODIES', '\n'.join(body(*s) for s in sources)), encoding='utf-8')
(a.out / 'source-manifest.json').write_text(json.dumps([
    {'file': f, 'sha256': hashlib.sha256((root / f).read_bytes()).hexdigest()}
    for f in dict(sources)
], indent=2), encoding='utf-8')
