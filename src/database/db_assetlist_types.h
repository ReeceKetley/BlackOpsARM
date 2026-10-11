#pragma once
#include <platform/bo1_native_width.h>
#include <cstddef>

#include "database.h"

struct XAsset // sizeof=0x8
{                                       // XREF: XAssetEntry/r
    XAssetType type;                    // XREF: DB_CreateDefaultEntry+7B/w
    XAssetHeader header;                // XREF: DB_CreateDefaultEntry+19/w
};

struct ScriptStringList // sizeof=0x8
{                                       // XREF: XAssetList/r
    int count;
    const char **strings;
};

struct XAssetList // sizeof=0x10
{                                       // XREF: .data:g_varXAssetList/r
    ScriptStringList stringList;
    int assetCount;
    XAsset *assets;
};

static_assert(sizeof(int) == 4);
static_assert(sizeof(ScriptStringList) == (2 * sizeof(void *)));
static_assert(sizeof(XAssetList) == (4 * sizeof(void *)));
static_assert(offsetof(XAssetList, assetCount) == (2 * sizeof(void *)));
static_assert(offsetof(XAssetList, assets) == (3 * sizeof(void *)));
static_assert(sizeof(XAssetHeader) == sizeof(void *));
static_assert(sizeof(XAsset) == (2 * sizeof(void *)));
static_assert(sizeof(XAssetType) == 4);
static_assert(offsetof(XAsset, header) == sizeof(void *));
static_assert(offsetof(ScriptStringList, strings) == sizeof(void *));
