// Native diagnostic client section, distinct from the retail SP client format.
// Capture owns only logical VM values and bytecode offsets. Native FX, sound,
// fake entities and physics handles must be checkpointed before client frames.
#include <client_mp/cl_cgame_mp.h>
#include <client_mp/cl_main_mp.h>
#include <client/client.h>
#include <cgame_mp/cg_local_mp.h>
#include <cgame/cg_scr_main.h>
#include <qcommon/threads.h>
#include <win32/win_main.h>
#include <cerrno>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <ragdoll/ragdoll_checkpoint.h>
#include <ragdoll/ragdoll_update.h>
#include <xanim/dobj_utils.h>
#include <xanim/xmodel.h>
#include <cmath>
#include "g_sp_native_owners_checkpoint.inl"
#include "g_sp_client_models_checkpoint.inl"

void G_SP_VerifyRenderedNativeOwners()
{
    if (!SP_QuiesceClientAudioCheckpoint())
        Com_Error(ERR_DROP, "Native owner probe requires joined audio");
    SP_VerifyLiveEffectsCheckpoint();
    SP_VerifyLiveAudioCheckpoint();
}

static void SP_WriteClientPhysics(MemoryFile *file)
{
    MemFile_WriteInt(file, 0x31594850); // PHY1, followed by typed owner records
    int count = 0;
    for (RagdollBody *body : g_ragdoll_body_pool)
        if (body->state != BS_DEAD)
            ++count;
    MemFile_WriteInt(file, count);
    for (RagdollBody *body : g_ragdoll_body_pool)
    {
        if (body->state == BS_DEAD)
            continue;
        RagdollSavedState saved;
        if (!Ragdoll_CaptureState(body, &saved) || body->localClientNum != 0)
            Com_Error(ERR_DROP, "Client checkpoint requires a stable, unattached ragdoll capture boundary");
        const centity_s *cent = CG_GetEntity(0, body->dobj);
        const DObj *obj = Ragdoll_BodyDObj(body);
        if ((!obj && saved.state != BS_DOBJ_WAIT) || (obj && !obj->numModels))
            Com_Error(ERR_DROP, "Client physics checkpoint lost its model owner");
        MemFile_WriteInt(file, body->dobj);
        MemFile_WriteInt(file, cent->nextState.lerp.useCount);
        MemFile_WriteCString(file, const_cast<char *>(obj && obj->numModels ? DObjGetModel(obj, 0)->name : ""));
        Ragdoll_WriteState(file, &saved);
    }
    Com_Printf(15, "Client physics checkpoint: captured %i logical ragdolls, no runtime physics IDs\n", count);
}

struct SPClientPhysicsOwner
{
    int number = 0, generation = 0;
    std::string model;
    RagdollSavedState state{};
};
static std::vector<SPClientPhysicsOwner> s_clientPhysicsOwners;

static void SP_VerifyClientPhysics(MemoryFile *reader)
{
    s_clientPhysicsOwners.clear();
    const unsigned char *original = MemFile_GetSegmentAddess(reader, 2);
    MemFile_MoveToSegment(reader, 2);
    int magic, count;
    MemFile_ReadData(reader, 4, reinterpret_cast<unsigned char *>(&magic));
    MemFile_ReadData(reader, 4, reinterpret_cast<unsigned char *>(&count));
    if (magic != 0x31594850 || count < 0 || count > 32)
        Com_Error(ERR_DROP, "Client physics checkpoint rejected its schema/count");
    static unsigned char bytes[0x40000];
    static MemoryFile writer;
    MemFile_InitForWriting(&writer, sizeof(bytes), bytes, true, true);
    MemFile_WriteInt(&writer, magic); MemFile_WriteInt(&writer, count);
    bool seen[MAX_CL_ENTNUM]{};
    for (int i = 0; i < count; ++i)
    {
        int number, generation;
        MemFile_ReadData(reader, 4, reinterpret_cast<unsigned char *>(&number));
        MemFile_ReadData(reader, 4, reinterpret_cast<unsigned char *>(&generation));
        if ((unsigned)number >= MAX_CL_ENTNUM || seen[number])
            Com_Error(ERR_DROP, "Client physics checkpoint rejected duplicate/invalid owner");
        seen[number] = true;
        const char *model = MemFile_ReadCString(reader);
        MemFile_WriteInt(&writer, number); MemFile_WriteInt(&writer, generation);
        MemFile_WriteCString(&writer, const_cast<char *>(model));
        // Own the name before another MemFile read reuses its scratch buffer.
        SPClientPhysicsOwner owner;
        owner.number = number; owner.generation = generation; owner.model = model;
        Ragdoll_ReadState(reader, &owner.state); Ragdoll_WriteState(&writer, &owner.state);
        if (owner.model.empty() && owner.state.state != BS_DOBJ_WAIT)
            Com_Error(ERR_DROP, "Client physics checkpoint rejected missing model identity");
        s_clientPhysicsOwners.push_back(std::move(owner));
    }
    MemFile_StartSegment(&writer, -1);
    int length;
    memcpy(&length, original, 4);
    if (length != writer.bytesUsed || memcmp(original, bytes, length))
        Com_Error(ERR_DROP, "Client physics checkpoint decoded graph differs");
    Com_Printf(15, "Client physics checkpoint: %i logical ragdolls RESERIALIZATION IDENTICAL (%i bytes); native client owners still pending\n", count, length);
}

