#include "ragdoll_checkpoint.h"
#include <universal/memfile.h>
#include <cmath>
#include <cstring>
#include <cgame_mp/cg_local_mp.h>
#include <cgame_mp/cg_main_mp.h>

static bool Ragdoll_CheckpointPendingOwnershipClean(const RagdollBody *body)
{
    if (body->m_bpg || body->m_list_bpcp || body->rope_id || body->rbc_dist) return false;
    for (const auto &bone : body->bones) if (bone.rigidBody) return false;
    for (const auto &joint : body->joints) if (joint.joint || joint.joint2) return false;
    return true;
}

static bool Ragdoll_CheckpointHasAttachment(const RagdollSavedState *s)
{
    return s->attachHitLoc != HITLOC_NONE || s->attachEntity != 0;
}
static bool Ragdoll_CheckpointAttachmentTarget(int localClient, int entity, int *present, int *generation)
{
    if (localClient != 0 || (unsigned)entity >= MAX_CL_ENTNUM || entity == ENTNUM_CSC_END) return false;
    const bool isCsc = EntNum_IsCsc(entity);
    if (isCsc ? (entity < MAX_GENTITIES || entity >= g_entNumCscEnd || entity - MAX_GENTITIES >= 512)
              : entity >= cg_entArrayCount) return false;
    const auto *target = CG_GetEntity(localClient, entity);
    const bool fake = entity >= MAX_GENTITIES && entity - MAX_GENTITIES < 512
        && EntNum_IsCsc(entity) && cg_fakeEntitiesInuseArray[entity - MAX_GENTITIES];
    *present = fake || target->nextValid || target->linkInfo || target->cScriptMover;
    *generation = *present ? target->nextState.lerp.useCount : 0;
    return true;
}
static bool Ragdoll_SavedStateValid(const RagdollSavedState *s)
{
    if (!s || (s->state != BS_DOBJ_WAIT && s->state != BS_VELOCITY_CAPTURE
        && s->state != BS_RUNNING && s->state != BS_IDLE)
        || s->definition < 0 || s->definition >= RAGDOLL_MAX_DEFS
        || s->numBones < 1 || s->numBones > 14 || s->numLerpBones < 0 || s->numLerpBones > 9
        || s->numJoints < 0 || s->numJoints > 28 || (unsigned)s->curOrientationBuffer > 1
        || s->stateMsec < 0 || s->stateFrames < 0 || s->stableTime < 0
        || (unsigned)s->gravityOverride > 1 || (unsigned)s->launchCount > 2)
        return false;
    if ((unsigned)s->attachHitLoc >= HITLOC_NUM || (unsigned)s->attachTargetPresent > 1
        || s->attachGeneration < 0 || (!s->attachTargetPresent && s->attachGeneration)) return false;
    if (Ragdoll_CheckpointHasAttachment(s))
    {
        if ((unsigned)s->attachEntity >= MAX_CL_ENTNUM || s->attachEntity == ENTNUM_CSC_END) return false;
    }
    else if (s->attachTargetPresent || s->attachGeneration) return false;
    const auto finite = [](const float *v, int count) {
        for (int i = 0; i < count; ++i)
            if (!std::isfinite(v[i]) || std::fabs(v[i]) > 100000.0f)
                return false;
        return true;
    };
    if (!finite(s->poseOffset, 3))
        return false;
    const PhysSavedBodyState emptyPhysics{};
    for (int i = s->state == BS_RUNNING ? s->numBones : 0; i < 14; ++i)
        if (memcmp(&s->physics[i], &emptyPhysics, sizeof(emptyPhysics))) return false;
    for (int i = 0; i < s->numBones; ++i)
    {
        const RagdollSavedBone &bone = s->bones[i];
        if (bone.parent < -1 || bone.parent >= s->numBones || (bone.parent == i && i != 0)
            || (unsigned)bone.animBones[0] > 255 || (unsigned)bone.animBones[1] > 255
            || !std::isfinite(bone.length) || bone.length < 0 || bone.length > 10000
            || !finite(bone.center, 3))
            return false;
        if (s->state == BS_RUNNING && !Phys_IsSavedBodyStateValid(&s->physics[i]))
            return false;
    }
    for (int buffer = 0; buffer < 2; ++buffer)
        for (int i = 0; i < s->numBones + s->numLerpBones; ++i)
            if (!finite(s->orientations[buffer][i].origin, 3)
                || !finite(s->orientations[buffer][i].orientation, 4))
                return false;
    for (int i = 0; i < s->numLerpBones; ++i)
        if (!finite(s->lerpOffsets[i].origin, 3) || !finite(s->lerpOffsets[i].orientation, 4))
            return false;
    for (int i = 0; i < s->launchCount; ++i)
        if ((unsigned)s->launchHitLoc[i] >= HITLOC_NUM || !finite(s->launchForce[i], 3))
            return false;
    return true;
}

