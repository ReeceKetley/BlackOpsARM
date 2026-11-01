#include <algorithm>
#include <cgame/cg_colltree.h>
#include <cgame/cg_world.h>
#include <cgame_mp/cg_pose_mp.h>
#include <client_mp/cl_parse_mp.h>
#include <xanim/dobj_utils.h>
#include <xanim/dobj_skel.h>
#include <qcommon/cm_world.h>
#include "g_sp_client_pose_payload.inl"

extern int cgCollWorldLocalClientNum;
struct SPClientPredictionOwners
{
    clientControllers_t controls[36]{};
    std::vector<SPClientPoseComponent> poses;
    float worldMins[3]{}, worldMaxs[3]{}, nearMins[3]{}, nearMaxs[3]{};
    std::vector<int> worldHits, nearHits;
    SPClientPredictionHistory history;
};
static SPClientPredictionOwners s_clientPredictionOwners;
static bool s_clientPredictionOwnersPresent;

static clientControllers_t *SP_CPCControlOwner(int ordinal)
{
    return ordinal < 32 ? &CG_GetLocalClientGlobals(0)->bgs.clientinfo[ordinal].control
        : &CG_GetLocalClientStaticGlobals(0)->corpseinfo[ordinal - 32].control;
}
static cpose_t *SP_CPCActualPose(const SPClientPoseComponent &o)
{
    if (o.kind == 2) return &CG_GetLocalClientGlobals(0)->viewModelPose;
    if (o.kind == 1) return &CG_GetLocalClientGlobals(0)->predictedPlayerEntity.pose;
    return &CG_GetEntity(0, o.index)->pose;
}
static void SP_CPCHitsCodec(SPNativeCheckpointCodec &c, std::vector<int> &hits)
{
    const int count = c.Int(static_cast<int>(hits.size()));
    if (count < 0 || count > MAX_CL_ENTNUM) Com_Error(ERR_DROP, "Client collision checkpoint rejected query count");
    if (c.reader) hits.resize(count);
    int previous = -1;
    for (auto &id : hits)
    {
        c.Field(id);
        if (!SP_ClientPoseSlot(0, id) || id <= previous) Com_Error(ERR_DROP, "Client collision checkpoint rejected query identity/order");
        previous = id;
    }
}
static void SP_ClientPredictionOwnersCodec(SPNativeCheckpointCodec &c, SPClientPredictionOwners &state)
{
    const int schema = c.Int(0x32435043); // CPC2; CPC1 selected by pose type alone.
    if (schema != 0x31435043 && schema != 0x32435043) Com_Error(ERR_DROP, "Client prediction owner schema mismatch");
    for (auto &owner : state.controls) SP_CPCControllerCodec(c, owner);
    const int count = c.Int(static_cast<int>(state.poses.size()));
    if (count < 2 || count > MAX_CL_ENTNUM + 1) Com_Error(ERR_DROP, "Client pose component rejected count");
    if (c.reader) state.poses.resize(count);
    bool seen[3][MAX_CL_ENTNUM]{};
    for (auto &o : state.poses)
    {
        SP_CPCPoseCodec(c, o, schema == 0x31435043);
        if (seen[o.kind][o.index]) Com_Error(ERR_DROP, "Client pose component duplicate owner");
        seen[o.kind][o.index] = true;
    }
    if (!seen[1][0] || !seen[2][0]) Com_Error(ERR_DROP, "Client pose component missing predicted/view owner");
    SP_CNSValue(c, state.worldMins); SP_CNSValue(c, state.worldMaxs);
    SP_CNSValue(c, state.nearMins); SP_CNSValue(c, state.nearMaxs);
    for (int i = 0; i < 3; ++i) if (state.worldMins[i] > state.worldMaxs[i] || state.nearMins[i] > state.nearMaxs[i])
        Com_Error(ERR_DROP, "Client collision checkpoint rejected query bounds");
    SP_CPCHitsCodec(c, state.worldHits); SP_CPCHitsCodec(c, state.nearHits);
    SP_CPCHistoryCodec(c, state.history);
}

