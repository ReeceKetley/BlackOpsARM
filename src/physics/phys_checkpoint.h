#pragma once

struct MemoryFile;

// Logical kinematics only. Shape, world, ownership and constraints belong to
// the canonical caller which creates the replacement body before applying it.
struct PhysSavedBodyState
{
    float origin[3];
    float quaternion[4];
    float linearVelocity[3];
    float angularVelocity[3];
    float gravity[3];
};
static_assert(sizeof(PhysSavedBodyState) == 64, "fixed32 logical physics record");

bool Phys_IsSavedBodyStateValid(const PhysSavedBodyState *state);
void Phys_CaptureBodyState(int id, PhysSavedBodyState *state);
void Phys_ApplyBodyState(int id, const PhysSavedBodyState *state);
bool Phys_VerifyAppliedBodyState(int id, const PhysSavedBodyState *state);
void Phys_WriteBodyState(MemoryFile *file, const PhysSavedBodyState *state);
void Phys_ReadBodyState(MemoryFile *file, PhysSavedBodyState *state);

// Independent model-body state. Geometry and native solver/contact owners are
// rebuilt by the caller's canonical factory, never copied from a save.
#define PHYS_SAVED_OBJECT_VECTORS(X) \
    X(m_last_position) X(m_moved_vec) X(m_inv_inertia) X(m_gravity_acc_vec) \
    X(m_t_vel) X(m_a_vel) X(m_last_t_vel) X(m_last_a_vel) X(m_force_sum) X(m_torque_sum)
#define PHYS_SAVED_OBJECT_FLOATS(X) \
    X(m_smallest_lambda) X(m_inv_mass) X(m_max_avel) X(m_max_delta_t) \
    X(m_stable_energy_time) X(m_largest_vel_sq) X(m_t_drag_coef) X(m_a_drag_coef)
#define PHYS_SAVED_OBJECT_USER_FLOATS(X) \
    X(friction) X(bounce) X(mass) X(m_time_since_last_event) X(m_time_since_last_reeval)
struct PhysSavedObjectState
{
    float matrix[4][3]{}, center[4][3]{};
#define PHYS_OBJECT_VEC(n) float n[3]{};
    PHYS_SAVED_OBJECT_VECTORS(PHYS_OBJECT_VEC)
#undef PHYS_OBJECT_VEC
#define PHYS_OBJECT_FLOAT(n) float n{};
    PHYS_SAVED_OBJECT_FLOATS(PHYS_OBJECT_FLOAT)
    PHYS_SAVED_OBJECT_USER_FLOATS(PHYS_OBJECT_FLOAT)
#undef PHYS_OBJECT_FLOAT
    float savedPos[3]{}, savedRot[3][3]{}, buoyancyBoxMin[3]{}, buoyancyBoxMax[3]{}, centerOfMassOffset[3]{};
    unsigned int bodyFlags = 0, userFlags = 0;
    int stableMinContacts = 0, underwater = 0, buoyancy = 0, trDuration = 0;
    int buoyantOffset = 0, rippleOffset = 0, geometryCount = 0, collisionCount = 0;
    int owner = 0;
    unsigned int collisionFlags[65][2]{};
};
static_assert(sizeof(PhysSavedObjectState) == 916, "fixed32 independent model-body record");
bool Phys_IsSavedObjectStateValid(const PhysSavedObjectState &state);
bool Phys_CaptureObjectState(int world, int id, PhysSavedObjectState &state);
bool Phys_ApplyObjectState(int world, int id, const PhysSavedObjectState &state);
struct PhysSavedClockState { int update = 0, snapshot = 0; };
bool Phys_CaptureCheckpointClock(PhysSavedClockState &state);
bool Phys_ApplyCheckpointClock(const PhysSavedClockState &state);

// Point/distance/hinge state. Body references are caller-owned logical ordinals;
// -1 names the environment. Native handles, allocator links and pointers stay
// in the running solver. Unused entries are zero for a canonical wire image.
struct PhysSavedConstraintState
{
    int kind = 0, first = -1, second = -1;
    float vectors[9][3]{};
    float values[8]{}, pulses[8]{};
    unsigned flags = 0, spring = 0;
};
static_assert(sizeof(PhysSavedConstraintState) == 192, "fixed32 logical joint record");
bool Phys_IsSavedConstraintStateValid(const PhysSavedConstraintState &state, int bodyCount);
bool Phys_CaptureConstraintState(int world, int kind, unsigned handle,
    const int *bodyIds, int bodyCount, PhysSavedConstraintState &state);
unsigned Phys_CreateSavedConstraint(int world, const int *bodyIds, int bodyCount,
    const PhysSavedConstraintState &state);
bool Phys_DestroyCheckpointConstraint(int kind, unsigned handle);
void Phys_WriteConstraintState(MemoryFile *file, const PhysSavedConstraintState &state, int bodyCount);
void Phys_ReadConstraintState(MemoryFile *file, PhysSavedConstraintState &state, int bodyCount);

// Runtime-only references supplied by the logical owner registry. Capture must
// account for every non-contact joint touching the cohort, including joints
// whose other endpoint is outside it. Outputs publish only after full closure.
struct PhysCheckpointJointRef { int kind = 0; unsigned handle = 0; };
bool Phys_CaptureObjectGraph(int world, const int *bodyIds, int bodyCount,
    const PhysCheckpointJointRef *joints, int jointCount,
    PhysSavedObjectState *bodies, PhysSavedConstraintState *constraints);
// Old-world teardown validates real body ownership and each named map joint,
// without requiring unrelated old constraints/kinematics to be saveable. The
// remaining joints stay with their old bodies and canonical body destruction.
// No live state changes; joint output publishes only after all refs validate.
bool Phys_CaptureRetiringJoints(int world, const int *bodyIds, int bodyCount,
    const PhysCheckpointJointRef *joints, int jointCount, PhysSavedConstraintState *states);
// Targets must be fresh, independent bodies. Failure retires new joints and
// restores the targets' previous logical body state; output handles stay intact.
bool Phys_ApplyObjectGraph(int world, const int *bodyIds, int bodyCount,
    const PhysSavedObjectState *bodies, const PhysSavedConstraintState *constraints,
    int jointCount, unsigned *handles);
