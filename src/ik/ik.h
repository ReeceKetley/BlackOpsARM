#pragma once

#include <universal/dvar.h>
#include <ik/ik_system.h>
#include <physics/phys_colgeom.h>
#include <physics/phys_traverse.h>


inline IKBoneNames &operator++(IKBoneNames &t)
{
    t = static_cast<IKBoneNames>((static_cast<int>(t) + 1));
    return t;
}
inline IKBoneNames operator++(IKBoneNames &t, int)
{
    IKBoneNames old = t;
    t = static_cast<IKBoneNames>((static_cast<int>(t) + 1));
    return old;
}

enum IKLayerNames : __int32
{                                       // XREF: ?IK_Layer_TerrainMapping@@YAXPAUIKState@@@Z/r
                                        // ?IKImport_ConditionLayerTest@@YA_N_NW4IKLayerNames@@QBW41@H@Z/r ...
    IKLAYER_CROUCH_ACTIVE          = 0x0,
    IKLAYER_PRONE_ACTIVE           = 0x1,
    IKLAYER_PRONE_SLOW_FADE_ACTIVE = 0x2,
    IKLAYER_NOT_MOVING             = 0x3,
    IKLAYER_NOT_MOVING_MED         = 0x4,
    IKLAYER_NOT_MOVING_FAST        = 0x5,
    IKLAYER_MINIGUN_ACTIVE         = 0x6,
    IKLAYER_SHOTGUN_ACTIVE         = 0x7,
    IKLAYER_PELVIS_TEST            = 0x8,
    IKLAYER_TERRAIN_MAPPING        = 0x9,
    IKLAYER_HAND_TEST              = 0xA,
    IKLAYER_LEFT_HAND_ON_GUN       = 0xB,
    IKLAYER_RIGHT_HAND_ON_GUN      = 0xC,
    IKLAYER_PLAYER_PITCH           = 0xD,
    IKLAYER_WII_HANDS              = 0xE,
    IKLAYER_VEHICLE_GUNNER_OFFSET  = 0xF,
    IKLAYER_TURRET_FIRING          = 0x10,
    IKLAYER_TURRET_OFFSET          = 0x11,
    IKLAYER_PLAYER_CONTROLLERS     = 0x12,
    IKLAYER_LOOK_AT_ENTITY         = 0x13,
    IKLAYER_PREVENT_HEAD_CLIP      = 0x14,
    IKLAYER_COUNT                  = 0x15, // XREF: .data:offLayer/s
};
inline IKLayerNames &operator++(IKLayerNames &t)
{
    t = static_cast<IKLayerNames>((static_cast<int>(t) + 1));
    return t;
}
inline IKLayerNames operator++(IKLayerNames &t, int)
{
    IKLayerNames old = t;
    t = static_cast<IKLayerNames>((static_cast<int>(t) + 1));
    return old;
}

enum IKWeaponClass : __int32
{                                       // XREF: IKState/r
    IKWEAPCLASS_DEFAULT = 0x0,
    IKWEAPCLASS_SHOTGUN = 0x1,
    IKWEAPCLASS_COUNT   = 0x2,
};

enum IKWeaponOffsetType : __int32
{                                       // XREF: IKState/r
                                        // ?IKImport_GetPitchWeaponOffset@@YAXPAUIKState@@_NQAM@Z/r
    IKWEAPOFS_DEFAULT  = 0x0,
    IKWEAPOFS_PISTOL   = 0x1,
    IKWEAPOFS_SHOULDER = 0x2,
    IKWEAPOFS_MINIGUN  = 0x3,
    IKWEAPOFS_GRENADE  = 0x4,
    IKWEAPOFS_COUNT    = 0x5,
};

