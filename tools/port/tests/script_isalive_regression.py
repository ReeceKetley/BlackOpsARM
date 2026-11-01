"""Generate a fixture for the actual builtin body using canonical entity/actor types.

The original SP executable's 0x7eff90 is the behavior oracle. VM providers are
isolated here so invalid arguments and life/health disagreements are repeatable.
"""
from pathlib import Path
import hashlib
import json

root = Path(__file__).resolve().parents[3]
source = root / 'src/game_mp/g_scr_main_mp.cpp'
text = source.read_text(encoding='utf-8')
start = text.index('void GScr_IsAlive()')
brace = text.index('{', start)
depth, end = 1, brace + 1
while depth:
    depth += (text[end] == '{') - (text[end] == '}')
    end += 1
out = root / 'build-android-audit/script-isalive-regression'
out.mkdir(exist_ok=True)
# Canonical actor construction includes an unused collision visitor. Keep its
# actual constructor; fail explicitly if this fixture ever enters collision.
traverse = (root / 'src/physics/phys_traverse.cpp').read_text(encoding='utf-8')
ctor_start = traverse.index('colgeom_visitor_t::colgeom_visitor_t()')
ctor_end = traverse.index('\n}', ctor_start) + 2
fixture = r'''
#include <game/actor.h>
#include <clientscript/cscr_vm.h>
#include <qcommon/common.h>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
ACTUAL_VISITOR_CTOR
bool Assert_MyHandler(const char*, int, int, const char*, ...) { std::abort(); }
void Vec3Max(const float*, const float*, float*) { std::abort(); }
void StatMon_Warning(int, int, char*) { std::abort(); }
void colgeom_visitor_t::intersect_box(float*, float*, int) { std::abort(); }
static gentity_s entity{};
static actor_s actor{};
static int argType, pointerType, result, entityReads, pointerReads;
static bool sp;
int Scr_GetType(unsigned int index, scriptInstance_t inst) {
    if(index || inst != SCRIPTINSTANCE_SERVER) std::abort();
    return argType;
}
unsigned int Scr_GetPointerType(unsigned int index, scriptInstance_t inst) {
    if(index || inst != SCRIPTINSTANCE_SERVER) std::abort();
    ++pointerReads; return pointerType;
}
gentity_s *Scr_GetEntity(unsigned int index) {
    if(index) std::abort(); ++entityReads; return &entity;
}
void Scr_AddInt(int value, scriptInstance_t inst) {
    if(inst != SCRIPTINSTANCE_SERVER || result != -1) std::abort();
    result = value;
}
bool Com_IsSPLevel() { return sp; }
ACTUAL_BUILTIN
int main() {
    unsigned checks = 0;
    for(bool singlePlayer : {false, true})
    for(bool hasActor : {false, true})
    for(int health : {-1, 0, 1, 100})
    for(int physicsLife : {0, 1, 2}) {
        sp = singlePlayer; argType=1; pointerType=19;
        entity.actor = hasActor ? &actor : nullptr;
        entity.health=health; actor.Physics.bIsAlive=physicsLife;
        result=-1; entityReads=pointerReads=0;
        GScr_IsAlive();
        int expected = singlePlayer && hasActor ? physicsLife != 0 : health > 0;
        if(result != expected || entityReads != 1 || pointerReads != 1) {
            std::fprintf(stderr,"FAIL SP=%d actor=%d health=%d life=%d result=%d expected=%d\n",
                sp,hasActor,health,physicsLife,result,expected); return 1;
        }
        ++checks;
    }
    for(int type : {0, 2, 6, 19}) {
        argType=type; result=-1; entityReads=pointerReads=0;
        GScr_IsAlive();
        if(result || entityReads || pointerReads) return 2;
        ++checks;
    }
    for(int pointer : {14, 20, 21}) {
        argType=1; pointerType=pointer; result=-1; entityReads=pointerReads=0;
        GScr_IsAlive();
        if(result || entityReads || pointerReads != 1) return 3;
        ++checks;
    }
    std::printf("PASS actual GScr_IsAlive: %u cases, canonical entity/actor types\n",checks);
}
'''.replace('ACTUAL_BUILTIN', text[start:end]).replace('ACTUAL_VISITOR_CTOR', traverse[ctor_start:ctor_end])
(out / 'fixture.cpp').write_text(fixture, encoding='utf-8')
(out / 'source.json').write_text(json.dumps({'source': str(source.relative_to(root)),
    'sha256': hashlib.sha256(source.read_bytes()).hexdigest(),
    'visitor_source_sha256': hashlib.sha256((root / 'src/physics/phys_traverse.cpp').read_bytes()).hexdigest(),
    'fixture_sha256': hashlib.sha256((out / 'fixture.cpp').read_bytes()).hexdigest(),
    'oracle': 'retail SP BlackOps.exe 0x7eff90, preserved retail-isalive-server.txt'}, indent=2), encoding='utf-8')
print(out)
