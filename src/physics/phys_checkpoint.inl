#include "phys_checkpoint.h"
#include <universal/memfile.h>
#include <cmath>
#include <cstring>

bool Phys_IsSavedBodyStateValid(const PhysSavedBodyState *state)
{
    if (!state)
        return false;
    const auto finite = [](const float *values, int count) {
        for (int i = 0; i < count; ++i)
            if (!std::isfinite(values[i]) || std::fabs(values[i]) > 100000.0f)
                return false;
        return true;
    };
    if (!finite(state->origin, 3) || !finite(state->quaternion, 4)
        || !finite(state->linearVelocity, 3) || !finite(state->angularVelocity, 3) || !finite(state->gravity, 3))
        return false;
    float norm = 0.0f;
    for (float q : state->quaternion)
        norm += q * q;
    return std::fabs(norm - 1.0f) <= 0.002f;
}

void Phys_CaptureBodyState(int id, PhysSavedBodyState *state)
{
    if (!id || !state)
        Com_Error(ERR_DROP, "Physics checkpoint requires an owned body");
    float axis[3][3];
    Phys_ObjGetPosition(id, state->origin, axis);
    AxisToQuat(axis, state->quaternion);
    Phys_ObjGetVelocities(id, state->linearVelocity, state->angularVelocity);
    Phys_NitrousVecToVec3(&Phys_GetUserData(id)->body->m_gravity_acc_vec, state->gravity);
    if (!Phys_IsSavedBodyStateValid(state))
        Com_Error(ERR_DROP, "Physics checkpoint contains invalid kinematics");
}

void Phys_ApplyBodyState(int id, const PhysSavedBodyState *state)
{
    if (!id || !Phys_IsSavedBodyStateValid(state))
        Com_Error(ERR_DROP, "Physics checkpoint cannot apply invalid kinematics");
    PhysSavedBodyState value = *state;
    Phys_ObjSetOrientation(id, value.origin, value.quaternion);
    Phys_ObjSetVelocity(id, value.linearVelocity);
    Phys_ObjSetAngularVelocityRaw(id, value.angularVelocity);
    Phys_Vec3ToNitrousVec(value.gravity, &Phys_GetUserData(id)->body->m_gravity_acc_vec);
}

bool Phys_VerifyAppliedBodyState(int id, const PhysSavedBodyState *state)
{
    if (!id || !Phys_IsSavedBodyStateValid(state)) return false;
    const auto *data = Phys_GetUserData(id);
    if (!data || !data->body) return false;
    // Verify the exact canonical destination, before another simulation step.
    // AxisToQuat(GetPosition()) is not a bitwise inverse of SetOrientation:
    // both quaternion conversion and the model/COM translation round floats.
    float axis[3][3], offset[3], rotated[3], center[3];
    QuatToAxis(state->quaternion, axis);
    Phys_NitrousVecToVec3(&data->cg2rb.w, offset);
    AxisTransformVec3(axis, offset, rotated);
    for (int i = 0; i < 3; ++i) center[i] = state->origin[i] - rotated[i];
    float actualAxis[3][3], actualCenter[3], velocity[3], angular[3], gravity[3];
    Phys_NitrousVecToVec3(&data->body->m_mat.x, actualAxis[0]);
    Phys_NitrousVecToVec3(&data->body->m_mat.y, actualAxis[1]);
    Phys_NitrousVecToVec3(&data->body->m_mat.z, actualAxis[2]);
    Phys_NitrousVecToVec3(&data->body->m_mat.w, actualCenter);
    Phys_ObjGetVelocities(id, velocity, angular);
    Phys_NitrousVecToVec3(&data->body->m_gravity_acc_vec, gravity);
    return !memcmp(axis, actualAxis, sizeof(axis))
        && !memcmp(center, actualCenter, sizeof(center))
        && !memcmp(velocity, state->linearVelocity, sizeof(velocity))
        && !memcmp(angular, state->angularVelocity, sizeof(angular))
        && !memcmp(gravity, state->gravity, sizeof(gravity));
}

void Phys_WriteBodyState(MemoryFile *file, const PhysSavedBodyState *state)
{
    if (!Phys_IsSavedBodyStateValid(state))
        Com_Error(ERR_DROP, "Physics checkpoint cannot write invalid kinematics");
    MemFile_WriteData(file, sizeof(*state), reinterpret_cast<unsigned char *>(const_cast<PhysSavedBodyState *>(state)));
}

void Phys_ReadBodyState(MemoryFile *file, PhysSavedBodyState *state)
{
    MemFile_ReadData(file, sizeof(*state), reinterpret_cast<unsigned char *>(state));
    if (!Phys_IsSavedBodyStateValid(state))
        Com_Error(ERR_DROP, "Physics checkpoint rejected invalid kinematics");
}
