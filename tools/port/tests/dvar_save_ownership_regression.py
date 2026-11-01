"""Exercise actual dvar backup/shutdown bodies with canonical dvar_s layout.

Only the interned string provider and lock boundary are isolated. A saved string
has its own reference even when its address equals a normal dvar string slot.
"""
from pathlib import Path
import argparse
import hashlib
import json

root = Path(__file__).resolve().parents[3]
p = argparse.ArgumentParser()
p.add_argument('--before', type=Path, required=True)
p.add_argument('--out', type=Path, required=True)
a = p.parse_args()
current = (root / 'src/universal/dvar.cpp').read_text(encoding='utf-8')

def function(source, signature):
    start = source.index(signature)
    brace = source.index('{', start)
    end, depth = brace + 1, 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

common = '\n'.join(function(current, sig) for sig in [
    'void __cdecl Dvar_FreeNameString(', 'bool __cdecl Dvar_ShouldFreeCurrentString(',
    'bool __cdecl Dvar_ShouldFreeLatchedString(', 'bool __cdecl Dvar_ShouldFreeResetString(',
    'void __cdecl Dvar_FreeString(', 'void Dvar_DiscardSavedValue('])
fixture = r'''
#include <universal/dvar.h>
#include <clientscript/cscr_main.h>
#include <platform/fast_critical_section.h>
#include <initializer_list>
#include <cstdio>
#include <cstdlib>
#include <cstring>
FastCriticalSection g_dvarCritSect;
dvar_s dvarPool[4096];
dvar_s *dvarHashTable[1024];
int dvarCount, dvar_modifiedFlags;
bool isDvarSystemActive;
const dvar_s *dvar_cheats, *dvar_restoreDvarsOnLive;
static const char *texts[]={nullptr,"american","other","third"};
static int refs[4],locks;
void Sys_LockWrite(FastCriticalSection*) { if(locks++) std::abort(); }
void Sys_UnlockWrite(FastCriticalSection*) { if(--locks) std::abort(); }
void FreeString(const char *str,int type,scriptInstance_t inst) {
    if(type!=11 || inst!=SCRIPTINSTANCE_SERVER) std::abort();
    for(int i=1;i<4;++i) if(str==texts[i]) { if(--refs[i]<0) std::abort(); return; }
    std::abort();
}
ACTUAL_COMMON
ACTUAL_SHUTDOWN
int main() {
    unsigned checks=0;
    for(int c=0;c<4;++c) for(int r=0;r<4;++r) for(int l=0;l<4;++l)
    for(int saved=0;saved<4;++saved) {
        // No override owns no saved value. A marked string backup is non-null.
        std::memset(dvarPool,0,sizeof(dvarPool)); std::memset(refs,0,sizeof(refs));
        auto &d=dvarPool[0]; d.type=DVAR_TYPE_STRING; dvarCount=1;
        d.current.string=texts[c]; d.reset.string=texts[r]; d.latched.string=texts[l];
        for(int i : {c,r,l}) if(i) refs[i]=1;
        d.loadedFromSaveGame=saved!=0; d.saved.string=texts[saved]; if(saved) ++refs[saved];
        Dvar_Shutdown();
        for(int i=1;i<4;++i) if(refs[i]) {
            std::fprintf(stderr,"LEAK current=%d reset=%d latched=%d saved=%d refs[%d]=%d\n",c,r,l,saved,i,refs[i]);
            return 1;
        }
        if(locks || dvarCount || d.loadedFromSaveGame) std::abort();
        ++checks;
    }
    // Replacing an override drops only the old backup's independent reference.
    auto &d=dvarPool[0]; std::memset(&d,0,sizeof(d)); std::memset(refs,0,sizeof(refs));
    d.type=DVAR_TYPE_STRING; d.current.string=d.saved.string=texts[1];
    d.loadedFromSaveGame=true; refs[1]=2;
    Dvar_DiscardSavedValue(&d);
    if(refs[1]!=1 || d.current.string!=texts[1] || d.loadedFromSaveGame || d.saved.string) std::abort();
    Dvar_DiscardSavedValue(&d); if(refs[1]!=1) std::abort(); ++checks;
    // Numeric unions must never be interpreted as owned string addresses.
    d.type=DVAR_TYPE_INT; d.loadedFromSaveGame=true; d.saved.integer=123;
    Dvar_DiscardSavedValue(&d); if(d.loadedFromSaveGame || d.saved.integer!=123) std::abort(); ++checks;
    std::printf("dvar save ownership %u PASS\n",checks);
}
'''
fixture = fixture.replace('ACTUAL_COMMON', common)
a.out.mkdir(parents=True, exist_ok=True)
for name, source in [('before', a.before.read_text(encoding='utf-8')), ('after', current)]:
    body = function(source, 'void __cdecl Dvar_Shutdown(')
    (a.out / (name + '.cpp')).write_text(fixture.replace('ACTUAL_SHUTDOWN', body), encoding='utf-8')
(a.out / 'source-manifest.json').write_text(json.dumps({
    'before_sha256': hashlib.sha256(a.before.read_bytes()).hexdigest(),
    'current_sha256': hashlib.sha256(current.encode()).hexdigest(),
    'canonical_header_sha256': hashlib.sha256((root / 'src/universal/dvar.h').read_bytes()).hexdigest()
}, indent=2), encoding='utf-8')
