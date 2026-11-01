// DYN4 map bindings. Native handles stay in the live map; records refer only
// to the ordered body cohort in the enclosing dynamic-owner inventory.
#include <physics/physconstraints_load_obj.h>
#include <gfx_d3d/r_material.h>
#include <clientscript/cscr_stringlist.h>
#include <physics/rope.h>
#include <physics/rope_checkpoint.h>
#include <cgame_mp/cg_main_mp.h>
struct SPDynJointBinding
{
    int active = 0;
    int rope = -1;
    int server[2] = {-1, -1}; // SPP3 ordinals for native entity endpoints.
    int centities[4]{};
    PhysSavedConstraintState joint{};
};
static std::vector<SPDynJointBinding> s_savedDynBindings;
static RopeSavedSystem s_savedDynRopes;
static int SP_DynJointKind(const PhysConstraint &c)
{ return c.type == CONSTRAINT_ROPE ? CONSTRAINT_DISTANCE : c.type; }
static bool SP_DynRopesValid(const std::vector<SPDynEntRecord> &records, const std::vector<SPDynJointBinding> &bindings,
    const RopeSavedSystem &ropes)
{
    if (!Rope_IsSavedCheckpointValid(ropes) || ropes.capacity != cm.max_ropes) return false;
    int ordinal = 0;
    for (unsigned i = 0; i < bindings.size(); ++i)
    {
        const auto &b = bindings[i];
        if (cm.constraints[i].type != CONSTRAINT_ROPE) { if (b.rope != -1) return false; continue; }
        if (b.rope != -1 && (b.rope != ordinal || b.rope >= ropes.highWater || !ropes.ropes[b.rope].active)) return false;
        ++ordinal;
    }
    if (ordinal != ropes.mapCount) return false;
    for (const auto &rope : ropes.ropes) if (rope.active)
        for (int n = 0; n < rope.constraints; ++n)
            if (rope.constraint[n].type == ROPE_DENTITY_CONSTRAINT && !SP_DynQueueOwner(records, rope.constraint[n].entity)) return false;
    return true;
}
static int SP_DynJointEndpoint(const std::vector<SPDynEntRecord> &records, int attach, int target)
{
    if (attach == ATTACH_POINT_WORLD) return -1;
    if (attach != ATTACH_POINT_DYNENT) return -2;
    int ordinal = 0;
    for (const auto &r : records) if (r.hasBody == 1)
    {
        if (target == r.index + (r.draw ? cm.originalDynEntCount + 256 : 0)) return ordinal;
        ++ordinal;
    }
    return -2;
}
static int SP_MixedJointEndpoint(const std::vector<SPDynEntRecord> &records, int attach, int target,
    int server, const std::vector<SPServerBodyRecord> *servers)
{
    if (server < -1 || (server >= 0 && (!servers || server >= static_cast<int>(servers->size()) || attach != ATTACH_POINT_ENT))) return -2;
    if (servers && attach == ATTACH_POINT_ENT)
    {
        if (server == -1) return -1; // DynEnt factory treats entity attachments as environment.
        int count = 0; for (const auto &r : records) count += r.hasBody == 1;
        return count + server;
    }
    return SP_DynJointEndpoint(records, attach, target);
}
static bool SP_MixedServerTargetsValid(const std::vector<SPDynJointBinding> &bindings,
    const std::vector<SPServerBodyRecord> &servers)
{
    for (unsigned i = 0; i < bindings.size(); ++i)
        for (int side = 0; side < 2; ++side) if (bindings[i].server[side] >= 0)
        {
            const int ordinal = bindings[i].server[side];
            if (ordinal >= static_cast<int>(servers.size())) return false;
            const auto &r = servers[ordinal]; const auto &ent = g_entities[r.number]; const auto &c = cm.constraints[i];
            if (!SP_ServerBodyMatchesEntity(r, ent) || !ent.targetname
                || ent.targetname != (side ? c.target_ent2 : c.target_ent1)) return false;
        }
    return true;
}
// Map light attachments borrow logical client entities, including live CSC
// primary lights. They are not restricted to the network snapshot pool.
static bool SP_DynCentitySlotValid(const PhysConstraint &c, int client, int number)
{
    if (number < -1) return false;
    if (number < 1024) return true;
    return c.type == CONSTRAINT_LIGHT && client == 0 && EntNum_IsCsc(number)
        && number < 1536 && number < g_entNumCscEnd && number != ENTNUM_CSC_END;
}
static bool SP_DynLightOwnerValid(const PhysConstraint &c, int number, bool used, const entityState_s *state)
{
    return SP_DynCentitySlotValid(c, 0, number) && number >= 1024 && used && state
        && state->number == number && state->eType == ET_PRIMARY_LIGHT
        && state->index.primaryLight == c.target_index2;
}
static bool SP_DynBindingsValid(const std::vector<SPDynEntRecord> &records, const std::vector<SPDynJointBinding> &bindings,
    const RopeSavedSystem *ropes = nullptr, bool retiring = false, const std::vector<SPServerBodyRecord> *servers = nullptr)
{
    if (cm.num_constraints > 4096 || bindings.size() != cm.num_constraints || (cm.num_constraints && !cm.constraints)) return false;
    if (retiring && ropes) return false;
    if (ropes && !SP_DynRopesValid(records, bindings, *ropes)) return false;
    if (servers)
    {
        if (servers->size() >= ENTITYNUM_WORLD) return false;
        for (unsigned i = 0; i < servers->size(); ++i)
        {
            const auto &r = (*servers)[i];
            if (r.number < 0 || r.number >= ENTITYNUM_WORLD
                || (!retiring && (r.state.owner != r.number || !Phys_IsSavedObjectStateValid(r.state)))) return false;
            for (unsigned n = 0; n < i; ++n) if ((*servers)[n].number == r.number) return false;
        }
    }
    int bodies = 0; for (const auto &r : records) bodies += r.hasBody == 1;
    if (servers) bodies += static_cast<int>(servers->size());
    for (unsigned i = 0; i < bindings.size(); ++i)
    {
        const auto &b = bindings[i]; const auto &c = cm.constraints[i];
        if (b.active != 0 && b.active != 1) return false;
        for (int server : b.server) if (server < -1 || (!servers && server != -1)
            || (servers && server >= static_cast<int>(servers->size())) || (!b.active && server != -1)) return false;
        if (!ropes && !retiring && b.rope != -1) return false;
        if (retiring && b.rope != -1 && (c.type != CONSTRAINT_ROPE || b.rope < 0 || b.rope >= cm.max_ropes)) return false;
        for (int client = 0; client < 4; ++client)
            if (!SP_DynCentitySlotValid(c, client, b.centities[client])) return false;
        if (!b.active) continue;
        int first = SP_MixedJointEndpoint(records, c.attach_point_type1, c.target_index1, b.server[0], servers);
        int second = SP_MixedJointEndpoint(records, c.attach_point_type2, c.target_index2, b.server[1], servers);
        // DynEnt_SetupConstraints normalizes an environment first endpoint to
        // second before every native point/distance/hinge factory call. JNT1
        // records that native order, not the original map declaration order.
        if (first == -1 && second >= 0) { first = second; second = -1; }
        const int kind = ropes || retiring ? SP_DynJointKind(c) : c.type;
        if (kind < CONSTRAINT_POINT || kind > CONSTRAINT_HINGE || b.joint.kind != kind
            || !Phys_IsSavedConstraintStateValid(b.joint, bodies)
            || b.joint.first != first || b.joint.second != second) return false;
    }
    return true;
}
static bool SP_DynCaptureJointGraph(std::vector<SPDynEntRecord> &records, std::vector<SPDynJointBinding> &bindings,
    RopeSavedSystem *ropes = nullptr, bool retiring = false, std::vector<SPServerBodyRecord> *servers = nullptr)
{
    if (cm.num_constraints > 4096 || (cm.num_constraints && !cm.constraints)) return false;
    if (ropes && !Rope_CaptureCheckpoint(*ropes))
    {
        Com_Printf(15, "Dynamic graph preflight: rope capture rejected slots %i capacity %i map %i total %i\n", g_ropeCount, cm.max_ropes, g_mapRopeCount, g_totalRopeCount);
        return false;
    }
    std::vector<int> ids;
    for (const auto &r : records) if (r.hasBody == 1)
        ids.push_back(cm.dynEntClientList[r.draw][r.index].physObjId);
    const int dynamicBodies = static_cast<int>(ids.size());
    if (servers) for (const auto &r : *servers)
    {
        if (!SP_ServerBodyMatchesEntity(r, g_entities[r.number])) return false;
        ids.push_back(g_entities[r.number].physObjId);
    }
    std::vector<PhysCheckpointJointRef> refs;
    bindings.clear(); bindings.resize(cm.num_constraints);
    for (unsigned i = 0; i < bindings.size(); ++i)
    {
        const auto &c = cm.constraints[i]; auto &b = bindings[i];
        if (!ropes && !retiring && c.rope_index != -1) return false; // Legacy DYN4 has no rope cohort.
        b.rope = ropes || retiring ? c.rope_index : -1;
        b.active = c.constraintHandle != 0; memcpy(b.centities, c.centity_num, sizeof(b.centities));
        if (b.active) refs.push_back({ropes || retiring ? SP_DynJointKind(c) : c.type, static_cast<unsigned>(c.constraintHandle)});
    }
    std::vector<PhysSavedObjectState> bodies(ids.size());
    std::vector<PhysSavedConstraintState> joints(refs.size());
    if (ids.empty()) { if (!refs.empty()) return false; }
    else if (retiring
        ? !Phys_CaptureRetiringJoints(0, ids.data(), static_cast<int>(ids.size()), refs.data(), static_cast<int>(refs.size()), joints.data())
        : !Phys_CaptureObjectGraph(0, ids.data(), static_cast<int>(ids.size()), refs.data(), static_cast<int>(refs.size()), bodies.data(), joints.data()))
    { Com_Printf(15, "Dynamic graph preflight: native %s rejected %u bodies / %u map joints\n", retiring ? "retirement ownership" : "closure", static_cast<unsigned>(ids.size()), static_cast<unsigned>(refs.size())); return false; }
    unsigned body = 0, joint = 0;
    if (!retiring) for (auto &r : records) if (r.hasBody == 1) r.body = bodies[body++];
    if (servers && !retiring) for (auto &r : *servers)
    {
        r.state = bodies[body++]; if (r.state.owner != r.number) return false;
    }
    for (auto &b : bindings) if (b.active) b.joint = joints[joint++];
    // Resolve only endpoints actually used by the native joint. A declared
    // entity anchor can legitimately be environment in the DynEnt factory.
    if (servers) for (unsigned i = 0; i < bindings.size(); ++i) if (bindings[i].active)
        for (int side = 0; side < 2; ++side)
        {
            const auto &c = cm.constraints[i]; auto &b = bindings[i];
            if ((side ? c.attach_point_type2 : c.attach_point_type1) != ATTACH_POINT_ENT) continue;
            for (unsigned n = 0; n < servers->size(); ++n)
                if ((*servers)[n].number == (side ? c.target_index2 : c.target_index1)
                    && (b.joint.first == dynamicBodies + static_cast<int>(n) || b.joint.second == dynamicBodies + static_cast<int>(n)))
                    b.server[side] = static_cast<int>(n);
        }
    // Capture/readback uses the actual live registry. Decode checks the saved
    // FDO1+CNS owners before teardown. Retirement runs after old CLIENT owners
    // are gone and must not dereference their former logical client slots.
    if (!retiring) for (unsigned i = 0; i < bindings.size(); ++i)
    {
        const int number = bindings[i].centities[0];
        if (number >= 1024 && (!SP_DynCentitySlotValid(cm.constraints[i], 0, number)
            || !SP_DynLightOwnerValid(cm.constraints[i], number, cg_fakeEntitiesInuseArray[number - 1024],
                &CG_GetEntity(0, number)->nextState)))
        { Com_Printf(15, "Dynamic graph preflight: missing live primary-light borrower map %u client entity %i\n", i, number); return false; }
    }
    const bool valid = SP_DynBindingsValid(records, bindings, ropes, retiring, servers)
        && (!servers || SP_MixedServerTargetsValid(bindings, *servers));
    if (!valid)
    {
        Com_Printf(15, "Dynamic graph preflight: map binding validation rejected; ropes %i slots %i map %i\n", ropes != nullptr, ropes ? ropes->highWater : 0, ropes ? ropes->mapCount : 0);
        if (ropes) for (unsigned i = 0; i < bindings.size(); ++i) if (bindings[i].rope >= 0)
            Com_Printf(15, "Dynamic graph preflight: binding %u rope %i active %i\n", i, bindings[i].rope,
                bindings[i].rope < ropes->highWater ? ropes->ropes[bindings[i].rope].active : -1);
    }
    return valid;
}
static void SP_DynJointBindingsCodec(MemoryFile *file, bool reading)
{
    const auto value = [&](auto &v) {
        if (reading) MemFile_ReadData(file, sizeof(v), reinterpret_cast<unsigned char *>(&v));
        else MemFile_WriteData(file, sizeof(v), reinterpret_cast<unsigned char *>(&v));
    };
    // Match logical asset fields individually. No native layout, padding,
    // material pointer or bone-string pointer enters the disk representation.
    int definitionIndex = -1;
    const auto definition = [&](const auto &field, const char *label) {
        unsigned char bytes[sizeof(field)]; memcpy(bytes, &field, sizeof(field));
        if (reading) MemFile_ReadData(file, sizeof(bytes), bytes); else MemFile_WriteData(file, sizeof(bytes), bytes);
        if (memcmp(bytes, &field, sizeof(field))) Com_Error(ERR_DROP, "Dynamic checkpoint map constraint %i field %s differs", definitionIndex, label);
    };
    const auto name = [&](const char *text) {
        const std::string expected = text ? text : "";
        if (expected.size() > 255) Com_Error(ERR_DROP, "Dynamic checkpoint constraint asset name invalid");
        if (reading) { if (expected != MemFile_ReadCString(file)) Com_Error(ERR_DROP, "Dynamic checkpoint constraint %i asset/string differs", definitionIndex); }
        else MemFile_WriteCString(file, const_cast<char *>(expected.c_str()));
    };
    int count = static_cast<int>(s_savedDynBindings.size()); value(count);
    if (count < 0 || count > 4096 || count != cm.num_constraints || (count && !cm.constraints))
        Com_Error(ERR_DROP, "Dynamic checkpoint map constraint inventory differs");
    if (reading) s_savedDynBindings.assign(count, SPDynJointBinding{});
    int bodyCount = 0; for (const auto &r : s_savedDynEnts) bodyCount += r.hasBody == 1;
    if (s_savedDynSchema >= 0x364E5944)
    {
        if (s_serverBodySchema != 0x33505053) Com_Error(ERR_DROP, "Unified physics checkpoint missing SPP3 owners");
        bodyCount += static_cast<int>(s_serverBodies.size());
    }
    for (int i = 0; i < count; ++i)
    {
        definitionIndex = i;
        const auto &c = cm.constraints[i]; auto &b = s_savedDynBindings[i];
#define SP_DYN_JOINT_DEF(n) definition(c.n, #n)
        name(c.targetname ? SL_ConvertToString(c.targetname, SCRIPTINSTANCE_SERVER) : ""); SP_DYN_JOINT_DEF(type);
        // Despite their names, target_ent fields hold SL target-name handles
        // during map fixup, not stable entity numbers. Match their text, too.
        const auto endpointDefinition = [&](int attach, int target) {
            // Entity slots are runtime identities, represented by SPP3 ordinals
            // below. They are not immutable map definition fields.
            const int logical = s_savedDynSchema >= 0x364E5944 && attach == ATTACH_POINT_ENT ? -1 : target;
            definition(logical, "target_index");
        };
        SP_DYN_JOINT_DEF(attach_point_type1); endpointDefinition(c.attach_point_type1, c.target_index1); name(c.target_ent1 ? SL_ConvertToString(c.target_ent1, SCRIPTINSTANCE_SERVER) : ""); name(c.target_bone1);
        SP_DYN_JOINT_DEF(attach_point_type2); endpointDefinition(c.attach_point_type2, c.target_index2); name(c.target_ent2 ? SL_ConvertToString(c.target_ent2, SCRIPTINSTANCE_SERVER) : ""); name(c.target_bone2);
        SP_DYN_JOINT_DEF(offset); SP_DYN_JOINT_DEF(pos); SP_DYN_JOINT_DEF(pos2); SP_DYN_JOINT_DEF(dir);
        SP_DYN_JOINT_DEF(flags); SP_DYN_JOINT_DEF(timeout); SP_DYN_JOINT_DEF(min_health); SP_DYN_JOINT_DEF(max_health);
        SP_DYN_JOINT_DEF(distance); SP_DYN_JOINT_DEF(damp); SP_DYN_JOINT_DEF(power); SP_DYN_JOINT_DEF(scale);
        SP_DYN_JOINT_DEF(spin_scale); SP_DYN_JOINT_DEF(minAngle); SP_DYN_JOINT_DEF(maxAngle);
#undef SP_DYN_JOINT_DEF
        name(c.material ? Material_GetName(c.material) : "");
        value(b.active); value(b.centities);
        if (s_savedDynSchema >= 0x354E5944) value(b.rope);
        if (s_savedDynSchema >= 0x364E5944) value(b.server);
        if (b.active != 0 && b.active != 1) Com_Error(ERR_DROP, "Dynamic checkpoint invalid map joint presence");
        if (b.active)
        {
            if (reading) Phys_ReadConstraintState(file, b.joint, bodyCount);
            else Phys_WriteConstraintState(file, b.joint, bodyCount);
        }
    }
    if (s_savedDynSchema == 0x344E5944 && !SP_DynBindingsValid(s_savedDynEnts, s_savedDynBindings)) Com_Error(ERR_DROP, "Dynamic checkpoint invalid map joint bindings");
}
static void SP_RetireDynJointBindings()
{
    // Validate the entire fresh-map cohort before retiring even one binding.
    std::vector<SPDynEntRecord> current;
    for (int draw = 0; draw < 2; ++draw)
        for (int i = 0; i < cm.dynEntCount[draw]; ++i)
        {
            if (!draw && i >= cm.originalDynEntCount && !usedExtraDynEnts[i - cm.originalDynEntCount]) continue;
            current.push_back(SP_DynCaptureRecord(draw, i, true));
        }
    std::vector<SPDynJointBinding> bindings;
    std::vector<SPServerBodyRecord> servers;
    const bool mixed = s_savedDynSchema >= 0x364E5944;
    if (mixed && !SP_CollectLiveServerBodies(servers, true)) Com_Error(ERR_DROP, "Unified physics checkpoint unsupported fresh server body owner");
    // CLIENT teardown has already cleared the old visual rope slots. Retire
    // the still-published native map joints without treating those obsolete
    // visual slots as another save candidate. Saved RPE1 and native graph
    // validation remain separate prerequisites for publication below.
    if (!SP_DynCaptureJointGraph(current, bindings, nullptr, s_savedDynSchema >= 0x354E5944, mixed ? &servers : nullptr)) Com_Error(ERR_DROP, "Dynamic checkpoint unsupported fresh map joint graph");
    for (unsigned i = 0; i < bindings.size(); ++i)
    {
        auto &c = cm.constraints[i];
        if (bindings[i].active && !Phys_DestroyCheckpointConstraint(SP_DynJointKind(c), static_cast<unsigned>(c.constraintHandle)))
            Com_Error(ERR_DROP, "Dynamic checkpoint lost old map joint during retirement");
        c.constraintHandle = 0;
        if (c.type == CONSTRAINT_LIGHT) for (auto &number : c.centity_num) number = -1;
    }
}
static void SP_PublishDynJointGraph()
{
    const bool hasRopes = s_savedDynSchema >= 0x354E5944;
    const bool mixed = s_savedDynSchema >= 0x364E5944;
    if (!SP_DynBindingsValid(s_savedDynEnts, s_savedDynBindings, hasRopes ? &s_savedDynRopes : nullptr, false, mixed ? &s_serverBodies : nullptr)) Com_Error(ERR_DROP, "Dynamic checkpoint lost staged map joint graph");
    std::vector<int> ids, slots;
    std::vector<PhysSavedObjectState> bodies;
    std::vector<PhysSavedConstraintState> joints;
    for (const auto &r : s_savedDynEnts) if (r.hasBody == 1)
    {
        ids.push_back(cm.dynEntClientList[r.draw][r.index].physObjId); bodies.push_back(r.body);
        Com_Printf(15, "Dynamic checkpoint: DYN4 body owner %i:%i fresh native %i\n", r.draw, r.index, ids.back());
    }
    if (mixed) for (const auto &r : s_serverBodies)
    { ids.push_back(g_entities[r.number].physObjId); bodies.push_back(r.state); }
    for (unsigned i = 0; i < s_savedDynBindings.size(); ++i) if (s_savedDynBindings[i].active)
    { slots.push_back(static_cast<int>(i)); joints.push_back(s_savedDynBindings[i].joint); }
    std::vector<unsigned> handles(joints.size());
    if ((!ids.empty() && !Phys_ApplyObjectGraph(0, ids.data(), static_cast<int>(ids.size()), bodies.data(), joints.data(), static_cast<int>(joints.size()), handles.data()))
        || (ids.empty() && !joints.empty())) Com_Error(ERR_DROP, "Dynamic checkpoint fresh native joint graph differs");
    for (unsigned i = 0; i < handles.size(); ++i) cm.constraints[slots[i]].constraintHandle = static_cast<int>(handles[i]);
    for (unsigned i = 0; i < s_savedDynBindings.size(); ++i)
    {
        memcpy(cm.constraints[i].centity_num, s_savedDynBindings[i].centities, sizeof(cm.constraints[i].centity_num));
        if (hasRopes) cm.constraints[i].rope_index = s_savedDynBindings[i].rope;
        if (mixed)
        {
            const auto &b = s_savedDynBindings[i];
            if (b.server[0] >= 0) cm.constraints[i].target_index1 = s_serverBodies[b.server[0]].number;
            if (b.server[1] >= 0) cm.constraints[i].target_index2 = s_serverBodies[b.server[1]].number;
        }
    }
    if (hasRopes && !Rope_ApplyCheckpoint(s_savedDynRopes)) Com_Error(ERR_DROP, "Dynamic checkpoint rope owners/resources changed after preflight");
    auto checkRecords = s_savedDynEnts; std::vector<SPDynJointBinding> checkBindings;
    RopeSavedSystem checkRopes;
    auto checkServers = s_serverBodies;
    if (!SP_DynCaptureJointGraph(checkRecords, checkBindings, hasRopes ? &checkRopes : nullptr, false, mixed ? &checkServers : nullptr)) Com_Error(ERR_DROP, "Dynamic checkpoint published graph not closed");
    if (hasRopes && !(checkRopes == s_savedDynRopes)) Com_Error(ERR_DROP, "Dynamic checkpoint rope graph readback differs");
    for (unsigned i = 0; i < checkRecords.size(); ++i)
        if (checkRecords[i].hasBody && memcmp(&checkRecords[i].body, &s_savedDynEnts[i].body, sizeof(checkRecords[i].body)))
            Com_Error(ERR_DROP, "Dynamic checkpoint native body readback differs");
    for (unsigned i = 0; i < checkBindings.size(); ++i)
        if (memcmp(&checkBindings[i], &s_savedDynBindings[i], sizeof(checkBindings[i])))
            Com_Error(ERR_DROP, "Dynamic checkpoint native binding readback differs");
    if (mixed) for (unsigned i = 0; i < checkServers.size(); ++i)
        if (memcmp(&checkServers[i].state, &s_serverBodies[i].state, sizeof(checkServers[i].state)))
            Com_Error(ERR_DROP, "Unified physics checkpoint server body readback differs");
    Com_Printf(15, "Dynamic checkpoint: DYN4 graph %u bodies / %u joints fresh map bindings IDENTICAL\n", static_cast<unsigned>(ids.size()), static_cast<unsigned>(joints.size()));
    if (hasRopes) Com_Printf(15, "Dynamic checkpoint: DYN5/RPE1 %i rope slots / %i map owners clock %i native publication IDENTICAL\n", s_savedDynRopes.highWater, s_savedDynRopes.mapCount, s_savedDynRopes.clock);
    if (mixed) Com_Printf(15, "Unified physics checkpoint: DYN6/SPP3 %u server bodies / %u total bodies / %u map joints native publication IDENTICAL\n", static_cast<unsigned>(s_serverBodies.size()), static_cast<unsigned>(ids.size()), static_cast<unsigned>(joints.size()));
}