static void SP_PreflightClientPhysics()
{
    for (const auto &owner : s_clientPhysicsOwners)
    {
        bool poseFound = false, modelFound = false;
        for (const auto &pose : s_clientEntityPoses.owners)
            if (!pose.kind && pose.index == owner.number && pose.nextUse == owner.generation)
                poseFound = true;
        for (const auto &model : s_clientEntityRegistry.models)
            if (model.handle == owner.number && !model.parts.empty() && model.parts[0].model == owner.model)
                modelFound = true;
        // The canonical MP/Zombies factory currently binds definition 1.
        // Reject an unsupported definition rather than silently substitute it.
        const bool waitingWithoutModel = owner.state.state == BS_DOBJ_WAIT && owner.model.empty();
        bool hasModelSlot = false;
        for (const auto &model : s_clientEntityRegistry.models)
            hasModelSlot |= model.handle == owner.number;
        if (!poseFound || (!modelFound && !waitingWithoutModel)
            || (waitingWithoutModel && hasModelSlot) || owner.state.definition != 1)
            Com_Error(ERR_DROP, "Client physics checkpoint rejected saved entity/model/definition identity");
        if (owner.state.attachHitLoc || owner.state.attachEntity)
        {
            // Classify with the saved entity range, not the fresh map's range.
            const int number = owner.state.attachEntity;
            const bool fake = number >= MAX_GENTITIES && (number < s_clientEntityPoses.end
                || number == ENTNUM_CSC_END || s_clientEntityPoses.bits == 10);
            if ((unsigned)number >= MAX_CL_ENTNUM || number == ENTNUM_CSC_END
                || (fake && (number >= s_clientEntityPoses.end || number - MAX_GENTITIES >= 512)))
                Com_Error(ERR_DROP, "Client physics checkpoint rejected attachment target slot");
            const SPClientEntityPose *target = nullptr;
            for (const auto &pose : s_clientEntityPoses.owners)
                if (!pose.kind && pose.index == owner.state.attachEntity) target = &pose;
            if (owner.state.attachEntity == owner.number
                || (target != nullptr) != (owner.state.attachTargetPresent != 0)
                || (target && target->nextUse != owner.state.attachGeneration))
                Com_Error(ERR_DROP, "Client physics checkpoint rejected attachment target generation/presence");
        }
    }
}

