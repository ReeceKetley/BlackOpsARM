// Included after phys_object_checkpoint.inl. Native allocation and membership
// stay with the canonical solver; the record contains no pointer/handle bytes.
#include <physics/physics_system.h>
#include <universal/narrow32.h>
#include <universal/memfile.h>
#include <vector>
#include <type_traits>

bool Phys_IsSavedConstraintStateValid(const PhysSavedConstraintState &s, int count)
{
    if (count < 1 || count > 4096 || s.kind < 1 || s.kind > 3
        || s.first < -1 || s.first >= count || s.second < -1 || s.second >= count
        || s.first == s.second) return false;
    const int vectors = s.kind == 3 ? 9 : 2, values = s.kind == 1 ? 3 : s.kind == 2 ? 5 : 1;
    const int pulses = s.kind == 3 ? 8 : 3;
    for (int i = 0; i < 9; ++i) for (float v : s.vectors[i])
        if (!std::isfinite(v) || (i >= vectors && v != 0)) return false;
    for (int i = 0; i < 8; ++i)
        if (!std::isfinite(s.values[i]) || !std::isfinite(s.pulses[i])
            || (i >= values && s.values[i] != 0) || (i >= pulses && s.pulses[i] != 0)) return false;
    if (s.kind == 1) return !s.flags && s.spring <= 1 && s.values[0] >= 0 && s.values[1] >= 0 && s.values[2] >= 0;
    if (s.spring || (s.flags & ~(s.kind == 2 ? 3u : 31u))) return false;
    if (s.kind == 2) return s.values[0] >= 0 && s.values[1] >= s.values[0]
        && s.values[2] >= s.values[0] && s.values[4] >= 0;
    if (s.values[0] < 0) return false;
    for (int i = 2; i < 9; ++i)
    {
        float norm = 0; for (float v : s.vectors[i]) norm += v * v;
        if (std::fabs(norm - 1) > 0.004f) return false;
    }
    const auto dot = [&](int a, int b) { float d = 0; for (int i = 0; i < 3; ++i) d += s.vectors[a][i] * s.vectors[b][i]; return d; };
    return std::fabs(dot(2, 6)) <= 0.004f && std::fabs(dot(4, 5)) <= 0.004f;
}
template<class T> static T *Phys_CheckpointFindJoint(phys_free_list<T> &pool, unsigned handle)
{
    if (!handle || pool.m_list_count < 0 || pool.m_list_count > 16384) return nullptr;
    T *found = nullptr; int count = 0;
    for (T *joint : pool)
    {
        if (++count > pool.m_list_count) return nullptr;
        // Compare only actual live allocations. Never decode/dereference a
        // stale or malformed external handle, including the pending sentinel1.
        if (BO1_NARROW_ENCODE(joint) == handle) found = joint;
    }
    return count == pool.m_list_count ? found : nullptr;
}
static rigid_body_constraint *Phys_CheckpointFindJoint(int kind, unsigned handle)
{
    if (!g_physics_system) return nullptr;
    if (kind == 1) return Phys_CheckpointFindJoint(g_physics_system->m_list_rbc_point, handle);
    if (kind == 2) return Phys_CheckpointFindJoint(g_physics_system->m_list_rbc_dist, handle);
    if (kind == 3) return Phys_CheckpointFindJoint(g_physics_system->m_list_rbc_hinge, handle);
    return nullptr;
}
static bool Phys_CheckpointResolveJointBodies(int world, const int *ids, int count, std::vector<rigid_body *> &bodies)
{
    if (!g_physics_system || !ids || count < 1 || count > 4096) return false;
    for (int i = 0; i < count; ++i)
    {
        auto *data = Phys_CheckpointObjectOwner(world, ids[i]);
        if (!data) return false;
        for (auto *body : bodies) if (body == data->body) return false;
        bodies.push_back(data->body);
    }
    return true;
}
template<class T> static void Phys_CheckpointJointFields(T &j, PhysSavedConstraintState &s, bool writingNative)
{
    int v = 0;
    const auto vector = [&](phys_vec3 &a) {
        if (writingNative) Phys_CheckpointWriteVector(a, s.vectors[v]);
        else Phys_CheckpointReadVector(s.vectors[v], a);
        ++v;
    };
    vector(j.m_b1_r_loc); vector(j.m_b2_r_loc);
    if constexpr (std::is_same_v<T, rigid_body_constraint_hinge>)
    {
        vector(j.m_b1_axis_loc); vector(j.m_b2_axis_loc); vector(j.m_b1_a1_loc); vector(j.m_b1_a2_loc);
        vector(j.m_b1_ref_loc); vector(j.m_b2_ref_min_loc); vector(j.m_b2_ref_max_loc);
    }
    int n = 0;
    const auto value = [&](float &a) { if (writingNative) a = s.values[n]; else s.values[n] = a; ++n; };
    if constexpr (std::is_same_v<T, rigid_body_constraint_point>)
    {
        value(j.m_stress); value(j.m_spring_k); value(j.m_damp_k);
        if (writingNative) j.m_spring_enabled = s.spring != 0;
        else { unsigned char raw = 0; memcpy(&raw, &j.m_spring_enabled, 1); s.spring = raw; }
    }
    else
    {
        if constexpr (std::is_same_v<T, rigid_body_constraint_distance>)
        {
            value(j.m_min_distance); value(j.m_max_distance); value(j.m_next_max_distance);
            value(j.m_max_distance_vel); value(j.m_damp_coef);
        }
        else value(j.m_damp_k);
        if (writingNative) j.m_flags = s.flags; else s.flags = j.m_flags;
    }
    const auto pulse = [&](auto &cache) {
        int i = 0; for (auto &p : cache) { if (writingNative) p.m_pulse_sum = s.pulses[i]; else s.pulses[i] = p.m_pulse_sum; ++i; }
    };
    if constexpr (std::is_same_v<T, rigid_body_constraint_hinge>) pulse(j.m_ps_cache);
    else pulse(j.m_ps_cache_list);
}
bool Phys_CaptureConstraintState(int world, int kind, unsigned handle,
    const int *ids, int count, PhysSavedConstraintState &state)
{
    auto *joint = Phys_CheckpointFindJoint(kind, handle);
    std::vector<rigid_body *> bodies;
    if (!joint || !Phys_CheckpointResolveJointBodies(world, ids, count, bodies)) return false;
    const auto ordinal = [&](rigid_body *body) {
        if (body == phys_sys::get_environment_rigid_body()) return -1;
        for (int i = 0; i < count; ++i) if (bodies[i] == body) return i;
        return -2;
    };
    PhysSavedConstraintState s; s.kind = kind; s.first = ordinal(joint->b1); s.second = ordinal(joint->b2);
    if (kind == 1) Phys_CheckpointJointFields(*static_cast<rigid_body_constraint_point *>(joint), s, false);
    else if (kind == 2) Phys_CheckpointJointFields(*static_cast<rigid_body_constraint_distance *>(joint), s, false);
    else Phys_CheckpointJointFields(*static_cast<rigid_body_constraint_hinge *>(joint), s, false);
    if (!Phys_IsSavedConstraintStateValid(s, count)) return false;
    state = s; return true;
}
bool Phys_DestroyCheckpointConstraint(int kind, unsigned handle)
{
    auto *joint = Phys_CheckpointFindJoint(kind, handle);
    if (!joint) return false;
    if (kind == 1) phys_sys::destroy(static_cast<rigid_body_constraint_point *>(joint));
    else if (kind == 2) phys_sys::destroy(static_cast<rigid_body_constraint_distance *>(joint));
    else phys_sys::destroy(static_cast<rigid_body_constraint_hinge *>(joint));
    return true;
}
unsigned Phys_CreateSavedConstraint(int world, const int *ids, int count, const PhysSavedConstraintState &state)
{
    if (!Phys_IsSavedConstraintStateValid(state, count)) return 0;
    std::vector<rigid_body *> bodies;
    if (!Phys_CheckpointResolveJointBodies(world, ids, count, bodies)) return 0;
    const auto body = [&](int i) { return i == -1 ? phys_sys::get_environment_rigid_body()
        : reinterpret_cast<environment_rigid_body *>(bodies[i]); };
    auto s = state; rigid_body_constraint *joint = nullptr;
    if (s.kind == 1)
    {
        auto *j = phys_sys::create_rbc_point(body(s.first), body(s.second), 1);
        if (j) Phys_CheckpointJointFields(*j, s, true); joint = j;
    }
    else if (s.kind == 2)
    {
        auto *j = phys_sys::create_rbc_dist(body(s.first), body(s.second), 1);
        if (j) Phys_CheckpointJointFields(*j, s, true); joint = j;
    }
    else
    {
        auto *j = phys_sys::create_rbc_hinge(body(s.first), body(s.second), 1);
        if (j) Phys_CheckpointJointFields(*j, s, true); joint = j;
    }
    if (!joint) return 0;
    joint->m_next = nullptr; // The new solver builds its own partition links.
    const unsigned handle = BO1_NARROW_ENCODE(joint);
    PhysSavedConstraintState check;
    if (Phys_CaptureConstraintState(world, s.kind, handle, ids, count, check) && !memcmp(&check, &state, sizeof(check))) return handle;
    Phys_DestroyCheckpointConstraint(s.kind, handle); return 0;
}
void Phys_WriteConstraintState(MemoryFile *file, const PhysSavedConstraintState &s, int count)
{
    if (!Phys_IsSavedConstraintStateValid(s, count)) Com_Error(ERR_DROP, "Physics checkpoint invalid logical joint");
    int magic = 0x31544E4A; auto value = s; // JNT1; legacy MemFile takes mutable bytes.
    MemFile_WriteData(file, sizeof(magic), reinterpret_cast<unsigned char *>(&magic));
    MemFile_WriteData(file, sizeof(value), reinterpret_cast<unsigned char *>(&value));
}
void Phys_ReadConstraintState(MemoryFile *file, PhysSavedConstraintState &state, int count)
{
    int magic; PhysSavedConstraintState s;
    MemFile_ReadData(file, sizeof(magic), reinterpret_cast<unsigned char *>(&magic));
    if (magic != 0x31544E4A) Com_Error(ERR_DROP, "Physics checkpoint joint schema mismatch");
    MemFile_ReadData(file, sizeof(s), reinterpret_cast<unsigned char *>(&s));
    if (!Phys_IsSavedConstraintStateValid(s, count)) Com_Error(ERR_DROP, "Physics checkpoint invalid decoded joint");
    state = s;
}

