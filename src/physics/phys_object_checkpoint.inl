// Included by phys_main.cpp with the canonical physics declarations in scope.
#include <climits>
#include <initializer_list>
#include <physics/physics_system_internal.h>
bool Phys_CaptureCheckpointClock(PhysSavedClockState &state)
{
    state = {physGlob.timeLastUpdate, physGlob.timeLastSnapshot};
    return state.update >= 0 && state.snapshot >= 0 && state.snapshot <= state.update;
}
bool Phys_ApplyCheckpointClock(const PhysSavedClockState &state)
{
    if (state.update < 0 || state.snapshot < 0 || state.snapshot > state.update) return false;
    // The caller has quiesced all physics owners and is installing logical
    // state. This changes the epoch without running a bootstrap solver step.
    physGlob.timeLastUpdate = state.update; physGlob.timeLastSnapshot = state.snapshot;
    return true;
}

static PhysObjUserData *Phys_CheckpointObjectOwner(int world, int id)
{
    if (world < 0 || world >= 3 || !id || id == -1) return nullptr;
    // Resolve only through the live world list. Do not dereference an opaque
    // old/stale ID, and do not assume an ID is a native address.
    int count = 0;
    for (auto *data = physGlob.objects_by_world[world].m_first; data; data = data->m_next_link)
    {
        if (++count > 8192) return nullptr;
        if (Phys_GetObjId(data) != id) continue;
        if (!data->body || data->vehicle || data->refcount != 1 || !data->m_bpb
            || data->body->m_userdata != data || (data->body->m_flags & ~15u)) return nullptr;
        return data;
    }
    return nullptr;
}
template<class T> static bool Phys_CheckpointPoolTouches(phys_free_list<T> &pool, const rigid_body *body)
{
    if (pool.m_list_count < 0 || pool.m_list_count > 16384) return true;
    int count = 0;
    for (T *joint : pool)
    {
        if (++count > pool.m_list_count || joint->b1 == body || joint->b2 == body) return true;
    }
    return count != pool.m_list_count;
}
static PhysObjUserData *Phys_CheckpointObject(int world, int id)
{
    auto *data = Phys_CheckpointObjectOwner(world, id);
    if (!data || !g_physics_system) return nullptr;
    const auto &p = data->body->m_partition_node;
    if (p.m_rbc_point_first || p.m_rbc_hinge_first || p.m_rbc_dist_first || p.m_rbc_ragdoll_first
        || p.m_rbc_wheel_first || p.m_rbc_angular_actuator_first || p.m_rbc_upright_first
        || p.m_rbc_custom_orientation_first || p.m_rbc_custom_path_first) return nullptr;
    // Partition links exist only on an island's head and are rebuilt by a
    // solver step. Inspect live pools as well, including newly created joints.
#define PHYS_BOUND_POOL(n) if (Phys_CheckpointPoolTouches(g_physics_system->n, data->body)) return nullptr;
    PHYS_BOUND_POOL(m_list_rbc_point) PHYS_BOUND_POOL(m_list_rbc_hinge) PHYS_BOUND_POOL(m_list_rbc_dist)
    PHYS_BOUND_POOL(m_list_rbc_ragdoll) PHYS_BOUND_POOL(m_list_rbc_wheel)
    PHYS_BOUND_POOL(m_list_rbc_angular_actuator) PHYS_BOUND_POOL(m_list_rbc_upright)
    PHYS_BOUND_POOL(m_list_rbc_custom_orientation) PHYS_BOUND_POOL(m_list_rbc_custom_path)
#undef PHYS_BOUND_POOL
    // Contact pairs remain transient solver owners, rebuilt by normal collision.
    return data;
}
static int Phys_CheckpointCollisionNodes(PhysObjUserData *data, broad_phase_base **nodes)
{
    if (!data->m_bpb->is_bpi() && !data->m_bpb->is_bpg()) return 0;
    int count = 1; nodes[0] = data->m_bpb;
    if (data->m_bpb->is_bpg())
        for (broad_phase_base *node = data->m_bpb->get_bpg()->m_list_bpi_head; node; node = node->m_list_bpb_next)
        {
            if (count == 65 || !node->is_bpi()) return 0;
            for (int i = 0; i < count; ++i) if (nodes[i] == node) return 0;
            nodes[count++] = node;
        }
    return count;
}
static void Phys_CheckpointReadVector(float *out, const phys_vec3 &v)
{
    out[0] = v.x; out[1] = v.y; out[2] = v.z;
}
static void Phys_CheckpointWriteVector(phys_vec3 &v, const float *in)
{
    v.x = in[0]; v.y = in[1]; v.z = in[2];
}
bool Phys_IsSavedObjectStateValid(const PhysSavedObjectState &s)
{
    const auto finite = [](const float *p, int n) { for (int i = 0; i < n; ++i) if (!std::isfinite(p[i])) return false; return true; };
    if (!finite(&s.matrix[0][0], 12) || !finite(&s.center[0][0], 12)
        || !finite(s.savedPos, 3) || !finite(&s.savedRot[0][0], 9)
        || !finite(s.buoyancyBoxMin, 3) || !finite(s.buoyancyBoxMax, 3) || !finite(s.centerOfMassOffset, 3)) return false;
#define PHYS_OBJECT_VEC(n) if (!finite(s.n, 3)) return false;
    PHYS_SAVED_OBJECT_VECTORS(PHYS_OBJECT_VEC)
#undef PHYS_OBJECT_VEC
#define PHYS_OBJECT_FLOAT(n) if (!std::isfinite(s.n)) return false;
    PHYS_SAVED_OBJECT_FLOATS(PHYS_OBJECT_FLOAT)
    PHYS_SAVED_OBJECT_USER_FLOATS(PHYS_OBJECT_FLOAT)
#undef PHYS_OBJECT_FLOAT
    if ((s.bodyFlags & ~15u) || s.m_inv_mass <= 0 || s.mass <= 0 || s.friction < 0 || s.bounce < 0
        || s.m_max_delta_t <= 0 || s.m_max_avel < 0 || s.m_t_drag_coef < 0 || s.m_a_drag_coef < 0
        || s.geometryCount < 1 || s.geometryCount > 4096 || s.collisionCount < 1 || s.collisionCount > 65
        || s.stableMinContacts < 0 || s.stableMinContacts > 64 || (unsigned)s.underwater > 1
        || (unsigned)s.trDuration > 65535 || (s.owner != -1 && (unsigned)s.owner >= 2048)) return false;
    for (int i = s.collisionCount; i < 65; ++i) if (s.collisionFlags[i][0] || s.collisionFlags[i][1]) return false;
    for (const auto *m : {s.matrix, s.center})
        for (int row = 0; row < 3; ++row)
            for (int other = row; other < 3; ++other)
            {
                float dot = 0; for (int k = 0; k < 3; ++k) dot += m[row][k] * m[other][k];
                if (std::fabs(dot - (row == other ? 1.0f : 0.0f)) > 0.004f) return false;
            }
    return true;
}
static bool Phys_CaptureObjectOwnerState(PhysObjUserData *data, PhysSavedObjectState &state)
{
    if (!data) return false;
    broad_phase_base *nodes[65]{}; const int count = Phys_CheckpointCollisionNodes(data, nodes); if (!count) return false;
    const long long buoyant = (long long)data->timeBuoyant - physGlob.timeLastUpdate;
    const long long ripple = (long long)data->timeRipple - physGlob.timeLastUpdate;
    if (buoyant < INT_MIN || buoyant > INT_MAX || ripple < INT_MIN || ripple > INT_MAX) return false;
    PhysSavedObjectState s{}; const auto &body = *data->body;
    const phys_vec3 *matrix[] = {&body.m_mat.x, &body.m_mat.y, &body.m_mat.z, &body.m_mat.w};
    const phys_vec3 *center[] = {&data->cg2rb.x, &data->cg2rb.y, &data->cg2rb.z, &data->cg2rb.w};
    for (int i = 0; i < 4; ++i) { Phys_CheckpointReadVector(s.matrix[i], *matrix[i]); Phys_CheckpointReadVector(s.center[i], *center[i]); }
#define PHYS_OBJECT_VEC(n) Phys_CheckpointReadVector(s.n, body.n);
    PHYS_SAVED_OBJECT_VECTORS(PHYS_OBJECT_VEC)
#undef PHYS_OBJECT_VEC
#define PHYS_OBJECT_FLOAT(n) s.n = body.n;
    PHYS_SAVED_OBJECT_FLOATS(PHYS_OBJECT_FLOAT)
#undef PHYS_OBJECT_FLOAT
#define PHYS_OBJECT_FLOAT(n) s.n = data->n;
    PHYS_SAVED_OBJECT_USER_FLOATS(PHYS_OBJECT_FLOAT)
#undef PHYS_OBJECT_FLOAT
    memcpy(s.savedPos, data->savedPos, sizeof(s.savedPos)); memcpy(s.savedRot, data->savedRot, sizeof(s.savedRot));
    memcpy(s.buoyancyBoxMin, data->buoyancyBoxMin, sizeof(s.buoyancyBoxMin)); memcpy(s.buoyancyBoxMax, data->buoyancyBoxMax, sizeof(s.buoyancyBoxMax));
    memcpy(s.centerOfMassOffset, data->centerOfMassOffset, sizeof(s.centerOfMassOffset));
    s.bodyFlags = body.m_flags; s.userFlags = data->m_flags; s.stableMinContacts = body.m_stable_min_contact_count;
    s.underwater = data->underwater; s.buoyancy = data->buoyancy; s.trDuration = data->trDuration;
    s.buoyantOffset = (int)buoyant; s.rippleOffset = (int)ripple; s.geometryCount = data->m_gjk_geom_list.m_geom_count; s.collisionCount = count;
    s.owner = data->id;
    for (int i = 0; i < count; ++i) { s.collisionFlags[i][0] = nodes[i]->m_env_collision_flags; s.collisionFlags[i][1] = nodes[i]->m_my_collision_type_flags; }
    if (!Phys_IsSavedObjectStateValid(s)) return false;
    state = s; return true;
}
bool Phys_CaptureObjectState(int world, int id, PhysSavedObjectState &state)
{
    return Phys_CaptureObjectOwnerState(Phys_CheckpointObject(world, id), state);
}
bool Phys_ApplyObjectState(int world, int id, const PhysSavedObjectState &s)
{
    if (!Phys_IsSavedObjectStateValid(s)) return false;
    auto *data = Phys_CheckpointObject(world, id); if (!data) return false;
    PhysSavedObjectState fresh{};
    if (!Phys_CaptureObjectState(world, id, fresh) || fresh.geometryCount != s.geometryCount
        || fresh.owner != s.owner || fresh.collisionCount != s.collisionCount || memcmp(fresh.center, s.center, sizeof(s.center))) return false;
    const long long buoyant = (long long)physGlob.timeLastUpdate + s.buoyantOffset;
    const long long ripple = (long long)physGlob.timeLastUpdate + s.rippleOffset;
    if (buoyant < INT_MIN || buoyant > INT_MAX || ripple < INT_MIN || ripple > INT_MAX) return false;
    broad_phase_base *nodes[65]{}; if (Phys_CheckpointCollisionNodes(data, nodes) != s.collisionCount) return false;
    auto &body = *data->body;
    phys_vec3 *matrix[] = {&body.m_mat.x, &body.m_mat.y, &body.m_mat.z, &body.m_mat.w};
    for (int i = 0; i < 4; ++i) Phys_CheckpointWriteVector(*matrix[i], s.matrix[i]);
#define PHYS_OBJECT_VEC(n) Phys_CheckpointWriteVector(body.n, s.n);
    PHYS_SAVED_OBJECT_VECTORS(PHYS_OBJECT_VEC)
#undef PHYS_OBJECT_VEC
#define PHYS_OBJECT_FLOAT(n) body.n = s.n;
    PHYS_SAVED_OBJECT_FLOATS(PHYS_OBJECT_FLOAT)
#undef PHYS_OBJECT_FLOAT
#define PHYS_OBJECT_FLOAT(n) data->n = s.n;
    PHYS_SAVED_OBJECT_USER_FLOATS(PHYS_OBJECT_FLOAT)
#undef PHYS_OBJECT_FLOAT
    memcpy(data->savedPos, s.savedPos, sizeof(s.savedPos)); memcpy(data->savedRot, s.savedRot, sizeof(s.savedRot));
    memcpy(data->buoyancyBoxMin, s.buoyancyBoxMin, sizeof(s.buoyancyBoxMin)); memcpy(data->buoyancyBoxMax, s.buoyancyBoxMax, sizeof(s.buoyancyBoxMax));
    memcpy(data->centerOfMassOffset, s.centerOfMassOffset, sizeof(s.centerOfMassOffset));
    body.m_flags = s.bodyFlags; body.m_stable_min_contact_count = s.stableMinContacts;
    data->m_flags = s.userFlags; data->underwater = s.underwater; data->buoyancy = s.buoyancy; data->trDuration = s.trDuration;
    data->timeBuoyant = (int)buoyant; data->timeRipple = (int)ripple;
    for (int i = 0; i < s.collisionCount; ++i) { nodes[i]->m_env_collision_flags = s.collisionFlags[i][0]; nodes[i]->m_my_collision_type_flags = s.collisionFlags[i][1]; }
    // Native tick, solver node, contact caches and partition links remain owned
    // by the new solver. Verify every persisted logical field, not old IDs.
    PhysSavedObjectState check{};
    return Phys_CaptureObjectState(world, id, check) && !memcmp(&check, &s, sizeof(s));
}
