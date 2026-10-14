#include "cg_effects_load_obj.h"
#include <universal/com_memory.h>
#include <qcommon/common.h>
#include <universal/com_files.h>
#include <universal/q_parse.h>
#include <universal/surfaceflags.h>
#include <EffectsCore/fx_load_obj.h>
#if defined(__ANDROID__)
#include <climits>
static int CG_CompareImpactFilesNative(const void *first, const void *second)
{
    return compare_impact_files(static_cast<const char **>(const_cast<void *>(first)),
                                static_cast<const char **>(const_cast<void *>(second)));
}
static const char *CG_FleshTypeNameNative(int index)
{
    return CG_FleshTypeToName(static_cast<unsigned int>(index));
}
#endif

const char *const g_TypeName[21] =
{
  "bullet_small_normal",
  "bullet_small_exit",
  "bullet_small_underwater",
  "bullet_large_normal",
  "bullet_large_exit",
  "shotgun_normal",
  "shotgun_exit",
  "bullet_ap_normal",
  "bullet_ap_exit",
  "bullet_xtreme_normal",
  "bullet_xtreme_exit",
  "grenade_bounce",
  "grenade_explode",
  "rocket_explode",
  "rocket_explode_xtreme",
  "projectile_dud",
  "mortar_shell",
  "tank_shell",
  "physics_impact",
  "bolt",
  "blade"
};

const char *const g_FleshTypeName[4] =
{
  "flesh_body_nonfatal",
  "flesh_body_fatal",
  "flesh_head_nonfatal",
  "flesh_head_fatal"
};

FxImpactTable *__cdecl CG_RegisterImpactEffects(const char *mapname)
{
    if ( useFastFile->current.enabled )
        return CG_RegisterImpactEffects_FastFile();
    else
        return CG_RegisterImpactEffects_LoadObj(mapname);
}

FxImpactTable *__cdecl CG_RegisterImpactEffects_LoadObj(const char *mapname)
{
    unsigned __int8 *v1; // eax
    int v2; // eax
    int v3; // eax
    EffectFile effectFile; // [esp+0h] [ebp-BD8h] BYREF
    char *listbuf; // [esp+B80h] [ebp-58h]
    int iBadCount; // [esp+B84h] [ebp-54h]
    unsigned int i; // [esp+B88h] [ebp-50h]
    FxImpactTable *fx; // [esp+B8Ch] [ebp-4Ch]
    char mapdir[68]; // [esp+B90h] [ebp-48h] BYREF

    Hunk_CheckTempMemoryClear();
    listbuf = (char *)Hunk_AllocateTempMemory(0x10000, "CG_RegisterImpactEffects");
    memset((unsigned __int8 *)&effectFile, 0, sizeof(effectFile));
    CG_RegisterImpactEffectsForDir((char*)"fx", &effectFile, listbuf);
    if ( mapname )
    {
        Com_sprintf(mapdir, 0x40u, "fx/maps/%s", mapname);
        CG_RegisterImpactEffectsForDir(mapdir, &effectFile, listbuf);
    }
#if defined(__ANDROID__)
    fx = (FxImpactTable *)Hunk_AllocAlign(sizeof(FxImpactTable), alignof(FxImpactTable), "CG_RegisterImpactEffects", 8);
    v1 = Hunk_AllocAlign(21 * sizeof(FxImpactEntry), alignof(FxImpactEntry), "CG_RegisterImpactEffects", 8);
#else
    fx = (FxImpactTable *)Hunk_AllocAlign(8u, 4, "CG_RegisterImpactEffects", 8);
    v1 = Hunk_AllocAlign(0xB7Cu, 4, "CG_RegisterImpactEffects", 8);
#endif
    fx->table = (FxImpactEntry *)v1;
    iBadCount = 0;
    for ( i = 0; i < 0x15; ++i )
    {
        v2 = CG_RegisterImpactEffects_NonFlesh(effectFile.nonflesh[i], g_TypeName[i], fx->table[i].nonflesh);
        iBadCount += v2;
        v3 = CG_RegisterImpactEffects_Flesh(
                     effectFile.flesh[i],
                     g_TypeName[i],
                     fx->table[i].flesh,
#if defined(__ANDROID__)
                     // Original linear slot: 21*31 - 161*4 + 31*i = 31*i+7.
                     // Surface type seven is flesh; address its actual row.
                     (char *)effectFile.nonflesh[i][7]);
#else
                     (char *)effectFile.flesh[-161][31 * i]);
#endif
        iBadCount += v3;
    }
    if ( iBadCount )
    {
        Com_PrintError(21, "ERROR: %i missing entries in effect CSV files (see console for details)", iBadCount);
        Hunk_ClearTempMemory();
        return 0;
    }
    else
    {
        fx->name = "";
        Hunk_ClearTempMemory();
        return fx;
    }
}

