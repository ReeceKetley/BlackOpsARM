// CBT1 preserves the bolt ring's borrowers, not a second entity image.
#include <cgame/cg_bolt.h>
static CGBoltSavedState s_clientBolts;
static bool s_clientBoltsPresent;

static void SP_ClientBoltsCodec(SPNativeCheckpointCodec &c, CGBoltSavedState &state)
{
    if (c.Int(0x31544243) != 0x31544243) Com_Error(ERR_DROP, "Client bolt checkpoint schema invalid");
    c.Field(state.cursor);
    for (auto &key : state.slots) { c.Field(key.number); c.Field(key.currentUse); c.Field(key.nextUse); }
    if (!CG_ValidateSavedBolts(state)) Com_Error(ERR_DROP, "Client bolt checkpoint rejected ring identities");
}
static void SP_PreflightClientBolts(bool legacy)
{
    if (!CG_ValidateSavedBolts(s_clientBolts))
        Com_Error(ERR_DROP, "Client bolt checkpoint missing entity owners");
    for (const auto &key : s_clientBolts.slots)
    {
        if (key.number == -1)
        {
            if (legacy) Com_Error(ERR_DROP, "Legacy client bolt pool cannot identify missing slot");
            continue;
        }
        if (key.number >= s_clientEntityRegistry.end || key.number >= s_clientEntityPoses.end)
            Com_Error(ERR_DROP, "Client bolt checkpoint rejected fake entity range");
        const auto &slot = s_clientEntityRegistry.slots[key.number - 1024];
        if (!slot.used || slot.type != ET_SCRIPTMOVER || slot.currentUse != key.currentUse || slot.nextUse != key.nextUse)
            Com_Error(ERR_DROP, "Client bolt checkpoint rejected fake entity generation");
        const SPClientEntityPose *pose = nullptr;
        for (const auto &p : s_clientEntityPoses.owners) if (!p.kind && p.index == key.number) { pose = &p; break; }
        const SPClientNetworkEntity *net = nullptr;
        for (const auto &n : s_clientNetworkState.entities) if (!n.kind && n.index == key.number) { net = &n; break; }
        if (!pose || !net || pose->type != ET_SCRIPTMOVER || pose->currentUse != key.currentUse || pose->nextUse != key.nextUse
            || net->next.number != key.number || net->next.eType != ET_SCRIPTMOVER
            || net->current.useCount != key.currentUse || net->next.lerp.useCount != key.nextUse)
            Com_Error(ERR_DROP, "Client bolt checkpoint disagrees with pose/network owners");
        if (legacy && (!(net->current.eFlags & 0x20) || !(net->next.lerp.eFlags & 0x20)
            || (net->next.lerp.eFlags2 & 0x80000) || net->next.index.brushmodel
            || net->next.lerp.u.player.meleeWeapon != 1023 || pose->linked || pose->moving))
            Com_Error(ERR_DROP, "Legacy client bolt pool has active history without saved cursor");
    }
}
static void SP_WriteClientBolts(MemoryFile *file)
{
    if (!CG_CaptureBolts(0, s_clientBolts)) Com_Error(ERR_DROP, "Client bolt checkpoint rejected live borrowers");
    SP_PreflightClientBolts(false);
    SPNativeCheckpointCodec c{nullptr, file}; SP_ClientBoltsCodec(c, s_clientBolts);
    Com_Printf(15, "Client bolt checkpoint: captured cursor %u / ten logical slots\n", s_clientBolts.cursor);
}
static void SP_DecodeClientBolts(SPNativeCheckpointCodec &c)
{
    s_clientBolts = {}; SP_ClientBoltsCodec(c, s_clientBolts);
    s_clientBoltsPresent = true;
}
static void SP_StageClientBolts(int version)
{
    if (!s_clientEntityPosesPresent || !s_clientNetworkPresent)
        Com_Error(ERR_DROP, "Client bolt checkpoint missing saved pose/network sections");
    if (version < 13)
    {
        // Old formats omitted the ring. Only an untouched initial pool can be
        // proved from the fresh map's identities and durable entity contents.
        if (!CG_CaptureBolts(0, s_clientBolts) || s_clientBolts.cursor)
            Com_Error(ERR_DROP, "Legacy client bolt pool has no pristine initial mapping");
    }
    SP_PreflightClientBolts(version < 13);
    s_clientBoltsPresent = true;
    if (version < 13) Com_Printf(15, "Client bolt checkpoint: LEGACY pristine empty pool proved; cursor seeded zero\n");
}
static void SP_PublishClientBolts()
{
    if (!s_clientBoltsPresent || !CG_ApplyBolts(0, s_clientBolts))
        Com_Error(ERR_DROP, "Client bolt checkpoint could not resolve live owners");
    CGBoltSavedState check{};
    if (!CG_CaptureBolts(0, check) || check.cursor != s_clientBolts.cursor)
        Com_Error(ERR_DROP, "Client bolt checkpoint live cursor differs");
    for (int i = 0; i < 10; ++i)
        if (check.slots[i].number != s_clientBolts.slots[i].number
            || check.slots[i].currentUse != s_clientBolts.slots[i].currentUse
            || check.slots[i].nextUse != s_clientBolts.slots[i].nextUse)
            Com_Error(ERR_DROP, "Client bolt checkpoint live borrower differs");
    Com_Printf(15, "Client bolt checkpoint: cursor %u / ten live borrower identities IDENTICAL before input frames\n", check.cursor);
}
