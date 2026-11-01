// Read-only coverage probe for native owners outside the VM/entity serializers.
#include <dynentity/DynEntity_load_obj.h>
#include <Glass/glass_server.h>
#include <Glass/glass_client.h>
#include <Glass/glass_renderer.h>
#include <physics/phys_checkpoint.h>
#include <physics/physics_system.h>
#include <physics/physconstraints_load_obj.h>
#include <physics/rope.h>
#include <game/g_scr_mover.h>
extern unsigned __int8 usedExtraDynEnts[256];

static void SP_VerifyNativeMapGraph(const std::vector<int> &ids, const std::vector<int> &owners)
{
    if (!Dvar_GetInt("cl_paused") || ids.empty() || ids.size() > 4096)
    {
        Com_Printf(15, "Resume graph probe: requires paused map and bounded body cohort\n"); return;
    }
    std::vector<PhysCheckpointJointRef> refs;
    std::vector<int> bindings;
    if (cm.num_constraints > 4096 || (cm.num_constraints && !cm.constraints)) return;
    for (int i = 0; i < cm.num_constraints; ++i)
        if (cm.constraints[i].constraintHandle)
        {
            refs.push_back({cm.constraints[i].type, static_cast<unsigned>(cm.constraints[i].constraintHandle)});
            bindings.push_back(i);
        }
    const int count = static_cast<int>(ids.size()), joints = static_cast<int>(refs.size());
    std::vector<PhysSavedObjectState> bodies(count), checkBodies(count);
    std::vector<PhysSavedConstraintState> constraints(joints), checkConstraints(joints);
    if (!Phys_CaptureObjectGraph(0, ids.data(), count, refs.data(), joints, bodies.data(), constraints.data()))
    {
        Com_Printf(15, "Resume graph probe: native graph not closed/supported; no allocations\n"); return;
    }
    const int before[] = {phys_sys::get_rigid_body_alloc_count(), phys_sys::get_rbc_point_alloc_count(),
        phys_sys::get_rbc_dist_alloc_count(), phys_sys::get_rbc_hinge_alloc_count()};
    std::vector<int> targets(count);
    std::vector<unsigned> handles(joints);
    bool allocated = true;
    for (int i = 0; i < count; ++i)
    {
        const auto absId = static_cast<unsigned short>(owners[i]);
        const auto *def = DynEnt_GetEntityDef(absId);
        DynEntityClient local = *DynEnt_GetClientEntity(absId); local.physObjId = 0;
        targets[i] = DynEntCl_CreateCheckpointPhysObj(def, &local, &DynEnt_GetClientPose(absId)->pose);
        Com_Printf(15, "Resume graph probe: body ordinal %i owner %i original %i fresh %i\n", i, owners[i], ids[i], targets[i]);
        if (!targets[i]) { allocated = false; break; }
    }
    const bool applied = allocated && Phys_ApplyObjectGraph(0, targets.data(), count, bodies.data(), constraints.data(), joints, handles.data());
    std::vector<PhysCheckpointJointRef> restored(joints);
    if (applied) for (int i = 0; i < joints; ++i) restored[i] = {constraints[i].kind, handles[i]};
    const bool identical = applied && Phys_CaptureObjectGraph(0, targets.data(), count, restored.data(), joints,
        checkBodies.data(), checkConstraints.data())
        && !memcmp(bodies.data(), checkBodies.data(), count * sizeof(bodies[0]))
        && (joints == 0 || !memcmp(constraints.data(), checkConstraints.data(), joints * sizeof(constraints[0])));
    bool retired = true;
    if (applied) for (int i = joints - 1; i >= 0; --i) retired &= Phys_DestroyCheckpointConstraint(constraints[i].kind, handles[i]);
    // These are unpublished probe allocations. The normal map-owner destroy
    // wrapper also clears cm binding handles, so retire through the underlying
    // canonical physics owner API instead.
    for (int target : targets) if (target) Phys_ObjDestroy(0, target);
    const int after[] = {phys_sys::get_rigid_body_alloc_count(), phys_sys::get_rbc_point_alloc_count(),
        phys_sys::get_rbc_dist_alloc_count(), phys_sys::get_rbc_hinge_alloc_count()};
    bool bindingsPreserved = true;
    for (int i = 0; i < joints; ++i)
        bindingsPreserved &= cm.constraints[bindings[i]].type == refs[i].kind
            && static_cast<unsigned>(cm.constraints[bindings[i]].constraintHandle) == refs[i].handle;
    const bool preserved = bindingsPreserved && Phys_CaptureObjectGraph(0, ids.data(), count, refs.data(), joints, checkBodies.data(), checkConstraints.data())
        && !memcmp(bodies.data(), checkBodies.data(), count * sizeof(bodies[0]))
        && (joints == 0 || !memcmp(constraints.data(), checkConstraints.data(), joints * sizeof(constraints[0])));
    Com_Printf(15, "Resume graph probe: native body/joint clone %s bodies %i joints %i retired %i original %i allocations %i/%i/%i/%i -> %i/%i/%i/%i\n",
        identical && retired && preserved && !memcmp(before, after, sizeof(before)) ? "PASS" : "FAIL", count, joints, retired, preserved,
        before[0], before[1], before[2], before[3], after[0], after[1], after[2], after[3]);
}

