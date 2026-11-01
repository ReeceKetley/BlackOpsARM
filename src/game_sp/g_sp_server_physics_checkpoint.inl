// SPP1: server world-0 script-model bodies. No old IDs or solver pointers.
#include <physics/phys_checkpoint.h>
#include <game/g_mover.h>
#include <xanim/dobj_utils.h>
#include <xanim/xmodel_utils.h>
#include <vector>
#include <clientscript/scr_const.h>
#include <qcommon/cm_load.h>
#include <physics/physconstraints_load_obj.h>
struct SPServerBodyRecord
{
    int number = 0, generation = 0, model = 0;
    std::string modelName;
    PhysSavedObjectState state{};
};
static std::vector<SPServerBodyRecord> s_serverBodies;
static bool s_serverBodiesStaged;
static int s_serverBodyEntityError = -1;
static int s_serverBodySchema = 0x32505053;
static PhysSavedClockState s_serverPhysicsClock;
static bool s_serverBodiesPublished;
static bool s_serverBodyEntitiesSeen[MAX_GENTITIES];

static bool SP_ServerBodyMatchesEntity(const SPServerBodyRecord &r, const gentity_s &ent)
{
    return ent.r.inuse && ent.s.number == r.number && ent.useCount == r.generation && ent.model == r.model
        && ent.s.eType == ET_SCRIPTMOVER && ent.classname == scr_const.script_model && !ent.tagInfo;
}
static bool SP_ReadLiveServerBody(const gentity_s &ent, SPServerBodyRecord &r, bool graph = false)
{
    r = {}; r.number = ent.s.number; r.generation = ent.useCount; r.model = ent.model;
    if (!SP_ServerBodyMatchesEntity(r, ent) || !r.model || ent.client || ent.actor || ent.scr_vehicle
        || !ent.physObjId || ent.physObjId == -1) return false;
    auto *obj = Com_GetServerDObj(r.number);
    if (!obj || obj->numModels != 1) return false;
    const auto *model = DObjGetModel(obj, 0);
    if (!model || !XModelGetName(model) || !*XModelGetName(model)) return false;
    r.modelName = XModelGetName(model);
    if (graph) return true; // State is captured with the complete native cohort.
    // G_CreatePhysicsObject would recreate these map constraints. Until their
    // complete owner graph is durable, reject them before committing a save.
    for (int i = 0; i < cm.num_constraints; ++i)
    {
        const auto &c = cm.constraints[i];
        if ((c.attach_point_type1 == ATTACH_POINT_ENT && c.target_ent1 == ent.targetname)
            || (c.attach_point_type2 == ATTACH_POINT_ENT && c.target_ent2 == ent.targetname)) return false;
    }
    return Phys_CaptureObjectState(0, ent.physObjId, r.state) && r.state.owner == r.number;
}
static bool SP_CollectLiveServerBodies(std::vector<SPServerBodyRecord> &records, bool graph)
{
    records.clear();
    for (const auto &ent : g_entities)
        if (ent.r.inuse && ent.physObjId && ent.physObjId != -1)
        {
            SPServerBodyRecord record;
            if (!SP_ReadLiveServerBody(ent, record, graph))
            {
                Com_Printf(15, "Server physics checkpoint: entity %i generation %i has unsupported shape/owner/constraint; match and archive retained\n", ent.s.number, ent.useCount);
                return false;
            }
            records.push_back(record);
        }
    return true;
}
static bool SP_ServerBodiesSupported(bool graph = false)
{
    std::vector<SPServerBodyRecord> records;
    return SP_CollectLiveServerBodies(records, graph);
}
static void SP_CaptureServerBodies(bool graph = false)
{
    s_serverBodies.clear(); s_serverBodiesStaged = false; s_serverBodiesPublished = false;
    s_serverBodySchema = graph ? 0x33505053 : 0x32505053;
    if (!Phys_CaptureCheckpointClock(s_serverPhysicsClock)) Com_Error(ERR_DROP, "Server physics checkpoint clock invalid");
    for (const auto &ent : g_entities)
        if (ent.r.inuse && ent.physObjId && ent.physObjId != -1)
        {
            SPServerBodyRecord record;
            if (!SP_ReadLiveServerBody(ent, record, graph)) Com_Error(ERR_DROP, "Server physics checkpoint lost supported capture owner %i", ent.s.number);
            s_serverBodies.push_back(record);
        }
    Com_Printf(15, "Server physics checkpoint: captured %u world0 model owners (%s); no runtime IDs\n", (unsigned)s_serverBodies.size(), graph ? "state deferred to unified graph" : "independent bodies");
}
static void SP_CanonicalServerBodyImage(const gentity_s &ent, gentity_s &image)
{
    image = ent;
    if (!ent.physObjId || ent.physObjId == -1) return;
    for (const auto &record : s_serverBodies)
        if (SP_ServerBodyMatchesEntity(record, ent)) { image.physObjId = 0; return; }
    Com_Error(ERR_DROP, "Server physics checkpoint missing body for entity image %i", ent.s.number);
}
static void SP_WriteServerBodies(MemoryFile *file)
{
    MemFile_WriteInt(file, s_serverBodySchema);
    if (s_serverBodySchema >= 0x32505053)
    {
        MemFile_WriteInt(file, s_serverPhysicsClock.update); MemFile_WriteInt(file, s_serverPhysicsClock.snapshot);
    }
    MemFile_WriteInt(file, (int)s_serverBodies.size());
    for (const auto &record : s_serverBodies)
    {
        MemFile_WriteInt(file, record.number); MemFile_WriteInt(file, record.generation); MemFile_WriteInt(file, record.model);
        MemFile_WriteCString(file, const_cast<char *>(record.modelName.c_str()));
        MemFile_WriteData(file, sizeof(record.state), (unsigned char *)&record.state);
    }
}
static void SP_ReadServerBodies(MemoryFile *file)
{
    const auto read = [file]() { int n; MemFile_ReadData(file, 4, (unsigned char *)&n); return n; };
    s_serverBodySchema = read(); s_serverBodiesPublished = false;
    if (s_serverBodySchema != 0x31505053 && s_serverBodySchema != 0x32505053 && s_serverBodySchema != 0x33505053) Com_Error(ERR_DROP, "Server physics checkpoint schema invalid");
    if (s_serverBodySchema >= 0x32505053)
    {
        s_serverPhysicsClock = {read(), read()};
        if (s_serverPhysicsClock.update < 0 || s_serverPhysicsClock.snapshot < 0 || s_serverPhysicsClock.snapshot > s_serverPhysicsClock.update)
            Com_Error(ERR_DROP, "Server physics checkpoint rejected clock phase");
    }
    const int count = read(); if (count < 0 || count >= ENTITYNUM_WORLD) Com_Error(ERR_DROP, "Server physics checkpoint count invalid");
    s_serverBodies.clear(); bool seen[MAX_GENTITIES]{};
    for (int i = 0; i < count; ++i)
    {
        SPServerBodyRecord r; r.number = read(); r.generation = read(); r.model = read(); r.modelName = MemFile_ReadCString(file);
        MemFile_ReadData(file, sizeof(r.state), (unsigned char *)&r.state);
        if (r.number < 0 || r.number >= ENTITYNUM_WORLD || seen[r.number] || r.model <= 0 || r.model >= 512
            || r.modelName.empty() || r.modelName.size() > 255 || r.state.owner != r.number || !Phys_IsSavedObjectStateValid(r.state))
            Com_Error(ERR_DROP, "Server physics checkpoint rejected logical record");
        seen[r.number] = true;
        auto *model = DB_FindXAssetHeader(ASSET_TYPE_XMODEL, const_cast<char *>(r.modelName.c_str()), false, -1).model;
        if (!model || strcmp(XModelGetName(model), r.modelName.c_str())) Com_Error(ERR_DROP, "Server physics checkpoint missing exact model asset");
        s_serverBodies.push_back(r);
    }
    s_serverBodiesStaged = true; memset(s_serverBodyEntitiesSeen, 0, sizeof(s_serverBodyEntitiesSeen));
    s_serverBodyEntityError = -1;
}
static void SP_CheckServerBodyEntity(const gentity_s &ent)
{
    if (!s_serverBodiesStaged) return;
    for (const auto &record : s_serverBodies)
        if (record.number == ent.s.number)
        {
            if (!SP_ServerBodyMatchesEntity(record, ent) || ent.physObjId)
                s_serverBodyEntityError = record.number;
            s_serverBodyEntitiesSeen[record.number] = true;
        }
}
static void SP_CheckServerBodyEntityInventory()
{
    if (s_serverBodiesStaged)
        for (const auto &r : s_serverBodies) if (!s_serverBodyEntitiesSeen[r.number]) s_serverBodyEntityError = r.number;
}
static void SP_PublishServerBodies(bool graph = false)
{
    if (!s_serverBodiesStaged || s_serverBodiesPublished) return;
    for (const auto &r : s_serverBodies)
    {
        auto &ent = g_entities[r.number];
        if (!SP_ServerBodyMatchesEntity(r, ent) || ent.physObjId) Com_Error(ERR_DROP, "Server physics checkpoint lost publication entity %i", r.number);
        auto *obj = Com_GetServerDObj(r.number);
        if (!obj || obj->numModels != 1 || strcmp(XModelGetName(DObjGetModel(obj, 0)), r.modelName.c_str()))
            Com_Error(ERR_DROP, "Server physics checkpoint lost rebuilt model %i", r.number);
        if (graph) G_CreateCheckpointPhysicsObject(&ent); else G_CreatePhysicsObject(&ent);
        if (!ent.physObjId || ent.physObjId == -1 || (!graph && !Phys_ApplyObjectState(0, ent.physObjId, r.state)))
            Com_Error(ERR_DROP, "Server physics checkpoint failed new body publication %i", r.number);
        Com_Printf(15, graph
            ? "Server physics checkpoint: entity %i generation %i model %s fresh world0 body owner %i; state deferred to unified graph\n"
            : "Server physics checkpoint: entity %i generation %i model %s LIVE logical body IDENTICAL in world0; new owner %i\n",
            r.number, r.generation, r.modelName.c_str(), ent.physObjId);
    }
    s_serverBodiesPublished = true;
}
static void SP_RebaseSavedPhysicsClock(int clientPhysicsTime)
{
    PhysSavedClockState clock{clientPhysicsTime, clientPhysicsTime};
    const bool saved = s_serverBodiesStaged && s_serverBodySchema >= 0x32505053;
    if (saved)
    {
        clock = s_serverPhysicsClock;
        if (clock.update != clientPhysicsTime) Com_Error(ERR_DROP, "Server/client checkpoint physics clocks disagree");
    }
    if (!Phys_ApplyCheckpointClock(clock)) Com_Error(ERR_DROP, "Saved physics clock cannot be published");
    Com_Printf(15, "Physics checkpoint: %s clock update %i snapshot %i installed before native bodies, no solver step\n",
        saved ? "durable" : "LEGACY saved-client seeded", clock.update, clock.snapshot);
}