static void SP_PublishClientPhysics()
{
    int running = 0, waiting = 0, velocity = 0;
    for (const auto &owner : s_clientPhysicsOwners)
    {
        centity_s *cent = CG_GetEntity(0, owner.number);
        const DObj *obj = Com_GetClientDObj(owner.number, 0);
        const bool waitingWithoutModel = owner.state.state == BS_DOBJ_WAIT && owner.model.empty();
        if (cent->nextState.lerp.useCount != owner.generation
            || (waitingWithoutModel ? obj != nullptr : (!obj || !obj->numModels || owner.model != DObjGetModel(obj, 0)->name))
            || cent->pose.ragdollHandle || cent->pose.killcamRagdollHandle)
            Com_Error(ERR_DROP, "Client physics checkpoint lost its fresh native owner");
        auto *body = const_cast<RagdollBody *>(Ragdoll_CreateRagdollForDObj(0, owner.state.definition, owner.number, false, false));
        if (!body)
            Com_Error(ERR_DROP, "Client physics checkpoint could not allocate a canonical body");
        // Transfer the factory reference immediately. Canonical entity shutdown
        // now owns cleanup even if a later geometry/readback check drops.
        cent->pose.ragdollHandle = static_cast<int>(BO1_NARROW_ENCODE(body));
        cent->pose.isRagdoll = 1;
        const bool pending = owner.state.state <= BS_VELOCITY_CAPTURE;
        if (!pending)
        {
            if (body->state != BS_VELOCITY_CAPTURE || !Ragdoll_BodyNewState(body, BS_IDLE))
                Com_Error(ERR_DROP, "Client physics checkpoint could not bind the fresh skeleton");
            memcpy(body->boneOrientations, owner.state.orientations, sizeof(body->boneOrientations));
            body->curOrientationBuffer = owner.state.curOrientationBuffer;
            body->spPhysGravityOverride = owner.state.gravityOverride != 0;
            body->spPhysGravity = owner.state.gravity;
            // ExitIdle builds real geometry/joints through the canonical factory.
            if (!Ragdoll_BodyNewState(body, BS_RUNNING))
                Com_Error(ERR_DROP, "Client physics checkpoint could not create fresh geometry");
        }
        if (!Ragdoll_ApplyState(body, &owner.state))
            Com_Error(ERR_DROP, "Client physics checkpoint rebuilt geometry/state differs");
        static RagdollSavedState check;
        bool verified = Ragdoll_CaptureState(body, &check);
        if (verified && body->state == BS_RUNNING)
        {
            for (int i = 0; i < owner.state.numBones; ++i)
                verified &= Phys_VerifyAppliedBodyState(body->bones[i].rigidBody, &owner.state.physics[i]);
            // Only the comparison scratch uses the original pose encoding,
            // after checking every real matrix/COM/velocity/gravity exactly.
            // The live body and subsequent captures retain their real values.
            if (verified) memcpy(check.physics, owner.state.physics, sizeof(check.physics));
        }
        if (!verified || memcmp(&check, &owner.state, sizeof(check)))
        {
            const auto *expected = reinterpret_cast<const unsigned char *>(&owner.state);
            const auto *actual = reinterpret_cast<const unsigned char *>(&check);
            size_t first = 0;
            while (first < sizeof(check) && expected[first] == actual[first]) ++first;
            Com_Printf(15, "Client physics readback: entity %i state %i first differing byte %u (physics offset %u)\n",
                owner.number, owner.state.state, static_cast<unsigned>(first), static_cast<unsigned>(offsetof(RagdollSavedState, physics)));
            for (int bone = 0; bone < owner.state.numBones; ++bone)
            {
                float savedValues[16], liveValues[16];
                memcpy(savedValues, &owner.state.physics[bone], sizeof(savedValues));
                memcpy(liveValues, &check.physics[bone], sizeof(liveValues));
                for (int field = 0; field < 16; ++field)
                    if (memcmp(&savedValues[field], &liveValues[field], sizeof(float)))
                        Com_Printf(15, "Client physics readback: bone %i field %i saved %.9g live %.9g\n",
                            bone, field, savedValues[field], liveValues[field]);
            }
            Com_Error(ERR_DROP, "Client physics checkpoint live state differs");
        }
        if (owner.state.attachHitLoc || owner.state.attachEntity)
            Com_Printf(15, "Client physics attachment: entity %i target %i present %i generation %i hitloc %i deferred reference REBOUND; no native constraint invented\n",
                owner.number, owner.state.attachEntity, owner.state.attachTargetPresent,
                owner.state.attachGeneration, owner.state.attachHitLoc);
        running += body->state == BS_RUNNING;
        waiting += body->state == BS_DOBJ_WAIT;
        velocity += body->state == BS_VELOCITY_CAPTURE;
    }
    Com_Printf(15, "Client physics checkpoint: FRESH PROCESS %i entity-bound ragdolls (%i running), metadata RESERIALIZATION IDENTICAL; live motion EXACT CANONICAL TRANSFORM\n",
        static_cast<int>(s_clientPhysicsOwners.size()), running);
    Com_Printf(15, "Client physics checkpoint: pending animation owners %i model wait / %i velocity capture; canonical updates deferred until frame handoff\n", waiting, velocity);
}

