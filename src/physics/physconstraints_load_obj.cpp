#include "physconstraints_load_obj.h"
#include <cstring>
#include <cstddef>
#include <universal/q_shared.h>
#include <universal/com_files.h>
#include <qcommon/common.h>
#include <universal/com_memory.h>
#include <database/db_registry.h>

const char *s_constraintTypeNames[10] =
{
  "none",
  "point",
  "distance",
  "hinge",
  "joint",
  "actuator",
  "fake_shake",
  "launch",
  "rope",
  "light"
};

cspField_t physConstraintsFields[58] =
{
  { "c1_type", offsetof(PhysConstraints, data[0].type), 15 },
  { "c1_bone1_name", offsetof(PhysConstraints, data[0].target_bone1), 0 },
  { "c1_bone2_name", offsetof(PhysConstraints, data[0].target_bone2), 0 },
  { "c1_offsetX", offsetof(PhysConstraints, data[0].offset[0]), 7 },
  { "c1_offsetY", offsetof(PhysConstraints, data[0].offset[1]), 7 },
  { "c1_offsetZ", offsetof(PhysConstraints, data[0].offset[2]), 7 },
  { "c1_timeout", offsetof(PhysConstraints, data[0].timeout), 4 },
  { "c1_min_health", offsetof(PhysConstraints, data[0].min_health), 4 },
  { "c1_max_health", offsetof(PhysConstraints, data[0].max_health), 4 },
  { "c1_damp", offsetof(PhysConstraints, data[0].damp), 7 },
  { "c1_power", offsetof(PhysConstraints, data[0].power), 7 },
  { "c1_spin_scale", offsetof(PhysConstraints, data[0].spin_scale), 7 },
  { "c1_shakescalex", offsetof(PhysConstraints, data[0].scale[0]), 7 },
  { "c1_shakescaley", offsetof(PhysConstraints, data[0].scale[1]), 7 },
  { "c1_shakescalez", offsetof(PhysConstraints, data[0].scale[2]), 7 },
  { "c1_min_angle", offsetof(PhysConstraints, data[0].minAngle), 7 },
  { "c1_max_angle", offsetof(PhysConstraints, data[0].maxAngle), 7 },
  { "c1_yaw", offsetof(PhysConstraints, data[0].scale[1]), 7 },
  { "c1_pitch", offsetof(PhysConstraints, data[0].scale[0]), 7 },
  { "c2_type", offsetof(PhysConstraints, data[1].type), 15 },
  { "c2_bone1_name", offsetof(PhysConstraints, data[1].target_bone1), 0 },
  { "c2_bone2_name", offsetof(PhysConstraints, data[1].target_bone2), 0 },
  { "c2_offsetX", offsetof(PhysConstraints, data[1].offset[0]), 7 },
  { "c2_offsetY", offsetof(PhysConstraints, data[1].offset[1]), 7 },
  { "c2_offsetZ", offsetof(PhysConstraints, data[1].offset[2]), 7 },
  { "c2_timeout", offsetof(PhysConstraints, data[1].timeout), 4 },
  { "c2_min_health", offsetof(PhysConstraints, data[1].min_health), 4 },
  { "c2_max_health", offsetof(PhysConstraints, data[1].max_health), 4 },
  { "c2_damp", offsetof(PhysConstraints, data[1].damp), 7 },
  { "c2_power", offsetof(PhysConstraints, data[1].power), 7 },
  { "c2_min_angle", offsetof(PhysConstraints, data[1].minAngle), 7 },
  { "c2_max_angle", offsetof(PhysConstraints, data[1].maxAngle), 7 },
  { "c3_type", offsetof(PhysConstraints, data[2].type), 15 },
  { "c3_bone1_name", offsetof(PhysConstraints, data[2].target_bone1), 0 },
  { "c3_bone2_name", offsetof(PhysConstraints, data[2].target_bone2), 0 },
  { "c3_offsetX", offsetof(PhysConstraints, data[2].offset[0]), 7 },
  { "c3_offsetY", offsetof(PhysConstraints, data[2].offset[1]), 7 },
  { "c3_offsetZ", offsetof(PhysConstraints, data[2].offset[2]), 7 },
  { "c3_timeout", offsetof(PhysConstraints, data[2].timeout), 4 },
  { "c3_min_health", offsetof(PhysConstraints, data[2].min_health), 4 },
  { "c3_max_health", offsetof(PhysConstraints, data[2].max_health), 4 },
  { "c3_damp", offsetof(PhysConstraints, data[2].damp), 7 },
  { "c3_power", offsetof(PhysConstraints, data[2].power), 7 },
  { "c3_min_angle", offsetof(PhysConstraints, data[2].minAngle), 7 },
  { "c3_max_angle", offsetof(PhysConstraints, data[2].maxAngle), 7 },
  { "c4_type", offsetof(PhysConstraints, data[3].type), 15 },
  { "c4_bone1_name", offsetof(PhysConstraints, data[3].target_bone1), 0 },
  { "c4_bone2_name", offsetof(PhysConstraints, data[3].target_bone2), 0 },
  { "c4_offsetX", offsetof(PhysConstraints, data[3].offset[0]), 7 },
  { "c4_offsetY", offsetof(PhysConstraints, data[3].offset[1]), 7 },
  { "c4_offsetZ", offsetof(PhysConstraints, data[3].offset[2]), 7 },
  { "c4_timeout", offsetof(PhysConstraints, data[3].timeout), 4 },
  { "c4_min_health", offsetof(PhysConstraints, data[3].min_health), 4 },
  { "c4_max_health", offsetof(PhysConstraints, data[3].max_health), 4 },
  { "c4_damp", offsetof(PhysConstraints, data[3].damp), 7 },
  { "c4_power", offsetof(PhysConstraints, data[3].power), 7 },
  { "c4_min_angle", offsetof(PhysConstraints, data[3].minAngle), 7 },
  { "c4_max_angle", offsetof(PhysConstraints, data[3].maxAngle), 7 }
};



