// Logical network/snapshot/prediction state; ordinary transport history and
// native cgame services still have to be established before any saved frames.
#include <cgame_mp/cg_predict_mp.h>
#include <bgame/bg_misc.h>
#include "g_sp_client_network_codecs.inl"

#define SP_CNS_CG_FIELDS(X) \
    X(clientNum) X(localClientNum) X(latestSnapshotNum) X(latestSnapshotTime) \
    X(frameInterpolation) X(frametime) X(time) X(oldTime) X(physicsTime) X(mapRestart) \
    X(lastPlayerStateOverride) X(predictedErrorTime) X(predictedError) \
    X(landChange) X(landTime) X(heightToCeiling) X(heightToCeilingTS)
#define SP_CNS_CL_FIELDS(X) \
    X(serverTime) X(oldServerTime) X(oldFrameServerTime) X(serverTimeDelta) \
    X(oldSnapServerTime) X(extrapolatedSnapshot) X(newSnapshots)
struct SPClientNetworkClock
{
#define CG_FIELD(n) decltype(cg_s::n) n{};
    SP_CNS_CG_FIELDS(CG_FIELD)
#undef CG_FIELD
    struct Client
    {
#define CL_FIELD(n) decltype(clientActive_t::n) n{};
        SP_CNS_CL_FIELDS(CL_FIELD)
#undef CL_FIELD
    } client;
};
struct SPClientSnapshotImage
{
    snapshot_s value{};
    SPClientPlayerImage player;
    std::string targets[1024];
};
struct SPClientNetworkEntity
{
    int kind = 0, index = 0;
    LerpEntityState current{};
    entityState_s next{};
    unsigned flags = 0;
    short previousEvent = 0;
    int miscTime = 0, muzzleTime = 0, bulletImpacts = 0, nextSlide = 0, fireTime = 0;
    unsigned short flagIndex = 0, flagState = 0;
    std::string target;
};
struct SPClientNetworkState
{
    SPClientNetworkClock clock;
    int current = 0, next = 0;
    std::vector<SPClientSnapshotImage> snapshots;
    SPClientPlayerImage predicted, interpolated;
    std::vector<SPClientNetworkEntity> entities;
};
static SPClientNetworkState s_clientNetworkState;
static bool s_clientNetworkPresent;