// BO1TODO: template these ugly muthas
int __cdecl CG_RegisterImpactEffects_Generic_31_char_const_____cdecl_int__(
    const char **szEffectFile,
    const char *pszTypeName,
    const FxEffectDef **fx,
    const char *(__cdecl *typeToNameFunc)(int),
    const char *defaultEffectName)
{
    const char *v5; // eax
    const char *effectName; // [esp+0h] [ebp-Ch]
    int iBadCount; // [esp+4h] [ebp-8h]
    int i; // [esp+8h] [ebp-4h]

    iBadCount = 0;
    for (i = 0; i < 31; ++i)
    {
        effectName = szEffectFile[i];
        if (!effectName)
            effectName = defaultEffectName;
        if (effectName)
        {
            if (!*effectName && defaultEffectName)
                effectName = defaultEffectName;
            if (*effectName)
                fx[i] = FX_Register((char*)effectName);
            else
                fx[i] = 0;
        }
        else
        {
            v5 = typeToNameFunc(i);
            Com_Printf(21, "no entry for effect type '%s' on surface/flesh type '%s'\n", pszTypeName, v5);
            ++iBadCount;
            fx[i] = 0;
        }
    }
    return iBadCount;
}

int __cdecl CG_RegisterImpactEffects_NonFlesh(
                const char **szEffectFile,
                const char *pszTypeName,
                const FxEffectDef **fx)
{
    return CG_RegisterImpactEffects_Generic_31_char_const_____cdecl_int__(
                     szEffectFile,
                     pszTypeName,
                     fx,
                     Com_SurfaceTypeToName,
                     0);
}

int __cdecl CG_RegisterImpactEffects_Generic_4_char_const_____cdecl_int__(
    const char **szEffectFile,
    const char *pszTypeName,
    const FxEffectDef **fx,
    const char *(__cdecl *typeToNameFunc)(int),
    const char *defaultEffectName)
{
    const char *v5; // eax
    const char *effectName; // [esp+0h] [ebp-Ch]
    int iBadCount; // [esp+4h] [ebp-8h]
    int i; // [esp+8h] [ebp-4h]

    iBadCount = 0;
    for (i = 0; i < 4; ++i)
    {
        effectName = szEffectFile[i];
        if (!effectName)
            effectName = defaultEffectName;
        if (effectName)
        {
            if (!*effectName && defaultEffectName)
                effectName = defaultEffectName;
            if (*effectName)
                fx[i] = FX_Register((char*)effectName);
            else
                fx[i] = 0;
        }
        else
        {
            v5 = typeToNameFunc(i);
            Com_Printf(21, "no entry for effect type '%s' on surface/flesh type '%s'\n", pszTypeName, v5);
            ++iBadCount;
            fx[i] = 0;
        }
    }
    return iBadCount;
}

int __cdecl CG_RegisterImpactEffects_Flesh(
                const char **szEffectFile,
                const char *pszTypeName,
                const FxEffectDef **fx,
                char *defaultEffectName)
{
    return CG_RegisterImpactEffects_Generic_4_char_const_____cdecl_int__(
                     szEffectFile,
                     pszTypeName,
                     fx,
#if defined(__ANDROID__)
                     CG_FleshTypeNameNative,
#else
                     (const char *(__cdecl *)(int))CG_FleshTypeToName,
#endif
                     defaultEffectName);
}