// A controller component query uses canonical base-animation and controller
// routines on the real DObj, with a temporary skeleton. Restore its actual
// prior skeleton/lock even on the engine's non-C++ recoverable error boundary.
static DObj *s_clientControllerScratch;
static DSkel s_previousControllerSkel;
static bool s_clientControllerScratchLocked;
static void SP_RestoreClientControllerScratch()
{
    if (!s_clientControllerScratch) return;
    DObj *obj = s_clientControllerScratch; s_clientControllerScratch = nullptr;
    obj->skel = s_previousControllerSkel;
    if (s_clientControllerScratchLocked) { s_clientControllerScratchLocked = false; DObjUnlock(obj); }
}
static DObjAnimMat SP_CPCQueryController(const SPClientPoseComponent &o)
{
    DObj *obj = Com_GetClientDObj(o.index, 0);
    if (o.kind || !obj || obj->locked || !obj->numBones || obj->numBones > DOBJ_MAX_PARTS || s_clientControllerScratch)
        Com_Error(ERR_DROP, "Client controller query requires its real unlocked model owner");
    alignas(16) static char scratch[DOBJ_MAX_PARTS * sizeof(DObjAnimMat)];
    memset(scratch, 0, sizeof(scratch));
    s_previousControllerSkel = obj->skel; s_clientControllerScratch = obj;
    DObjLock(obj); s_clientControllerScratchLocked = true; obj->skel = {};
    DObjCreateSkel(obj, scratch, 1);
    int partBits[5]{static_cast<int>(0x80000000u), 0, 0, 0, 0};
    auto *mat = DObjGetRotTransArray(obj); DObjCalcBaseAnim(obj, mat, partBits);
    const cpose_t *pose = SP_CPCActualPose(o);
    if (o.payloadKind == CPC_PLAYER) BG_Player_DoControllers(&pose->player, obj, partBits);
    else CG_Actor_DoControllers(pose, obj, partBits);
    const DObjAnimMat result = mat[0]; SP_RestoreClientControllerScratch(); return result;
}
static void SP_CPCReadPose(cpose_t *pose, int kind, int index, SPClientPoseComponent &o)
{
    o = {}; o.kind = kind; o.index = index; o.type = pose->eType; o.unionType = pose->eTypeUnion;
    const int entityType = kind ? o.type : CG_GetEntity(0, index)->nextState.eType;
    o.payloadKind = SP_CPCOwnerPayloadKind(kind, index, o.type, entityType);
    // Native RAM lease/capture only. The codec above writes selected fields,
    // never this union image, padding, or its pointer bytes.
    static_assert(sizeof(SPClientPosePayload) == sizeof(CEntVehicleInfo));
    memcpy(&o.payload, &pose->vehicle, sizeof(o.payload));
    if (o.payloadKind == CPC_PLAYER)
    {
        if (pose->player.control)
        {
            for (int i = 0; i < 36; ++i) if (pose->player.control == SP_CPCControlOwner(i)) { o.control = i; break; }
            if (o.control < 0) Com_Error(ERR_DROP, "Client pose component has unknown controller owner");
        }
        o.payload.player.control = nullptr;
    }
    if (o.payloadKind == CPC_TURRET && pose->turret.playerUsing)
    {
        if (pose->turret.viewAngles != CG_GetLocalClientGlobals(0)->predictedPlayerState.viewangles)
            Com_Error(ERR_DROP, "Client turret pose has unknown view-angle owner");
        o.payload.turret.viewAngles = nullptr;
    }
    if (!kind && cgEntCollNodes[0][index].sector)
    {
        o.linked = true; const auto &node = cgEntCollNodes[0][index];
        memcpy(o.linkMins, node.linkMins, sizeof(o.linkMins)); memcpy(o.linkMaxs, node.linkMaxs, sizeof(o.linkMaxs));
    }
    if (kind == 2)
    {
        memcpy(o.origin, pose->origin, sizeof(o.origin)); memcpy(o.angles, pose->angles, sizeof(o.angles));
        memcpy(o.mins, pose->absmin, sizeof(o.mins)); memcpy(o.maxs, pose->absmax, sizeof(o.maxs));
        o.wetness = pose->wetness; o.burnTime = pose->startBurnTime;
    }
    const int payloadKind = o.payloadKind;
    o.queried = !kind && Com_GetClientDObj(index, 0) && (payloadKind == CPC_ACTOR || (payloadKind == CPC_PLAYER && o.control >= 0));
    if (o.queried) o.reference = SP_CPCQueryController(o);
}
static std::vector<int> SP_CPCAreaQuery(const float *mins, const float *maxs)
{
    int hits[MAX_CL_ENTNUM]; const int count = CG_AreaEntities(mins, maxs, hits, MAX_CL_ENTNUM, -1);
    if (count < 0 || count >= MAX_CL_ENTNUM) Com_Error(ERR_DROP, "Client collision checkpoint query overflow");
    std::vector<int> result(hits, hits + count); std::sort(result.begin(), result.end()); return result;
}
static void SP_CPCReadHistory(SPClientPredictionHistory &history)
{
    const auto *cl = CL_GetLocalClientGlobals(0); history = {}; history.number = CL_GetCurrentCmdNumber(0);
    if (history.number < 0 || history.number > 0x7FFFFF00) Com_Error(ERR_DROP, "Client history capture rejected command counter");
    const int count = std::min(128, history.number + 1); history.commands.resize(count);
    for (int i = 0; i < count; ++i) if (!CL_GetUserCmd(0, history.number - count + 1 + i, &history.commands[i]))
        Com_Error(ERR_DROP, "Client history capture lost live command");
    history.archiveIndex = cl->clientArchiveIndex; memcpy(history.archive, cl->clientArchive, sizeof(history.archive));
    memcpy(history.viewangles, cl->viewangles, sizeof(history.viewangles));
}
static void SP_CPCReadCurrent(SPClientPredictionOwners &state)
{
    state = {};
    for (int i = 0; i < 36; ++i) state.controls[i] = *SP_CPCControlOwner(i);
    for (const auto &p : s_clientEntityPoses.owners)
    {
        state.poses.emplace_back(); SP_CPCReadPose(&SP_ClientPoseEntity(p)->pose, p.kind, p.index, state.poses.back());
    }
    state.poses.emplace_back(); SP_CPCReadPose(&CG_GetLocalClientGlobals(0)->viewModelPose, 2, 0, state.poses.back());
    memcpy(state.worldMins, cgEntCollWorld[0].mins, sizeof(state.worldMins)); memcpy(state.worldMaxs, cgEntCollWorld[0].maxs, sizeof(state.worldMaxs));
    const auto *cg = CG_GetLocalClientGlobals(0);
    for (int i = 0; i < 3; ++i) { state.nearMins[i] = cg->predictedPlayerState.origin[i] - 64; state.nearMaxs[i] = cg->predictedPlayerState.origin[i] + 64; }
    if (cgCollWorldLocalClientNum != 0) Com_Error(ERR_DROP, "Client collision checkpoint lost local world owner");
    state.worldHits = SP_CPCAreaQuery(state.worldMins, state.worldMaxs); state.nearHits = SP_CPCAreaQuery(state.nearMins, state.nearMaxs);
    bool linked[MAX_CL_ENTNUM]{}; for (const auto &o : state.poses) if (o.linked) linked[o.index] = true;
    for (int i = 0; i < MAX_CL_ENTNUM; ++i) if (bool(cgEntCollNodes[0][i].sector) != linked[i])
        Com_Error(ERR_DROP, "Client collision checkpoint missing pose owner entity %i sector %u captured %i validSlot %i",
            i, cgEntCollNodes[0][i].sector, int(linked[i]), int(SP_ClientPoseSlot(0, i)));
    SP_CPCReadHistory(state.history);
}
static void SP_WriteClientPredictionOwners(MemoryFile *file)
{
    SP_CPCReadCurrent(s_clientPredictionOwners); SPNativeCheckpointCodec c{nullptr, file}; SP_ClientPredictionOwnersCodec(c, s_clientPredictionOwners);
    int links = 0, refs = 0, queries = 0;
    for (const auto &o : s_clientPredictionOwners.poses) { links += o.linked; refs += o.control >= 0; queries += o.queried; }
    Com_Printf(15, "Client prediction owners: captured %i pose payloads / %i controller borrowers / %i controller queries / %i collision links / %i commands / 256 origin histories; world queries %i/%i\n",
        static_cast<int>(s_clientPredictionOwners.poses.size()), refs, queries, links, static_cast<int>(s_clientPredictionOwners.history.commands.size()), static_cast<int>(s_clientPredictionOwners.worldHits.size()), static_cast<int>(s_clientPredictionOwners.nearHits.size()));
}
static void SP_DecodeClientPredictionOwners(SPNativeCheckpointCodec &c)
{
    s_clientPredictionOwners = {}; SP_ClientPredictionOwnersCodec(c, s_clientPredictionOwners);
    if (s_clientPredictionOwners.poses.size() != s_clientEntityPoses.owners.size() + 1)
        Com_Error(ERR_DROP, "Client prediction components disagree with entity inventory");
    for (int i = 0; i < s_clientEntityPoses.owners.size(); ++i)
    {
        const auto &o = s_clientPredictionOwners.poses[i]; const auto &p = s_clientEntityPoses.owners[i];
        if (o.kind != p.kind || o.index != p.index || o.type != p.poseType || o.unionType != p.poseUnion)
            Com_Error(ERR_DROP, "Client pose component identity differs");
        if (o.payloadKind != SP_CPCOwnerPayloadKind(p.kind, p.index, p.poseType, p.type))
            Com_Error(ERR_DROP, "Client pose component payload differs from logical entity (legacy CPC1 may lack FX state)");
        if (o.queried)
        {
            bool found = false; for (const auto &model : s_clientEntityRegistry.models) found |= model.handle == o.index;
            if (!found) Com_Error(ERR_DROP, "Client controller query lost saved model owner");
        }
    }
    if (s_clientPredictionOwners.poses.back().kind != 2) Com_Error(ERR_DROP, "Client prediction components lost viewmodel owner");
    float mins[3], maxs[3]; CM_ModelBounds(0, mins, maxs);
    if (memcmp(mins, s_clientPredictionOwners.worldMins, sizeof(mins)) || memcmp(maxs, s_clientPredictionOwners.worldMaxs, sizeof(maxs)))
        Com_Error(ERR_DROP, "Client collision checkpoint world geometry differs");
    s_clientPredictionOwnersPresent = true;
    Com_Printf(15, "Client prediction owners: pose/controller/collision/input identities validated BEFORE teardown\n");
}