template<class T> static bool Phys_CheckpointGraphPoolClosed(phys_free_list<T> &pool,
    const std::vector<rigid_body *> &bodies, const std::vector<rigid_body_constraint *> &joints)
{
    if (pool.m_list_count < 0 || pool.m_list_count > 16384) return false;
    int count = 0;
    for (T *joint : pool)
    {
        if (++count > pool.m_list_count) return false;
        bool touches = false;
        for (const auto *body : bodies) touches |= joint->b1 == body || joint->b2 == body;
        if (!touches) continue;
        bool accounted = false;
        for (const auto *saved : joints) accounted |= saved == joint;
        if (!accounted) return false;
    }
    return count == pool.m_list_count;
}
bool Phys_CaptureObjectGraph(int world, const int *ids, int count,
    const PhysCheckpointJointRef *refs, int jointCount,
    PhysSavedObjectState *states, PhysSavedConstraintState *constraints)
{
    if (!states || jointCount < 0 || jointCount > 4096 || (jointCount && (!refs || !constraints))) return false;
    std::vector<rigid_body *> bodies;
    if (!Phys_CheckpointResolveJointBodies(world, ids, count, bodies)) return false;
    std::vector<rigid_body_constraint *> joints;
    std::vector<PhysSavedConstraintState> savedJoints(jointCount);
    for (int i = 0; i < jointCount; ++i)
    {
        auto *joint = Phys_CheckpointFindJoint(refs[i].kind, refs[i].handle);
        if (!joint || !Phys_CaptureConstraintState(world, refs[i].kind, refs[i].handle, ids, count, savedJoints[i])) return false;
        for (const auto *other : joints) if (other == joint) return false;
        joints.push_back(joint);
    }
    // Checking all pools prevents a supported joint from masking an omitted
    // joint or a different, unsupported constraint class on the same body.
#define PHYS_GRAPH_POOL(n) if (!Phys_CheckpointGraphPoolClosed(g_physics_system->n, bodies, joints)) return false;
    PHYS_GRAPH_POOL(m_list_rbc_point) PHYS_GRAPH_POOL(m_list_rbc_dist) PHYS_GRAPH_POOL(m_list_rbc_hinge)
    PHYS_GRAPH_POOL(m_list_rbc_ragdoll) PHYS_GRAPH_POOL(m_list_rbc_wheel)
    PHYS_GRAPH_POOL(m_list_rbc_angular_actuator) PHYS_GRAPH_POOL(m_list_rbc_upright)
    PHYS_GRAPH_POOL(m_list_rbc_custom_orientation) PHYS_GRAPH_POOL(m_list_rbc_custom_path)
#undef PHYS_GRAPH_POOL
    std::vector<PhysSavedObjectState> savedBodies(count);
    for (int i = 0; i < count; ++i)
        if (!Phys_CaptureObjectOwnerState(Phys_CheckpointObjectOwner(world, ids[i]), savedBodies[i])) return false;
    for (int i = 0; i < count; ++i) states[i] = savedBodies[i];
    for (int i = 0; i < jointCount; ++i) constraints[i] = savedJoints[i];
    return true;
}
bool Phys_CaptureRetiringJoints(int world, const int *ids, int count,
    const PhysCheckpointJointRef *refs, int jointCount, PhysSavedConstraintState *states)
{
    if (jointCount < 0 || jointCount > 4096 || (jointCount && (!refs || !states))) return false;
    std::vector<rigid_body *> bodies;
    if (!Phys_CheckpointResolveJointBodies(world, ids, count, bodies)) return false;
    std::vector<rigid_body_constraint *> joints;
    std::vector<PhysSavedConstraintState> captured(jointCount);
    for (int i = 0; i < jointCount; ++i)
    {
        auto *joint = Phys_CheckpointFindJoint(refs[i].kind, refs[i].handle);
        if (!joint || !Phys_CaptureConstraintState(world, refs[i].kind, refs[i].handle, ids, count, captured[i])) return false;
        for (const auto *other : joints) if (other == joint) return false;
        joints.push_back(joint);
    }
    for (int i = 0; i < jointCount; ++i) states[i] = captured[i];
    return true;
}
bool Phys_ApplyObjectGraph(int world, const int *ids, int count,
    const PhysSavedObjectState *states, const PhysSavedConstraintState *constraints,
    int jointCount, unsigned *handles)
{
    if (!states || jointCount < 0 || jointCount > 4096 || (jointCount && (!constraints || !handles))) return false;
    std::vector<rigid_body *> bodies;
    if (!Phys_CheckpointResolveJointBodies(world, ids, count, bodies)) return false;
    std::vector<PhysSavedObjectState> previous(count);
    for (int i = 0; i < count; ++i)
    {
        if (!Phys_IsSavedObjectStateValid(states[i]) || !Phys_CaptureObjectState(world, ids[i], previous[i])
            || previous[i].geometryCount != states[i].geometryCount || previous[i].owner != states[i].owner
            || previous[i].collisionCount != states[i].collisionCount || memcmp(previous[i].center, states[i].center, sizeof(states[i].center))) return false;
    }
    for (int i = 0; i < jointCount; ++i)
        if (!Phys_IsSavedConstraintStateValid(constraints[i], count)) return false;
    std::vector<unsigned> created(jointCount);
    int applied = 0, allocated = 0;
    for (; applied < count; ++applied)
        if (!Phys_ApplyObjectState(world, ids[applied], states[applied])) break;
    if (applied == count)
        for (; allocated < jointCount; ++allocated)
        {
            created[allocated] = Phys_CreateSavedConstraint(world, ids, count, constraints[allocated]);
            if (!created[allocated]) break;
        }
    if (applied == count && allocated == jointCount)
    {
        for (int i = 0; i < jointCount; ++i) handles[i] = created[i];
        return true;
    }
    for (int i = allocated - 1; i >= 0; --i)
        if (!Phys_DestroyCheckpointConstraint(constraints[i].kind, created[i]))
            Com_Error(ERR_DROP, "Physics checkpoint joint rollback lost native owner");
    // Include the failing body: a post-apply readback can fail after mutation.
    for (int i = 0; i < count && i <= applied; ++i)
        if (!Phys_ApplyObjectState(world, ids[i], previous[i]))
            Com_Error(ERR_DROP, "Physics checkpoint body rollback differs");
    return false;
}
