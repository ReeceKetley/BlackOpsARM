"""Generate actual SP/MP tag-query wrapper tests with canonical level/tag storage.

The lookup/VM providers are isolated so missing DObjs/tags, successful/cached
lookups and invalid inputs are repeatable. This does not emulate DObj geometry.
Original SP server methods0x4a8330/0x500030 are retained disassembly oracles.
"""
from pathlib import Path
import argparse, hashlib, json
ROOT = Path(__file__).resolve().parents[3]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--source', type=Path, default=ROOT/'src/game_mp/g_scr_main_mp.cpp')
p.add_argument('--out', type=Path, required=True)
a=p.parse_args(); text=a.source.read_text(encoding='utf-8')
def body(name):
    start=text.index('void __cdecl '+name+'('); brace=text.index('{',start); end=brace+1; depth=1
    while depth:
        depth+=(text[end]=='{')-(text[end]=='}'); end+=1
    return text[start:end]
fixture=r"""
#include <game_mp/g_main_mp.h>
#include <game_mp/g_scr_main_mp.h>
#include <clientscript/cscr_vm.h>
#include <qcommon/common.h>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
level_locals_t level{};
static gentity_s entity{};
static bool sp, invalidTag;
static int mode, outputs, conversions, lookups, errorFlag;
static float output[3];
struct VMError {};
bool Com_IsSPLevel() { return sp; }
gentity_s *GetEntity(scr_entref_t ref) {
    if(ref.classnum || ref.entnum) throw VMError{};
    return &entity;
}
unsigned int Scr_GetConstLowercaseString(unsigned int index,scriptInstance_t inst) {
    if(index || inst!=SCRIPTINSTANCE_SERVER) std::abort();
    if(invalidTag)throw VMError{};
    return 123;
}
int GScr_UpdateTagInternal(gentity_s *ent,unsigned int name,cached_tag_mat_t *cache,int showError) {
    if(ent!=&entity || name!=123 || cache!=&level.cachedTagMat)std::abort();
    ++lookups;errorFlag=showError;
    if(mode>=2) { if(showError)throw VMError{};return 0; }
    for(int row=0;row<4;++row)for(int col=0;col<3;++col)cache->tagMat[row][col]=float(10*row+col);
    return 1;
}
void G_SP_TestPlanTagQueryMissing(const gentity_s*,unsigned int,bool) {}
void Scr_AddVector(float *value,scriptInstance_t inst) {
    if(inst!=SCRIPTINSTANCE_SERVER || outputs++)std::abort();
    for(int i=0;i<3;++i)output[i]=value[i];
}
void AxisToAngles(const float (*axis)[3],float *angles) {
    if(axis!=level.cachedTagMat.tagMat)std::abort();
    ++conversions;for(int i=0;i<3;++i)angles[i]=axis[0][i]+100;
}
ACTUAL_ORIGIN
ACTUAL_ANGLES
int main() {
    unsigned checks=0;
    for(bool singlePlayer : {false,true})for(bool angle : {false,true})for(int state : {0,1,2,3}) {
        sp=singlePlayer;mode=state;invalidTag=false;outputs=conversions=lookups=0;errorFlag=-1;
        for(auto &row:level.cachedTagMat.tagMat)for(float &v:row)v=-999;
        bool error=false;
        try { if(angle)GScr_GetTagAngles(scr_entref_t(0));else GScr_GetTagOrigin(scr_entref_t(0)); }catch(VMError&){error=true;}
        const bool success=state<2;
        if(error!=(!singlePlayer&&!success)||outputs!=(success?1:0)||conversions!=(success&&angle?1:0)
            || lookups!=1 || (state>=2 && errorFlag!=(singlePlayer?0:1))) {
            std::fprintf(stderr,"FAIL SP=%d angles=%d state=%d error=%d outputs=%d conversions=%d flag=%d\n",sp,angle,state,error,outputs,conversions,errorFlag);return 1;
        }
        if(success)for(int i=0;i<3;++i)if(output[i]!=(angle?100+i:30+i))return 2;
        ++checks;
    }
    for(bool singlePlayer : {false,true})for(bool angle : {false,true})for(bool badEntity : {false,true}) {
        sp=singlePlayer;invalidTag=!badEntity;outputs=conversions=lookups=0;bool error=false;
        try { scr_entref_t ref(0);ref.entnum=badEntity?1:0;if(angle)GScr_GetTagAngles(ref);else GScr_GetTagOrigin(ref); }catch(VMError&){error=true;}
        if(!error||outputs||lookups||conversions)return 3;++checks;
    }
    std::printf("PASS actual SP/MP tag wrappers: %u canonical-storage cases; undefined on SP missing lookup, MP/input errors retained\n",checks);
}
"""
fixture=fixture.replace('ACTUAL_ORIGIN',body('GScr_GetTagOrigin')).replace('ACTUAL_ANGLES',body('GScr_GetTagAngles'))
a.out.mkdir(parents=True,exist_ok=True)
(a.out/'fixture.cpp').write_text(fixture,encoding='utf-8')
(a.out/'source.json').write_text(json.dumps({'source':str(a.source),'source_sha256':hashlib.sha256(a.source.read_bytes()).hexdigest(),'fixture_sha256':hashlib.sha256(fixture.encode()).hexdigest(),'oracle':'Original SP gettagorigin0x4a8330/gettagangles0x500030; missing returns with no VM output. MP wrapper behavior retained.','scope':'Actual wrapper bodies, canonical entity/level/tag cache, isolated lookup/VM/angle providers.'},indent=2)+'\n',encoding='utf-8')
print(a.out)