static void SP_CNSSnapshotCodec(SPNativeCheckpointCodec &c, SPClientSnapshotImage &image)
{
    auto &v = image.value;
    SP_CNSValue(c, v.snapFlags); SP_CNSValue(c, v.ping); SP_CNSValue(c, v.serverTime); SP_CNSValue(c, v.physicsTime);
    SP_CNSPlayerCodec(c, image.player); v.ps = image.player.value;
    SP_CNSValue(c, v.numEntities); SP_CNSValue(c, v.numClients);
    if (v.numEntities < 0 || v.numEntities > 1024 || v.numClients < 0 || v.numClients > 32)
        Com_Error(ERR_DROP, "Client snapshot checkpoint rejected count");
    bool entities[MAX_CL_ENTNUM]{}, clients[32]{};
    for (int i = 0; i < v.numEntities; ++i)
    {
        auto &e = v.entities[i]; SP_CNSEntityCodec(c, e, image.targets[i]);
        if (EntNum_IsCsc(e.number) || entities[e.number]) Com_Error(ERR_DROP, "Client snapshot checkpoint duplicate/nonnetwork entity");
        entities[e.number] = true;
        if (!image.targets[i].empty()) Com_Error(ERR_DROP, "Client network snapshot unexpectedly owns a script string");
    }
    for (int i = 0; i < v.numClients; ++i)
    {
        auto &cl = v.clients[i]; SP_CNSValue(c, cl);
        if ((unsigned)cl.clientIndex >= 32 || clients[cl.clientIndex]) Com_Error(ERR_DROP, "Client snapshot checkpoint duplicate client");
        clients[cl.clientIndex] = true;
    }
    SP_CNSValue(c, v.matchState.index); SP_CNSValue(c, v.matchState.archivedState); SP_CNSValue(c, v.matchState.unarchivedState);
    SP_CNSValue(c, v.serverCommandSequence);
}
static void SP_CNSNetworkEntityCodec(SPNativeCheckpointCodec &c, SPClientNetworkEntity &o)
{
    c.Field(o.kind); c.Field(o.index);
    if (!SP_ClientPoseSlot(o.kind, o.index)) Com_Error(ERR_DROP, "Client network entity checkpoint rejected slot");
    SP_CNSValue(c, o.current); SP_CNSEntityCodec(c, o.next, o.target);
    if ((!o.kind && o.next.number != o.index) || (o.flags & 0xE0000000u))
        Com_Error(ERR_DROP, "Client network entity checkpoint rejected identity/flags");
    c.Field(o.flags); c.Field(o.previousEvent); c.Field(o.miscTime); c.Field(o.muzzleTime);
    c.Field(o.bulletImpacts); c.Field(o.nextSlide); c.Field(o.fireTime); c.Field(o.flagIndex); c.Field(o.flagState);
    if (o.flags & 0xE0000000u) Com_Error(ERR_DROP, "Client network entity checkpoint reserved flags");
}
static void SP_CNSClockCodec(SPNativeCheckpointCodec &c, SPClientNetworkClock &clock)
{
#define CG_FIELD(n) SP_CNSValue(c, clock.n);
    SP_CNS_CG_FIELDS(CG_FIELD)
#undef CG_FIELD
#define CL_FIELD(n) SP_CNSValue(c, clock.client.n);
    SP_CNS_CL_FIELDS(CL_FIELD)
#undef CL_FIELD
    if (clock.clientNum != 0 || clock.localClientNum != 0 || clock.frametime < 0
        || clock.frameInterpolation < 0 || clock.frameInterpolation > 1)
        Com_Error(ERR_DROP, "Client snapshot checkpoint rejected clock/interpolation");
}
static void SP_ClientNetworkCodec(SPNativeCheckpointCodec &c, SPClientNetworkState &state)
{
    if (c.Int(0x31534E43) != 0x31534E43) Com_Error(ERR_DROP, "Client network checkpoint schema mismatch"); // CNS1
    SP_CNSClockCodec(c, state.clock); c.Field(state.current); c.Field(state.next);
    const int count = c.Int(static_cast<int>(state.snapshots.size()));
    if (count < 1 || count > 2 || state.current != 0 || state.next < 0 || state.next >= count || state.next != count - 1)
        Com_Error(ERR_DROP, "Client snapshot checkpoint rejected sharing");
    if (c.reader) state.snapshots.resize(count);
    for (auto &s : state.snapshots) SP_CNSSnapshotCodec(c, s);
    if (state.snapshots[state.next].value.serverTime < state.snapshots[state.current].value.serverTime)
        Com_Error(ERR_DROP, "Client snapshot checkpoint reversed time");
    SP_CNSPlayerCodec(c, state.predicted); SP_CNSPlayerCodec(c, state.interpolated);
    // This bounded interpolation query does not have native vehicle services.
    for (const auto &s : state.snapshots) if (s.player.value.eFlags & 0x4000)
        Com_Error(ERR_DROP, "Client snapshot checkpoint vehicle interpolation services pending");
    const int owners = c.Int(static_cast<int>(state.entities.size()));
    if (owners < 1 || owners > MAX_CL_ENTNUM) Com_Error(ERR_DROP, "Client network checkpoint rejected owner count");
    if (c.reader) state.entities.resize(owners);
    bool seen[2][MAX_CL_ENTNUM]{};
    for (auto &o : state.entities)
    {
        SP_CNSNetworkEntityCodec(c, o);
        if (seen[o.kind][o.index]) Com_Error(ERR_DROP, "Client network checkpoint duplicate owner");
        seen[o.kind][o.index] = true;
    }
}
static void SP_CNSReadClock(SPClientNetworkClock &clock)
{
    const auto &cg = *CG_GetLocalClientGlobals(0); const auto &cl = *CL_GetLocalClientGlobals(0);
#define CG_FIELD(n) memcpy(&clock.n, &cg.n, sizeof(clock.n));
    SP_CNS_CG_FIELDS(CG_FIELD)
#undef CG_FIELD
#define CL_FIELD(n) memcpy(&clock.client.n, &cl.n, sizeof(clock.client.n));
    SP_CNS_CL_FIELDS(CL_FIELD)
#undef CL_FIELD
}
static void SP_CNSApplyClock(const SPClientNetworkClock &clock)
{
    auto &cg = *CG_GetLocalClientGlobals(0); auto &cl = *CL_GetLocalClientGlobals(0);
#define CG_FIELD(n) memcpy(&cg.n, &clock.n, sizeof(clock.n));
    SP_CNS_CG_FIELDS(CG_FIELD)
#undef CG_FIELD
#define CL_FIELD(n) memcpy(&cl.n, &clock.client.n, sizeof(clock.client.n));
    SP_CNS_CL_FIELDS(CL_FIELD)
#undef CL_FIELD
}
static void SP_CNSReadEntity(const centity_s *cent, int kind, int index, SPClientNetworkEntity &o)
{
    o = {}; o.kind = kind; o.index = index; o.current = cent->currentState; o.next = cent->nextState; o.flags = CENT_FLAGS(cent);
    if (o.next.targetname) o.target = SL_ConvertToString(o.next.targetname, SCRIPTINSTANCE_CLIENT);
    o.previousEvent = cent->previousEventSequence; o.miscTime = cent->miscTime; o.muzzleTime = cent->lastMuzzleFlash;
    o.bulletImpacts = cent->numBulletImpacts; o.nextSlide = cent->nextSlideFX; o.fireTime = cent->fireTime;
    o.flagIndex = cent->flagIndex; o.flagState = cent->flagState;
}
static void SP_CNSApplyEntity(centity_s *cent, const SPClientNetworkEntity &o)
{
    cent->currentState = o.current; cent->nextState = o.next; CENT_FLAGS(cent) = o.flags;
    cent->previousEventSequence = o.previousEvent; cent->miscTime = o.miscTime; cent->lastMuzzleFlash = o.muzzleTime;
    cent->numBulletImpacts = o.bulletImpacts; cent->nextSlideFX = o.nextSlide; cent->fireTime = o.fireTime;
    cent->flagIndex = o.flagIndex; cent->flagState = o.flagState;
}
static void SP_CNSReadSnapshot(const snapshot_s &s, SPClientSnapshotImage &out)
{
    out.value = s; SP_CNSReadPlayer(s.ps, out.player);
    for (int i = 0; i < s.numEntities; ++i)
        if (s.entities[i].targetname) out.targets[i] = SL_ConvertToString(s.entities[i].targetname, SCRIPTINSTANCE_CLIENT);
}
static void SP_CNSReadCurrent(SPClientNetworkState &state)
{
    state = {}; auto *cg = CG_GetLocalClientGlobals(0); SP_CNSReadClock(state.clock);
    if (!cg->snap || !cg->nextSnap) Com_Error(ERR_DROP, "Client checkpoint requires a live snapshot pair");
    state.next = cg->nextSnap == cg->snap ? 0 : 1; state.snapshots.resize(state.next + 1);
    SP_CNSReadSnapshot(*cg->snap, state.snapshots[0]);
    if (state.next) SP_CNSReadSnapshot(*cg->nextSnap, state.snapshots[1]);
    SP_CNSReadPlayer(cg->predictedPlayerState, state.predicted);
    // The canonical no-input interpolation writes only predictedPlayerState.
    // Refuse unsupported vehicle services before calling it, including capture.
    for (const auto &s : state.snapshots) if (s.player.value.eFlags & 0x4000)
        Com_Error(ERR_DROP, "Client checkpoint vehicle interpolation services pending");
    static playerState_s previous; previous = cg->predictedPlayerState;
    CG_InterpolatePlayerState(0, 0); SP_CNSReadPlayer(cg->predictedPlayerState, state.interpolated); cg->predictedPlayerState = previous;
    for (const auto &p : s_clientEntityPoses.owners)
    {
        state.entities.emplace_back(); SP_CNSReadEntity(SP_ClientPoseEntity(p), p.kind, p.index, state.entities.back());
    }
}
static void SP_WriteClientNetwork(MemoryFile *file)
{
    SP_CNSReadCurrent(s_clientNetworkState); SPNativeCheckpointCodec c{nullptr, file}; SP_ClientNetworkCodec(c, s_clientNetworkState);
    Com_Printf(15, "Client network checkpoint: captured %i entities / %i shared snapshots, clock %i, predicted command %i; canonical interpolation reference saved\n",
        static_cast<int>(s_clientNetworkState.entities.size()), static_cast<int>(s_clientNetworkState.snapshots.size()), s_clientNetworkState.clock.time, s_clientNetworkState.predicted.value.commandTime);
}
static void SP_DecodeClientNetwork(SPNativeCheckpointCodec &c)
{
    s_clientNetworkState = {}; SP_ClientNetworkCodec(c, s_clientNetworkState);
    if (s_clientNetworkState.entities.size() != s_clientEntityPoses.owners.size()) Com_Error(ERR_DROP, "Client network/pose owner count differs");
    for (int i = 0; i < s_clientNetworkState.entities.size(); ++i)
    {
        const auto &o = s_clientNetworkState.entities[i]; const auto &p = s_clientEntityPoses.owners[i];
        if (o.kind != p.kind || o.index != p.index || o.next.number != p.number || o.current.useCount != p.currentUse
            || o.next.lerp.useCount != p.nextUse || o.next.eType != p.type)
            Com_Error(ERR_DROP, "Client network checkpoint disagrees with pose identities");
    }
    s_clientNetworkPresent = true;
    Com_Printf(15, "Client network checkpoint: typed snapshots/prediction/entities validated BEFORE teardown\n");
}