enum IKEntityStatusFunctions : __int32
{                                       // XREF: ?IK_GetStatus@@YA_NPAUIKState@@W4IKEntityStatusFunctions@@@Z/r
    IKSTATUS_SPECTATING             = 0x0,
    IKSTATUS_PRONE                  = 0x1,
    IKSTATUS_CROUCHING              = 0x2,
    IKSTATUS_FIRING                 = 0x3,
    IKSTATUS_MOUNTED_TURRET         = 0x4,
    IKSTATUS_APPLY_TERRAIN_MAPPING  = 0x5,
    IKSTATUS_BYPASS_TERRAIN_MAPPING = 0x6,
    IKSTATUS_APPLY_LEFT_HAND_IK     = 0x7,
    IKSTATUS_APPLY_RIGHT_HAND_IK    = 0x8,
    IKSTATUS_MOVING                 = 0x9,
    IKSTATUS_COUNT                  = 0xA,
};





struct IKJointVars // sizeof=0x1C
{                                       // XREF: IKState/r
    bool bValid;
    // padding byte
    // padding byte
    // padding byte
    float UpperLength;
    float UpperIKc;
    float UpperIKInvc;
    float LowerLength;
    float LowerIKc;
    float LowerIKInvc;
};

struct __declspec(align(4)) ikLerpInfo_t // sizeof=0xC
{                                       // XREF: IKState/r
    int lastUpdateTime;
    float lerp;
    bool goalState;
    // padding byte
    // padding byte
    // padding byte
};

struct IKCollisionCache // sizeof=0x6BC
{                                       // XREF: IKState/r
    int hitIndex;                       // XREF: IKImport_Profiler(IKState *)+1C/r
                                        // IKImport_Profiler(IKState *)+2B/w ...
    colgeom_visitor_inlined_t<200> proximity_data;
};

struct __declspec(align(16)) IKState // sizeof=0xE60
{                                       // XREF: .data:IKState * * ikStates/r
    int inUse;                          // XREF: IK_FindFreeIKState+30/r
                                        // IK_ResetTime(void)+36/r
    int entityNum;                      // XREF: IK_AllocateLocalClientMemory(void *,int)+9/w
                                        // IK_GetIKStateForEntity(int,uchar *,bool)+F7/r ...
    bool isServer;                      // XREF: IKImport_GetBoneStrings(char * * const)+36/w
                                        // IKImport_GetBoneStrings(char * * const):loc_86E69F/o ...
    // padding byte
    // padding byte
    // padding byte
    int localClientIndex;
    //$5C27F401DEE3D99C87E4E6F6FAC05DC6 ___u4;
    union //$5C27F401DEE3D99C87E4E6F6FAC05DC6 // sizeof=0x4
    {                                       // XREF: IKState/r
        unsigned __int8 *model;
        unsigned __int8 *localModel;
    };
    int isIKModel;
    //$215420D412D800E242F2B271FC5EBB99 ___u6;
    union //$215420D412D800E242F2B271FC5EBB99 // sizeof=0x4
    {                                       // XREF: IKState/r
        IKSystem *localIkSystem;
        IKSystem *ikSystem;
    };
    int timeMS;
    int frameUsed;
    unsigned __int8 ikBoneToObjBone[24];
                                        // XREF: IKImport_GetBoneStrings(char * * const)+85/w
                                        // IKImport_GetBoneStrings(char * * const)+99/o ...
    unsigned __int8 objBoneToIKBone[164];
                                        // XREF: IKImport_UpdateCollisionCache(IKState *)+12B/r
                                        // IKImport_InitCollisionCache(IKState *)+1F/o ...
    int bJointVarsValid;
    IKJointVars jointVars[4];
    float footHeightFromGround;
    int animatedIKBonesBits[1];
    int objBoneCalcBits[4];
    int modifiedIKBones;
    float footMats[2][4][4];
    float handMats[2][4][4];
    float leftHandFromRightHandMat[4][4];
    float entityXform[4][4];
    float entityLegsXform[4][4];
    float (*matArrayPreIK)[4][4];
    float (*matArrayPostIK)[4][4];
    float (*matArrayXforms)[4][4];
    float (*matArrayCache)[4][4];
    float (*matArrayCachePre)[4][4];
    int cacheActive;
    float rootOffset[3];
    float rootQuat[4];
    float origin[3];
    float angles[3];
    float lastTerrainMappingFootOffsetZ[2];
    float lastTerrainMappingPelvisZ;
    float lastTerrainMappingOrigin[3];
    float lastTerrainMappingTraceZ[2];
    float lastTerrainMappingTraceFootPos[2][2];
    float leftHandLerp;
    float rightHandLerp;
    int terrainDisableTime;
    ikLerpInfo_t lerpInfo[21];
    int lastFrameMS[21];
    bool bHasActiveLayers;
    // padding byte
    // padding byte
    // padding byte
    int lastMovedTime;
    int lastNotMovedTime;
    int lastUpdateLayerLerpsTime;
    int statusFlags;
    int lastUpdateStatusFlags;
    int lastIsIKEntityFrameNum;
    float controllers[6][3];
    float controller_origin_angles[3];
    bool hasControllers;
    // padding byte
    // padding byte
    // padding byte
    // padding byte
    // padding byte
    // padding byte
    // padding byte
    DvarValue dvarValues[23];
    float velocity[3];
    float gunnerCrouch;                 // XREF: IKImport_InitCollisionCache(IKState *)+6/r
                                        // IKImport_InitCollisionCache(IKState *)+10/r ...
    IKCollisionCache collisionCache;    // XREF: IKImport_Profiler(IKState *)+1C/r
                                        // IKImport_Profiler(IKState *)+2B/w ...
    float leftHandOffset[3];
    float leftHandRotation[3];
    float leftHandOffsetPrev[3];
    float leftHandRotationPrev[3];
    int leftHandChangeTime;
    float leftHandLastProximityLerp;
    IKWeaponOffsetType weaponOffsetType;
    float lastFootTraceNormals[2][3];
    float lastFootTraceGoalNormals[2][3];
    float lastCollisionEntityZ;
    int footTraceCollisionHint[4];
    float actorLookAtEntityAngles[3];
    float actorLookAtEntityScale;
    bool bHasLegSolverLayer;
    bool bHasArmSolverLayer;
    // padding byte
    // padding byte
    IKWeaponClass weaponClass;
    // padding byte
    // padding byte
    // padding byte
    // padding byte
    // padding byte
    // padding byte
    // padding byte
    // padding byte
    // padding byte
    // padding byte
    // padding byte
    // padding byte
};