bool Ragdoll_CaptureState(RagdollBody *body, RagdollSavedState *s)
{
    if (!body || !s || !Ragdoll_BodyInUse(body)
        || (body->state != BS_DOBJ_WAIT && body->state != BS_VELOCITY_CAPTURE
            && body->state != BS_RUNNING && body->state != BS_IDLE)
        || body->hang_point || body->debug_hang_point || body->storedPhysObjId
        || body->rope_id || body->rbc_dist)
        return false; // Real attached constraints still need their typed owner checkpoint.
    if (body->state <= BS_VELOCITY_CAPTURE && !Ragdoll_CheckpointPendingOwnershipClean(body)) return false;
    *s = {};
    s->attachHitLoc = body->storedAttachHitLoc;
    s->attachEntity = body->storedAttachEnt;
    if (Ragdoll_CheckpointHasAttachment(s)
        && (s->attachEntity == body->dobj || !Ragdoll_CheckpointAttachmentTarget(body->localClientNum,
            s->attachEntity, &s->attachTargetPresent, &s->attachGeneration))) return false;
    s->definition = body->ragdollDef;
    s->state = body->state;
    s->stateMsec = body->stateMsec; s->stateFrames = body->stateFrames;
    s->velCaptureMsec = body->velCaptureMsec; s->stableTime = body->stableTime;
    s->numBones = body->numBones; s->numLerpBones = body->numLerpBones; s->numJoints = body->numJoints;
    s->curOrientationBuffer = body->curOrientationBuffer;
    s->flags = body->flags; s->gravityOverride = body->spPhysGravityOverride; s->gravity = body->spPhysGravity;
    s->launchCount = body->storedLaunchCount;
    memcpy(s->launchHitLoc, body->storedLaunchHitLoc, sizeof(s->launchHitLoc));
    memcpy(s->launchForce, body->storedLaunchForce, sizeof(s->launchForce));
    memcpy(s->poseOffset, body->poseOffset, sizeof(s->poseOffset));
    memcpy(s->orientations, body->boneOrientations, sizeof(s->orientations));
    memcpy(s->lerpOffsets, body->lerpBoneOffsets, sizeof(s->lerpOffsets));
    if (body->numBones < 1 || body->numBones > 14)
        return false;
    for (int i = 0; i < body->numBones; ++i)
    {
        const Bone &bone = body->bones[i];
        s->bones[i].parent = bone.parentBone;
        s->bones[i].animBones[0] = bone.animBones[0]; s->bones[i].animBones[1] = bone.animBones[1];
        s->bones[i].length = bone.length;
        memcpy(s->bones[i].center, bone.center, sizeof(bone.center));
        if (body->state == BS_RUNNING)
        {
            if (!bone.rigidBody)
                return false;
            Phys_CaptureBodyState(bone.rigidBody, &s->physics[i]);
        }
    }
    return Ragdoll_SavedStateValid(s);
}