static void SP_PreflightClientDynLights(int version)
{
    for (unsigned i = 0; i < s_savedDynBindings.size(); ++i)
    {
        const int number = s_savedDynBindings[i].centities[0];
        if (number < 1024) continue;
        if (version < 8 || !SP_DynCentitySlotValid(cm.constraints[i], 0, number))
            Com_Error(ERR_DROP, "Saved map light requires its complete logical client owner");
        const auto &slot = s_clientEntityRegistry.slots[number - 1024];
        const SPClientNetworkEntity *network = nullptr;
        for (const auto &owner : s_clientNetworkState.entities)
            if (!owner.kind && owner.index == number) { network = &owner; break; }
        if (!network || slot.type != ET_PRIMARY_LIGHT || slot.currentUse != network->current.useCount
            || slot.nextUse != network->next.lerp.useCount
            || !SP_DynLightOwnerValid(cm.constraints[i], number, slot.used, &network->next))
            Com_Error(ERR_DROP, "Saved map light %u lost its primary-light owner %i", i, number);
    }
}

struct SPClientNetworkLease
{
    centity_s *cent = nullptr;
    SPClientNetworkEntity previous;
    unsigned acquiredTarget = 0;
};
static std::vector<SPClientNetworkLease> s_clientNetworkLeases;
static bool s_clientNetworkAttached;
static bool s_clientNetworkHandedOff;
static SPClientNetworkClock s_previousNetworkClock;
static snapshot_s *s_previousSnap, *s_previousNextSnap;
static playerState_s s_previousPredicted;
static bool s_previousFakeUsed[512];
static unsigned s_previousFakeFlags[512];
static int s_previousFakeCount, s_previousFakeMap, s_previousFakeScript;