void G_SP_VerifyRenderedRagdollCheckpoint()
{
    static RagdollSavedState saved[32], decoded[32];
    RagdollBody *owners[32]{};
    int count = 0, rigidBodies = 0, movingBones = 0;
    static unsigned char bytes[0x40000];
    static MemoryFile writer, reader;
    MemFile_InitForWriting(&writer, sizeof(bytes), bytes, true, true);
    for (RagdollBody *body : g_ragdoll_body_pool)
    {
        if (body->state == BS_DEAD)
            continue;
        if (count == 32 || !Ragdoll_CaptureState(body, &saved[count]))
            Com_Error(ERR_DROP, "Rendered ragdoll checkpoint could not capture a stable body");
        owners[count] = body;
        Ragdoll_WriteState(&writer, &saved[count]);
        rigidBodies += saved[count].numBones;
        if (saved[count].state == BS_RUNNING)
            for (int i = 0; i < saved[count].numBones; ++i)
                for (float velocity : saved[count].physics[i].linearVelocity)
                    movingBones += std::fabs(velocity) > 0.01f;
        ++count;
    }
    if (!count)
        Com_Error(ERR_DROP, "Rendered ragdoll checkpoint requires actual simulated ragdolls");
    MemFile_StartSegment(&writer, -1);
    MemFile_InitForReading(&reader, writer.bytesUsed, bytes, true);
    for (int i = 0; i < count; ++i)
    {
        Ragdoll_ReadState(&reader, &decoded[i]);
        if (memcmp(&saved[i], &decoded[i], sizeof(saved[i])))
            Com_Error(ERR_DROP, "Rendered ragdoll checkpoint wire roundtrip differs");
    }
    MemFile_MoveToSegment(&reader, -1);
    // Exercise the real physics lifetime: remove the old constraints/bodies,
    // recreate from canonical definitions, then bind the decoded logical state.
    for (int i = 0; i < count; ++i)
    {
        RagdollBody *body = owners[i];
        if (body->state == BS_RUNNING && !Ragdoll_BodyNewState(body, BS_IDLE))
            Com_Error(ERR_DROP, "Rendered ragdoll checkpoint could not retire old physics");
        if (!Ragdoll_BodyNewState(body, BS_RUNNING) || !Ragdoll_ApplyState(body, &decoded[i]))
            Com_Error(ERR_DROP, "Rendered ragdoll checkpoint could not rebuild/apply actual body owners");
        RagdollSavedState check;
        if (!Ragdoll_CaptureState(body, &check))
            Com_Error(ERR_DROP, "Rendered ragdoll checkpoint could not read new physics owners");
        if (check.state == BS_RUNNING)
            for (int bone = 0; bone < check.numBones; ++bone)
            {
                const auto &a = decoded[i].physics[bone]; const auto &b = check.physics[bone];
                float dot = 0.0f;
                for (int axis = 0; axis < 4; ++axis) dot += a.quaternion[axis] * b.quaternion[axis];
                if (std::fabs(dot) < 0.9999f)
                    Com_Error(ERR_DROP, "Rendered ragdoll checkpoint rotation changed");
                for (int axis = 0; axis < 3; ++axis)
                    if (std::fabs(a.origin[axis] - b.origin[axis]) > 0.002f
                        || std::fabs(a.linearVelocity[axis] - b.linearVelocity[axis]) > 0.0001f
                        || std::fabs(a.angularVelocity[axis] - b.angularVelocity[axis]) > 0.0001f
                        || a.gravity[axis] != b.gravity[axis])
                        Com_Error(ERR_DROP, "Rendered ragdoll checkpoint kinematics changed");
        }
        memset(check.physics, 0, sizeof(check.physics));
        RagdollSavedState expected = decoded[i];
        memset(expected.physics, 0, sizeof(expected.physics));
        if (memcmp(&expected, &check, sizeof(check)))
        {
            const auto *a = reinterpret_cast<const unsigned char *>(&expected);
            const auto *b = reinterpret_cast<const unsigned char *>(&check);
            unsigned offset = 0;
            while (offset < sizeof(check) && a[offset] == b[offset]) ++offset;
            Com_Error(ERR_DROP, "Rendered ragdoll checkpoint logical interpolation/state changed at byte %u (%u/%u)", offset, a[offset], b[offset]);
        }
    }
    Com_Printf(15, "loadgame physics: RESTORED %i ragdolls, %i real rigid bodies rebuilt, moving velocity components %i; codec exact, pose/velocity/gravity verified, real frames continue\n", count, rigidBodies, movingBones);
}

static bool SP_ClientCheckpointAllowed()
{
    if (!G_SP_CheckpointOption("bo1_client_checkpoint") || !Sys_IsMainThread() || Sys_IsHeadless()
        || !G_SP_IsSPLevel() || SP_IsFrontendMapForSave() || !Dvar_GetBool("zombiemode")
        || !Dvar_GetBool("sv_running") || CL_GetLocalClientConnectionState(0) != CA_ACTIVE
        || svs.clients[0].header.state != CS_ACTIVE || svs.clients[0].bIsTestClient
        || svs.clients[0].header.netchan.remoteAddress.type != NA_LOOPBACK)
    {
        Com_Printf(15, "Client checkpoint: requires explicit gate and an active main-thread local solo Zombies client\n");
        return false;
    }
    Com_SyncThreads();
    if (!SP_DynEntCaptureSupported()) return false;
    if (!SP_QuiesceClientAudioCheckpoint())
        return false;
    int connected = 0;
    for (int i = 0; i < level.maxclients; ++i)
    {
        if (level.clients[i].sess.connected == CON_DISCONNECTED && svs.clients[i].header.state < CS_CONNECTED)
            continue;
        if (i != 0 || level.clients[i].sess.connected != CON_CONNECTED)
            return false;
        ++connected;
    }
    // Refuse unsupported movement before freezing/serializing either world.
    // A public save rejection must leave the live match and previous save intact.
    const auto *cg = CG_GetLocalClientGlobals(0);
    if (!cg->nextSnap || !SP_CPMPlayerSupported(cg->nextSnap->ps))
    {
        Com_Printf(15, "Client checkpoint: unsupported local movement boundary\n");
        return false;
    }
    if (connected != 1 || !g_entities[0].client || g_entities[0].health <= 0
        || !Scr_IsStackClear(SCRIPTINSTANCE_CLIENT) || !Scr_IsStackClear(SCRIPTINSTANCE_SERVER)
        || gScrVmPub[SCRIPTINSTANCE_CLIENT].function_count || gScrVmPub[SCRIPTINSTANCE_SERVER].function_count)
    {
        Com_Printf(15, "Client checkpoint: rejected boundary maxclients %i connected %i health %i stacks %i/%i functions %i/%i\n",
            level.maxclients, connected, g_entities[0].health, Scr_IsStackClear(SCRIPTINSTANCE_SERVER),
            Scr_IsStackClear(SCRIPTINSTANCE_CLIENT), gScrVmPub[SCRIPTINSTANCE_SERVER].function_count,
            gScrVmPub[SCRIPTINSTANCE_CLIENT].function_count);
        return false;
    }
    // Unsupported shapes/constraints still reject before freezing either world.
    if (s_publicZombieCapture && !SP_ServerBodiesSupported(true)) return false;
    // Expected unsupported corpse states must reject a save without dropping
    // the match. Reuse the production capture validator before freezing audio.
    for (RagdollBody *body : g_ragdoll_body_pool)
    {
        if (body->state == BS_DEAD) continue;
        RagdollSavedState state;
        if (body->localClientNum != 0 || body->ragdollDef != 1 || !Ragdoll_CaptureState(body, &state))
        {
            Com_Printf(15, "Client checkpoint: unsupported ragdoll boundary; live session retained\n");
            return false;
        }
    }
    return true;
}