PhysConstraints *__cdecl PhysConstraintsLoadFile(const char *name, void *(__cdecl *Alloc)(int))
{
    int i; // [esp+20h] [ebp-405Ch]
    char dest[64]; // [esp+24h] [ebp-4058h] BYREF
    char buffer[16384]; // [esp+64h] [ebp-4018h] BYREF
    const char *last; // [esp+4068h] [ebp-14h]
    signed int v7; // [esp+406Ch] [ebp-10h]
    int f; // [esp+4070h] [ebp-Ch] BYREF
    unsigned __int8 *dst; // [esp+4074h] [ebp-8h]
    int len; // [esp+4078h] [ebp-4h]

    last = "PHYSCONSTRAINTS";
    len = strlen("PHYSCONSTRAINTS");
    if ( !strlen(name) )
        return 0;
    if ( Com_sprintf(dest, 0x40u, "physconstraints/%s", name) >= 0 )
    {
        v7 = FS_FOpenFileByMode(dest, &f, FS_READ);
        if ( v7 >= 0 )
        {
            FS_Read((unsigned __int8 *)buffer, len, f);
            buffer[len] = 0;
            if ( !strncmp(buffer, last, len) )
            {
                if ( v7 - len < 0x4000 )
                {
                    FS_Read((unsigned __int8 *)buffer, v7 - len, f);
                    buffer[v7 - len] = 0;
                    FS_FCloseFile(f);
                    if ( Info_Validate(buffer) )
                    {
                        dst = (unsigned __int8 *)Alloc(sizeof(PhysConstraints));
                        if ( !dst
                            && !Assert_MyHandler(
                                        "C:\\projects_pc\\cod\\codsrc\\src\\physics\\physconstraints_load_obj.cpp",
                                        234,
                                        0,
                                        "%s",
                                        "physConstraints") )
                        {
                            __debugbreak();
                        }
                        memset(dst, 0, sizeof(PhysConstraints));
                        for ( i = 0; i < 16; ++i )
                        {
                            auto &constraint = reinterpret_cast<PhysConstraints *>(dst)->data[i];
                            constraint.target_bone1 = "";
                            constraint.attach_point_type1 = ATTACH_POINT_BONE;
                            constraint.target_bone2 = "";
                            constraint.attach_point_type2 = ATTACH_POINT_BONE;
                        }
                        if ( ParseConfigStringToStruct(
                                     dst,
                                     physConstraintsFields,
                                     58,
                                     buffer,
                                     16,
                                     (int (__cdecl *)(unsigned __int8 *, const char *, const int, const int))CON_ParseSpecificField,
                                     PhysConstraints_Strcpy) )
                        {
                            PhysConstraintsPostLoadFixup((PhysConstraints *)dst);
                            return (PhysConstraints *)dst;
                        }
                        else
                        {
                            return 0;
                        }
                    }
                    else
                    {
                        Com_PrintError(20, "ERROR: physics constraints file [%s] is not valid\n", name);
                        return 0;
                    }
                }
                else
                {
                    Com_PrintError(20, "ERROR: physics constraints file [%s] is to big\n", name);
                    FS_FCloseFile(f);
                    return 0;
                }
            }
            else
            {
                Com_PrintError(20, "ERROR: file [%s] is not a physics constraints file\n", name);
                FS_FCloseFile(f);
                return 0;
            }
        }
        else
        {
            Com_PrintError(20, "ERROR: physics constraints '%s' not found\n", name);
            return 0;
        }
    }
    else
    {
        Com_PrintError(20, "ERROR: filename '%s' too long\n", dest);
        return 0;
    }
}

