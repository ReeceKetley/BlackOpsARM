// DYN1 placed; DYN2 sparse; DYN3 timed; DYN4 joints; DYN5 ropes; DYN6 server cohort.
#include <dynentity/DynEntity_load_obj.h>
#include <client/client.h>
#include <cgame_mp/cg_local_mp.h>
#include <EffectsCore/fx_unique_handle.h>
#include <EffectsCore/fx_system.h>
#include <universal/surfaceflags.h>
#include <database/db_registry.h>
#include <gfx_d3d/r_init.h>
#include <physics/physpreset_load_obj.h>
extern int numExtraDynEnts, gNumFadingDynEnts, gNumBurningDynEnts;
extern unsigned __int8 usedExtraDynEnts[256];
extern DynEnt_BurnData gDynEntBurnData[16];
extern DynEnt_FadeData gDynEntFadeData[32];
struct SPDynEntRecord
{
    int draw = 0, index = 0, type = 0, brush = 0, physicsBrush = 0;
    std::string model, destroyedModel, preset;
    GfxPlacement initial{};
    DynEntityPose pose{};
    DynEntityClient client{};
    int serverFlags = -1, serverHealth = 0, hasBody = 0;
    PhysSavedObjectState body{};
};
static std::vector<SPDynEntRecord> s_savedDynEnts;
static bool s_savedDynEntsPresent, s_savedDynEntsPublished, s_savedDynTopologyPublished;
static int s_savedDynEntCounts[4], s_savedDynSchema = 0x364E5944;
static int s_savedDynHighWater, s_savedDynExtraCount;
static unsigned char s_savedDynUsed[256];
static int s_savedDynQueueClock;
static std::vector<DynEnt_BurnData> s_savedDynBurns;
static std::vector<DynEnt_FadeData> s_savedDynFades;
static std::string SP_DynModelName(const XModel *model) { return model ? XModelGetName(model) : ""; }
static bool SP_DynSpawned(const SPDynEntRecord &r) { return !r.draw && r.index >= cm.originalDynEntCount; }
static bool SP_DynPlacementValid(const GfxPlacement &p)
{
    for (float f : p.origin) if (!std::isfinite(f)) return false;
    float norm = 0; for (float f : p.quat) { if (!std::isfinite(f)) return false; norm += f * f; }
    return std::fabs(norm - 1) <= 0.004f;
}
static XAssetHeader SP_DynExistingAsset(XAssetType type, const std::string &name)
{
    // Find only real loaded assets. DB_FindXAssetHeader creates default entries
    // for absent model/preset names even when errorIfMissing is false.
    XAssetHeader result{};
    Sys_EnterCriticalSection(CRITSECT_DBHASH);
    const auto *entry = DB_FindXAssetEntry(type, name.c_str());
    if (entry && entry->entry.zoneIndex) result = entry->entry.asset.header;
    Sys_LeaveCriticalSection(CRITSECT_DBHASH);
    return result;
}
static bool SP_DynSpawnDefinition(const SPDynEntRecord &r, DynEntityDef &d)
{
    if (!SP_DynSpawned(r) || r.index >= cm.originalDynEntCount + 256 || r.type != DYNENT_TYPE_CLUTTER
        || r.brush || r.physicsBrush || !r.destroyedModel.empty() || r.model.empty() || r.model.size() > 255
        || r.preset.empty() || r.preset.size() > 255 || !SP_DynPlacementValid(r.initial) || r.serverFlags != -1) return false;
    auto *model = SP_DynExistingAsset(ASSET_TYPE_XMODEL, r.model).model;
    auto *preset = SP_DynExistingAsset(ASSET_TYPE_PHYSPRESET, r.preset).physPreset;
    if (!model || !preset || SP_DynModelName(model) != r.model || !preset->name || r.preset != preset->name
        || (model->physPreset && model->physPreset != preset)) return false;
    // The canonical spawned factory initializes precisely this definition.
    // Asset references are resolved afresh, never persisted as native pointers.
    d = {}; d.type = DYNENT_TYPE_CLUTTER; d.pose = r.initial; d.xModel = model;
    d.physPreset = preset; d.flags = 2; d.contents = model->contents;
    for (auto &constraint : d.physConstraints) constraint = 511;
    return true;
}
static bool SP_DynSpawnMatchesLive(const SPDynEntRecord &r)
{
    if (r.index >= cm.dynEntCount[0]) return false;
    DynEntityDef wanted{}; if (!SP_DynSpawnDefinition(r, wanted)) return false;
    const auto &d = cm.dynEntDefList[0][r.index];
    return d.type == wanted.type && !memcmp(&d.pose, &wanted.pose, sizeof(d.pose))
        && d.xModel == wanted.xModel && !d.destroyedxModel && !d.brushModel && !d.physicsBrushModel
        && !d.destroyFx && !d.destroySound && !d.destroyPieces && d.physPreset == wanted.physPreset
        && !memcmp(d.physConstraints, wanted.physConstraints, sizeof(d.physConstraints))
        && d.health == 0 && d.flags == 2 && d.contents == wanted.contents && !d.targetname && !d.target;
}
static bool SP_DynDefinitionMatches(const SPDynEntRecord &r, bool graph = false)
{
    if ((unsigned)r.draw >= 2 || r.index < 0) return false;
    if (SP_DynSpawned(r)) { DynEntityDef d{}; return SP_DynSpawnDefinition(r, d); }
    if (r.index >= cm.dynEntCount[r.draw]) return false;
    const auto &d = cm.dynEntDefList[r.draw][r.index];
    if (r.type != d.type || r.type <= DYNENT_TYPE_INVALID || r.type >= DYNENT_TYPE_COUNT) return false;
    if (r.hasBody)
    {
        if (!d.physPreset || !DynEnt_GetEntityProps(d.type)->usePhysics) return false;
        for (short constraint : d.physConstraints) if (constraint != 511 && constraint != -1)
            if (!graph || constraint < 0 || constraint >= cm.num_constraints || !cm.constraints) return false;
    }
    return r.brush == d.brushModel && r.physicsBrush == d.physicsBrushModel
        && r.model == SP_DynModelName(d.xModel) && r.destroyedModel == SP_DynModelName(d.destroyedxModel)
        && !memcmp(&r.initial, &d.pose, sizeof(r.initial));
}
static bool SP_DynStateValid(const SPDynEntRecord &r, bool timed = false)
{
    if (r.client.physObjId || r.client.lightingHandle || (r.hasBody != 0 && r.hasBody != 1)
        || r.serverFlags < -1 || r.serverFlags > 65535 || (r.client.flags & ~15u)
        || (timed ? (r.client.burnTime > 30000 || r.client.fadeTime > 1000)
                  : ((r.client.flags & 8) || r.client.burnTime || r.client.fadeTime))) return false;
    return SP_DynPlacementValid(r.pose.pose) && std::isfinite(r.pose.radius) && r.pose.radius >= 0
        && (!r.hasBody || (r.body.owner == -1 && Phys_IsSavedObjectStateValid(r.body)));
}
static const SPDynEntRecord *SP_DynQueueOwner(const std::vector<SPDynEntRecord> &records, unsigned id)
{
    for (const auto &r : records)
        if (id == unsigned(r.index + (r.draw ? cm.originalDynEntCount + 256 : 0))) return &r;
    return nullptr;
}
static bool SP_DynQueuesValid(const std::vector<SPDynEntRecord> &records, int clock,
    const std::vector<DynEnt_BurnData> &burns, const std::vector<DynEnt_FadeData> &fades)
{
    if (clock < 0 || burns.size() > 16 || fades.size() > 32) return false;
    for (unsigned i = 0; i < burns.size(); ++i)
    {
        const auto &b = burns[i]; const auto *r = SP_DynQueueOwner(records, b.id);
        if (!r || !(r->client.flags & 8) || !(b.fx >> 16) || (b.fx >> 16) > 1024) return false;
        for (unsigned j = 0; j < i; ++j) if (burns[j].id == b.id || burns[j].fx == b.fx) return false;
    }
    for (unsigned i = 0; i < fades.size(); ++i)
    {
        const auto &f = fades[i]; const auto *r = SP_DynQueueOwner(records, f.id);
        // Normal fade retirement destroys spawned models, never placed/brush owners.
        if (!r || !SP_DynSpawned(*r) || !(r->client.flags & 3) || f.startTime < 0 || f.startTime > clock) return false;
        for (unsigned j = 0; j < i; ++j) if (fades[j].id == f.id) return false;
    }
    for (const auto &r : records)
    {
        const unsigned id = r.index + (r.draw ? cm.originalDynEntCount + 256 : 0);
        bool burning = false, fading = false;
        for (const auto &b : burns) burning |= b.id == id;
        for (const auto &f : fades) fading |= f.id == id;
        if (bool(r.client.flags & 8) != burning || (r.client.fadeTime && !fading)) return false;
    }
    return true;
}
static bool SP_DynBurnEffectMatches(const DynEnt_BurnData &b, const FxEffectContainer *owner)
{
    if (!owner || owner->effect.uniqueHandle != b.fx || !owner->effect.def
        || strcmp(owner->effect.def->name, "destructibles/fx_dest_fire_trail_sm")) return false;
    const auto &e = owner->effect;
    if (e.msecLastUpdate < e.msecBegin) return false;
    return FX_NeedsBoltUpdate(e.def)
        ? e.boltAndSortOrder.type == 2 && e.boltAndSortOrder.absDynEntId == b.id
        : e.boltAndSortOrder.type == 0;
}
static bool SP_DynStagedEffectsValid(const FxSystemBuffers &buffers, const FxSystemShared &shared)
{
    if (!s_savedDynEntsPresent) return true;
    if (shared.firstFreeEffect - shared.firstActiveEffect > 1024) return false;
    const unsigned stride = sizeof(FxEffectContainer) / 4;
    for (const auto &b : s_savedDynBurns)
    {
        const unsigned nodeIndex = (b.fx >> 16) - 1;
        if (nodeIndex >= 1024) return false;
        const auto &node = buffers.uniqueHandleDb.nodes[nodeIndex];
        if (node.handle != b.fx || node.nextNode != -1 || node.effect % stride || node.effect / stride >= 1024) return false;
        bool active = false;
        for (unsigned n = shared.firstActiveEffect; n != shared.firstFreeEffect; ++n)
            active |= shared.allEffectHandles[n & 1023] == node.effect;
        if (!active || !SP_DynBurnEffectMatches(b, &buffers.effects[node.effect / stride])) return false;
    }
    return true;
}
static bool SP_DynTopologyValid(int highWater, int count, const unsigned char *used)
{
    const int extent = highWater - cm.originalDynEntCount;
    if (extent < 0 || extent > 256 || count < 0 || count > extent || highWater > 4096) return false;
    int actual = 0;
    for (int i = 0; i < 256; ++i)
    {
        if (used[i] > 1 || (i >= extent && used[i])) return false;
        actual += used[i];
    }
    if (actual != count) return false;
    // These renderer capacities were established from the map's loaded array
    // capacities, before client init reduced the logical count to originals.
    return !extent || (cm.dynEntDefList[0] && cm.dynEntClientList[0] && cm.dynEntPoseList[0] && cm.dynEntCollList[0]
        && rgp.world && rgp.world->dpvsDyn.dynEntClientCount[0] >= (unsigned)highWater);
}
static SPDynEntRecord SP_DynCaptureRecord(int draw, int i, bool graph = false)
{
    SPDynEntRecord r; const auto &d = cm.dynEntDefList[draw][i];
    r.draw = draw; r.index = i; r.type = d.type; r.brush = d.brushModel; r.physicsBrush = d.physicsBrushModel;
    r.model = SP_DynModelName(d.xModel); r.destroyedModel = SP_DynModelName(d.destroyedxModel); r.initial = d.pose;
    r.pose = cm.dynEntPoseList[draw][i]; r.client = cm.dynEntClientList[draw][i];
    r.hasBody = r.client.physObjId != 0;
    if (SP_DynSpawned(r)) r.preset = d.physPreset && d.physPreset->name ? d.physPreset->name : "";
    if (r.hasBody && !graph && !Phys_CaptureObjectState(0, r.client.physObjId, r.body)) r.hasBody = -1;
    r.client.physObjId = 0; r.client.lightingHandle = 0;
    if (i < cm.dynEntCount[draw + 2]) { const auto &s = cm.dynEntServerList[draw][i]; r.serverFlags = s.flags; r.serverHealth = s.health; }
    return r;
}
#include "g_sp_dynent_joints_checkpoint.inl"
static bool SP_DynEntCaptureSupported()
{
    if (gNumBurningDynEnts < 0 || gNumBurningDynEnts > 16 || gNumFadingDynEnts < 0 || gNumFadingDynEnts > 32
        || !SP_DynTopologyValid(cm.dynEntCount[0], numExtraDynEnts, usedExtraDynEnts))
    {
        Com_Printf(15, "Dynamic checkpoint: invalid queue count or spawned topology; live match and prior archive retained\n");
        return false;
    }
    std::vector<SPDynEntRecord> records;
    for (int draw = 0; draw < 2; ++draw)
        for (int i = 0; i < cm.dynEntCount[draw]; ++i)
        {
            if (!draw && i >= cm.originalDynEntCount && !usedExtraDynEnts[i - cm.originalDynEntCount]) continue;
            const auto r = SP_DynCaptureRecord(draw, i, true);
            if (!SP_DynDefinitionMatches(r, true) || (SP_DynSpawned(r) && !SP_DynSpawnMatchesLive(r))) return false;
            records.push_back(r);
        }
    std::vector<SPDynJointBinding> bindings; RopeSavedSystem ropes;
    std::vector<SPServerBodyRecord> servers;
    if (!SP_CollectLiveServerBodies(servers, true) || !SP_DynCaptureJointGraph(records, bindings, &ropes, false, &servers)
        || ropes.clock > CG_GetLocalClientGlobals(0)->time) return false;
    for (const auto &r : records) if (!SP_DynStateValid(r, true)) return false;
    const std::vector<DynEnt_BurnData> burns(gDynEntBurnData, gDynEntBurnData + gNumBurningDynEnts);
    const std::vector<DynEnt_FadeData> fades(gDynEntFadeData, gDynEntFadeData + gNumFadingDynEnts);
    if (!SP_DynQueuesValid(records, CG_GetLocalClientGlobals(0)->time, burns, fades)) return false;
    for (const auto &b : burns) if (!SP_DynBurnEffectMatches(b, UniqueHandleToEffect(0, b.fx))) return false;
    return true;
}
static void SP_CaptureDynEnts()
{
    if (!SP_DynEntCaptureSupported()) Com_Error(ERR_DROP, "Dynamic checkpoint unsupported native owner graph");
    s_savedDynSchema = 0x364E5944;
    if (s_serverBodySchema != 0x33505053) Com_Error(ERR_DROP, "Unified physics capture requires paired SPP3 owners");
    s_savedDynQueueClock = CG_GetLocalClientGlobals(0)->time;
    s_savedDynBurns.assign(gDynEntBurnData, gDynEntBurnData + gNumBurningDynEnts);
    s_savedDynFades.assign(gDynEntFadeData, gDynEntFadeData + gNumFadingDynEnts);
    s_savedDynEnts.clear(); s_savedDynEntsPresent = true; s_savedDynEntsPublished = s_savedDynTopologyPublished = false;
    for (int i = 0; i < 4; ++i) s_savedDynEntCounts[i] = i ? cm.dynEntCount[i] : cm.originalDynEntCount;
    s_savedDynHighWater = cm.dynEntCount[0]; s_savedDynExtraCount = numExtraDynEnts;
    memcpy(s_savedDynUsed, usedExtraDynEnts, sizeof(s_savedDynUsed));
    for (int draw = 0; draw < 2; ++draw)
        for (int i = 0; i < s_savedDynEntCounts[draw]; ++i) s_savedDynEnts.push_back(SP_DynCaptureRecord(draw, i, true));
    for (int i = 0; i < 256; ++i) if (s_savedDynUsed[i])
        s_savedDynEnts.push_back(SP_DynCaptureRecord(0, cm.originalDynEntCount + i, true));
    if (!SP_DynCaptureJointGraph(s_savedDynEnts, s_savedDynBindings, &s_savedDynRopes, false, &s_serverBodies)) Com_Error(ERR_DROP, "Dynamic checkpoint joint graph changed during capture");
}
static void SP_DynEntCodec(MemoryFile *file, bool reading)
{
    const auto value = [file, reading](auto &v) { if (reading) MemFile_ReadData(file, sizeof(v), (unsigned char *)&v); else MemFile_WriteData(file, sizeof(v), (unsigned char *)&v); };
    const auto name = [file, reading](std::string &s) { if (reading) s = MemFile_ReadCString(file); else MemFile_WriteCString(file, const_cast<char *>(s.c_str())); if (s.size() > 255) Com_Error(ERR_DROP, "Dynamic checkpoint name too long"); };
    value(s_savedDynSchema); if (s_savedDynSchema != 0x314E5944 && s_savedDynSchema != 0x324E5944 && s_savedDynSchema != 0x334E5944 && s_savedDynSchema != 0x344E5944 && s_savedDynSchema != 0x354E5944 && s_savedDynSchema != 0x364E5944) Com_Error(ERR_DROP, "Dynamic checkpoint schema invalid");
    if ((s_savedDynSchema >= 0x364E5944) != (s_serverBodySchema == 0x33505053))
        Com_Error(ERR_DROP, "Unified physics checkpoint server/dynamic schema pairing differs");
    if (reading) { s_savedDynEnts.clear(); s_savedDynEntsPresent = false; s_savedDynEntsPublished = s_savedDynTopologyPublished = false; }
    for (int i = 0; i < 4; ++i) { value(s_savedDynEntCounts[i]); if (s_savedDynEntCounts[i] != (i ? cm.dynEntCount[i] : cm.originalDynEntCount)) Com_Error(ERR_DROP, "Dynamic checkpoint map inventory differs"); }
    unsigned cursor = 0;
    const auto record = [&](int draw, int i, bool spawned)
    {
        if (reading) s_savedDynEnts.emplace_back();
        if (cursor >= s_savedDynEnts.size()) Com_Error(ERR_DROP, "Dynamic checkpoint incomplete inventory");
        auto &r = s_savedDynEnts[cursor++]; r.draw = draw; r.index = i;
        value(r.type); value(r.brush); value(r.physicsBrush); name(r.model); name(r.destroyedModel); value(r.initial);
        if (spawned) name(r.preset);
        value(r.pose); value(r.client); value(r.serverFlags); value(r.serverHealth); value(r.hasBody);
        if (r.hasBody) value(r.body);
        if (!SP_DynDefinitionMatches(r, s_savedDynSchema >= 0x344E5944) || !SP_DynStateValid(r, s_savedDynSchema >= 0x334E5944)
            || (i < s_savedDynEntCounts[draw + 2]) != (r.serverFlags >= 0)) Com_Error(ERR_DROP, "Dynamic checkpoint rejected owner/state");
    };
    for (int draw = 0; draw < 2; ++draw)
        for (int i = 0; i < s_savedDynEntCounts[draw]; ++i) record(draw, i, false);
    if (s_savedDynSchema >= 0x324E5944)
    {
        int original = cm.originalDynEntCount; value(original);
        value(s_savedDynHighWater); value(s_savedDynExtraCount); value(s_savedDynUsed);
        if (original != cm.originalDynEntCount || !SP_DynTopologyValid(s_savedDynHighWater, s_savedDynExtraCount, s_savedDynUsed))
            Com_Error(ERR_DROP, "Dynamic checkpoint rejected sparse topology");
        for (int i = 0; i < 256; ++i) if (s_savedDynUsed[i]) record(0, original + i, true);
    }
    else { s_savedDynHighWater = cm.originalDynEntCount; s_savedDynExtraCount = 0; memset(s_savedDynUsed, 0, sizeof(s_savedDynUsed)); }
    if (cursor != s_savedDynEnts.size()) Com_Error(ERR_DROP, "Dynamic checkpoint unexpected records");
    if (s_savedDynSchema >= 0x334E5944)
    {
        value(s_savedDynQueueClock);
        int burns = (int)s_savedDynBurns.size(), fades = (int)s_savedDynFades.size(); value(burns); value(fades);
        if (burns < 0 || burns > 16 || fades < 0 || fades > 32) Com_Error(ERR_DROP, "Dynamic checkpoint rejected queue count");
        if (reading) { s_savedDynBurns.resize(burns); s_savedDynFades.resize(fades); }
        for (auto &b : s_savedDynBurns) { value(b.id); value(b.fx); }
        for (auto &f : s_savedDynFades) { value(f.id); value(f.startTime); }
        if (!SP_DynQueuesValid(s_savedDynEnts, s_savedDynQueueClock, s_savedDynBurns, s_savedDynFades))
            Com_Error(ERR_DROP, "Dynamic checkpoint rejected timed owner graph");
    }
    else { s_savedDynQueueClock = 0; s_savedDynBurns.clear(); s_savedDynFades.clear(); }
    if (s_savedDynSchema >= 0x344E5944) SP_DynJointBindingsCodec(file, reading);
    else s_savedDynBindings.clear();
    if (s_savedDynSchema >= 0x354E5944)
    {
        if (reading) Rope_ReadCheckpoint(file, s_savedDynRopes); else Rope_WriteCheckpoint(file, s_savedDynRopes);
        if (s_savedDynRopes.clock > s_savedDynQueueClock || !SP_DynBindingsValid(s_savedDynEnts, s_savedDynBindings, &s_savedDynRopes,
            false, s_savedDynSchema >= 0x364E5944 ? &s_serverBodies : nullptr))
            Com_Error(ERR_DROP, "Dynamic checkpoint invalid rope/map owner graph");
    }
    else s_savedDynRopes = {};
    s_savedDynEntsPresent = true;
}
static void SP_PublishDynEntServer()
{
    if (!s_savedDynEntsPresent) return;
    for (const auto &r : s_savedDynEnts) if (!SP_DynDefinitionMatches(r, s_savedDynSchema >= 0x344E5944)) Com_Error(ERR_DROP, "Dynamic checkpoint lost map owner");
    for (const auto &r : s_savedDynEnts) if (r.serverFlags >= 0)
    {
        DynEnt_UnlinkEntity((DynEntityCollType)(r.draw + 2), r.index);
        auto &s = cm.dynEntServerList[r.draw][r.index]; s.flags = r.serverFlags; s.health = r.serverHealth;
        if (s.flags & 1) { if (r.draw) DynEntSv_LinkBrush(r.index); else DynEntSv_LinkModel(r.index); }
        if (s.flags != r.serverFlags || s.health != r.serverHealth) Com_Error(ERR_DROP, "Dynamic checkpoint server readback differs");
    }
}
static void SP_PublishDynEntTopology()
{
    if (!s_savedDynEntsPresent || s_savedDynTopologyPublished) return;
    if (!SP_DynTopologyValid(s_savedDynHighWater, s_savedDynExtraCount, s_savedDynUsed)
        || !SP_DynTopologyValid(cm.dynEntCount[0], numExtraDynEnts, usedExtraDynEnts))
        Com_Error(ERR_DROP, "Dynamic checkpoint lost sparse storage capacity");
    for (const auto &r : s_savedDynEnts) if (!SP_DynDefinitionMatches(r, s_savedDynSchema >= 0x344E5944)) Com_Error(ERR_DROP, "Dynamic checkpoint lost topology definition");
    // Old CLIENT/FX holders have retired; remove any fresh-map spawned owners
    // through their canonical destruction path before replacing their slots.
    DynEnt_ResetBurning(); DynEnt_ResetFading();
    DynEntCl_SP_CleanupSpawnedDynEnts();
    cm.dynEntCount[0] = s_savedDynHighWater;
    const int extent = s_savedDynHighWater - cm.originalDynEntCount;
    for (int i = 0; i < extent; ++i)
    {
        const int index = cm.originalDynEntCount + i;
        cm.dynEntClientList[0][index] = {}; cm.dynEntPoseList[0][index] = {}; cm.dynEntCollList[0][index] = {};
    }
    numExtraDynEnts = 0; memset(usedExtraDynEnts, 0, sizeof(usedExtraDynEnts));
    for (const auto &r : s_savedDynEnts) if (SP_DynSpawned(r))
    {
        auto &d = const_cast<DynEntityDef &>(cm.dynEntDefList[0][r.index]);
        if (!SP_DynSpawnDefinition(r, d)) Com_Error(ERR_DROP, "Dynamic checkpoint missing spawned definition after preflight");
        cm.dynEntClientList[0][r.index] = r.client;
        cm.dynEntPoseList[0][r.index] = r.pose;
        // Publish only initialized owners. If linking aborts, normal cleanup
        // sees precisely the slots that already acquired live ownership.
        usedExtraDynEnts[r.index - cm.originalDynEntCount] = 1; ++numExtraDynEnts;
        if (r.client.flags & 1) DynEntCl_LinkModel(r.index);
        if (!SP_DynSpawnMatchesLive(r)) Com_Error(ERR_DROP, "Dynamic checkpoint spawned definition readback differs");
    }
    if (numExtraDynEnts != s_savedDynExtraCount || memcmp(usedExtraDynEnts, s_savedDynUsed, sizeof(s_savedDynUsed)))
        Com_Error(ERR_DROP, "Dynamic checkpoint sparse ownership readback differs");
    s_savedDynTopologyPublished = true;
    Com_Printf(15, "Dynamic checkpoint: sparse topology published %i spawned owners / extent %i before saved FX; native bodies deferred\n", s_savedDynExtraCount, extent);
}
static void SP_PublishDynEntClient()
{
    if (!s_savedDynEntsPresent || s_savedDynEntsPublished) return;
    SP_PublishDynEntTopology();
    for (const auto &r : s_savedDynEnts) if (!SP_DynDefinitionMatches(r, s_savedDynSchema >= 0x344E5944) || (SP_DynSpawned(r) && !SP_DynSpawnMatchesLive(r))) Com_Error(ERR_DROP, "Dynamic checkpoint lost publication owner");
    if (s_savedDynSchema >= 0x354E5944 && !Rope_CanApplyCheckpoint(s_savedDynRopes))
        Com_Error(ERR_DROP, "Dynamic checkpoint rope resource/owner preflight failed");
    if (s_savedDynSchema >= 0x364E5944 && (s_serverBodySchema != 0x33505053
        || !SP_MixedServerTargetsValid(s_savedDynBindings, s_serverBodies)))
        Com_Error(ERR_DROP, "Unified physics checkpoint server target identity differs");
    if (s_savedDynSchema >= 0x344E5944) SP_RetireDynJointBindings();
    for (const auto &r : s_savedDynEnts)
    {
        const auto &d = cm.dynEntDefList[r.draw][r.index]; auto &c = cm.dynEntClientList[r.draw][r.index];
        if (c.physObjId) DynEnt_DestroyPhysics(PHYS_WORLD_DYNENT, &d, &c);
        DynEntCl_UnlinkEntity(r.index, (DynEntityCollType)r.draw);
        const auto lighting = c.lightingHandle; c = r.client; c.lightingHandle = lighting;
        cm.dynEntPoseList[r.draw][r.index] = r.pose;
        if (r.hasBody)
        {
            c.physObjId = DynEntCl_CreateCheckpointPhysObj(&d, &c, &r.pose.pose);
            if (!c.physObjId || (s_savedDynSchema < 0x344E5944 && !Phys_ApplyObjectState(0, c.physObjId, r.body))) Com_Error(ERR_DROP, "Dynamic checkpoint fresh body differs");
        }
        if (c.flags & 1) { if (r.draw) DynEntCl_LinkBrush(r.index); else DynEntCl_LinkModel(r.index); }
        DynEntityClient check = c; check.physObjId = 0; check.lightingHandle = 0;
        const auto &pose = cm.dynEntPoseList[r.draw][r.index];
        if (memcmp(&check, &r.client, sizeof(check)) || memcmp(&pose.pose, &r.pose.pose, sizeof(r.pose.pose)))
            Com_Error(ERR_DROP, "Dynamic checkpoint client/pose readback differs");
        if ((c.flags & 1) && !r.draw && pose.radius != XModelGetRadius(DynEntCl_GetCurrentXModel(&d, &c)))
            Com_Error(ERR_DROP, "Dynamic checkpoint did not rebuild its model radius cache");
    }
    if (s_savedDynSchema >= 0x364E5944) SP_PublishServerBodies(true);
    if (s_savedDynSchema >= 0x344E5944) SP_PublishDynJointGraph();
    if (!SP_DynQueuesValid(s_savedDynEnts, s_savedDynQueueClock, s_savedDynBurns, s_savedDynFades))
        Com_Error(ERR_DROP, "Dynamic checkpoint lost timed owners");
    for (const auto &b : s_savedDynBurns)
        if (!SP_DynBurnEffectMatches(b, UniqueHandleToEffect(0, b.fx))) Com_Error(ERR_DROP, "Dynamic checkpoint lost live burn FX owner");
    if (gNumBurningDynEnts || gNumFadingDynEnts) Com_Error(ERR_DROP, "Dynamic checkpoint fresh queues not empty");
    memset(gDynEntBurnData, 0, sizeof(gDynEntBurnData)); memset(gDynEntFadeData, 0, sizeof(gDynEntFadeData));
    for (unsigned i = 0; i < s_savedDynBurns.size(); ++i) gDynEntBurnData[i] = s_savedDynBurns[i];
    for (unsigned i = 0; i < s_savedDynFades.size(); ++i) gDynEntFadeData[i] = s_savedDynFades[i];
    gNumBurningDynEnts = (int)s_savedDynBurns.size(); gNumFadingDynEnts = (int)s_savedDynFades.size();
    s_savedDynEntsPublished = true;
    Com_Printf(15, "Dynamic checkpoint: %i burning / %i fading leases reconnected to original owners and FX; saved deadlines retained, NO EFFECT RESTART\n", gNumBurningDynEnts, gNumFadingDynEnts);
    Com_Printf(15, "Dynamic checkpoint: %u owners (%i spawned) published with native collision/render links; saved health/flags/timing IDENTICAL\n", (unsigned)s_savedDynEnts.size(), s_savedDynExtraCount);
}