void G_SP_RoundTripClientNativeOwners()
{
    if (!SP_ClientCheckpointAllowed()) return;
    const int frame = level.framenum, time = level.time;
    G_SP_VerifyRenderedNativeOwners();
    Com_Printf(15, "Client native roundtrip: COMPLETE at serverTime %i frame %i; same live VM retained, real frames continue\n", time, frame);
}

static std::string SP_ClientCheckpointRoot()
{
    // Keep original server-only diagnostic generations intact during bring-up.
    return std::string(Sys_EngineUserRoot()) + "/" + SP_ClientCheckpointDirectory();
}

static void SP_WriteClientCheckpointHeader(MemoryFile *file)
{
    MemFile_WriteInt(file, 0x31504C43); // CLP1
    MemFile_WriteInt(file, 15);
    MemFile_WriteInt(file, level.time);
    const cg_s *cg = CG_GetLocalClientGlobals(0);
    MemFile_WriteInt(file, cg->time);
    MemFile_WriteInt(file, cg->snap ? cg->snap->serverTime : -1);
    MemFile_WriteInt(file, gScrVarPub[SCRIPTINSTANCE_CLIENT].checksum);
    MemFile_WriteInt(file, gScrCompilePub[SCRIPTINSTANCE_CLIENT].programLen);
    MemFile_WriteInt(file, static_cast<int>(gScrVarPub[SCRIPTINSTANCE_CLIENT].endScriptBuffer
        - gScrVarPub[SCRIPTINSTANCE_CLIENT].programBuffer));
    MemFile_WriteInt(file, VARIABLELIST_PARENT_SIZE(SCRIPTINSTANCE_CLIENT));
    MemFile_WriteInt(file, VARIABLELIST_CHILD_BEGIN(SCRIPTINSTANCE_CLIENT));
}