static void SP_DetachClientNetwork()
{
    if (!s_clientNetworkAttached) return;
    s_clientNetworkAttached = false;
    if (s_clientNetworkHandedOff)
    {
        // Live entities may have changed or died. Their fields and snapshots
        // now belong to the normal lifecycle, not the bounded probe rollback.
        s_clientNetworkHandedOff = false;
        for (const auto &lease : s_clientNetworkLeases)
            if (lease.acquiredTarget) SL_RemoveRefToString(SCRIPTINSTANCE_CLIENT, lease.acquiredTarget);
        s_clientNetworkLeases.clear();
        return;
    }
    auto *cg = CG_GetLocalClientGlobals(0);
    cg->snap = s_previousSnap; cg->nextSnap = s_previousNextSnap; cg->predictedPlayerState = s_previousPredicted; SP_CNSApplyClock(s_previousNetworkClock);
    for (auto &lease : s_clientNetworkLeases)
    {
        SP_CNSApplyEntity(lease.cent, lease.previous);
        if (lease.acquiredTarget) SL_RemoveRefToString(SCRIPTINSTANCE_CLIENT, lease.acquiredTarget);
    }
    s_clientNetworkLeases.clear();
    for (int i = 0; i < 512; ++i) { cg_fakeEntitiesInuseArray[i] = s_previousFakeUsed[i]; cg_fakeEntitiesArray[i].flags = s_previousFakeFlags[i]; }
    cg_fakeEntitiesInuseCount[0] = s_previousFakeCount; cg_fakeEntitiesInuseCountFromMap = s_previousFakeMap; cg_fakeEntitiesInuseCountFromLoadScript = s_previousFakeScript;
}
static void SP_HandoffClientNetwork()
{
    if (!s_clientNetworkAttached || s_clientNetworkHandedOff)
        Com_Error(ERR_DROP, "Client network handoff requires attached checkpoint owners");
    // CScr_FreeEntityFields / Scr_SetString owns the reference in each CSC
    // targetname slot once canonical frames start. It may consume it before
    // shutdown (script mutation, deletion or snapshot transition).
    for (auto &lease : s_clientNetworkLeases)
        if (EntNum_IsCsc(lease.cent->nextState.number)) lease.acquiredTarget = 0;
    s_clientNetworkHandedOff = true;
}
static void SP_PrepareClientNetworkRetirement()
{
    // A publication error may retire the newly bound entities before the
    // first frame handoff. Their canonical CSC shutdown consumes targetname
    // references too; do not then release or roll back those dead slots.
    if (s_clientNetworkAttached && !s_clientNetworkHandedOff)
        SP_HandoffClientNetwork();
}
static void SP_AttachClientNetwork()
{
    if (!s_clientNetworkPresent) return;
    if (s_clientNetworkAttached) Com_Error(ERR_DROP, "Client network probe already attached");
    auto *cg = CG_GetLocalClientGlobals(0);
    s_previousSnap = cg->snap; s_previousNextSnap = cg->nextSnap; s_previousPredicted = cg->predictedPlayerState; SP_CNSReadClock(s_previousNetworkClock);
    s_previousFakeCount = cg_fakeEntitiesInuseCount[0]; s_previousFakeMap = cg_fakeEntitiesInuseCountFromMap; s_previousFakeScript = cg_fakeEntitiesInuseCountFromLoadScript;
    for (int i = 0; i < 512; ++i) { s_previousFakeUsed[i] = cg_fakeEntitiesInuseArray[i]; s_previousFakeFlags[i] = cg_fakeEntitiesArray[i].flags; }
    s_clientNetworkLeases.reserve(s_clientNetworkState.entities.size()); s_clientNetworkAttached = true;
    for (auto &s : s_clientNetworkState.snapshots) { SP_CNSBindPlayer(s.player); s.value.ps = s.player.value; }
    SP_CNSBindPlayer(s_clientNetworkState.predicted); SP_CNSBindPlayer(s_clientNetworkState.interpolated);
    cg->snap = &s_clientNetworkState.snapshots[s_clientNetworkState.current].value;
    cg->nextSnap = &s_clientNetworkState.snapshots[s_clientNetworkState.next].value;
    cg->predictedPlayerState = s_clientNetworkState.predicted.value; SP_CNSApplyClock(s_clientNetworkState.clock);
    for (int i = 0; i < s_clientNetworkState.entities.size(); ++i)
    {
        auto &o = s_clientNetworkState.entities[i]; centity_s *cent = SP_ClientPoseEntity(s_clientEntityPoses.owners[i]);
        s_clientNetworkLeases.emplace_back(); auto &lease = s_clientNetworkLeases.back(); lease.cent = cent;
        SP_CNSReadEntity(cent, o.kind, o.index, lease.previous);
        if (!o.target.empty()) lease.acquiredTarget = SL_GetString_(SCRIPTINSTANCE_CLIENT, o.target.c_str(), 0, 6);
        SP_CNSApplyEntity(cent, o); cent->nextState.targetname = lease.acquiredTarget;
    }
    for (int i = 0; i < 512; ++i) { cg_fakeEntitiesInuseArray[i] = s_clientEntityRegistry.slots[i].used; cg_fakeEntitiesArray[i].flags = s_clientEntityRegistry.slots[i].flags; }
    cg_fakeEntitiesInuseCount[0] = s_clientEntityRegistry.count; cg_fakeEntitiesInuseCountFromMap = s_clientEntityRegistry.fromMap; cg_fakeEntitiesInuseCountFromLoadScript = s_clientEntityRegistry.fromScript;
    if (CG_GetPredictedPlayerState(0) != &cg->predictedPlayerState) Com_Error(ERR_DROP, "Client prediction consumer identity differs");
    static SPClientNetworkState actual; SP_CNSReadCurrent(actual);
    static unsigned char expectedBytes[0x200000], actualBytes[0x200000]; static MemoryFile expected, check;
    MemFile_InitForWriting(&expected, sizeof(expectedBytes), expectedBytes, true, true);
    MemFile_InitForWriting(&check, sizeof(actualBytes), actualBytes, true, true);
    SPNativeCheckpointCodec a{nullptr, &expected}, b{nullptr, &check}; SP_ClientNetworkCodec(a, s_clientNetworkState); SP_ClientNetworkCodec(b, actual);
    MemFile_StartSegment(&expected, -1); MemFile_StartSegment(&check, -1);
    if (expected.bytesUsed != check.bytesUsed || memcmp(expectedBytes, actualBytes, expected.bytesUsed))
        Com_Error(ERR_DROP, "Client network live state/canonical interpolation differs");
    int used = 0; for (int i = 0; i < 512; ++i) used += cg_fakeEntitiesInuseArray[i];
    if (used != cg_fakeEntitiesInuseCount[0]) Com_Error(ERR_DROP, "Client network fake pool publication differs");
    Com_Printf(15, "Client network publication: FRESH PROCESS %i entity wire states / %i fake slots / %i shared snapshots, clock/prediction and actual canonical interpolation LIVE RESERIALIZATION IDENTICAL; NO CLIENT FRAMES\n",
        static_cast<int>(actual.entities.size()), used, static_cast<int>(actual.snapshots.size()));
}
#undef SP_CNS_CG_FIELDS
#undef SP_CNS_CL_FIELDS