bool Ragdoll_ApplyState(RagdollBody *body, const RagdollSavedState *s)
{
    if (!Ragdoll_SavedStateValid(s) || !body || !Ragdoll_BodyInUse(body)
        || (body->state != BS_DOBJ_WAIT && body->state != BS_VELOCITY_CAPTURE
            && body->state != BS_RUNNING && body->state != BS_IDLE)
        || body->ragdollDef != s->definition || body->numBones != s->numBones
        || body->numLerpBones != s->numLerpBones || body->numJoints != s->numJoints
        || body->hang_point || body->debug_hang_point || body->storedPhysObjId
        || body->rope_id || body->rbc_dist)
        return false;
    if (Ragdoll_CheckpointHasAttachment(s))
    {
        int present, generation;
        if (s->attachEntity == body->dobj || !Ragdoll_CheckpointAttachmentTarget(body->localClientNum,
                s->attachEntity, &present, &generation)
            || present != s->attachTargetPresent || generation != s->attachGeneration) return false;
    }
    const bool pending = s->state <= BS_VELOCITY_CAPTURE;
    if (pending)
    {
        // These states own animation samples, not bodies or joints. Do not
        // run their entry/update callbacks while restoring the saved boundary.
        // The next real Ragdoll_BodyUpdate will resume the normal state machine.
        if (body->state > BS_VELOCITY_CAPTURE || !Ragdoll_CheckpointPendingOwnershipClean(body))
            return false;
    }
    else if (body->state != BS_RUNNING && body->state != BS_IDLE) return false;
    for (int i = 0; i < s->numBones; ++i)
    {
        if (pending && s->state == BS_DOBJ_WAIT) continue; // Bind on canonical wait exit.
        if (body->bones[i].parentBone != s->bones[i].parent
            || body->bones[i].animBones[0] != s->bones[i].animBones[0]
            || body->bones[i].animBones[1] != s->bones[i].animBones[1]
            || std::fabs(body->bones[i].length - s->bones[i].length) > 0.01f)
            return false;
    }
    if (s->state == BS_RUNNING && body->state == BS_IDLE && !Ragdoll_BodyNewState(body, BS_RUNNING))
        return false;
    if (s->state == BS_IDLE && body->state == BS_RUNNING && !Ragdoll_BodyNewState(body, BS_IDLE))
        return false;
    // Canonical creation transforms bind-pose bones through world space. Its
    // float roundoff can change length/center slightly after the corpse moves.
    // Check the rebuilt collision geometry before retaining the saved metadata.
    for (int i = 0; i < s->numBones; ++i)
    {
        if (!pending && std::fabs(body->bones[i].length - s->bones[i].length) > 0.01f)
            return false;
        for (int axis = 0; axis < 3; ++axis)
            if (!pending && std::fabs(body->bones[i].center[axis] - s->bones[i].center[axis]) > 0.01f)
                return false;
        if (pending)
        {
            body->bones[i].parentBone = s->bones[i].parent;
            body->bones[i].animBones[0] = static_cast<unsigned char>(s->bones[i].animBones[0]);
            body->bones[i].animBones[1] = static_cast<unsigned char>(s->bones[i].animBones[1]);
        }
        body->bones[i].length = s->bones[i].length;
        memcpy(body->bones[i].center, s->bones[i].center, sizeof(s->bones[i].center));
    }
    if (s->state == BS_RUNNING)
        for (int i = 0; i < s->numBones; ++i)
        {
            if (!body->bones[i].rigidBody)
                return false;
            Phys_ApplyBodyState(body->bones[i].rigidBody, &s->physics[i]);
        }
    memcpy(body->boneOrientations, s->orientations, sizeof(s->orientations));
    memcpy(body->lerpBoneOffsets, s->lerpOffsets, sizeof(s->lerpOffsets));
    memcpy(body->poseOffset, s->poseOffset, sizeof(s->poseOffset));
    if (pending) body->state = static_cast<RagdollBodyState>(s->state);
    body->stateMsec = s->stateMsec; body->stateFrames = s->stateFrames;
    body->velCaptureMsec = s->velCaptureMsec; body->stableTime = s->stableTime;
    body->curOrientationBuffer = s->curOrientationBuffer;
    body->flags = s->flags; body->spPhysGravityOverride = s->gravityOverride != 0; body->spPhysGravity = s->gravity;
    body->storedAttachHitLoc = static_cast<hitLocation_t>(s->attachHitLoc);
    body->storedAttachEnt = s->attachEntity;
    body->storedLaunchCount = s->launchCount;
    memcpy(body->storedLaunchHitLoc, s->launchHitLoc, sizeof(s->launchHitLoc));
    memcpy(body->storedLaunchForce, s->launchForce, sizeof(s->launchForce));
    return true;
}

void Ragdoll_WriteState(MemoryFile *file, const RagdollSavedState *s)
{
    static_assert(sizeof(int) == 4 && sizeof(BoneOrientation) == 32 && sizeof(RagdollSavedBone) == 28);
    static_assert(offsetof(RagdollSavedState, attachHitLoc) == 3148 && sizeof(RagdollSavedState) == 3164,
        "retain the exact RAG1 prefix and pointer-free RAG2 extension");
    if (!Ragdoll_SavedStateValid(s))
        Com_Error(ERR_DROP, "Ragdoll checkpoint cannot write invalid state");
    const bool attached = Ragdoll_CheckpointHasAttachment(s);
    const int size = attached ? sizeof(*s) : offsetof(RagdollSavedState, attachHitLoc);
    MemFile_WriteInt(file, attached ? 0x32474152 : 0x31474152); // RAG2 or byte-identical legacy RAG1
    MemFile_WriteData(file, size, reinterpret_cast<unsigned char *>(const_cast<RagdollSavedState *>(s)));
}

void Ragdoll_ReadState(MemoryFile *file, RagdollSavedState *s)
{
    int magic;
    MemFile_ReadData(file, 4, reinterpret_cast<unsigned char *>(&magic));
    if (magic != 0x31474152 && magic != 0x32474152)
        Com_Error(ERR_DROP, "Ragdoll checkpoint schema mismatch");
    *s = {};
    const int size = magic == 0x32474152 ? sizeof(*s) : offsetof(RagdollSavedState, attachHitLoc);
    MemFile_ReadData(file, size, reinterpret_cast<unsigned char *>(s));
    if (!Ragdoll_SavedStateValid(s) || (magic == 0x32474152) != Ragdoll_CheckpointHasAttachment(s))
        Com_Error(ERR_DROP, "Ragdoll checkpoint rejected invalid state");
}