static bool SP_PersistClientCheckpoint(const bo1::android::save_store::Snapshot &snapshot, std::string &error)
{
    const int parent = open(Sys_EngineUserRoot(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (parent < 0) { error = "Cannot open the save directory"; return false; }
    const bool created = !mkdirat(parent, SP_ClientCheckpointDirectory(), 0700) || errno == EEXIST;
    const bool synced = created && !fsync(parent);
    close(parent);
    if (!synced) { error = "Cannot create or flush the save directory"; return false; }
    return bo1::android::save_store::Write(SP_ClientCheckpointRoot().c_str(), snapshot, error);
}

void G_SP_CaptureClientCheckpoint()
{
    s_publicZombieCaptureSucceeded = false;
    if (!SP_ClientCheckpointAllowed())
        return;
    if (!SP_FreezeAudioCheckpoint())
        Com_Error(ERR_DROP, "Client capture could not freeze mixer/occlusion worker");
    if (s_publicZombieCapture && s_publicZombieCaptureErrorProbe)
    {
        s_publicZombieCaptureErrorProbe = false;
        Com_Printf(15, "Save capture recovery probe: actual mixer/occlusion frozen, raising one ERR_DROP\n");
        Com_Error(ERR_DROP, "Save capture recovery probe");
    }
    SPSaveGame *save = SP_GetSaveBuffer(0);
    if (!SP_SaveMemory_IsAvailable(save))
        Com_Error(ERR_DROP, "Client checkpoint write buffer is busy");
    SPSaveGameRequest request{};
    I_strncpyz(request.filename, Dvar_GetString("mapname"), sizeof(request.filename));
    request.suppressPrint = true;
    if (!SP_SaveGame_Serialize(&request, g_spSaveMapChecksum, save))
        Com_Error(ERR_DROP, "Client checkpoint server serialization failed");
    MemoryFile *file = &save->memFile[1]; // segment0 opened by the real reset
    SP_WriteClientCheckpointHeader(file);
    Scr_SavePre(SCRIPTINSTANCE_CLIENT);
    Scr_CheckSaveObjects(SCRIPTINSTANCE_CLIENT);
    MemFile_StartSegment(file, 1);
    Scr_Save(SCRIPTINSTANCE_CLIENT, file);
    MemFile_StartSegment(file, 2);
    SP_WriteClientPhysics(file);
    MemFile_StartSegment(file, 3);
    SP_WriteClientNativeOwners(file);
    MemFile_StartSegment(file, 4);
    SP_WriteClientEntityRegistry(file);
    SP_WriteClientGenericMaterials(file);
    MemFile_StartSegment(file, 5);
    SP_WriteClientAnimations(file);
    MemFile_StartSegment(file, -1);
    if (!SP_SaveMemory_IsValid(save))
        Com_Error(ERR_DROP, "Client checkpoint exceeded its owned save buffers");
    save->header.pad = file->bufferSize;
    bo1::android::save_store::Snapshot snapshot;
    if (s_publicZombieCapture) snapshot.flags = bo1::android::save_store::SoloResumePayload;
    const auto *metadata = reinterpret_cast<const unsigned char *>(&save->header);
    snapshot.metadata.assign(metadata, metadata + sizeof(save->header));
    snapshot.game.assign(save->memFile[0].buffer, save->memFile[0].buffer + save->header.bodySize);
    snapshot.client.assign(file->buffer, file->buffer + save->header.pad);
    std::string error;
    const bool persisted = SP_PersistClientCheckpoint(snapshot, error);
    if (!persisted)
    {
        SP_SaveMemory_Abort(save);
        if (!SND_EndOcclusionCheckpoint() || !SD_EndCheckpoint())
            Com_Error(ERR_DROP, "Client capture could not release worker/mixer after failed write");
        if (s_publicZombieCapture)
        {
            Com_Printf(15, "Zombies save: disk write failed, session retained: %s\n", error.c_str());
            return;
        }
        Com_Error(ERR_DROP, "Client checkpoint durable write failed: %s", error.c_str());
    }
    Com_Printf(15, "Client checkpoint: DURABLE serverTime %i clientTime %i VM time %u objects %u threads %u gameBytes %i clientBytes %i; native client restoration still incomplete\n",
        level.time, CG_GetLocalClientGlobals(0)->time, gScrVarPub[SCRIPTINSTANCE_CLIENT].time,
        gScrVarPub[SCRIPTINSTANCE_CLIENT].savecount, gScrVarPub[SCRIPTINSTANCE_CLIENT].numScriptThreads,
        save->header.bodySize, save->header.pad);
    SP_SaveMemory_Abort(save); // diagnostic owns disk bytes, never the live memory continue slot
    if (!SND_EndOcclusionCheckpoint() || !SD_EndCheckpoint())
        Com_Error(ERR_DROP, "Client capture could not release worker/mixer");
    if (s_publicZombieCapture) s_publicZombieCaptureSucceeded = true;
    if (!s_publicZombieCapture && Dvar_GetBool("bo1_client_checkpoint_quit"))
        Com_Quit_f();
}

static SPClientPlayerImage s_retainedClientPlayers[3];
static void SP_HandoffClientFrames()
{
    auto *cg = CG_GetLocalClientGlobals(0); auto *cl = CL_GetLocalClientGlobals(0);
    auto *cgs = CG_GetLocalClientStaticGlobals(0);
    CG_AdoptCheckpointSnapshots(0, cg->snap, cg->nextSnap);
    SP_CNSReadPlayer(cg->snap->ps, s_retainedClientPlayers[0]); SP_CNSBindPlayer(s_retainedClientPlayers[0]);
    cg->snap->ps = s_retainedClientPlayers[0].value;
    if (cg->nextSnap != cg->snap)
    {
        SP_CNSReadPlayer(cg->nextSnap->ps, s_retainedClientPlayers[1]); SP_CNSBindPlayer(s_retainedClientPlayers[1]);
        cg->nextSnap->ps = s_retainedClientPlayers[1].value;
    }
    SP_CNSReadPlayer(cg->predictedPlayerState, s_retainedClientPlayers[2]); SP_CNSBindPlayer(s_retainedClientPlayers[2]);
    cg->predictedPlayerState = s_retainedClientPlayers[2].value;
    int number, time; CL_GetCurrentSnapshotNumber(0, &number, &time);
    cg->latestSnapshotNum = cgs->processedSnapshotNum = number; cg->latestSnapshotTime = time;
    // The paired server restart has a new transport epoch for this same world.
    const int epoch = cl->snap.snapFlags & 4;
    cg->snap->snapFlags = (cg->snap->snapFlags & ~4) | epoch;
    cg->nextSnap->snapFlags = (cg->nextSnap->snapFlags & ~4) | epoch;
    cl->serverTimeDelta = cl->serverTime - cls.realtime;
    cl->oldServerTime = cl->oldFrameServerTime = cl->serverTime;
    cl->newSnapshots = true;
    for (auto &packet : cl->outPackets)
    {
        packet.p_cmdNumber = cl->cmdNumber; packet.p_serverTime = cl->serverTime; packet.p_realtime = cls.realtime;
    }
    CG_ClearItemList(); CG_SavePlayerState(0);
    CG_InitClientEntityCaches(0);
    CG_RegisterWeapon(0, cg->predictedPlayerState.weapon);
    CG_RegisterWeapon(0, cg->predictedPlayerState.offHandIndex);
    for (const auto &held : cg->predictedPlayerState.heldWeapons) if (held.weapon) CG_RegisterWeapon(0, held.weapon);
    SP_HandoffClientNetwork();
    SP_PublishClientWeaponSelection();
    SP_PublishClientBolts();
    SP_PublishClientHud();
    SP_RestorePublicSavedDvars("frame handoff");
    if (G_SP_IsSavedWorldFrozen()) G_RestoreSavedClientInputEpoch();
    G_SP_BeginResumeLifecycleTrace();
    // Native owners and the saved VM now belong to canonical frames. Permit
    // the ordinary paused client/transport acknowledgement before the SERVER
    // freeze is released; only the temporary fresh VM needed the callback gate.
    if (s_continuousZombieResume) s_savedClientPublicationReady = true;
    if (!s_continuousZombieResume) G_SP_ReleaseSavedWorldFreeze();
    Com_Printf(15, "Client frame handoff: actual snapshot buffers adopted, new transport %i/time %i, saved clock %i/cmd %i, native caches/weapons initialized\n",
        number, time, cg->time, cl->cmdNumber);
}

void G_SP_VerifyClientCheckpoint()
{
    if (!SP_ClientCheckpointAllowed())
        return;
    bo1::android::save_store::Snapshot snapshot;
    std::string error;
    const std::string root = SP_ClientCheckpointRoot();
    if (!bo1::android::save_store::Read(root.c_str(), snapshot, error))
    {
        Com_Printf(15, "Client checkpoint: verification rejected: %s\n", error.c_str());
        return;
    }
    if (s_continuousZombieResume && (snapshot.flags != bo1::android::save_store::SoloResumePayload
        || snapshot.generation != s_publicZombieGeneration))
        Com_Error(ERR_DROP, "Zombies resume archive changed between SERVER and CLIENT publication");
    SPMemorySaveHeader header{};
    if (snapshot.metadata.size() != sizeof(header) || snapshot.client.empty()
        || snapshot.client.size() > sizeof(s_saveBufferA1))
        Com_Error(ERR_DROP, "Client checkpoint metadata/client size mismatch");
    memcpy(&header, snapshot.metadata.data(), sizeof(header));
    if (header.pad != snapshot.client.size() || !memchr(header.mapName, 0, sizeof(header.mapName))
        || I_stricmp(header.mapName, Dvar_GetString("mapname")))
        Com_Error(ERR_DROP, "Client checkpoint map/payload identity mismatch");
    static MemoryFile reader;
    MemFile_InitForReading(&reader, static_cast<int>(snapshot.client.size()), snapshot.client.data(), true);
    const auto read = []() { int value; MemFile_ReadData(&reader, 4, reinterpret_cast<unsigned char *>(&value)); return value; };
    const int magic = read(), version = read();
    if (magic != 0x31504C43 || version < 1 || version > 15)
        Com_Error(ERR_DROP, "Client checkpoint schema mismatch");
    const bool retain = s_continuousZombieResume || Dvar_GetInt("bo1_restore_client_frames") > 0;
    if (retain && (!G_SP_CheckpointOption("bo1_restore_saved_client") || version < 11
        || !G_SP_CheckpointOption("bo1_client_fx_publish") || !G_SP_CheckpointOption("bo1_client_audio_publish")
        || (!s_continuousZombieResume && !Dvar_GetInt("bo1_restore_render_frames"))))
        Com_Error(ERR_DROP, "Saved client frames require the paired complete checkpoint format");
    if ((G_SP_CheckpointOption("bo1_client_fx_publish") || G_SP_CheckpointOption("bo1_client_audio_publish")) && version < 3)
        Com_Error(ERR_DROP, "Fresh FX publication requires a native client checkpoint section");
    const int savedServerTime = read(), savedClientTime = read(), savedSnapshotTime = read();
    if (read() != gScrVarPub[SCRIPTINSTANCE_CLIENT].checksum
        || read() != gScrCompilePub[SCRIPTINSTANCE_CLIENT].programLen
        || read() != gScrVarPub[SCRIPTINSTANCE_CLIENT].endScriptBuffer - gScrVarPub[SCRIPTINSTANCE_CLIENT].programBuffer
        || read() != VARIABLELIST_PARENT_SIZE(SCRIPTINSTANCE_CLIENT)
        || read() != VARIABLELIST_CHILD_BEGIN(SCRIPTINSTANCE_CLIENT))
        Com_Error(ERR_DROP, "Client checkpoint compiled script/table identity mismatch");
    if (version >= 3)
        SP_VerifyClientNativeOwners(&reader);
    s_clientEntityPosesPresent = false;
    s_clientNetworkPresent = false;
    s_clientPredictionOwnersPresent = false;
    s_clientCharactersPresent = false;
    s_clientMovementPresent = false;
    s_clientWeaponSelectionPresent = false;
    s_clientBoltsPresent = false;
    s_clientHudPresent = false;
    s_clientGenericMaterialsPresent = false;
    if (version >= 4) SP_StageClientEntityRegistry(&reader, version);
    if (s_savedDynEntsPresent && s_savedDynSchema >= 0x334E5944)
    {
        if (s_savedDynQueueClock != s_clientNetworkState.clock.time
            || (!s_savedDynBurns.empty() && (!s_decodedClientEffectByteCount
                || !SP_DynStagedEffectsValid(s_decodedClientEffects.buffers, s_decodedClientEffects.container.shared))))
            Com_Error(ERR_DROP, "Dynamic checkpoint timed owners differ from saved client clock/FX graph");
    }
    if (s_savedDynEntsPresent && s_savedDynSchema >= 0x354E5944)
    {
        if (!s_clientEntityPosesPresent || !s_clientNetworkPresent || s_savedDynRopes.clock > s_clientNetworkState.clock.time)
            Com_Error(ERR_DROP, "Rope checkpoint missing saved client entity pool/clock");
        for (const auto &rope : s_savedDynRopes.ropes) if (rope.active)
            for (int n = 0; n < rope.constraints; ++n)
                if (rope.constraint[n].type == ROPE_CENTITY_CONSTRAINT && !SP_ClientPoseSlot(0, rope.constraint[n].entity))
                    Com_Error(ERR_DROP, "Rope checkpoint anchor outside restored client entity pool");
    }
    s_clientAnimationsPresent = false;
    if (version >= 5) SP_StageClientAnimations(&reader);
    s_clientPhysicsOwners.clear();
    if (version >= 2) SP_VerifyClientPhysics(&reader);
    if (retain) SP_PreflightClientPhysics();
    if (retain)
    {
        // Canonical entity shutdown MT-frees entity tree headers; character
        // roots only clear their nodes. A cross-class alias needs a separate
        // ownership transfer before those normal frees may safely run.
        for (int id = 1; id <= s_clientAnimations.trees.size(); ++id)
        {
            int entityOwners = 0, otherOwners = 0;
            for (const auto &root : s_clientAnimations.owners.roots) if (root.tree == id)
            {
                if (!root.kind && root.index >= 32) ++entityOwners; else ++otherOwners;
            }
            if (entityOwners > 1 || (entityOwners && otherOwners))
                Com_Error(ERR_DROP, "Saved client frames require compatible native tree-header ownership");
        }
    }
    // Decode-only until native integer handles in script values can be rebound.
    // Canonical old entity/FX/tree teardown must run while its OLD VM is live.
    if (G_SP_CheckpointOption("bo1_client_audio_publish")) SP_BeginFreshAudioTeardown();
    if (retain) CG_RetireCheckpointOwners(0);
    else CL_ShutdownCGame(0);
    if (G_SP_CheckpointOption("bo1_client_audio_publish")) SP_RetireFreshAudioConsumers();
    CScr_ClearConstServerStringCache();
    Scr_ShutdownSystem(SCRIPTINSTANCE_CLIENT, 1, 1);
    for (unsigned i = 0; i < CLASS_NUM_COUNT; ++i)
        Scr_RemoveClassMap(SCRIPTINSTANCE_CLIENT, i);
    SP_RestorePublicSavedDvars("old CLIENT teardown");
    MemFile_MoveToSegment(&reader, 1);
    Scr_Load(SCRIPTINSTANCE_CLIENT, &reader);
    static unsigned char checkBuffer[0x140000];
    static MemoryFile writer;
    MemFile_InitForWriting(&writer, sizeof(checkBuffer), checkBuffer, true, true);
    Scr_Save(SCRIPTINSTANCE_CLIENT, &writer); // retain decoder's original save IDs
    MemFile_StartSegment(&writer, -1);
    const unsigned char *original = MemFile_GetSegmentAddess(&reader, 1);
    int length;
    memcpy(&length, original, sizeof(length));
    if (length != writer.bytesUsed || memcmp(original, checkBuffer, length))
        Com_Error(ERR_DROP, "Client checkpoint decoded graph is not byte-identical");
    Com_Printf(15, "Client checkpoint: VM RESERIALIZATION IDENTICAL generation %llu serverTime %i clientTime %i snapshotTime %i VM time %u objects %u threads %u graphBytes %i; native publication follows\n",
        static_cast<unsigned long long>(snapshot.generation), savedServerTime, savedClientTime, savedSnapshotTime,
        gScrVarPub[SCRIPTINSTANCE_CLIENT].time, gScrVarPub[SCRIPTINSTANCE_CLIENT].savecount,
        gScrVarPub[SCRIPTINSTANCE_CLIENT].numScriptThreads, length);
    Scr_LoadPost(SCRIPTINSTANCE_CLIENT);
    if (retain) SP_PublishClientGenericMaterials();
    if (retain) SP_RebaseSavedPhysicsClock(s_clientNetworkState.clock.physicsTime);
    if (version >= 4 && G_SP_CheckpointOption("bo1_client_fx_publish"))
        SP_VerifyFreshClientModels(retain);
    if (retain) SP_PublishClientPhysics();
    if (retain) SP_PublishDynEntTopology();
    if (G_SP_CheckpointOption("bo1_client_fx_publish"))
        SP_VerifyFreshClientEffectsPublication(retain);
    if (G_SP_CheckpointOption("bo1_client_audio_publish"))
        SP_VerifyFreshClientAudioPublication(retain);
    MemFile_MoveToSegment(&reader, -1);
    if (retain) { SP_HandoffClientFrames(); return; }
    Com_Quit_f();
    Com_Error(ERR_FATAL, "Client checkpoint verification quit unexpectedly returned");
}
