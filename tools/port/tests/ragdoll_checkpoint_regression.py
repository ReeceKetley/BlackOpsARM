"""Generate a fixture using the production validators/codecs and actual native types."""
from pathlib import Path
import argparse, hashlib, json

root = Path(__file__).resolve().parents[3]
p = argparse.ArgumentParser()
p.add_argument('--out', type=Path, required=True)
a = p.parse_args()
paths = ['src/physics/phys_checkpoint.inl', 'src/ragdoll/ragdoll_checkpoint.inl', 'src/game_sp/g_sp_client_checkpoint.inl']

def function(path, signature):
    text = (root/path).read_text(encoding='utf-8')
    start = text.index(signature)
    end = text.index('{', start) + 1
    depth = 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]

code = r'''
#include <ragdoll/ragdoll_checkpoint.h>
#include <cgame_mp/cg_local_mp.h>
#include <cgame_mp/cg_main_mp.h>
#include <universal/memfile.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <initializer_list>
#include <vector>
#include <string>
static int checks;
static void check(bool value) { if (!value) std::abort(); ++checks; }
void Com_Error(errorParm_t, const char *, ...) { throw std::runtime_error("rejected"); }
bool Assert_MyHandler(const char *,int,int,const char *,...) { std::abort(); }
void MemFile_ReadData(MemoryFile *f, int size, unsigned char *out) {
    if (size < 0 || size > f->bufferSize-f->bytesUsed) Com_Error(ERR_DROP,"truncated");
    memcpy(out,f->buffer+f->bytesUsed,size); f->bytesUsed+=size;
}
void MemFile_WriteDataInternal(MemoryFile *, unsigned, unsigned char *) { Com_Error(ERR_DROP,"unexpected cache flush"); }
void MemFile_WriteInt(MemoryFile *f,int value) { MemFile_WriteData(f,4,(unsigned char*)&value); }
static centity_s targetEntities[MAX_CL_ENTNUM];
static fake_centity_s fakeTargets[512];
centity_s *cg_entitiesArray[1]={targetEntities};
fake_centity_s *cg_fakeEntitiesArray=fakeTargets;
bool cg_fakeEntitiesInuseArray[512]{};
int cg_entArrayCount=MAX_CL_ENTNUM,g_entNumBits=11,g_entNumCscEnd=ENTNUM_CSC_END;
static int transitions, physicsCalls;
bool Ragdoll_BodyInUse(RagdollBody*b){return b && b->references>0;}
char Ragdoll_BodyNewState(RagdollBody*b,RagdollBodyState state){++transitions;b->state=state;return 1;}
void Phys_CaptureBodyState(int,PhysSavedBodyState*){++physicsCalls;std::abort();}
void Phys_ApplyBodyState(int,const PhysSavedBodyState*){++physicsCalls;std::abort();}
struct SPClientPhysicsOwner {int number=0,generation=0;std::string model;RagdollSavedState state{};};
static std::vector<SPClientPhysicsOwner> s_clientPhysicsOwners;
struct SPClientEntityPose {int kind,index,nextUse;};
static struct {int bits=11,end=ENTNUM_CSC_END;std::vector<SPClientEntityPose> owners;} s_clientEntityPoses;
struct Part {std::string model;};struct Model {int handle;std::vector<Part> parts;};
static struct {std::vector<Model> models;} s_clientEntityRegistry;
ACTUAL_FUNCTIONS
int main() {
    static unsigned char bytes[sizeof(RagdollSavedState)+4];
    static MemoryFile file;
    RagdollSavedState saved{};
    saved.state=BS_RUNNING; saved.numBones=14; saved.numLerpBones=9; saved.numJoints=28;
    saved.curOrientationBuffer=1; saved.launchCount=2; saved.gravityOverride=1; saved.gravity=-800;
    for(int i=0;i<14;++i) {
        saved.bones[i].parent=i?i-1:0; saved.bones[i].animBones[0]=i;
        saved.bones[i].length=3.5f+i; saved.bones[i].center[0]=1.75f+i*.5f;
        auto &body=saved.physics[i]; body.quaternion[3]=1;
        for(int j=0;j<3;++j) {body.origin[j]=100.25f+i*3+j; body.linearVelocity[j]=-30.f+i+j; body.angularVelocity[j]=.2f+i+j;}
        body.gravity[2]=-800;
    }
    check(Ragdoll_SavedStateValid(&saved));
    file.buffer=bytes; file.bufferSize=sizeof(bytes);
    Ragdoll_WriteState(&file,&saved); check(file.cacheBufferUsed==3152);
    memcpy(bytes,file.cacheBuffer,3152);
    file.bytesUsed=0; RagdollSavedState decoded{}; Ragdoll_ReadState(&file,&decoded);
    check(!memcmp(&saved,&decoded,sizeof(saved)));
    for(int length=0;length<3152;++length) {
        file.bytesUsed=0; file.bufferSize=length; bool rejected=false;
        try {Ragdoll_ReadState(&file,&decoded);} catch(const std::runtime_error&) {rejected=true;}
        check(rejected);
    }
    const auto invalid=[&](RagdollSavedState bad) {
        check(!Ragdoll_SavedStateValid(&bad)); file.bytesUsed=0;file.cacheBufferUsed=0;file.bufferSize=sizeof(bytes);bool rejected=false;
        try {Ragdoll_WriteState(&file,&bad);}catch(const std::runtime_error&){rejected=true;}
        check(rejected && file.bytesUsed==0 && file.cacheBufferUsed==0);
    };
    auto bad=saved; bad.numBones=15;invalid(bad);
    bad=saved;bad.numLerpBones=10;invalid(bad);
    bad=saved;bad.numJoints=29;invalid(bad);
    bad=saved;bad.curOrientationBuffer=2;invalid(bad);
    bad=saved;bad.state=BS_TUNNEL_TEST;invalid(bad);
    bad=saved;bad.bones[1].parent=1;invalid(bad);
    bad=saved;bad.bones[2].animBones[1]=-1;invalid(bad);
    bad=saved;bad.physics[0].quaternion[3]=0;invalid(bad);
    bad=saved;bad.physics[0].gravity[2]=std::numeric_limits<float>::infinity();invalid(bad);
    bad=saved;bad.physics[0].angularVelocity[0]=std::numeric_limits<float>::quiet_NaN();invalid(bad);
    bad=saved;bad.launchCount=3;invalid(bad);
    bad=saved;bad.launchHitLoc[0]=HITLOC_NUM;invalid(bad);
    bad=saved;bad.state=BS_IDLE; memset(bad.physics,0,sizeof(bad.physics));check(Ragdoll_SavedStateValid(&bad));
    // Real capture/apply bodies retain both partial animation samples and all
    // counters without calling a physics factory or advancing the state machine.
    for(auto state:{BS_DOBJ_WAIT,BS_VELOCITY_CAPTURE}) {
        RagdollBody original{};original.references=1;original.state=state;original.ragdollDef=1;
        original.numBones=12;original.numJoints=11;original.curOrientationBuffer=1;
        original.stateFrames=1;original.stateMsec=33;original.velCaptureMsec=17;
        original.boneOrientations[1][0].origin[0]=41.25f;
        original.boneOrientations[1][0].orientation[3]=1;
        for(int i=0;i<12;++i){original.bones[i].parentBone=state==BS_DOBJ_WAIT?0:(i?i-1:0);original.bones[i].animBones[0]=i;}
        RagdollSavedState partial{};check(Ragdoll_CaptureState(&original,&partial));
        RagdollBody fresh=original;fresh.state=BS_VELOCITY_CAPTURE;fresh.stateMsec=fresh.stateFrames=0;
        memset(fresh.boneOrientations,0,sizeof(fresh.boneOrientations));
        if(state==BS_DOBJ_WAIT)for(int i=0;i<12;++i)fresh.bones[i].parentBone=i?i-1:0;
        check(Ragdoll_ApplyState(&fresh,&partial));RagdollSavedState recaptured{};
        check(Ragdoll_CaptureState(&fresh,&recaptured));check(!memcmp(&partial,&recaptured,sizeof(partial)));
        check(!transitions && !physicsCalls);
        for(int fault=0;fault<9;++fault){auto broken=original;
            if(fault==0)broken.bones[13].rigidBody=123;
            if(fault==1)broken.joints[27].joint=123;
            if(fault==2)broken.joints[0].joint2=123;
            if(fault==3)broken.m_bpg=reinterpret_cast<broad_phase_group*>(1);
            if(fault==4)broken.m_list_bpcp=reinterpret_cast<broad_phase_collision_pair*>(1);
            if(fault==5)broken.rope_id=1;
            if(fault==6)broken.rbc_dist=reinterpret_cast<void*>(1);
            if(fault==7)broken.storedPhysObjId=123;
            if(fault==8)broken.debug_hang_point=reinterpret_cast<rigid_body_constraint_point*>(1);
            check(!Ragdoll_CaptureState(&broken,&recaptured));check(!Ragdoll_ApplyState(&broken,&partial));
        }
        partial.physics[13].origin[0]=1;check(!Ragdoll_SavedStateValid(&partial));
    }
    // Deferred attachment uses real canonical centity lookup and fixed logical
    // keys. No physics factory or invented constraint participates.
    RagdollBody attached{};attached.references=1;attached.ragdollDef=1;attached.dobj=13;
    attached.state=BS_VELOCITY_CAPTURE;attached.numBones=1;attached.bones[0].parentBone=0;
    attached.storedAttachHitLoc=HITLOC_HEAD;attached.storedAttachEnt=31;
    targetEntities[31].nextValid=1;targetEntities[31].nextState.lerp.useCount=7;
    RagdollSavedState attachment{};check(Ragdoll_CaptureState(&attached,&attachment));
    check(attachment.attachHitLoc==HITLOC_HEAD && attachment.attachEntity==31
        && attachment.attachTargetPresent==1 && attachment.attachGeneration==7);
    {auto bad=attachment;bad.attachHitLoc=HITLOC_NUM;invalid(bad);
     bad=attachment;bad.attachEntity=MAX_CL_ENTNUM;invalid(bad);
     bad=attachment;bad.attachTargetPresent=2;invalid(bad);
     bad=attachment;bad.attachGeneration=-1;invalid(bad);
     bad=attachment;bad.attachTargetPresent=0;invalid(bad);
     bad=attachment;bad.attachHitLoc=0;bad.attachEntity=0;invalid(bad);}
    file={};file.buffer=bytes;file.bufferSize=sizeof(bytes);Ragdoll_WriteState(&file,&attachment);
    check(file.cacheBufferUsed==sizeof(bytes));memcpy(bytes,file.cacheBuffer,sizeof(bytes));
    file.bytesUsed=0;Ragdoll_ReadState(&file,&decoded);check(!memcmp(&attachment,&decoded,sizeof(decoded)));
    for(int length=0;length<(int)sizeof(bytes);++length){file.bytesUsed=0;file.bufferSize=length;bool rejected=false;
        try{Ragdoll_ReadState(&file,&decoded);}catch(const std::runtime_error&){rejected=true;}check(rejected);}
    // Noncanonical RAG2 with an absent extension must not silently shrink to RAG1.
    memset(bytes+4+offsetof(RagdollSavedState,attachHitLoc),0,16);file.bytesUsed=0;file.bufferSize=sizeof(bytes);
    {bool rejected=false;try{Ragdoll_ReadState(&file,&decoded);}catch(const std::runtime_error&){rejected=true;}check(rejected);}
    auto freshAttachment=attached;freshAttachment.storedAttachHitLoc=HITLOC_NONE;freshAttachment.storedAttachEnt=0;
    targetEntities[31].nextState.lerp.useCount=8;check(!Ragdoll_ApplyState(&freshAttachment,&attachment));
    check(freshAttachment.storedAttachHitLoc==HITLOC_NONE && freshAttachment.storedAttachEnt==0);
    targetEntities[31].nextState.lerp.useCount=7;check(Ragdoll_ApplyState(&freshAttachment,&attachment));
    check(freshAttachment.storedAttachHitLoc==HITLOC_HEAD && freshAttachment.storedAttachEnt==31);
    targetEntities[31].nextValid=0;check(Ragdoll_CaptureState(&attached,&attachment));
    check(!attachment.attachTargetPresent && !attachment.attachGeneration);
    check(Ragdoll_ApplyState(&freshAttachment,&attachment));
    targetEntities[31].nextValid=1;check(!Ragdoll_ApplyState(&freshAttachment,&attachment));
    targetEntities[31].nextValid=0;
    for(int index:{-1,ENTNUM_CSC_END,MAX_CL_ENTNUM}){attached.storedAttachEnt=index;check(!Ragdoll_CaptureState(&attached,&attachment));}
    attached.storedAttachEnt=1537;g_entNumBits=10;check(!Ragdoll_CaptureState(&attached,&attachment));g_entNumBits=11;
    attached.storedAttachEnt=31;cg_entArrayCount=31;check(!Ragdoll_CaptureState(&attached,&attachment));cg_entArrayCount=MAX_CL_ENTNUM;
    attached.storedAttachEnt=1024;cg_fakeEntitiesInuseArray[0]=true;fakeTargets[0].cent.nextState.lerp.useCount=9;
    check(Ragdoll_CaptureState(&attached,&attachment));check(attachment.attachTargetPresent==1 && attachment.attachGeneration==9);
    cg_fakeEntitiesInuseArray[0]=false;
    check(!transitions && !physicsCalls);
    // Execute actual production identity preflight for absent-model wait,
    // including stale generations and wrong model/state/definition rejection.
    const auto preflight=[&](bool accept){bool rejected=false;try{SP_PreflightClientPhysics();}catch(const std::runtime_error&){rejected=true;}check(rejected!=accept);};
    SPClientPhysicsOwner owner;owner.number=13;owner.generation=4;owner.state.definition=1;owner.state.state=BS_DOBJ_WAIT;
    s_clientPhysicsOwners={owner};s_clientEntityPoses.owners={{0,13,4}};preflight(true);
    s_clientEntityRegistry.models={{13,{{"corpse"}}}};preflight(false);
    s_clientPhysicsOwners[0].model="corpse";preflight(true);
    s_clientPhysicsOwners[0].state.state=BS_VELOCITY_CAPTURE;preflight(true);
    s_clientPhysicsOwners[0].model="wrong";preflight(false);
    s_clientPhysicsOwners[0].model="corpse";s_clientEntityPoses.owners[0].nextUse=5;preflight(false);
    s_clientEntityPoses.owners[0].nextUse=4;s_clientEntityPoses.owners[0].kind=1;preflight(false);
    s_clientEntityPoses.owners[0].kind=0;s_clientPhysicsOwners[0].state.definition=0;preflight(false);
    s_clientPhysicsOwners[0].state.definition=1;s_clientPhysicsOwners[0].model="";s_clientEntityRegistry.models.clear();
    for(auto state:{BS_VELOCITY_CAPTURE,BS_RUNNING,BS_IDLE}){s_clientPhysicsOwners[0].state.state=state;preflight(false);}
    s_clientPhysicsOwners[0].state.state=BS_DOBJ_WAIT;s_clientEntityPoses.owners.clear();preflight(false);
    s_clientPhysicsOwners[0].state.attachHitLoc=HITLOC_HEAD;s_clientPhysicsOwners[0].state.attachEntity=31;
    s_clientPhysicsOwners[0].state.attachTargetPresent=1;s_clientPhysicsOwners[0].state.attachGeneration=7;
    s_clientEntityPoses.owners={{0,13,4},{0,31,7}};preflight(true);
    s_clientEntityPoses.owners[1].nextUse=8;preflight(false);
    s_clientEntityPoses.owners.resize(1);preflight(false);
    s_clientPhysicsOwners[0].state.attachTargetPresent=0;s_clientPhysicsOwners[0].state.attachGeneration=0;preflight(true);
    s_clientEntityPoses.owners.push_back({0,31,7});preflight(false);
    s_clientEntityPoses.owners.resize(1);s_clientPhysicsOwners[0].state.attachEntity=13;preflight(false);
    s_clientPhysicsOwners[0].state.attachEntity=1537;s_clientEntityPoses.bits=10;preflight(false);
    s_clientEntityPoses.bits=11;preflight(true);
    std::printf("actual ragdoll/physics checkpoint %i checks PASS, record %zu bytes, pointer %zu bytes\n",checks,sizeof(saved),sizeof(void*));
}
'''
functions = [function(paths[0], 'bool Phys_IsSavedBodyStateValid(')]
functions += [function(paths[1], name) for name in ['static bool Ragdoll_CheckpointPendingOwnershipClean(', 'static bool Ragdoll_CheckpointHasAttachment(', 'static bool Ragdoll_CheckpointAttachmentTarget(', 'static bool Ragdoll_SavedStateValid(', 'bool Ragdoll_CaptureState(', 'bool Ragdoll_ApplyState(', 'void Ragdoll_WriteState(', 'void Ragdoll_ReadState(']]
functions += [function(paths[2], 'static void SP_PreflightClientPhysics(')]
a.out.mkdir(parents=True, exist_ok=False)
(a.out/'fixture.cpp').write_text(code.replace('ACTUAL_FUNCTIONS', '\n'.join(functions)), encoding='utf-8')
(a.out/'source-manifest.json').write_text(json.dumps([{'file': x, 'sha256': hashlib.sha256((root/x).read_bytes()).hexdigest()} for x in paths + ['src/ragdoll/ragdoll_checkpoint.h','src/ragdoll/ragdoll.h','src/cgame_mp/cg_local_mp.h','src/cgame_mp/cg_main_mp.h']], indent=2), encoding='utf-8')