const char *__cdecl CG_FleshTypeToName(unsigned int fleshTypeId)
{
    if ( fleshTypeId >= 4
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\cgame\\cg_effects_load_obj.cpp",
                    70,
                    0,
                    "%s\n\t(fleshTypeId) = %i",
                    "(fleshTypeId >= 0 && fleshTypeId < (sizeof( g_FleshTypeName ) / (sizeof( g_FleshTypeName[0] ) * (sizeof( g_Fle"
                    "shTypeName ) != 4 || sizeof( g_FleshTypeName[0] ) <= 4))))",
                    fleshTypeId) )
    {
        __debugbreak();
    }
    return g_FleshTypeName[fleshTypeId];
}

void __cdecl CG_RegisterImpactEffectsForDir(char *dir, EffectFile *effectFile, char *listbuf)
{
#if defined(__ANDROID__)
    const char *base[4097];
#else
    void *base[4097]; // [esp+10h] [ebp-4020h] BYREF
#endif
    void *buffer; // [esp+4014h] [ebp-1Ch]
    unsigned int num; // [esp+4018h] [ebp-18h]
    int len; // [esp+401Ch] [ebp-14h]
    char *v7; // [esp+4020h] [ebp-10h]
    char *qpath; // [esp+4024h] [ebp-Ch]
    int f; // [esp+4028h] [ebp-8h] BYREF
    unsigned int i; // [esp+402Ch] [ebp-4h]

    num = FS_GetFileList(dir, (char*)"csv", FS_LIST_PURE_ONLY, listbuf, 0x10000);
    if ( num )
    {
        if ( num > 0x1000 )
            num = 4096;
        qpath = listbuf;
        for ( i = 0; i < num; ++i )
        {
            base[i] = qpath;
#if defined(__ANDROID__)
            const char *nameEnd = static_cast<const char *>(memchr(qpath, 0, listbuf + 0x10000 - qpath));
            if (!nameEnd)
            {
                Com_PrintError(21, "ERROR: unterminated impact CSV file list\n");
                return;
            }
            qpath = const_cast<char *>(nameEnd) + 1;
#else
            qpath += strlen(qpath) + 1;
#endif
        }
#if defined(__ANDROID__)
        qsort(base, num, sizeof(*(base)), CG_CompareImpactFilesNative);
#else
        qsort(base, num, sizeof(*(base)), (int (__cdecl *)(const void *, const void *))compare_impact_files);
#endif
        for ( i = 0; i < num; ++i )
        {
            qpath = va("%s/%s", dir, (const char *)base[i]);
            len = FS_FOpenFileByMode(qpath, &f, FS_READ);
            if ( len >= 0 )
            {
#if defined(__ANDROID__)
                if (len == INT_MAX)
                {
                    FS_FCloseFile(f);
                    Com_PrintError(21, "ERROR: impact CSV file exceeds temporary allocation capacity\n");
                    return;
                }
#endif
                Hunk_CheckTempMemoryHighClear();
                buffer = (void *)Hunk_AllocateTempMemoryHigh(len + 1, "CG_RegisterImpactEffects");
#if defined(__ANDROID__)
                const bool complete = FS_Read((unsigned __int8 *)buffer, len, f) == static_cast<unsigned int>(len);
#else
                FS_Read((unsigned __int8 *)buffer, len, f);
#endif
                FS_FCloseFile(f);
#if defined(__ANDROID__)
                if (!complete)
                {
                    Hunk_ClearTempMemoryHigh();
                    Com_PrintError(21, "ERROR: truncated impact CSV file '%s'\n", qpath);
                    return;
                }
#endif
                *((_BYTE *)buffer + len) = 0;
                Com_BeginParseSession(qpath);
                Com_SetCSV(1);
                v7 = CG_ParseImpactEffects(qpath, (const char *)buffer, 21, g_TypeName, effectFile);
                Com_EndParseSession();
                Hunk_ClearTempMemoryHigh();
                if ( v7 )
                {
                    Com_PrintError(21, "ERROR: %s", v7);
                    return;
                }
            }
        }
    }
}

