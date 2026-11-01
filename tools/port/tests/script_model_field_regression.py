"""Generate the actual generic F_MODEL case regression using canonical entity storage.

The isolated model-name/VM sinks record the index. Other switch branches and
live gameplay are deliberately outside this fixture. Run before/after source
variants into separate output directories to preserve controlled evidence.
"""
from pathlib import Path
import argparse
import hashlib
import json

ROOT = Path(__file__).resolve().parents[3]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--source', type=Path, default=ROOT / 'src/game_mp/g_spawn_mp.cpp')
p.add_argument('--out', type=Path, default=ROOT / 'build-android-audit/script-model-field-regression/current')
a = p.parse_args()
text = a.source.read_text(encoding='utf-8')
start = text.index('void __cdecl GScr_GetGenericField(')
start = text.index('        case F_MODEL:', start)
end = text.index('        default:', start)
case = text[start:end]
TEMPLATE = '\n#include <game/actor.h>\n#include <cstdio>\n#include <cstdlib>\n#include <cstring>\n#include <cstddef>\n#include <initializer_list>\nstatic unsigned int result, lookups;\nunsigned int G_ModelName(unsigned int index) { ++lookups; if(index>=512) std::abort(); return index+1568; }\nvoid Scr_AddConstString(unsigned int value, scriptInstance_t) { result=value; }\nvoid ReadModel(unsigned char *b,int ofs) {\n    unsigned int v6;\n    switch(0) {\n    case 0:\nACTUAL_MODEL_CASE\n    default:std::abort();\n    }\n}\nint main() {\n    static_assert(sizeof(((gentity_s*)0)->model)==2);\n    unsigned checks=0;\n    gentity_s ent{};\n    for(unsigned int index=0;index<512;++index) {\n        ent.model=(unsigned short)index; result=0;lookups=0;\n        ReadModel((unsigned char*)&ent,offsetof(gentity_s,model));\n        if(result!=index+1568||lookups!=1) { std::fprintf(stderr,"FAIL canonical model index=%u result=%u expected=%u\\n",index,result,index+1568);return 1; }\n        ++checks;\n    }\n    for(int offset : {0,1,2,3,7}) for(unsigned short index : {0,1,255,256,257,511}) {\n        unsigned char bytes[16];std::memset(bytes,0xA5,sizeof(bytes));std::memcpy(bytes+offset,&index,2);\n        result=0;lookups=0;ReadModel(bytes,offset);\n        if(result!=index+1568||lookups!=1)return 2;\n        ++checks;\n    }\n    std::printf("PASS actual F_MODEL case: %u checks, canonical2byte field, full0..511range and unaligned poison neighbors\\n",checks);\n}\n'
fixture = TEMPLATE.replace('ACTUAL_MODEL_CASE', case.split(':', 1)[1].strip())
a.out.mkdir(parents=True, exist_ok=True)
(a.out / 'fixture.cpp').write_text(fixture, encoding='utf-8')
(a.out / 'actual-case.txt').write_text(case, encoding='utf-8')
(a.out / 'source.json').write_text(json.dumps({'source':str(a.source),
    'source_sha256':hashlib.sha256(a.source.read_bytes()).hexdigest(),
    'fixture_sha256':hashlib.sha256(fixture.encode('utf-8')).hexdigest(),
    'scope':'Actual F_MODEL branch; canonical 2-byte gentity model field, indices0..511, poisoned neighbors and unaligned buffers; isolated VM output/model name.'},indent=2)+'\n',encoding='utf-8')
print(a.out)
