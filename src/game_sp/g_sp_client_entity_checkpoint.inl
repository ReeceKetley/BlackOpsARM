// CET1 reconnects logical pose and motion owners. It is not a complete centity
// image: trajectories, pose unions, caches, shaders and physics stay separate.
#include <cgame_mp/cg_ents_mp.h>
#include <cgame_mp/cg_animscripted_mp.h>

struct SPClientEntityPose
{
    int kind = 0, index = 0, number = 0, currentUse = 0, nextUse = 0;
    entityType_t type = ET_GENERAL;
    unsigned char poseType = 0, poseUnion = 0;
    float origin[3]{}, angles[3]{}, mins[3]{}, maxs[3]{}, wetness = 0;
    int burnTime = 0;
    bool linked = false, moving = false;
    cLinkInfo_s link{};
    std::string tag;
    cgScriptMover_s mover{};
};
struct SPClientEntityPoses
{
    int bits = 0, end = 0;
    std::vector<SPClientEntityPose> owners;
};
static SPClientEntityPoses s_clientEntityPoses;
static bool s_clientEntityPosesPresent;

static bool SP_ClientPoseSlot(int kind, int index)
{
    return kind == 1 ? index == 0 : kind == 0 && SP_ClientAnimRootIndex(0, index);
}
static void SP_ClientMoverCodec(SPNativeCheckpointCodec &c, cgScriptMover_s &m)
{
    const auto finite = [&c](float &v) { c.Field(v); if (!std::isfinite(v)) Com_Error(ERR_DROP, "Client mover checkpoint rejected nonfinite motion"); };
    finite(m.decelTime); finite(m.aDecelTime); finite(m.speed); finite(m.aSpeed); finite(m.midTime); finite(m.aMidTime);
    for (auto &v : m.pos1) finite(v);
    for (auto &v : m.pos2) finite(v);
    for (auto &v : m.pos3) finite(v);
    for (auto &v : m.apos1) finite(v);
    for (auto &v : m.apos2) finite(v);
    for (auto &v : m.apos3) finite(v);
}
static void SP_ClientEntityPoseCodec(SPNativeCheckpointCodec &c, SPClientEntityPose &o)
{
    c.Field(o.kind); c.Field(o.index); c.Field(o.number); c.Field(o.currentUse); c.Field(o.nextUse);
    c.Field(o.type); c.Field(o.poseType); c.Field(o.poseUnion);
    if (!SP_ClientPoseSlot(o.kind, o.index) || !SP_ClientPoseSlot(0, o.number)
        || (o.kind == 0 && o.number != o.index) || o.type < 0)
        Com_Error(ERR_DROP, "Client pose checkpoint rejected entity identity");
    const auto finite = [&c](float &v) { c.Field(v); if (!std::isfinite(v)) Com_Error(ERR_DROP, "Client pose checkpoint rejected nonfinite pose"); };
    for (auto &v : o.origin) finite(v);
    for (auto &v : o.angles) finite(v);
    for (auto &v : o.mins) finite(v);
    for (auto &v : o.maxs) finite(v);
    finite(o.wetness); c.Field(o.burnTime);
    c.Field(o.linked); c.Field(o.moving);
    if (o.linked)
    {
        c.Field(o.link.linkEnt); c.Field(o.link.linkTag); SP_ClientOwnerName(c, o.tag, true);
        // Positive native string IDs are deliberately not written. 1 denotes
        // a named tag; zero and -1 retain the engine's two non-tag link modes.
        if (o.link.linkTag < -1 || o.link.linkTag > 1 || (o.link.linkTag == 1) != !o.tag.empty()
            || !SP_ClientPoseSlot(0, o.link.linkEnt) || o.link.linkEnt == o.number)
            Com_Error(ERR_DROP, "Client pose checkpoint rejected link identity");
        for (auto &row : o.link.axis) for (auto &v : row) finite(v);
        for (auto &v : o.link.relative_angles) finite(v);
        c.Field(o.link.angles_set);
    }
    if (o.moving) SP_ClientMoverCodec(c, o.mover);
}
static void SP_ClientEntityPosesCodec(SPNativeCheckpointCodec &c, SPClientEntityPoses &state)
{
    if (c.Int(0x31544543) != 0x31544543) Com_Error(ERR_DROP, "Client pose checkpoint schema mismatch");
    c.Field(state.bits); c.Field(state.end);
    if (state.bits != g_entNumBits || state.end != g_entNumCscEnd)
        Com_Error(ERR_DROP, "Client pose checkpoint entity pool differs");
    const int count = c.Int(static_cast<int>(state.owners.size()));
    if (count < 1 || count > MAX_CL_ENTNUM) Com_Error(ERR_DROP, "Client pose checkpoint rejected owner count");
    if (c.reader) state.owners.resize(count);
    bool seen[2][MAX_CL_ENTNUM]{};
    const SPClientEntityPose *byNumber[MAX_CL_ENTNUM]{};
    for (auto &o : state.owners)
    {
        SP_ClientEntityPoseCodec(c, o);
        if (seen[o.kind][o.index]) Com_Error(ERR_DROP, "Client pose checkpoint duplicate owner");
        seen[o.kind][o.index] = true;
        if (!o.kind) byNumber[o.number] = &o;
    }
    if (!seen[1][0]) Com_Error(ERR_DROP, "Client pose checkpoint missing predicted player");
    // A delayed network parent need not be in the current snapshot. Fake
    // parents must exist in the separately validated FDO1 pool inventory.
    for (const auto &start : state.owners)
    {
        if (start.kind) continue;
        bool visited[MAX_CL_ENTNUM]{}; const auto *at = &start;
        while (at->linked && at->link.linkEnt != 1023)
        {
            if (visited[at->number]) Com_Error(ERR_DROP, "Client pose checkpoint cyclic links");
            visited[at->number] = true;
            const SPClientEntityPose *parent = byNumber[at->link.linkEnt];
            if (!parent) break;
            at = parent;
        }
    }
}
static centity_s *SP_ClientPoseEntity(const SPClientEntityPose &o)
{
    return o.kind ? &CG_GetLocalClientGlobals(0)->predictedPlayerEntity : CG_GetEntity(0, o.index);
}
static void SP_ReadClientEntityPose(const centity_s *cent, int kind, int index, SPClientEntityPose &o)
{
    o = {}; o.kind = kind; o.index = index; o.number = cent->nextState.number;
    o.currentUse = cent->currentState.useCount; o.nextUse = cent->nextState.lerp.useCount; o.type = cent->nextState.eType;
    o.poseType = cent->pose.eType; o.poseUnion = cent->pose.eTypeUnion;
    memcpy(o.origin, cent->pose.origin, sizeof(o.origin)); memcpy(o.angles, cent->pose.angles, sizeof(o.angles));
    memcpy(o.mins, cent->pose.absmin, sizeof(o.mins)); memcpy(o.maxs, cent->pose.absmax, sizeof(o.maxs));
    o.wetness = cent->pose.wetness; o.burnTime = cent->pose.startBurnTime;
    o.linked = cent->linkInfo != nullptr; o.moving = cent->cScriptMover != nullptr;
    if (o.linked)
    {
        o.link = *cent->linkInfo;
        if (o.link.linkTag > 0) { o.tag = SL_ConvertToString(o.link.linkTag, SCRIPTINSTANCE_SERVER); o.link.linkTag = 1; }
    }
    if (o.moving) o.mover = *cent->cScriptMover;
}
static void SP_ReadClientEntityPoses(SPClientEntityPoses &state)
{
    state = {}; state.bits = g_entNumBits; state.end = g_entNumCscEnd;
    for (int i = 0; i < MAX_CL_ENTNUM; ++i) if (SP_ClientPoseSlot(0, i))
    {
        const centity_s *cent = CG_GetEntity(0, i);
        const bool fake = EntNum_IsCsc(i) && cg_fakeEntitiesInuseArray[i - 1024];
        if (!fake && !cent->nextValid && !cent->linkInfo && !cent->cScriptMover) continue;
        state.owners.emplace_back(); SP_ReadClientEntityPose(cent, 0, i, state.owners.back());
    }
    state.owners.emplace_back(); SP_ReadClientEntityPose(&CG_GetLocalClientGlobals(0)->predictedPlayerEntity, 1, 0, state.owners.back());
}
static void SP_WriteClientEntityPoses(MemoryFile *file)
{
    SP_ReadClientEntityPoses(s_clientEntityPoses);
    SPNativeCheckpointCodec c{nullptr, file}; SP_ClientEntityPosesCodec(c, s_clientEntityPoses);
    int links = 0, movers = 0;
    for (const auto &o : s_clientEntityPoses.owners) { links += o.linked; movers += o.moving; }
    Com_Printf(15, "Client entity poses: captured %i logical owners / %i links / %i script movers; no runtime cache or physics IDs\n", static_cast<int>(s_clientEntityPoses.owners.size()), links, movers);
}
static void SP_DecodeClientEntityPoses(SPNativeCheckpointCodec &c)
{
    s_clientEntityPoses = {}; SP_ClientEntityPosesCodec(c, s_clientEntityPoses);
}
static void SP_PreflightClientEntityPoses()
{
    bool seenFake[512]{};
    for (const auto &o : s_clientEntityPoses.owners)
    {
        if (o.linked && o.link.linkTag == 1 && !SL_FindString(o.tag.c_str(), SCRIPTINSTANCE_SERVER))
            Com_Error(ERR_DROP, "Client pose checkpoint missing canonical link tag");
        if (o.linked && EntNum_IsCsc(o.link.linkEnt) && !s_clientEntityRegistry.slots[o.link.linkEnt - 1024].used)
            Com_Error(ERR_DROP, "Client pose checkpoint missing fake parent");
        if (o.kind || !EntNum_IsCsc(o.index)) continue;
        const auto &slot = s_clientEntityRegistry.slots[o.index - 1024]; seenFake[o.index - 1024] = true;
        if (!slot.used || slot.currentUse != o.currentUse || slot.nextUse != o.nextUse || slot.type != o.type
            || slot.linked != o.linked || memcmp(slot.origin, o.origin, sizeof(o.origin)) || memcmp(slot.angles, o.angles, sizeof(o.angles))
            || (o.linked && (slot.parent != o.link.linkEnt || slot.tagMode != o.link.linkTag || slot.tag != o.tag
                || slot.anglesSet != o.link.angles_set || memcmp(slot.axis, o.link.axis, sizeof(slot.axis))
                || memcmp(slot.relative, o.link.relative_angles, sizeof(slot.relative)))))
            Com_Error(ERR_DROP, "Client pose checkpoint disagrees with fake inventory");
    }
    for (int i = 0; i < 512; ++i) if (s_clientEntityRegistry.slots[i].used != seenFake[i])
        Com_Error(ERR_DROP, "Client pose checkpoint incomplete fake inventory");
    s_clientEntityPosesPresent = true;
    Com_Printf(15, "Client entity poses: durable RESERIALIZATION IDENTICAL with FDO1, identities checked before teardown\n");
}

