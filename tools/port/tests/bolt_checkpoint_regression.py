"""Generate tests of production bolt borrowers, codec and durable preflight.

Uses canonical entity types. Memfile transport and other checkpoint registries
are isolated fixture services; this is not evidence of an in-game impact.
"""
from pathlib import Path
import argparse, hashlib, json

root = Path(__file__).resolve().parents[3]
p = argparse.ArgumentParser()
p.add_argument('--out', type=Path, required=True)
a = p.parse_args()
paths = ['src/cgame/cg_bolt_checkpoint.inl', 'src/cgame/cg_bolt.h',
         'src/game_sp/g_sp_client_bolt_checkpoint.inl']

def function(signature):
    text = (root / paths[2]).read_text(encoding='utf-8')
    start = text.index(signature)
    end = text.index('{', start) + 1
    depth = 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]

code = r'''
#include <cgame/cg_bolt.h>
#include <cgame_mp/cg_local_mp.h>
#include <cgame_mp/cg_main_mp.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <vector>
#include <limits>
static int checks;
static void check(bool value) { if (!value) std::abort(); ++checks; }
void Com_Error(errorParm_t,const char *,...) { throw std::runtime_error("rejected"); }
static fake_centity_s fakeEntities[512];
fake_centity_s *cg_fakeEntitiesArray = fakeEntities;
bool cg_fakeEntitiesInuseArray[512]{};
int g_entNumCscEnd = ENTNUM_CSC_END;
centity_s *g_BoltEnt[1][10]{};
unsigned g_BoltIndex[1]{};
#include <cgame/cg_bolt_checkpoint.inl>
// The codec's backing transport is isolated; bytes are bounded on every read.
struct SPNativeCheckpointCodec {
    std::vector<unsigned char> &bytes; bool read = false; size_t offset = 0;
    template<class T> void Field(T &v) {
        if (read) {
            if (bytes.size()-offset < sizeof(v)) Com_Error(ERR_DROP,"truncated");
            memcpy(&v,bytes.data()+offset,sizeof(v)); offset+=sizeof(v);
        } else { const auto *p = reinterpret_cast<const unsigned char *>(&v); bytes.insert(bytes.end(),p,p+sizeof(v)); }
    }
    int Int(int v) { Field(v); return v; }
};
// Minimal registry views around real entity/network types; production preflight
// below checks every field it consumes, with no live owner services substituted.
struct SPClientEntityPose { int kind=0,index=0,currentUse=0,nextUse=0; entityType_t type=ET_SCRIPTMOVER; bool linked=false,moving=false; };
struct SPClientNetworkEntity { int kind=0,index=0; LerpEntityState current{}; entityState_s next{}; };
static struct {struct Slot {bool used=false; int type=0,currentUse=0,nextUse=0;} slots[512]; int end=1536;} s_clientEntityRegistry;
static struct {int end=1536; std::vector<SPClientEntityPose> owners;} s_clientEntityPoses;
static struct {std::vector<SPClientNetworkEntity> entities;} s_clientNetworkState;
static CGBoltSavedState s_clientBolts;
ACTUAL_FUNCTIONS
static void reset() {
    memset(fakeEntities,0,sizeof(fakeEntities)); memset(cg_fakeEntitiesInuseArray,0,sizeof(cg_fakeEntitiesInuseArray));
    CG_ClearBoltReferences(0); g_entNumCscEnd=1536;
    s_clientEntityRegistry={}; s_clientEntityRegistry.end=1536;
    s_clientEntityPoses.owners.clear();s_clientEntityPoses.end=1536;s_clientNetworkState.entities.clear();
    for(int i=0;i<10;++i) {
        auto &cent=fakeEntities[i].cent;cg_fakeEntitiesInuseArray[i]=true;
        cent.nextState.number=1024+i;cent.nextState.eType=ET_SCRIPTMOVER;
        cent.currentState.useCount=1000+i;cent.nextState.lerp.useCount=1;
        cent.currentState.eFlags=cent.nextState.lerp.eFlags=0x20;
        cent.nextState.lerp.u.player.meleeWeapon=1023;g_BoltEnt[0][i]=&cent;
        s_clientEntityRegistry.slots[i]={true,ET_SCRIPTMOVER,1000+i,1};
        s_clientEntityPoses.owners.push_back({0,1024+i,1000+i,1});
        SPClientNetworkEntity net;net.index=1024+i;net.current=cent.currentState;net.next=cent.nextState;s_clientNetworkState.entities.push_back(net);
    }
    check(CG_CaptureBolts(0,s_clientBolts));
}
int main() {
    reset();check(sizeof(unsigned)==4);
    for (unsigned cursor : {0u,1u,9u,10u,11u,1000000017u,std::numeric_limits<unsigned>::max()}) {
        for(int rotation=0;rotation<10;++rotation) {
            for(int i=0;i<10;++i)g_BoltEnt[0][i]=&fakeEntities[(i+rotation)%10].cent;
            g_BoltIndex[0]=cursor;CGBoltSavedState saved;check(CG_CaptureBolts(0,saved));
            CG_ClearBoltReferences(0);check(CG_ApplyBolts(0,saved));check(g_BoltIndex[0]==cursor);
            check(g_BoltEnt[0][cursor%10]==&fakeEntities[(cursor%10+rotation)%10].cent);
            std::vector<unsigned char> bytes;SPNativeCheckpointCodec out{bytes};SP_ClientBoltsCodec(out,saved);check(bytes.size()==128);
            CGBoltSavedState decoded;SPNativeCheckpointCodec in{bytes,true};SP_ClientBoltsCodec(in,decoded);
            check(decoded.cursor==cursor);for(int i=0;i<10;++i)check(decoded.slots[i].number==saved.slots[i].number);
            for(size_t n=0;n<bytes.size();++n) {auto cut=bytes;cut.resize(n);SPNativeCheckpointCodec r{cut,true};bool rejected=false;try{SP_ClientBoltsCodec(r,decoded);}catch(...){rejected=true;}check(rejected);}
        }
    }
    reset();auto saved=s_clientBolts;
    const auto rejectAtomic=[&](const CGBoltSavedState &bad) {
        g_BoltIndex[0]=77;centity_s *before[10];memcpy(before,g_BoltEnt[0],sizeof(before));
        check(!CG_ApplyBolts(0,bad));check(g_BoltIndex[0]==77);check(!memcmp(before,g_BoltEnt[0],sizeof(before)));
    };
    auto bad=saved;bad.slots[9]=bad.slots[0];rejectAtomic(bad);
    for(int number : {-2,0,1023,1536,2048,2147483647}) {bad=saved;bad.slots[9].number=number;rejectAtomic(bad);}
    bad=saved;++bad.slots[9].currentUse;rejectAtomic(bad);bad=saved;++bad.slots[9].nextUse;rejectAtomic(bad);
    cg_fakeEntitiesInuseArray[9]=false;rejectAtomic(saved);cg_fakeEntitiesInuseArray[9]=true;
    fakeEntities[9].cent.nextState.eType=ET_GENERAL;rejectAtomic(saved);fakeEntities[9].cent.nextState.eType=ET_SCRIPTMOVER;
    g_entNumCscEnd=1033;rejectAtomic(saved);g_entNumCscEnd=1536;
    CGBoltSavedState output=saved;g_BoltEnt[0][9]=reinterpret_cast<centity_s *>(uintptr_t(1));
    check(!CG_CaptureBolts(0,output));check(output.slots[9].number==saved.slots[9].number);
    reset();g_BoltEnt[0][9]=g_BoltEnt[0][0];check(!CG_CaptureBolts(0,output));
    reset();g_BoltEnt[0][9]=nullptr;check(CG_CaptureBolts(0,output));check(output.slots[9].number==-1);
    check(CG_ApplyBolts(0,output));check(g_BoltEnt[0][9]==nullptr);
    bad=output;bad.slots[9].currentUse=1;rejectAtomic(bad);
    check(!CG_CaptureBolts(1,output));check(!CG_ApplyBolts(1,saved));
    const auto preflight=[&](bool legacy,bool accept) {bool rejected=false;try{SP_PreflightClientBolts(legacy);}catch(...){rejected=true;}check(rejected!=accept);};
    reset();preflight(false,true);preflight(true,true);
    s_clientBolts.cursor=19;preflight(false,true);
    s_clientEntityRegistry.slots[9].used=false;preflight(false,false);s_clientEntityRegistry.slots[9].used=true;
    ++s_clientEntityPoses.owners[9].currentUse;preflight(false,false);--s_clientEntityPoses.owners[9].currentUse;
    ++s_clientNetworkState.entities[9].next.lerp.useCount;preflight(false,false);--s_clientNetworkState.entities[9].next.lerp.useCount;
    s_clientEntityPoses.owners[9].moving=true;preflight(false,true);preflight(true,false);s_clientEntityPoses.owners[9].moving=false;
    auto &n=s_clientNetworkState.entities[9];n.next.lerp.eFlags2|=0x80000;preflight(false,true);preflight(true,false);n.next.lerp.eFlags2=0;
    n.next.index.brushmodel=7;preflight(true,false);n.next.index.brushmodel=0;
    n.current.eFlags=0;preflight(true,false);n.current.eFlags=0x20;
    n.next.lerp.u.player.meleeWeapon=1022;preflight(true,false);n.next.lerp.u.player.meleeWeapon=1023;
    s_clientEntityRegistry.end=1033;preflight(false,false);s_clientEntityRegistry.end=1536;
    s_clientBolts.slots[9]={};preflight(false,true);preflight(true,false);
    std::printf("production bolt checkpoint %i checks PASS, pointer %zu bytes\n",checks,sizeof(void*));
}
'''
functions = [function(x) for x in ['static void SP_ClientBoltsCodec(', 'static void SP_PreflightClientBolts(']]
a.out.mkdir(parents=True, exist_ok=False)
(a.out/'fixture.cpp').write_text(code.replace('ACTUAL_FUNCTIONS', '\n'.join(functions)),encoding='utf-8')
(a.out/'source-manifest.json').write_text(json.dumps([{'file':x,'sha256':hashlib.sha256((root/x).read_bytes()).hexdigest()} for x in paths],indent=2),encoding='utf-8')
