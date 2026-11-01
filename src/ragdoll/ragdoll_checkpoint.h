#pragma once
#include "ragdoll.h"
#include <physics/phys_checkpoint.h>

struct MemoryFile;

// Versioned logical state, never RagdollBody's native pointers or physics IDs.
struct RagdollSavedBone
{
    int parent;
    int animBones[2];
    float length;
    float center[3];
};

struct RagdollSavedState
{
    int definition;
    int state;
    int stateMsec, stateFrames, velCaptureMsec, stableTime;
    int numBones, numLerpBones, numJoints, curOrientationBuffer;
    int flags, gravityOverride, gravity;
    int launchCount, launchHitLoc[2];
    float launchForce[2][3];
    float poseOffset[3];
    BoneOrientation orientations[2][23];
    BoneOrientation lerpOffsets[9];
    RagdollSavedBone bones[14];
    PhysSavedBodyState physics[14];
    // RAG2 extension. These are logical entity keys, never a native constraint ID.
    int attachHitLoc, attachEntity, attachTargetPresent, attachGeneration;
};

bool Ragdoll_CaptureState(RagdollBody *body, RagdollSavedState *saved);
bool Ragdoll_ApplyState(RagdollBody *body, const RagdollSavedState *saved);
void Ragdoll_WriteState(MemoryFile *file, const RagdollSavedState *saved);
void Ragdoll_ReadState(MemoryFile *file, RagdollSavedState *saved);