char *__cdecl CG_ParseImpactEffects(
                const char *filename,
                const char *buf,
                int iTypeCount,
                const char *const *pszTypeName,
                EffectFile *effectFile)
{
    char v6; // al
    char *v7; // [esp+8h] [ebp-34h]
    parseInfo_t *v8; // [esp+Ch] [ebp-30h]
    int v9; // [esp+10h] [ebp-2Ch]
    unsigned int i; // [esp+20h] [ebp-1Ch]
    int iEffectType; // [esp+24h] [ebp-18h]
    char *effectName; // [esp+2Ch] [ebp-10h]
    int iFleshType; // [esp+30h] [ebp-Ch]
    int iSurfaceType; // [esp+34h] [ebp-8h]
    parseInfo_t *token; // [esp+38h] [ebp-4h]
    parseInfo_t *tokena; // [esp+38h] [ebp-4h]
    parseInfo_t *tokenb; // [esp+38h] [ebp-4h]

    while ( 1 )
    {
        token = Com_Parse(&buf);
        if ( !buf )
            return 0;
        if ( token->token[0] && token->token[0] != 35 )
            break;
LABEL_5:
        Com_SkipRestOfLine(&buf);
    }
    for ( iEffectType = 0; iEffectType < iTypeCount && I_stricmp(pszTypeName[iEffectType], token->token); ++iEffectType )
        ;
    if ( iEffectType == iTypeCount )
        return va("unknown effect type '%s' in first column of file '%s'", token->token, filename);
    tokena = Com_ParseOnLine(&buf);
    if ( !tokena->token[0] )
        return va("missing surface/flesh type in second column of file '%s'", filename);
    iSurfaceType = Com_SurfaceTypeFromName(tokena->token);
    for ( i = 0; i < 4; ++i )
    {
        if ( !I_stricmp(tokena->token, g_FleshTypeName[i]) )
        {
            iFleshType = i;
            goto LABEL_20;
        }
    }
    iFleshType = -1;
LABEL_20:
    if ( iSurfaceType >= 0
        && iFleshType >= 0
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\cgame\\cg_effects_load_obj.cpp",
                    176,
                    0,
                    "%s",
                    "iSurfaceType < 0 || iFleshType < 0") )
    {
        __debugbreak();
    }
    if ( iSurfaceType < 0 && iFleshType < 0 )
        return va("unknown surface/flesh type '%s' in second column of file '%s'", tokena->token, filename);
    tokenb = Com_ParseOnLine(&buf);
    v9 = strlen(tokenb->token);
    if ( v9 < 64 )
    {
        effectName = (char *)Hunk_AllocateTempMemory(v9 + 1, "CG_ParseImpactEffects");
#if defined(__ANDROID__)
        memcpy(effectName, tokenb->token, v9 + 1);
#else
        v8 = tokenb;
        v7 = effectName;
        do
        {
            v6 = v8->token[0];
            *v7 = v8->token[0];
            v8 = (parseInfo_t *)((char *)v8 + 1);
            ++v7;
        }
        while ( v6 );
#endif
        if ( iSurfaceType < 0
            && iFleshType < 0
            && !Assert_MyHandler(
                        "C:\\projects_pc\\cod\\codsrc\\src\\cgame\\cg_effects_load_obj.cpp",
                        188,
                        0,
                        "%s",
                        "iSurfaceType >= 0 || iFleshType >= 0") )
        {
            __debugbreak();
        }
        if ( iSurfaceType < 0 )
            effectFile->flesh[iEffectType][iFleshType] = effectName;
        else
            effectFile->nonflesh[iEffectType][iSurfaceType] = effectName;
        goto LABEL_5;
    }
    return va(
                     "effect filename '%s' in third column of file '%s' is longer than %i characters",
                     tokenb->token,
                     filename,
                     63);
}

int __cdecl compare_impact_files(const char **pe0, const char **pe1)
{
    return I_stricmp(*pe0, *pe1);
}

FxImpactTable *__cdecl CG_RegisterImpactEffects_FastFile()
{
    return DB_FindXAssetHeader(ASSET_TYPE_IMPACT_FX, (char*)"", 1, -1).impactFx;
}

