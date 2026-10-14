#pragma once
#include <universal/dvar.h>
enum IKBoneNames : __int32
{                                       // XREF: IKJointBones/r
                                        // IKJointBones/r ...
#if defined(__clang__)
    IKBONE_NONE         = -1,
#else
    IKBONE_NONE         = 0xFFFFFFFF,
#endif
    IKBONE_FIRST        = 0x0,
    IKBONE_ORIGIN       = 0x0,
    IKBONE_PELVIS       = 0x1,
    IKBONE_SPINE        = 0x2,
    IKBONE_SPINE1       = 0x3,
    IKBONE_SPINE2       = 0x4,
    IKBONE_NECK         = 0x5,
    IKBONE_HEAD         = 0x6,
    IKBONE_LCLAVICLE    = 0x7,
    IKBONE_LUPPERARM    = 0x8,
    IKBONE_LFOREARM     = 0x9,
    IKBONE_LHAND        = 0xA,
    IKBONE_LWRIST_TWIST = 0xB,
    IKBONE_RCLAVICLE    = 0xC,
    IKBONE_RUPPERARM    = 0xD,
    IKBONE_RFOREARM     = 0xE,
    IKBONE_RHAND        = 0xF,
    IKBONE_LTHIGH       = 0x10,
    IKBONE_LCALF        = 0x11,
    IKBONE_LFOOT        = 0x12,
    IKBONE_RTHIGH       = 0x13,
    IKBONE_RCALF        = 0x14,
    IKBONE_RFOOT        = 0x15,
    IKBONE_RWEAPON      = 0x16,
    IKBONE_COUNT        = 0x17,
};

struct IKJointBones // sizeof=0x10
{                                       // XREF: IKSystem/r
    IKBoneNames childBone;              // XREF: IK_InitSystem(void)+7C/o
                                        // IK_InitSystem(void)+AA/o ...
    IKBoneNames jointBone;
    IKBoneNames parentBone;
    IKBoneNames baseBone;
};

struct IKSystem // sizeof=0x18C
{                                       // XREF: .data:IKSystem ikSystem/r
    bool bInitialized;                  // XREF: IK_InitSystem(void)+10D/w
                                        // IKImport_SetIKPartBits(DObj const *,int *):loc_86E887/r ...
    // padding byte
    // padding byte
    // padding byte
    char *boneStrings[23];              // XREF: IK_InitSystem(void)+13/o
    IKBoneNames ikBoneParents[23];      // XREF: IK_InitSystem(void)+45/w
                                        // IKImport_DrawDebugSkeleton(IKState *)+122/r ...
    IKJointBones joints[4];             // XREF: IK_InitSystem(void):loc_86CECE/o
                                        // IK_InitSystem(void)+7C/o ...
    int systemFrameNumber;              // XREF: IK_UpdateTimeAll(int,int,bool)+A5/w
                                        // IK_ResetTime(void):loc_86DBC4/w
    unsigned __int16 *ikImportBoneStringVars;
                                        // XREF: IKImport_GetBoneStrings(char * * const):loc_86E69F/w
    unsigned __int16 *ikImportMiscBoneStringVars;
                                        // XREF: IKImport_GetBoneStrings(char * * const)+99/w
    //$61DB84DF2E94E53D7788C0F08FDC33D5 ___u7;
    union //$61DB84DF2E94E53D7788C0F08FDC33D5 // sizeof=0x4
    {                                       // XREF: IKSystem/r
        unsigned __int8 *localClipMap;
        unsigned __int8 *clipMap;
    };
    const dvar_s *dvars[23];            // XREF: IK_UpdateDvarValues(IKState *)+4/r
                                        // IK_UpdateDvarValues(IKState *)+42/r ...
    bool inViewer;                      // XREF: IK_InitSystem(void)+106/w
                                        // IK_UpdateTimeAll(int,int,bool)+AD/w ...
    // padding byte
    // padding byte
    // padding byte
    float leftHandOffset[3];
    float leftHandRotation[3];
    int leftHandEntnum;
    int leftHandSwitchTime;
};
extern IKSystem ikSystem;
extern IKBoneNames IKBoneParents[23];