struct SPClientPoseComponentLease { cpose_t *pose; unsigned char bytes[sizeof(SPClientPosePayload)]; };
static std::vector<SPClientPoseComponentLease> s_clientPoseComponentLeases;
static clientControllers_t s_previousPoseControls[36];
static cpose_t s_previousViewModelPose;
static CgEntCollWorld s_previousClientCollWorld;
static CgEntCollNode s_previousClientCollNodes[MAX_CL_ENTNUM];
static int s_previousClientCollLocal;
static usercmd_s s_previousClientCommands[128];
static ClientArchiveData s_previousClientArchive[256];
static int s_previousClientCommandNumber, s_previousClientArchiveIndex;
static float s_previousClientInputAngles[3];
static bool s_clientPredictionOwnersAttached;
static void SP_DetachClientPredictionOwners()
{
    SP_RestoreClientControllerScratch();
    if (!s_clientPredictionOwnersAttached) return;
    s_clientPredictionOwnersAttached = false;
    cgEntCollWorld[0] = s_previousClientCollWorld; memcpy(cgEntCollNodes[0], s_previousClientCollNodes, sizeof(s_previousClientCollNodes));
    cgCollWorldLocalClientNum = s_previousClientCollLocal;
    for (const auto &lease : s_clientPoseComponentLeases) memcpy(&lease.pose->vehicle, lease.bytes, sizeof(lease.bytes));
    s_clientPoseComponentLeases.clear(); CG_GetLocalClientGlobals(0)->viewModelPose = s_previousViewModelPose;
    for (int i = 0; i < 36; ++i) *SP_CPCControlOwner(i) = s_previousPoseControls[i];
    auto *cl = CL_GetLocalClientGlobals(0);
    memcpy(cl->cmds, s_previousClientCommands, sizeof(s_previousClientCommands)); cl->cmdNumber = s_previousClientCommandNumber;
    memcpy(cl->clientArchive, s_previousClientArchive, sizeof(s_previousClientArchive)); cl->clientArchiveIndex = s_previousClientArchiveIndex;
    memcpy(cl->viewangles, s_previousClientInputAngles, sizeof(s_previousClientInputAngles));
}
static void SP_CPCApplyPose(cpose_t *pose, const SPClientPoseComponent &o)
{
    memset(&pose->vehicle, 0, sizeof(CEntVehicleInfo));
    switch (o.payloadKind)
    {
    case CPC_PLAYER: pose->player = o.payload.player; pose->player.control = o.control < 0 ? nullptr : SP_CPCControlOwner(o.control); break;
    case CPC_TURRET:
        pose->turret = o.payload.turret;
        if (pose->turret.playerUsing) pose->turret.viewAngles = CG_GetLocalClientGlobals(0)->predictedPlayerState.viewangles;
        break;
    case CPC_VEHICLE: pose->vehicle = o.payload.vehicle; break;
    case CPC_FX: pose->fx = o.payload.fx; break;
    case CPC_ACTOR: pose->actor = o.payload.actor; break;
    default: pose->general = o.payload.general; break;
    }
    if (o.kind == 2)
    {
        pose->eType = o.type; pose->eTypeUnion = o.unionType;
        memcpy(pose->origin, o.origin, sizeof(o.origin)); memcpy(pose->angles, o.angles, sizeof(o.angles));
        memcpy(pose->absmin, o.mins, sizeof(o.mins)); memcpy(pose->absmax, o.maxs, sizeof(o.maxs));
        pose->wetness = o.wetness; pose->startBurnTime = o.burnTime;
    }
}
static void SP_AttachClientPredictionOwners()
{
    if (!s_clientPredictionOwnersPresent) return;
    if (s_clientPredictionOwnersAttached) Com_Error(ERR_DROP, "Client prediction component probe already attached");
    auto *cl = CL_GetLocalClientGlobals(0);
    for (int i = 0; i < 36; ++i) s_previousPoseControls[i] = *SP_CPCControlOwner(i);
    s_previousViewModelPose = CG_GetLocalClientGlobals(0)->viewModelPose;
    s_previousClientCollWorld = cgEntCollWorld[0]; memcpy(s_previousClientCollNodes, cgEntCollNodes[0], sizeof(s_previousClientCollNodes));
    s_previousClientCollLocal = cgCollWorldLocalClientNum;
    memcpy(s_previousClientCommands, cl->cmds, sizeof(s_previousClientCommands)); s_previousClientCommandNumber = cl->cmdNumber;
    memcpy(s_previousClientArchive, cl->clientArchive, sizeof(s_previousClientArchive)); s_previousClientArchiveIndex = cl->clientArchiveIndex;
    memcpy(s_previousClientInputAngles, cl->viewangles, sizeof(s_previousClientInputAngles));
    s_clientPoseComponentLeases.reserve(s_clientPredictionOwners.poses.size()); s_clientPredictionOwnersAttached = true;
    for (int i = 0; i < 36; ++i) *SP_CPCControlOwner(i) = s_clientPredictionOwners.controls[i];
    for (const auto &o : s_clientPredictionOwners.poses)
    {
        s_clientPoseComponentLeases.emplace_back(); auto &lease = s_clientPoseComponentLeases.back(); lease.pose = SP_CPCActualPose(o);
        memcpy(lease.bytes, &lease.pose->vehicle, sizeof(lease.bytes)); SP_CPCApplyPose(lease.pose, o);
    }
    cgCollWorldLocalClientNum = 0; CG_ClearEntityCollWorld(0);
    for (const auto &o : s_clientPredictionOwners.poses) if (o.linked) CG_LinkEntityColl(0, o.index, o.linkMins, o.linkMaxs);
    const auto &history = s_clientPredictionOwners.history; memset(cl->cmds, 0, sizeof(cl->cmds)); cl->cmdNumber = history.number;
    const int first = history.number - static_cast<int>(history.commands.size()) + 1;
    for (int i = 0; i < history.commands.size(); ++i) cl->cmds[(first + i) & 127] = history.commands[i];
    memcpy(cl->clientArchive, history.archive, sizeof(history.archive)); cl->clientArchiveIndex = history.archiveIndex;
    memcpy(cl->viewangles, history.viewangles, sizeof(history.viewangles));
    const auto &latest = history.archive[(history.archiveIndex + 255) & 255]; ClientArchiveData lookup{};
    if (!CL_GetPredictedOriginForServerTime(cl, latest.serverTime, lookup.origin, lookup.velocity, lookup.viewangles, &lookup.bobCycle, &lookup.movementDir)
        || memcmp(lookup.origin, latest.origin, sizeof(lookup.origin)) || memcmp(lookup.velocity, latest.velocity, sizeof(lookup.velocity))
        || memcmp(lookup.viewangles, latest.viewangles, sizeof(lookup.viewangles)) || lookup.bobCycle != latest.bobCycle || lookup.movementDir != latest.movementDir)
        Com_Error(ERR_DROP, "Client prediction archive native query differs");
    static SPClientPredictionOwners actual; SP_CPCReadCurrent(actual);
    static unsigned char expectedBytes[0x140000], actualBytes[0x140000]; static MemoryFile expected, check;
    MemFile_InitForWriting(&expected, sizeof(expectedBytes), expectedBytes, true, true); MemFile_InitForWriting(&check, sizeof(actualBytes), actualBytes, true, true);
    SPNativeCheckpointCodec a{nullptr, &expected}, b{nullptr, &check}; SP_ClientPredictionOwnersCodec(a, s_clientPredictionOwners); SP_ClientPredictionOwnersCodec(b, actual);
    MemFile_StartSegment(&expected, -1); MemFile_StartSegment(&check, -1);
    if (expected.bytesUsed != check.bytesUsed || memcmp(expectedBytes, actualBytes, expected.bytesUsed))
        Com_Error(ERR_DROP, "Client pose/controller/collision/input live readback differs");
    int links = 0, refs = 0, queries = 0;
    for (const auto &o : actual.poses) { links += o.linked; refs += o.control >= 0; queries += o.queried; }
    Com_Printf(15, "Client prediction owners: FRESH PROCESS %i pose payloads / %i native controller borrowers / %i original controller queries / %i rebuilt collision links; real world/near queries %i/%i, %i CL_GetUserCmd and predicted-origin archive lookup LIVE RESERIALIZATION IDENTICAL; NO PMOVE OR CLIENT FRAMES\n",
        static_cast<int>(actual.poses.size()), refs, queries, links, static_cast<int>(actual.worldHits.size()), static_cast<int>(actual.nearHits.size()), static_cast<int>(actual.history.commands.size()));
}