void __cdecl IK_AllocateLocalClientMemory(void *ikStatesBuf, int localClientIndex);
void __cdecl IK_InitSystem();
IKState *__cdecl IK_GetIKStateForEntity(int entityNum, unsigned __int8 *model, bool bScanAll);

void __cdecl IK_InitializeIKState(IKState *ikState);
IKState *__cdecl IK_CreateIKForEntity(int entityNum, unsigned __int8 *model);
IKState *__cdecl IK_FindFreeIKState(int entityNum, unsigned __int8 *model);
// local variable allocation has failed, the output may be wrong!
void    IK_GetJointVars(IKState *ikState);
bool __cdecl IK_IsCalcBone(IKState *ikState, int ikBoneIndex);
void __cdecl IK_UpdateTimeAll(int time, int localClientIndex, bool inViewer);
void __cdecl IK_ResetTime();
void __cdecl IK_UpdateDvarValues(IKState *ikState);
void __cdecl IK_UpdateLayerLerps(IKState *ikState);
bool __cdecl IK_GetStatus(IKState *ikState, IKEntityStatusFunctions statusFunc);
void __cdecl IK_CheckSolverLayers(IKState *ikState);
void __cdecl IK_UpdateStatusFunctions(IKState *ikState);
void __cdecl IK_UpdateEntity(unsigned int entityNum, unsigned __int8 *model, bool isLocalBones, int *calcBoneBits);


extern IKState *ikStates[2];
extern IKSystem ikSystem;

extern float ikLayerLerpTimes[22];
extern int ikLayerRealLayers[22];
extern int ikLegSolverLayers[21];
extern int ikArmSolverLayers[21];

extern IKBoneNames IKBoneParents[23];
extern bool ikEssentialBones[28];