// Explicit developer probe. A temporary joint is allocated and retired while
// paused; it never participates in a solver step or replaces the map's joint.
static void SP_InspectMapJoints()
{
    if (cm.dynEntCount[0] > cm.originalDynEntCount + 256) return;
    std::vector<int> ids, owners;
    for (int draw = 0; draw < 2; ++draw)
        for (int i = 0; i < cm.dynEntCount[draw]; ++i)
            if ((draw || i < cm.originalDynEntCount || usedExtraDynEnts[i - cm.originalDynEntCount])
                && cm.dynEntClientList[draw] && cm.dynEntClientList[draw][i].physObjId)
            {
                ids.push_back(cm.dynEntClientList[draw][i].physObjId);
                owners.push_back(DynEnt_GetClientAbsId(static_cast<unsigned short>(i), static_cast<DynEntityDrawType>(draw)));
            }
    if (Cmd_Argc() == 2 && !strcmp(Cmd_Argv(1), "verify-graph"))
    {
        SP_VerifyNativeMapGraph(ids, owners); return;
    }
    const bool verify = Cmd_Argc() == 3 && !strcmp(Cmd_Argv(1), "verify-joint");
    const int selected = verify ? atoi(Cmd_Argv(2)) : -1;
    if (verify && (!Dvar_GetInt("cl_paused") || selected < 0 || selected >= cm.num_constraints))
    {
        Com_Printf(15, "Resume joint probe: requires paused map and valid constraint index\n");
        return;
    }
    if (cm.num_constraints < 0 || cm.num_constraints > 4096 || (cm.num_constraints && !cm.constraints))
        Com_Error(ERR_DROP, "Resume inventory constraint list invalid");
    for (int i = 0; i < cm.num_constraints; ++i)
    {
        const auto &c = cm.constraints[i];
        PhysSavedConstraintState state;
        const bool captured = c.constraintHandle && Phys_CaptureConstraintState(0, c.type,
            static_cast<unsigned>(c.constraintHandle), ids.data(), static_cast<int>(ids.size()), state);
        Com_Printf(15, "Resume joint probe: index %i type %i live %i attach %i:%i/%i:%i rope %i captured %i owners %i/%i\n",
            i, c.type, c.constraintHandle != 0, c.attach_point_type1, c.target_index1,
            c.attach_point_type2, c.target_index2, c.rope_index, captured,
            captured && state.first >= 0 ? owners[state.first] : -1,
            captured && state.second >= 0 ? owners[state.second] : -1);
        if (!verify || selected != i) continue;
        if (!captured) { Com_Printf(15, "Resume joint probe: selected joint unsupported/inactive\n"); continue; }
        unsigned char bytes[4096]{}; MemoryFile writer{}, reader{};
        MemFile_InitForWriting(&writer, sizeof(bytes), bytes, true, true);
        Phys_WriteConstraintState(&writer, state, static_cast<int>(ids.size()));
        MemFile_StartSegment(&writer, -1);
        MemFile_InitForReading(&reader, writer.bytesUsed, bytes, true);
        PhysSavedConstraintState decoded;
        Phys_ReadConstraintState(&reader, decoded, static_cast<int>(ids.size()));
        const int before[] = {phys_sys::get_rbc_point_alloc_count(), phys_sys::get_rbc_dist_alloc_count(), phys_sys::get_rbc_hinge_alloc_count()};
        const unsigned temporary = Phys_CreateSavedConstraint(0, ids.data(), static_cast<int>(ids.size()), decoded);
        PhysSavedConstraintState copied, original;
        const bool same = temporary && Phys_CaptureConstraintState(0, c.type, temporary, ids.data(), static_cast<int>(ids.size()), copied)
            && !memcmp(&state, &decoded, sizeof(state)) && !memcmp(&state, &copied, sizeof(state));
        const bool retired = temporary && Phys_DestroyCheckpointConstraint(c.type, temporary);
        const int after[] = {phys_sys::get_rbc_point_alloc_count(), phys_sys::get_rbc_dist_alloc_count(), phys_sys::get_rbc_hinge_alloc_count()};
        const bool preserved = Phys_CaptureConstraintState(0, c.type, static_cast<unsigned>(c.constraintHandle),
            ids.data(), static_cast<int>(ids.size()), original) && !memcmp(&state, &original, sizeof(state));
        Com_Printf(15, "Resume joint probe: index %i JNT1 native clone %s retired %i original %i allocations %i/%i/%i -> %i/%i/%i bytes %i\n",
            i, same && retired && preserved && !memcmp(before, after, sizeof(before)) ? "PASS" : "FAIL",
            retired, preserved, before[0], before[1], before[2], after[0], after[1], after[2], writer.bytesUsed);
    }
}

