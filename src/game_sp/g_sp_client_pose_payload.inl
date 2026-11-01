// CPC2 active union members only. The union's native pointers are represented
// by canonical owner ordinals; never copy the native union image to disk.
enum SPClientPosePayloadKind { CPC_GENERAL, CPC_PLAYER, CPC_TURRET, CPC_VEHICLE, CPC_FX, CPC_ACTOR };
static int SP_CPCPayloadKind(int type)
{
    switch (type)
    {
    case ET_PLAYER: return CPC_PLAYER;
    case ET_MG42: return CPC_TURRET;
    case ET_HELICOPTER: case ET_VEHICLE: case ET_VEHICLE_CORPSE: return CPC_VEHICLE;
    case ET_FX: case ET_LOOP_FX: return CPC_FX;
    case ET_ACTOR: case ET_ACTOR_CORPSE: return CPC_ACTOR;
    default: return CPC_GENERAL;
    }
}
static int SP_CPCOwnerPayloadKind(int kind, int index, int poseType, int entityType)
{
    // Client-script FX run CG_Fx/CG_LoopFx directly, without updating the
    // rendering pose's type/union tag. Their logical entity owns CEntFx.
    if (!kind && EntNum_IsCsc(index) && (entityType == ET_FX || entityType == ET_LOOP_FX))
        return CPC_FX;
    return SP_CPCPayloadKind(poseType);
}
union SPClientPosePayload
{
    CEntPlayerInfo player;
    CEntTurretInfo turret;
    CEntVehicleInfo vehicle;
    CEntFx fx;
    CEntActorInfo actor;
    CEntGeneral general;
    SPClientPosePayload() : vehicle{} {}
};
struct SPClientPoseComponent
{
    int kind = 0, index = 0, control = -1;
    unsigned char type = 0, unionType = 0;
    unsigned char payloadKind = CPC_GENERAL;
    SPClientPosePayload payload;
    bool linked = false, queried = false;
    float linkMins[3]{}, linkMaxs[3]{};
    // Only the viewmodel lacks a CET1 pose-core record.
    float origin[3]{}, angles[3]{}, mins[3]{}, maxs[3]{}, wetness = 0;
    int burnTime = 0;
    DObjAnimMat reference{};
};
static void SP_CPCControllerCodec(SPNativeCheckpointCodec &c, clientControllers_t &control)
{
    SP_CNSValue(c, control.angles); SP_CNSValue(c, control.tag_origin_angles); SP_CNSValue(c, control.tag_origin_offset);
}
static bool SP_CPCSlot(int kind, int index)
{
    return kind == 2 ? index == 0 : SP_ClientPoseSlot(kind, index);
}
static void SP_CPCPoseCodec(SPNativeCheckpointCodec &c, SPClientPoseComponent &o, bool legacy = false)
{
    c.Field(o.kind); c.Field(o.index); c.Field(o.type); c.Field(o.unionType);
    if (legacy) o.payloadKind = SP_CPCPayloadKind(o.type);
    else c.Field(o.payloadKind);
    if (!SP_CPCSlot(o.kind, o.index) || (o.kind == 2 && o.type != 21))
        Com_Error(ERR_DROP, "Client pose component rejected owner");
    if (o.payloadKind != SP_CPCPayloadKind(o.type)
        && (o.kind || !EntNum_IsCsc(o.index) || o.payloadKind != CPC_FX))
        Com_Error(ERR_DROP, "Client pose component rejected payload selector");
    switch (o.payloadKind)
    {
    case CPC_PLAYER:
        c.Field(o.control);
        if (o.control < -1 || o.control >= 36) Com_Error(ERR_DROP, "Client pose component rejected controller root");
        SP_CNSValue(c, o.payload.player.tag); SP_CNSValue(c, o.payload.player.waterHeight);
        SP_CNSValue(c, o.payload.player.nextWaterHeightCheck); SP_CNSValue(c, o.payload.player.nextRippleTime);
        break;
    case CPC_TURRET:
    {
        auto &v = o.payload.turret; c.Field(v.playerUsing);
        if (!v.playerUsing) { SP_CNSValue(c, v.angles.pitch); SP_CNSValue(c, v.angles.yaw); }
        SP_CNSValue(c, v.barrelPitch); SP_CNSValue(c, v.pivotOffset); SP_CNSValue(c, v.firingOffset);
        SP_CNSValue(c, v.tag_aim); SP_CNSValue(c, v.tag_aim_pivot); SP_CNSValue(c, v.tag_aim_animated);
        SP_CNSValue(c, v.tag_flash); SP_CNSValue(c, v.tag_weapon); break;
    }
    case CPC_VEHICLE:
    {
        auto &v = o.payload.vehicle;
#define CPC_V(n) SP_CNSValue(c, v.n);
        CPC_V(pitch) CPC_V(yaw) CPC_V(roll) CPC_V(barrelPitch) CPC_V(steerYaw) CPC_V(steerPitch) CPC_V(steerRoll)
        CPC_V(gunnerPitch) CPC_V(gunnerYaw) CPC_V(time) CPC_V(wheelBoneIndex) CPC_V(nitrousWheelRotation)
        CPC_V(nitrousWheelYaw) CPC_V(wheelHeight) CPC_V(tag_body) CPC_V(tag_turret) CPC_V(tag_barrel)
        CPC_V(tag_barrel_recoil) CPC_V(tag_gunner_turret) CPC_V(tag_gunner_barrel) CPC_V(tag_extra_tank_wheels)
        CPC_V(tag_minigun_spin) CPC_V(minigun_rotation) CPC_V(extra_wheel_rot_scale) CPC_V(barrelRecoil)
#undef CPC_V
        break;
    }
    case CPC_FX:
        // A logical reference into the separately saved FX owner graph. This
        // bounded component probe does not dereference it before FX publication.
        c.Field(o.payload.fx.triggerTime); c.Field(o.payload.fx.effect); break;
    case CPC_ACTOR:
    {
        auto &v = o.payload.actor;
        SP_CNSValue(c, v.proneType); SP_CNSValue(c, v.pitch); SP_CNSValue(c, v.roll); SP_CNSValue(c, v.headBone);
        SP_CNSValue(c, v.height); SP_CNSValue(c, v.waterHeight); SP_CNSValue(c, v.nextWaterHeightCheck); SP_CNSValue(c, v.nextRippleTime);
        break;
    }
    default:
        c.Field(o.payload.general.contents); c.Field(o.payload.general.linked);
        SP_CNSValue(c, o.payload.general.mins); SP_CNSValue(c, o.payload.general.maxs); break;
    }
    c.Field(o.linked);
    if (o.linked)
    {
        if (o.kind) Com_Error(ERR_DROP, "Client pose component collision owner must be an entity");
        SP_CNSValue(c, o.linkMins); SP_CNSValue(c, o.linkMaxs);
        for (int i = 0; i < 3; ++i) if (o.linkMins[i] > o.linkMaxs[i]) Com_Error(ERR_DROP, "Client pose component rejected collision bounds");
    }
    if (o.kind == 2)
    {
        SP_CNSValue(c, o.origin); SP_CNSValue(c, o.angles); SP_CNSValue(c, o.mins); SP_CNSValue(c, o.maxs);
        SP_CNSValue(c, o.wetness); c.Field(o.burnTime);
    }
    c.Field(o.queried);
    if (o.queried)
    {
        const int kind = o.payloadKind;
        if (o.kind || (kind != CPC_PLAYER && kind != CPC_ACTOR) || (kind == CPC_PLAYER && o.control < 0))
            Com_Error(ERR_DROP, "Client pose component rejected controller query");
        SP_CNSValue(c, o.reference.quat); SP_CNSValue(c, o.reference.trans); SP_CNSValue(c, o.reference.transWeight);
    }
}
struct SPClientPredictionHistory
{
    int number = 0, archiveIndex = 0;
    std::vector<usercmd_s> commands;
    ClientArchiveData archive[256]{};
    float viewangles[3]{};
};
static void SP_CPCUserCmdCodec(SPNativeCheckpointCodec &c, usercmd_s &v)
{
    c.Field(v.serverTime); SP_CNSValue(c, v.button_bits.array); SP_CNSValue(c, v.angles);
    c.Field(v.weapon); c.Field(v.offHandIndex); c.Field(v.lastWeaponAltModeSwitch);
    c.Field(v.forwardmove); c.Field(v.rightmove); c.Field(v.upmove); c.Field(v.pitchmove); c.Field(v.yawmove);
    SP_CNSValue(c, v.meleeChargeYaw); c.Field(v.meleeChargeDist); SP_CNSValue(c, v.rollmove);
    SP_CNSValue(c, v.selectedLocation); c.Field(v.selectedYaw);
    if (v.button_bits.array[1] & 0xFFF80000u) Com_Error(ERR_DROP, "Client input checkpoint rejected button bits");
}
static void SP_CPCArchiveCodec(SPNativeCheckpointCodec &c, ClientArchiveData &v)
{
    c.Field(v.serverTime); SP_CNSValue(c, v.origin); SP_CNSValue(c, v.velocity);
    c.Field(v.bobCycle); c.Field(v.movementDir); SP_CNSValue(c, v.viewangles);
}
static void SP_CPCHistoryCodec(SPNativeCheckpointCodec &c, SPClientPredictionHistory &h)
{
    c.Field(h.number); c.Field(h.archiveIndex); SP_CNSValue(c, h.viewangles);
    const int count = c.Int(static_cast<int>(h.commands.size()));
    if (h.number < 0 || h.number > 0x7FFFFF00 || (unsigned)h.archiveIndex >= 256
        || count != std::min(128, h.number + 1)) Com_Error(ERR_DROP, "Client input checkpoint rejected history range");
    if (c.reader) h.commands.resize(count);
    for (auto &cmd : h.commands) SP_CPCUserCmdCodec(c, cmd);
    for (auto &entry : h.archive) SP_CPCArchiveCodec(c, entry);
}