struct SPClientPoseLease
{
    centity_s *cent = nullptr;
    // Only scalar fields changed below are restored; native caches never move.
    SPClientEntityPose before;
    cLinkInfo_s *previousLink = nullptr;
    cLinkInfo_s *link = nullptr;
    cgScriptMover_s *mover = nullptr;
};
static std::vector<SPClientPoseLease> s_clientPoseLeases;
void G_SP_NotifyClientCheckpointLinkFree(void *memory)
{
    for (auto &lease : s_clientPoseLeases) if (lease.link == memory) lease.link = nullptr;
}
void G_SP_NotifyClientCheckpointMoverFree(void *memory)
{
    // Snapshot retirement may free a restored mover during ordinary play.
    // Drop the lease before MT can reuse its address for a different owner.
    for (auto &lease : s_clientPoseLeases) if (lease.mover == memory)
    {
        lease.mover = nullptr;
        Com_Printf(15, "Client pose checkpoint: saved mover entity %i generation %i retired by canonical snapshot owner before memory reuse\n",
            lease.cent->nextState.number, lease.cent->nextState.lerp.useCount);
    }
}
static void SP_ApplyClientPoseScalars(centity_s *cent, const SPClientEntityPose &o)
{
    cent->nextState.number = o.number; cent->currentState.useCount = o.currentUse;
    cent->nextState.lerp.useCount = o.nextUse; cent->nextState.eType = o.type;
    cent->pose.eType = o.poseType; cent->pose.eTypeUnion = o.poseUnion;
    memcpy(cent->pose.origin, o.origin, sizeof(o.origin)); memcpy(cent->pose.angles, o.angles, sizeof(o.angles));
    memcpy(cent->pose.absmin, o.mins, sizeof(o.mins)); memcpy(cent->pose.absmax, o.maxs, sizeof(o.maxs));
    cent->pose.wetness = o.wetness; cent->pose.startBurnTime = o.burnTime;
}
static void SP_DetachClientEntityPoses()
{
    for (auto &lease : s_clientPoseLeases)
    {
        // Network shutdown retains a reusable link cache. Its prior borrower
        // is leased, never freed or mistaken for our newly allocated owner.
        if (!lease.cent->linkInfo || lease.cent->linkInfo == lease.link) lease.cent->linkInfo = lease.previousLink;
        if (lease.link)
        {
            MT_Free(reinterpret_cast<unsigned char *>(lease.link), sizeof(cLinkInfo_s), SCRIPTINSTANCE_SERVER);
        }
        if (lease.mover)
        {
            if (lease.cent->cScriptMover == lease.mover) lease.cent->cScriptMover = nullptr;
            MT_Free(reinterpret_cast<unsigned char *>(lease.mover), sizeof(cgScriptMover_s), SCRIPTINSTANCE_SERVER);
        }
        SP_ApplyClientPoseScalars(lease.cent, lease.before);
    }
    s_clientPoseLeases.clear();
}
static void SP_AttachClientEntityPoses()
{
    if (!s_clientEntityPosesPresent) return;
    if (!s_clientPoseLeases.empty()) Com_Error(ERR_DROP, "Client pose probe already owns slots");
    s_clientPoseLeases.reserve(s_clientEntityPoses.owners.size());
    for (const auto &o : s_clientEntityPoses.owners)
    {
        centity_s *cent = SP_ClientPoseEntity(o);
        if (cent->cScriptMover) Com_Error(ERR_DROP, "Client pose probe old script mover still live");
        s_clientPoseLeases.emplace_back(); auto &lease = s_clientPoseLeases.back(); lease.cent = cent;
        SP_ReadClientEntityPose(cent, o.kind, o.index, lease.before);
        lease.previousLink = cent->linkInfo; cent->linkInfo = nullptr;
        SP_ApplyClientPoseScalars(cent, o);
        if (o.linked)
        {
            lease.link = reinterpret_cast<cLinkInfo_s *>(MT_Alloc(sizeof(cLinkInfo_s), 18, SCRIPTINSTANCE_SERVER));
            *lease.link = o.link;
            if (o.link.linkTag == 1)
            {
                lease.link->linkTag = SL_FindString(o.tag.c_str(), SCRIPTINSTANCE_SERVER);
                if (!lease.link->linkTag) Com_Error(ERR_DROP, "Client pose probe canonical link tag disappeared");
            }
            cent->linkInfo = lease.link;
        }
        if (o.moving)
        {
            lease.mover = reinterpret_cast<cgScriptMover_s *>(MT_Alloc(sizeof(cgScriptMover_s), 22, SCRIPTINSTANCE_SERVER));
            *lease.mover = o.mover; cent->cScriptMover = lease.mover;
        }
    }
    static SPClientEntityPoses actual; actual = {}; actual.bits = g_entNumBits; actual.end = g_entNumCscEnd;
    int links = 0, movers = 0;
    for (const auto &o : s_clientEntityPoses.owners)
    {
        const centity_s *cent = SP_ClientPoseEntity(o);
        actual.owners.emplace_back(); SP_ReadClientEntityPose(cent, o.kind, o.index, actual.owners.back());
        const cpose_t *pose = o.kind ? &cent->pose : CG_GetPose(0, o.index);
        float origin[3]; CG_GetPoseOrigin(pose, origin);
        if (pose != &cent->pose || memcmp(origin, o.origin, sizeof(origin))
            || (!o.kind && memcmp(CG_GetEntityOrigin(0, o.index), o.origin, sizeof(origin))))
            Com_Error(ERR_DROP, "Client pose probe actual consumer differs");
        links += o.linked; movers += o.moving;
    }
    static unsigned char expectedBytes[0x140000], actualBytes[0x140000]; static MemoryFile expected, check;
    MemFile_InitForWriting(&expected, sizeof(expectedBytes), expectedBytes, true, true);
    MemFile_InitForWriting(&check, sizeof(actualBytes), actualBytes, true, true);
    SPNativeCheckpointCodec a{nullptr, &expected}, b{nullptr, &check};
    SP_ClientEntityPosesCodec(a, s_clientEntityPoses); SP_ClientEntityPosesCodec(b, actual);
    MemFile_StartSegment(&expected, -1); MemFile_StartSegment(&check, -1);
    if (expected.bytesUsed != check.bytesUsed || memcmp(expectedBytes, actualBytes, expected.bytesUsed))
        Com_Error(ERR_DROP, "Client pose probe live logical state differs");
    Com_Printf(15, "Client entity poses: FRESH PROCESS %i actual pose consumers / %i links / %i script movers LIVE RESERIALIZATION IDENTICAL; NO CLIENT FRAMES\n", static_cast<int>(actual.owners.size()), links, movers);
}
