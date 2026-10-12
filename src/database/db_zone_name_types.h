#pragma once
#include <cstddef>

enum FF_DIR : __int32
{                                       // XREF: XZoneName/r
                                        // UI_LoadModsMap/r ...
    FFD_DEFAULT  = 0x0,
    FFD_MOD_DIR  = 0x1,
    FFD_USER_MAP = 0x2,
};

struct __declspec(align(4)) XZoneName // sizeof=0x50
{                                       // XREF: .data:g_zoneNames/r
    char name[64];                      // XREF: DB_LinkXAssetEntry+236/r
                                        // DB_LinkXAssetEntry+A1E/r ...
    int flags;                          // XREF: DB_OverrideAsset+64/r
                                        // DB_OverrideAsset+82/r ...
    int fileSize;                       // XREF: DB_TryLoadXFileInternal+34D/w
                                        // DB_ReferencedFFChecksums(void)+B3/r
    FF_DIR dir;                         // XREF: DB_TryLoadXFileInternal+362/w
                                        // DB_ReferencedFFNameList(void)+A3/r ...
    bool loaded;                        // XREF: DB_IsZoneLoaded(char const *)+50/r
                                        // DB_IsZoneTypeLoaded(int)+44/r ...
    // padding byte
    // padding byte
    // padding byte
};

static_assert(sizeof(FF_DIR)==4 && sizeof(XZoneName)==80);
static_assert(offsetof(XZoneName,flags)==64 && offsetof(XZoneName,loaded)==76);
extern XZoneName g_zoneNames[33];
