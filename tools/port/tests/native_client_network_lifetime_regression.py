"""Exercise the actual CNS handoff/detach bodies against tracked string owners."""
from pathlib import Path
import argparse

p = argparse.ArgumentParser()
p.add_argument('--out', required=True)
a = p.parse_args()
root = Path(__file__).resolve().parents[3]
source = (root/'src/game_sp/g_sp_client_network_checkpoint.inl').read_text()

def body(name):
    start = source.index('static void '+name+'(')
    end = source.index('{', start) + 1
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

code = r'''
#include <vector>
#include <map>
#include <stdexcept>
#include <cassert>
#include <cstdio>
struct State { unsigned number=0, targetname=0; };
struct centity_s { State nextState; int value=0; };
struct SPClientNetworkEntity { int value=0; };
struct SPClientNetworkLease { centity_s *cent; SPClientNetworkEntity previous; unsigned acquiredTarget; };
struct CG { void *snap=nullptr, *nextSnap=nullptr; int predictedPlayerState=0; } cg;
struct Fake { unsigned flags=0; } cg_fakeEntitiesArray[512];
bool cg_fakeEntitiesInuseArray[512], s_previousFakeUsed[512];
unsigned s_previousFakeFlags[512];
int cg_fakeEntitiesInuseCount[1], cg_fakeEntitiesInuseCountFromMap, cg_fakeEntitiesInuseCountFromLoadScript;
int s_previousFakeCount, s_previousFakeMap, s_previousFakeScript;
int s_previousPredicted=0, s_previousNetworkClock=0;
void *s_previousSnap=nullptr, *s_previousNextSnap=nullptr;
bool s_clientNetworkAttached=false, s_clientNetworkHandedOff=false;
std::vector<SPClientNetworkLease> s_clientNetworkLeases;
std::map<unsigned,int> refs;
int rollbacks=0, releases=0;
constexpr int ERR_DROP=1, SCRIPTINSTANCE_CLIENT=1;
void Com_Error(int,const char*) { throw std::runtime_error("rejected"); }
bool EntNum_IsCsc(unsigned n) { return n>=1024 && n<1536; }
CG *CG_GetLocalClientGlobals(int) { return &cg; }
void SP_CNSApplyClock(int) { ++rollbacks; }
void SP_CNSApplyEntity(centity_s *c,const SPClientNetworkEntity &v) { ++rollbacks;c->value=v.value; }
void SL_RemoveRefToString(int,unsigned s) { assert(refs[s]>0);--refs[s];++releases; }
'''
code += body('SP_DetachClientNetwork') + '\n' + body('SP_HandoffClientNetwork')
code += '\n' + body('SP_PrepareClientNetworkRetirement')
code += r'''
int main() {
 centity_s csc{{1100,7},42}, wire{{12,8},43};
 refs[7]=refs[8]=1; s_clientNetworkAttached=true;
 s_clientNetworkLeases={{&csc,{1},7},{&wire,{2},8}};
 SP_HandoffClientNetwork();
 assert(s_clientNetworkLeases[0].acquiredTarget==0);
 assert(s_clientNetworkLeases[1].acquiredTarget==8);
 bool rejected=false;try {SP_HandoffClientNetwork();}catch(...) {rejected=true;}assert(rejected);
 // Canonical deletion or script assignment consumes the transferred CSC ref.
 SL_RemoveRefToString(1,csc.nextState.targetname);csc.nextState.targetname=0;
 csc.value=123;wire.value=456;cg.predictedPlayerState=789;
 SP_DetachClientNetwork();SP_DetachClientNetwork();
 assert(refs[7]==0 && refs[8]==0 && releases==2 && rollbacks==0);
 assert(csc.value==123 && wire.value==456 && cg.predictedPlayerState==789);
 assert(!s_clientNetworkAttached && !s_clientNetworkHandedOff && s_clientNetworkLeases.empty());
 // Bounded verification still rolls back and owns/releases both references.
 refs[7]=refs[8]=1;s_clientNetworkAttached=true;
 s_clientNetworkLeases={{&csc,{1},7},{&wire,{2},8}};
 SP_DetachClientNetwork();SP_DetachClientNetwork();
 assert(refs[7]==0 && refs[8]==0 && releases==4 && rollbacks==3);
 assert(csc.value==1 && wire.value==2);
 rejected=false;try {SP_HandoffClientNetwork();}catch(...) {rejected=true;}assert(rejected);
 // Publication fails after retained models but before first-frame handoff.
 // Canonical entity retirement consumes CSC fields before lease detachment.
 refs[7]=refs[8]=1;s_clientNetworkAttached=true;
 csc.nextState.targetname=7;
 s_clientNetworkLeases={{&csc,{1},7},{&wire,{2},8}};
 SP_PrepareClientNetworkRetirement();SP_PrepareClientNetworkRetirement();
 SL_RemoveRefToString(1,csc.nextState.targetname);csc.nextState.targetname=0;
 SP_DetachClientNetwork();SP_PrepareClientNetworkRetirement();
 assert(refs[7]==0 && refs[8]==0 && releases==6 && rollbacks==3);
 assert(!s_clientNetworkAttached && !s_clientNetworkHandedOff && s_clientNetworkLeases.empty());
 std::puts("actual CNS handoff/detach ownership, mutation/deletion, rollback and idempotence PASS");
}
'''
out = Path(a.out)
out.mkdir(parents=True, exist_ok=False)
(out/'fixture.cpp').write_text(code, encoding='utf-8')
