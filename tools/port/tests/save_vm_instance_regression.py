"""Exercise actual save-pre table boundaries with differently sized VM owners."""
from pathlib import Path
import argparse
import hashlib
import json

root = Path(__file__).resolve().parents[3]
p = argparse.ArgumentParser()
p.add_argument('--out', type=Path, required=True)
p.add_argument('--source', type=Path, default=root / 'src/clientscript/cscr_save.cpp')
a = p.parse_args()
s = a.source.read_bytes().decode('cp1252')
start = s.index('void Scr_SavePre(')
end = s.index('{', start) + 1
depth = 1
while depth:
    depth += (s[end] == '{') - (s[end] == '}')
    end += 1
fixture = r'''
#include <clientscript/cscr_variable.h>
#include <clientscript/cscr_save.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <vector>
scrVarPub_t gScrVarPub[2]{};
scrVarGlob_t gScrVarGlob[2]{};
static scr_classStruct_t classes[2][CLASS_NUM_COUNT];
scr_classStruct_t *gScrClassMap[2]={classes[0],classes[1]};
unsigned int g_scrVarChildBegin[2]={0x10000,0x8000};
static std::vector<unsigned> seen;
void Scr_AddSaveObject(scriptInstance_t,unsigned id){seen.push_back(id);}
static void Scr_SaveRootStack(scriptInstance_t,const VariableStackBuffer*){std::abort();}
#define SAVE_OBJECT_LIMIT VARIABLELIST_PARENT_SIZE(SCRIPTINSTANCE_SERVER)
#define SAVE_CHILD_BEGIN VARIABLELIST_CHILD_BEGIN(SCRIPTINSTANCE_SERVER)
ACTUAL_PRE
int main(){
 unsigned checks=0;
 for(scriptInstance_t vm:{SCRIPTINSTANCE_SERVER,SCRIPTINSTANCE_CLIENT}){
  const unsigned capacity=VARIABLELIST_PARENT_SIZE(vm);
  // Backing allocation is deliberately large enough for the old wrong memset:
  // logical owner's end is guarded, so failure is deterministic, not heap UB.
  std::vector<unsigned short> forward(0x10000+64,0xA55A),reverse(0x10000+64,0xA55A);
  std::vector<VariableValueInternal> vars(0x10000+16);
  auto &pub=gScrVarPub[vm];pub.saveIdMap=forward.data();pub.saveIdMapRev=reverse.data();
  pub.gameId=3;pub.levelId=7;pub.animId=8;pub.timeArrayId=9;pub.pauseArrayId=10;pub.freeEntList=11;
  gScrVarGlob[vm].variableList=vars.data();
  vars[VARIABLELIST_CHILD_BEGIN(vm)+3].w.type=VAR_POINTER;
  vars[VARIABLELIST_CHILD_BEGIN(vm)+3].u.u.pointerValue=123;
  vars[0x10000+3].w.type=VAR_INTEGER; // wrong instance must not be read
  if(vm==SCRIPTINSTANCE_SERVER){vars[0x10000+3].w.type=VAR_POINTER;vars[0x10000+3].u.u.pointerValue=123;}
  seen.clear();Scr_SavePre(vm);
  for(unsigned i=0;i<capacity;++i){if(forward[i] || reverse[i])return 2;++checks;}
  for(unsigned i=capacity;i<forward.size();++i){
   if(forward[i]!=0xA55A || reverse[i]!=0xA55A){std::printf("wrong VM map boundary cleared at %u\n",i);return 1;}++checks;
  }
  if(seen.size()!=16 || seen.back()!=123){std::puts("wrong VM child table used for game root");return 1;}++checks;
 }
 std::printf("VM save table ownership %u checks PASS\n",checks);
}
'''
a.out.mkdir(parents=True, exist_ok=False)
(a.out / 'fixture.cpp').write_text(fixture.replace('ACTUAL_PRE', s[start:end]), encoding='utf-8')
(a.out / 'source-manifest.json').write_text(json.dumps({'source': str(a.source), 'sha256': hashlib.sha256(a.source.read_bytes()).hexdigest()}, indent=2), encoding='utf-8')