static int s_dynMotionPauseOwner = -1;
static void SP_PollDynEntMotionPause()
{
    if (s_dynMotionPauseOwner < 0) return;
    if (s_continuousZombieResume || CL_GetLocalClientConnectionState(0) != CA_ACTIVE
        || !G_SP_IsSPLevel() || SP_IsFrontendMapForSave()) { s_dynMotionPauseOwner = -1; return; }
    if (Dvar_GetInt("cl_paused")) return;
    Com_SyncThreads();
    const int index = s_dynMotionPauseOwner;
    if (index >= cm.dynEntCount[0] || (index >= cm.originalDynEntCount && !usedExtraDynEnts[index - cm.originalDynEntCount]))
    { s_dynMotionPauseOwner = -1; return; }
    PhysSavedObjectState body;
    const int id = cm.dynEntClientList[0][index].physObjId;
    if (!id || !Phys_CaptureObjectState(0, id, body)) return;
    float speed2 = 0; for (float v : body.m_t_vel) speed2 += v * v;
    if (speed2 <= 1) return;
    s_dynMotionPauseOwner = -1;
    Com_Printf(15, "Dynamic motion boundary: actual owner %i body %i clock %i velocity %.9g %.9g %.9g; invoking canonical pause, no body state changed\n",
        index, id, CG_GetLocalClientGlobals(0)->time, body.m_t_vel[0], body.m_t_vel[1], body.m_t_vel[2]);
    char command[] = "pause"; Cmd_ExecuteSingleCommand(0, 0, command);
}
void G_SP_ProbeDynEntCheckpoint()
{
    if (!Sys_IsMainThread() || CL_GetLocalClientConnectionState(0) != CA_ACTIVE
        || !G_SP_IsSPLevel() || SP_IsFrontendMapForSave()) return;
    Com_SyncThreads();
    if (Cmd_Argc() == 1)
    {
        Com_Printf(15, "Dynamic probe: topology original %u highwater %u used %i rendererCapacity %u\n",
            cm.originalDynEntCount, cm.dynEntCount[0], numExtraDynEnts, rgp.world ? rgp.world->dpvsDyn.dynEntClientCount[0] : 0);
        for (int i = 0; i < cm.dynEntCount[0]; ++i)
        {
            if (i >= cm.originalDynEntCount && !usedExtraDynEnts[i - cm.originalDynEntCount]) continue;
            const auto &d = cm.dynEntDefList[0][i]; const auto &c = cm.dynEntClientList[0][i];
            Com_Printf(15, "Dynamic probe: index %i model %s type %i health %i/%i flags %u body %i physics %i constraints %i/%i/%i/%i burns %i\n",
                i, SP_DynModelName(d.xModel).c_str(), d.type, c.health, d.health, c.flags, c.physObjId,
                d.physPreset != nullptr && DynEnt_GetEntityProps(d.type)->usePhysics,
                d.physConstraints[0], d.physConstraints[1], d.physConstraints[2], d.physConstraints[3], Com_SurfaceBurns(DynEnt_GetSurfaceType(&d)));
            if (c.physObjId)
            {
                PhysSavedObjectState body;
                if (Phys_CaptureObjectState(0, c.physObjId, body))
                    Com_Printf(15, "Dynamic body observation: index %i native %i matrix-origin %.9g %.9g %.9g velocity %.9g %.9g %.9g flags %u\n",
                        i, c.physObjId, body.matrix[3][0], body.matrix[3][1], body.matrix[3][2],
                        body.m_t_vel[0], body.m_t_vel[1], body.m_t_vel[2], body.bodyFlags);
            }
        }
        Com_Printf(15, "Dynamic queues: burning %i fading %i\n", gNumBurningDynEnts, gNumFadingDynEnts);
        for (int i = 0; i < gNumBurningDynEnts; ++i)
        {
            const auto r = gDynEntBurnData[i]; const auto *effect = UniqueHandleToEffect(0, r.fx);
            Com_Printf(15, "Dynamic burn owner: slot %i id %u fx %u valid %i played %i definition %s\n", i, r.id, r.fx,
                effect != nullptr, effect ? FX_GetEffectPlayedTime(0, r.fx) : -1, effect && effect->effect.def ? effect->effect.def->name : "");
            if (effect && DynEnt_Valid(r.id))
            {
                const auto &fx = effect->effect; const auto *pose = DynEnt_GetClientPose(r.id);
                Com_Printf(15, "Dynamic burn attachment: id %u boltType %u boltOwner %u effectOrigin %.6f %.6f %.6f ownerOrigin %.6f %.6f %.6f\n",
                    r.id, (unsigned)fx.boltAndSortOrder.type, (unsigned)fx.boltAndSortOrder.absDynEntId,
                    fx.frameNow.origin[0], fx.frameNow.origin[1], fx.frameNow.origin[2],
                    pose->pose.origin[0], pose->pose.origin[1], pose->pose.origin[2]);
            }
        }
        for (int i = 0; i < gNumFadingDynEnts; ++i)
            Com_Printf(15, "Dynamic fade owner: slot %i id %u start %i\n", i, gDynEntFadeData[i].id, gDynEntFadeData[i].startTime);
        return;
    }
    const int index = Cmd_Argc() == 3 ? atoi(Cmd_Argv(2)) : -1;
    if (!strcmp(Cmd_Argv(1), "trace-sleep") && Cmd_Argc() == 3)
    {
        if (strcmp(Cmd_Argv(2), "0") && strcmp(Cmd_Argv(2), "1")) return;
        DynEntCl_SetCheckpointSleepTrace(index == 1);
        Com_Printf(15, "Dynamic sleep trace: observation %s\n", index == 1 ? "enabled" : "disabled");
        return;
    }
    if (!strcmp(Cmd_Argv(1), "watch-motion") && index == -1)
    { s_dynMotionPauseOwner = -1; return; }
    if (!strcmp(Cmd_Argv(1), "retire"))
    {
        if (index < cm.originalDynEntCount || index >= cm.dynEntCount[0]
            || !usedExtraDynEnts[index - cm.originalDynEntCount]) return;
        DynEntCl_DestroyEntityModel(index);
        Com_Printf(15, "Dynamic probe: normal retire factory/action index %i; live spawned %i highwater %u\n", index, numExtraDynEnts, cm.dynEntCount[0]);
        return;
    }
    if (index < 0 || index >= cm.dynEntCount[0]
        || (index >= cm.originalDynEntCount && !usedExtraDynEnts[index - cm.originalDynEntCount])) return;
    auto &c = cm.dynEntClientList[0][index]; const auto &d = cm.dynEntDefList[0][index];
    if (!strcmp(Cmd_Argv(1), "state"))
    {
        const auto &p = cm.dynEntPoseList[0][index];
        Com_Printf(15, "Dynamic owner state: index %i native %i health %i flags %u start %i quaternion %.9g %.9g %.9g %.9g origin %.9g %.9g %.9g radius %.9g\n",
            index, c.physObjId, c.health, c.flags, c.physicsStartTime,
            p.pose.quat[0], p.pose.quat[1], p.pose.quat[2], p.pose.quat[3],
            p.pose.origin[0], p.pose.origin[1], p.pose.origin[2], p.radius);
        return;
    }
    if (!(c.flags & 3)) return;
    if (!strcmp(Cmd_Argv(1), "watch-motion"))
    {
        if (s_continuousZombieResume) return;
        s_dynMotionPauseOwner = index;
        Com_Printf(15, "Dynamic motion boundary: armed owner %i; observes ordinary physics only\n", index);
        return;
    }
    float origin[3]; memcpy(origin, cm.dynEntPoseList[0][index].pose.origin, sizeof(origin));
    const float direction[3] = {1, 0, 0};
    if (!strcmp(Cmd_Argv(1), "impact"))
    {
        if (!d.xModel || !Dvar_GetInt("cl_paused")) return;
        float mins[3], maxs[3], axis[3][3], center[3];
        XModelGetBounds(d.xModel, mins, maxs);
        UnitQuatToAxis(cm.dynEntPoseList[0][index].pose.quat, axis);
        memcpy(center, origin, sizeof(center));
        for (int n = 0; n < 3; ++n)
            for (int k = 0; k < 3; ++k) center[k] += axis[n][k] * (mins[n] + maxs[n]) * 0.5f;
        // Trace through the real model. The ordinary impact handler performs
        // collision selection, native body creation, constraints and impulse.
        // A miss creates nothing; no direct activation/constraint bypass.
        int hit = 0;
        for (int n = 0; n < 3 && !c.physObjId; ++n)
        {
            float start[3], end[3]; const float radius = (maxs[n] - mins[n]) * 0.5f + 8.0f;
            for (int k = 0; k < 3; ++k) { start[k] = center[k] + radius * axis[n][k]; end[k] = center[k] - radius * axis[n][k]; }
            hit += DynEntCl_DynEntImpactEvent(0, 1022, start, end, 0, false) != 0;
        }
        Com_Printf(15, "Dynamic probe: normal traced impact index %i hits %i body %i\n", index, hit, c.physObjId);
        return;
    }
    if (!strcmp(Cmd_Argv(1), "spawn"))
    {
        if (!d.xModel || !d.physPreset || !rgp.world
            || rgp.world->dpvsDyn.dynEntClientCount[0] < (unsigned)cm.originalDynEntCount + 256) return;
        // Explicit native factory exercise. No archive edits or synthetic FX
        // events: use the same model/preset and ordinary force/create owners.
        origin[0] += 20.0f * (numExtraDynEnts + 1); origin[2] += 30.0f;
        const float force[3] = {10, 0, 10};
        const auto spawned = DynEntCl_CreateEntityModel(d.xModel, cm.dynEntPoseList[0][index].pose.quat, origin, origin, force, 0, d.physPreset);
        Com_Printf(15, "Dynamic probe: normal spawn factory/action source %i index %u; live spawned %i highwater %u\n", index, spawned, numExtraDynEnts, cm.dynEntCount[0]);
        return;
    }
    if (!strcmp(Cmd_Argv(1), "flame"))
    {
        // Explicit normal flame-damage exercise, requiring the real loaded
        // effect asset and the model's normal burnable surface classification.
        if (!SP_DynExistingAsset(ASSET_TYPE_FX, "destructibles/fx_dest_fire_trail_sm").fx)
        {
            Com_Printf(15, "Dynamic probe: exact normal burn asset absent; no flame action taken\n");
            return;
        }
        DynEntCl_FlameDamage(0, index, DYNENT_COLL_CLIENT_MODEL, origin, direction, 1);
    }
    else if (!strcmp(Cmd_Argv(1), "extinguish")) DynEntCl_SetBurning(index, DYNENT_DRAW_MODEL, false);
    else if (!strcmp(Cmd_Argv(1), "fade"))
    {
        if (index < cm.originalDynEntCount) return;
        DynEntCl_SetFadeOut(index, DYNENT_DRAW_MODEL);
    }
    else if (!strcmp(Cmd_Argv(1), "damage"))
    {
        // Explicit developer action through ordinary damage owners. Never edit
        // saved payloads or manufacture snapshot/script notification events.
        if (index < cm.dynEntCount[2] && (cm.dynEntServerList[0][index].flags & 1))
            DynEntSv_Damage(index, DYNENT_DRAW_MODEL, origin, direction, 1, false);
        DynEntCl_Damage(0, index, DYNENT_COLL_CLIENT_MODEL, origin, direction, 1);
    }
    else if (!strcmp(Cmd_Argv(1), "activate"))
    {
        if (!d.physPreset || !DynEnt_GetEntityProps(d.type)->usePhysics) return;
        for (short constraint : d.physConstraints) if (constraint != 511 && constraint != -1) return;
        if (!c.physObjId) c.physObjId = DynEntCl_CreatePhysObj(&d, &c, &cm.dynEntPoseList[0][index].pose);
    }
    else return;
    Com_Printf(15, "Dynamic probe: normal %s factory/action index %i health %i flags %u body %i\n",
        Cmd_Argv(1), index, c.health, c.flags, c.physObjId);
}