static void SP_InspectMapOwnerDefinitions()
{
    if (!cm.isInUse || !cm.name || cm.num_constraints > 4096 || (cm.num_constraints && !cm.constraints))
    { Com_Printf(15, "Resume map definitions: unavailable; collision map/inventory not loaded\n"); return; }
    int kinds[NUM_CONSTRAINT_TYPES]{}, unknown = 0, activeRopes = -1;
    const bool ropeStorage = cm.max_ropes >= 0 && cm.max_ropes <= 4096 && g_ropeCount >= 0
        && g_ropeCount <= cm.max_ropes && g_ropes == cm.ropes && (!cm.max_ropes || g_ropes);
    if (ropeStorage) { activeRopes = 0; for (int i = 0; i < g_ropeCount; ++i) activeRopes += g_ropes[i].m_in_use != 0; }
    Com_Printf(15, "Resume map definitions: map %s bsp %s constraints %u ropeCapacity %i mapRopes %i totalRopes %i ropeStorage %i activeRopes %i; definitions only, native graph support not assessed\n",
        Dvar_GetString("mapname"), cm.name, cm.num_constraints, cm.max_ropes, g_mapRopeCount, g_totalRopeCount, ropeStorage, activeRopes);
    for (unsigned i = 0; i < cm.num_constraints; ++i)
    {
        const auto &c = cm.constraints[i];
        if (c.type >= CONSTRAINT_NONE && c.type < NUM_CONSTRAINT_TYPES) ++kinds[c.type]; else ++unknown;
        Com_Printf(15, "Resume map definition: index %u kind %i attach %i:%i/%i:%i ropeSlot %i handlePresent %i flags %i timeout %i\n",
            i, c.type, c.attach_point_type1, c.target_index1, c.attach_point_type2, c.target_index2,
            c.rope_index, c.constraintHandle != 0, c.flags, c.timeout);
    }
    for (int kind = 0; kind < NUM_CONSTRAINT_TYPES; ++kind)
        Com_Printf(15, "Resume map definition count: kind %i count %i\n", kind, kinds[kind]);
    Com_Printf(15, "Resume map definitions END: map %s count %u unknown %i\n", Dvar_GetString("mapname"), cm.num_constraints, unknown);
}
void G_SP_InspectMapOwners()
{
    if (Sys_IsMainThread() && Cmd_Argc() == 2 && !strcmp(Cmd_Argv(1), "definitions"))
    { Com_SyncThreads(); SP_InspectMapOwnerDefinitions(); return; }
    if (!Sys_IsMainThread() || CL_GetLocalClientConnectionState(0) != CA_ACTIVE
        || !G_SP_IsSPLevel() || SP_IsFrontendMapForSave()) return;
    Com_SyncThreads();
    GlassCl_WaitUpdate();
    if (Cmd_Argc() == 3 && !strcmp(Cmd_Argv(1), "retire-server"))
    {
        const int number = atoi(Cmd_Argv(2));
        if (!Dvar_GetInt("cl_paused") || number < 0 || number >= ENTITYNUM_WORLD) return;
        auto &ent = g_entities[number]; SPServerBodyRecord owner;
        if (!SP_ReadLiveServerBody(ent, owner, true) || !ent.targetname) return;
        bool attached = false;
        for (unsigned i = 0; i < cm.num_constraints; ++i)
        {
            const auto &c = cm.constraints[i];
            attached |= c.type == CONSTRAINT_ROPE
                && ((c.attach_point_type1 == ATTACH_POINT_ENT && c.target_ent1 == ent.targetname)
                    || (c.attach_point_type2 == ATTACH_POINT_ENT && c.target_ent2 == ent.targetname));
        }
        if (!attached) return;
        const int native = ent.physObjId;
        G_FreeEntity(&ent);
        Com_Printf(15, "Resume server probe: normal G_FreeEntity entity %i generation %i retired native %i; inuse %i\n",
            number, owner.generation, native, ent.r.inuse);
        return;
    }
    if (Cmd_Argc() == 3 && !strcmp(Cmd_Argv(1), "launch-server"))
    {
        const int number = atoi(Cmd_Argv(2));
        if (!Dvar_GetInt("cl_paused") || number < 0 || number >= ENTITYNUM_WORLD) return;
        auto &ent = g_entities[number]; auto *obj = Com_GetServerDObj(number);
        if (!ent.r.inuse || ent.classname != scr_const.script_model || !ent.model || !ent.targetname
            || ent.tagInfo || ent.physObjId || !obj || obj->numModels != 1) return;
        bool attached = false;
        for (unsigned i = 0; i < cm.num_constraints; ++i)
        {
            const auto &c = cm.constraints[i];
            attached |= c.type == CONSTRAINT_ROPE && !c.constraintHandle
                && ((c.attach_point_type1 == ATTACH_POINT_ENT && c.target_ent1 == ent.targetname)
                    || (c.attach_point_type2 == ATTACH_POINT_ENT && c.target_ent2 == ent.targetname));
        }
        if (!attached) return;
        const float point[3]{}, force[3]{20, 0, 20};
        // Same operation as the script physicslaunch builtin. The next normal
        // server frame creates the real body and matching map constraint.
        ScriptMover_SetupPhysicsLaunch(&ent.s.lerp.pos, &ent.s.lerp.apos, point, force);
        Com_Printf(15, "Resume server probe: queued normal physicslaunch entity %i generation %i; body creation deferred to normal server frame\n", number, ent.useCount);
        return;
    }
    Com_Printf(15, "Resume map owners: map %s server %i client %i physics %i/%i constraints %i ropes %i\n",
        Dvar_GetString("mapname"), level.time, CG_GetLocalClientGlobals(0)->time,
        physGlob.timeLastUpdate, physGlob.timeLastSnapshot, cm.num_constraints, cm.max_ropes);
    for (int world = 0; world < 3; ++world)
    {
        int count = 0;
        for (auto *body = physGlob.objects_by_world[world].m_first; body; body = body->m_next_link)
            if (++count > 8192) Com_Error(ERR_DROP, "Resume inventory physics list invalid");
        Com_Printf(15, "Resume map owners: physics world %i bodies %i\n", world, count);
    }
    for (const auto &ent : g_entities) if (ent.r.inuse && ent.physObjId && ent.physObjId != -1)
        Com_Printf(15, "Resume server owner: entity %i generation %i model %i target %s native %i trajectory %i/%i\n",
            ent.s.number, ent.useCount, ent.model, ent.targetname ? SL_ConvertToString(ent.targetname, SCRIPTINSTANCE_SERVER) : "",
            ent.physObjId, ent.s.lerp.pos.trType, ent.s.lerp.apos.trType);
    for (int draw = 0; draw < 2; ++draw)
    {
        int bodies = 0, changedHealth = 0, changedPose = 0;
        const int count = cm.dynEntCount[draw];
        if (count && (!cm.dynEntDefList[draw] || !cm.dynEntClientList[draw] || !cm.dynEntPoseList[draw]))
            Com_Error(ERR_DROP, "Resume inventory dynamic owner arrays missing");
        for (int i = 0; i < count; ++i)
        {
            const auto &def = cm.dynEntDefList[draw][i];
            const auto &client = cm.dynEntClientList[draw][i];
            const auto &pose = cm.dynEntPoseList[draw][i];
            bodies += client.physObjId != 0;
            changedHealth += client.health != def.health;
            changedPose += memcmp(&pose.pose, &def.pose, sizeof(pose.pose)) != 0;
        }
        Com_Printf(15, "Resume map owners: dynent draw %i clients %i servers %u original models %u bodies %i healthChanges %i poseChanges %i\n",
            draw, count, cm.dynEntCount[draw + 2], cm.originalDynEntCount, bodies, changedHealth, changedPose);
    }
    SP_InspectMapJoints();
    int serverStates[4]{}, clientStates[4]{};
    if (svGlasses.numGlasses > 1000 || (clGlasses && clGlasses->numGlasses > 1000))
        Com_Error(ERR_DROP, "Resume inventory glass owner count invalid");
    for (unsigned i = 0; i < svGlasses.numGlasses; ++i) ++serverStates[svGlasses.glasses[i].state.val.i & 3];
    if (clGlasses)
        for (unsigned i = 0; i < clGlasses->numGlasses; ++i) ++clientStates[clGlasses->glasses[i].state.val.i & 3];
    Com_Printf(15, "Resume map owners: glass server %u states %i/%i/%i/%i nextShatter %i interval %i; client %u states %i/%i/%i/%i\n",
        svGlasses.numGlasses, serverStates[0], serverStates[1], serverStates[2], serverStates[3],
        svGlasses.nextShatterTime, svGlasses.shatterIntervalsMS, clGlasses ? clGlasses->numGlasses : 0,
        clientStates[0], clientStates[1], clientStates[2], clientStates[3]);
    if (clGlasses && clGlasses->renderer)
    {
        const auto &r = *clGlasses->renderer;
        Com_Printf(15, "Resume map owners: glass renderer frame %u time %i pendingActions %u groupChanges %i; observation only\n",
            r.frame, r.timeLastUpdate, r.actionInputIndex - r.actionOutputIndex, r.numGroupChanges);
    }
}