void __cdecl PhysConstraints_Strcpy(unsigned __int8 *member, const char *keyValue)
{
    char v2; // [esp+3h] [ebp-25h]
    unsigned __int8 *v3; // [esp+8h] [ebp-20h]
    const char *v4; // [esp+Ch] [ebp-1Ch]
    unsigned __int8 *buf; // [esp+20h] [ebp-8h]

    if ( *keyValue )
    {
        buf = Hunk_AllocLowAlign(strlen(keyValue) + 1, 1, "SetPhysConstraintsString", 0);
        v4 = keyValue;
        v3 = buf;
        do
        {
            v2 = *v4;
            *v3++ = *v4++;
        }
        while ( v2 );
        *(const char **)member = (const char *)buf;
    }
    else
    {
        *(const char **)member = "";
    }
}

int __cdecl CON_ParseSpecificField(unsigned __int8 *pStruct, const char *pValue, int fieldType)
{
    const char *v3; // eax
    PhysConstraint *constraint; // [esp+4h] [ebp-Ch]
    int i; // [esp+Ch] [ebp-4h]

    auto constraints = reinterpret_cast<PhysConstraints *>(pStruct);
    constraint = &constraints->data[constraints->count];
    if ( fieldType == 15 )
    {
        for ( i = 0; i < 10; ++i )
        {
            if ( !I_stricmp(pValue, s_constraintTypeNames[i]) )
            {
                if ( i )
                {
                    if (constraints->count >= 16)
                        Com_Error(ERR_DROP, "Constraints array overflow");
                    constraint->type = (ConstraintType)i;
                    ++constraints->count;
                }
                break;
            }
        }
        if ( i == 10 )
            Com_Error(ERR_DROP, "Unknown constraint type [%s]", pValue);
        return 1;
    }
    else
    {
        v3 = va("Bad constraint field type %i\n", fieldType);
        if ( !Assert_MyHandler("C:\\projects_pc\\cod\\codsrc\\src\\physics\\physconstraints_load_obj.cpp", 147, 0, v3) )
            __debugbreak();
        Com_Error(ERR_DROP, "Bad constraint field type %i", fieldType);
        return 0;
    }
}

void __cdecl PhysConstraintsPostLoadFixup(PhysConstraints *physConstraints)
{
    PhysConstraint *point_constraint; // [esp+8h] [ebp-10h]
    int ci; // [esp+10h] [ebp-8h]
    signed int count; // [esp+14h] [ebp-4h]

    count = physConstraints->count;
    for ( ci = 0; ci < count; ++ci )
    {
        if ( physConstraints->data[ci].type == CONSTRAINT_ACTUATOR )
        {
            if ( physConstraints->count >= 0x10 )
                Com_Error(ERR_DROP, "Cosntraints array overflow");
            point_constraint = &physConstraints->data[physConstraints->count++];
            memcpy(point_constraint, &physConstraints->data[ci], sizeof(PhysConstraint));
            point_constraint->type = CONSTRAINT_POINT;
        }
        else if ( physConstraints->data[ci].type == CONSTRAINT_LAUNCH )
        {
            physConstraints->data[ci].scale[2] = 0.0f;
        }
    }
}

PhysConstraints *__cdecl PhysConstraintsPrecache(const char *name, void *(__cdecl *Alloc)(int))
{
    PhysConstraints *physConstraints; // [esp+0h] [ebp-4h]
    PhysConstraints *physConstraintsa; // [esp+0h] [ebp-4h]

    if ( !name
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\physics\\physconstraints_load_obj.cpp",
                    262,
                    0,
                    "%s",
                    "name") )
    {
        __debugbreak();
    }
    if ( !*name
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\physics\\physconstraints_load_obj.cpp",
                    263,
                    0,
                    "%s",
                    "name[0]") )
    {
        __debugbreak();
    }
    physConstraints = (PhysConstraints *)Hunk_FindDataForFile(8, name);
    if ( physConstraints )
        return physConstraints;
    if ( !Alloc )
        return 0;
    physConstraintsa = PhysConstraintsLoadFile(name, Alloc);
    if ( physConstraintsa )
    {
        physConstraintsa->name = Hunk_SetDataForFile(8, name, physConstraintsa, Alloc);
        return physConstraintsa;
    }
    else
    {
        Com_PrintError(20, "ERROR: Cannot find physics constraints '%s'.\n", name);
        return 0;
    }
}

void __cdecl PhysConstraintsUpdateField(char *name, char *keyvalue)
{
    PhysConstraints *physConstraints; // [esp+0h] [ebp-4h]

    physConstraints = GetPhysConstraints(name);
    if ( physConstraints )
    {
        if ( ParseConfigStringToStruct(
                     (unsigned __int8 *)physConstraints,
                     physConstraintsFields,
                     58,
                     keyvalue,
                     16,
                     (int (__cdecl *)(unsigned __int8 *, const char *, const int, const int))CON_ParseSpecificField,
                     PhysConstraints_Strcpy) )
        {
            PhysConstraintsPostLoadFixup(physConstraints);
        }
    }
}

PhysConstraints *__cdecl GetPhysConstraints(char *name)
{
    return DB_FindXAssetHeader(ASSET_TYPE_PHYSCONSTRAINTS, name, 1, -1).physConstraints;
}

